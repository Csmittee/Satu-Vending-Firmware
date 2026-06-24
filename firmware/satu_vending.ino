// ============================================================
// SATU VENDING MACHINE — Main Firmware
// Board: ESP32-S3 (ESP32-8048S070C), 7" capacitive touch
// Flash: 16MB | PSRAM: 8MB OPI — NEVER change PSRAM setting
// ============================================================
// CHANGE LOG:
//   R2   — Fixed compile error, full state machine, NVS
//   R3   — Arduino_GFX, slot config, gift option, sacred water
//   R3.2 — laneErrorCount/laneDisabled use MAX_SLOTS_HW
//   R4   — Runtime grid: recalcGrid() after /hello
//          Setup code screen: polling loop when status==pending
//          Boot PIN: drawBootPinScreen() if NVS boot_pin set
//          Service mode: 5-tab dispatch, settings actions 401/402
//          Factory reset: factoryResetBackend() → NVS wipe on HTTP 200 only
//          nuke command: wipes NVS, restarts
//          Debug screen: 5s hold bottom-left in idle
//          _onItemRemoved: esp_random(), reportCompletion with slotIdx
//          Payment timeout: reportCompletion false
//   R5   — WiFi NVS provisioning: STATE_WIFI_SETUP + drawWifiSetupScreen()
//          initWiFi() NVS-first (nvs_ssid/nvs_pass), config.h fallback
//          No credentials → WiFi setup touchscreen, restart after save
//   R6   — R-128: motor stop on IR sensor trigger, synchronous vendProduct()
//          R-129: pin-lock flap via RELAY_FLAP (defined in config.h)
//          R-131: showPaymentAccepted() 1.5s green banner before vend
//          R-137: font audit — FreeSans hierarchy throughout ui.h
//   R7   — R-148: STATE_GIFT_OPTION entry guard 250ms (carry-over touch fix)
//          R-149: vendProduct() polls commands every 500ms (sensor_triggered fix)
//          Include order: network.h before hardware.h (CommandList dependency)
//   R8   — R-150: touchReadOnce() shared GT911 read per idle tick (3-tap drop fix)
//          R-151: resetGiftTouchDebounce() on STATE_GIFT_OPTION entry + guard 500ms
//          R-152: PRODUCT_SELECTION_TIMEOUT in config.h replaces magic number 15
//   R9   — R-153: STATE_CONFIRMING — confirm screen between gift option and QR
//          Back added to gift option screen → returns to STATE_PRODUCT_SELECTION
//          createOrder() now only called on Confirm touch (no D1 rows from abandoned flows)
//   R10  — D-11: STATE_WELCOME on every boot — bilingual EN/TH language selector
//          g_lang_th set from g_lang_th_default (NVS "lang" key) at boot
//          getTouchedWelcome() handles EN/TH selection; tap elsewhere → STATE_IDLE
//          Welcome screen idle timeout resets language to default and redraws
// ============================================================

#include "config.h"
#include "state_machine.h"
#include "network.h"
#include "hardware.h"
#include "ui.h"

// ── State machine globals ──────────────────────────────────────────────
MachineState  currentState    = STATE_STARTUP;
MachineState  lastState       = STATE_STARTUP;
unsigned long stateStartTime  = 0;
String        currentOrderId  = "";
int           selectedSlot    = -1;
bool          wantSacredWater = false;
String        currentUserId   = "";
int           g_service_tab   = 0;

// ── Lane error tracking ───────────────────────────────────────────────────────────
int  laneErrorCount[NUM_SLOTS] = {0};
bool laneDisabled[NUM_SLOTS]   = {false};

// ── QR / payment globals ────────────────────────────────────────────────────────
String currentQrData  = "";
int    currentAmount  = 0;

// ── Timing ────────────────────────────────────────────────────────────────────
static unsigned long lastHeartbeatMs   = 0;
static unsigned long lastCommandPollMs = 0;
static unsigned long paymentPollTimer  = 0;
static unsigned long paymentStartMs    = 0;

#define COMMAND_POLL_INTERVAL  30000
#define PAYMENT_POLL_INTERVAL   3000
#define PAYMENT_TIMER_INTERVAL  1000

// ── setState ───────────────────────────────────────────────────────────────────
void setState(MachineState newState) {
  lastState      = currentState;
  currentState   = newState;
  stateStartTime = millis();
  Serial.printf("[STATE] %d → %d\n", (int)lastState, (int)newState);
  if (newState == STATE_GIFT_OPTION) resetGiftTouchDebounce();  // R-151
}

// ── Forward declarations ──────────────────────────────────────────────────────
void handleCommands();
void runStateMachine();
void runTimers();
void _proceedToPayment();
void _onPaymentConfirmed();
void _onItemDropped();
void _onLaneEmpty(int lane);

// ── PIN validation ──────────────────────────────────────────────────────────────
static bool _validatePin(String entered) {
  Preferences prefs;
  prefs.begin("satu", true);
  String stored = prefs.getString("svc_pin", "");
  bool enabled  = prefs.getBool("svc_pin_en", false);
  prefs.end();
  if (!enabled || stored.isEmpty()) return true;  // no PIN set = always pass
  return entered == stored;
}

// Blocking PIN entry loop. Returns true if PIN accepted, false if cancelled (timeout).
static bool _runPinEntryLoop(unsigned long timeoutMs = 30000) {
  // Draw overlay
  int boxW = 360, boxH = 360;
  int boxX = SCR_W/2 - boxW/2;
  int boxY = SCR_H/2 - boxH/2;
  _fillRoundRect(boxX, boxY, boxW, boxH, 12, gfx->color565(10, 8, 18));
  _drawRoundRect(boxX, boxY, boxW, boxH, 12, C_GOLD);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(2);
  const char* title = "Service PIN";
  gfx->setCursor(boxX + boxW/2 - strlen(title)*12/2, boxY + 12);
  gfx->print(title);

  int npX = boxX + 20;
  int npY = boxY + 78;
  _drawNumpad(npX, npY);

  // PIN display
  int pdX = npX, pdY = boxY + 46;
  int pdW = NP_COLS * (NP_KEY_W + NP_GAP) - NP_GAP;
  gfx->fillRect(pdX, pdY, pdW, 26, gfx->color565(20,16,30));
  gfx->drawRect(pdX, pdY, pdW, 26, C_GOLD);

  String entered = "";
  unsigned long startMs = millis();

  while (millis() - startMs < timeoutMs) {
    int key = getTouchedNumpad(npX, npY);
    if (key == -1) { delay(20); continue; }

    if (key == 10) {  // DEL
      if (entered.length() > 0) entered.remove(entered.length() - 1);
    } else if (key == 11) {  // OK
      if (_validatePin(entered)) return true;
      // Wrong PIN — flash red
      gfx->fillRect(pdX, pdY, pdW, 26, C_RED);
      delay(400);
      gfx->fillRect(pdX, pdY, pdW, 26, gfx->color565(20,16,30));
      gfx->drawRect(pdX, pdY, pdW, 26, C_GOLD);
      entered = "";
    } else if (entered.length() < 8) {
      entered += String(key);
    }

    // Redraw PIN display (masked)
    gfx->fillRect(pdX + 2, pdY + 2, pdW - 4, 22, gfx->color565(20,16,30));
    gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
    String masked = "";
    for (size_t i = 0; i < entered.length(); i++) masked += "*";
    gfx->setCursor(pdX + 8, pdY + 5);
    gfx->print(masked.c_str());
  }
  return false;  // timed out
}

// ── Boot PIN entry loop ───────────────────────────────────────────────────────────
static void _runBootPinLoop() {
  Preferences prefs;
  prefs.begin("satu", true);
  bool bootPinEnabled = prefs.getBool("boot_pin", false);
  prefs.end();
  if (!bootPinEnabled) return;

  drawBootPinScreen();

  int npX = SCR_W/2 - (NP_COLS * (NP_KEY_W + NP_GAP))/2 + 60;
  int npY = STATUS_H + 40;
  int pdX = npX, pdY = npY - 36;
  int pdW = NP_COLS * (NP_KEY_W + NP_GAP) - NP_GAP;

  String entered = "";
  bool   unlocked = false;

  while (!unlocked) {
    int key = getTouchedNumpad(npX, npY);
    if (key == -1) { delay(20); continue; }
    if (key == 10) {
      if (entered.length() > 0) entered.remove(entered.length() - 1);
    } else if (key == 11) {
      if (_validatePin(entered)) {
        unlocked = true;
      } else {
        gfx->fillRect(pdX + 2, pdY + 2, pdW - 4, 24, C_RED);
        delay(400);
        gfx->fillRect(pdX + 2, pdY + 2, pdW - 4, 24, gfx->color565(20,16,30));
        entered = "";
      }
    } else if (entered.length() < 8) {
      entered += String(key);
    }
    // Update PIN display
    gfx->fillRect(pdX + 2, pdY + 2, pdW - 4, 24, gfx->color565(20,16,30));
    gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
    String masked = "";
    for (size_t i = 0; i < entered.length(); i++) masked += "*";
    gfx->setCursor(pdX + 8, pdY + 6);
    gfx->print(masked.c_str());
  }
}

// ── NVS wipe ───────────────────────────────────────────────────────────────────────────
static void _clearNVS() {
  Preferences prefs;
  prefs.begin("satu", false);
  prefs.clear();
  prefs.end();
  Serial.println("[NVS] Cleared");
}

// ════════════════════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[SATU] Booting R10...");

  initHardware();
  Serial.println("[SATU] Hardware OK");

  initUI();
  drawBootScreen("Connecting to WiFi...");
  Serial.println("[SATU] Display OK");

  JsonDocument helloDoc;
  initWiFi(helloDoc);

  // R5: if no WiFi credentials in NVS, show provisioning screen.
  // drawWifiSetupScreen() is blocking — calls saveWifiAndReboot()
  // which restarts the device after persisting credentials to NVS.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SATU] No WiFi — showing setup screen");
    setState(STATE_WIFI_SETUP);
    drawWifiSetupScreen();  // never returns — device restarts on CONNECT
    ESP.restart();          // safety fallback
  }

  String machineStatus = helloDoc["status"] | "";
  Serial.printf("[SATU] Machine status: %s\n", machineStatus.c_str());

  // Pending: show setup code and poll until active
  if (machineStatus == "pending") {
    extern String g_setupCode;
    String code = helloDoc["setup_code"] | g_setupCode;
    if (code.isEmpty()) code = g_setupCode;

    unsigned long pendingStart = millis();
    const unsigned long POLL_INTERVAL = 30000;
    const unsigned long MAX_PENDING   = 30UL * 60UL * 1000UL;  // 30 min

    while (machineStatus == "pending" && millis() - pendingStart < MAX_PENDING) {
      unsigned long nextPollIn = POLL_INTERVAL - ((millis() - pendingStart) % POLL_INTERVAL);
      String countdown = String(nextPollIn / 1000) + "s";
      drawSetupCodeScreen(code, countdown);
      delay(5000);

      // Re-poll /hello
      helloDoc.clear();
      reloadHello(helloDoc);
      machineStatus = helloDoc["status"] | "";
      if (machineStatus == "active") break;
    }

    if (machineStatus != "active") {
      drawErrorScreen("Setup timeout\nRestart device");
      delay(10000);
      ESP.restart();
    }
  }

  // Active: load config and slots
  if (helloDoc.containsKey("slots")) {
    loadSlotsFromJson(helloDoc["slots"].as<JsonArray>());
    Serial.println("[SATU] Slots loaded");
  }

  // Apply runtime grid from NVS (populated by _sendHello in network.h)
  loadConfigFromNVS();
  recalcGrid();

  // Boot PIN gate
  _runBootPinLoop();

  // D-11 R10: start with welcome screen — language selector before idle
  g_lang_th = g_lang_th_default;
  setState(STATE_WELCOME);
  drawWelcomeScreen();

  Serial.println("[SATU] Ready");
}

// ════════════════════════════════════════════════════════════════════════════
//  MAIN LOOP
// ════════════════════════════════════════════════════════════════════════════
void loop() {
  runStateMachine();
  runTimers();
  delay(10);
}

// ════════════════════════════════════════════════════════════════════════════
//  STATE MACHINE
// ════════════════════════════════════════════════════════════════════════════
void runStateMachine() {
  unsigned long now     = millis();
  unsigned long elapsed = now - stateStartTime;

  switch (currentState) {

    // ── WELCOME ────────────────────────────────────────────────────────────────────
    // D-11 R10: shown on every boot. EN/TH selection → sets g_lang_th.
    // Tap anywhere else → proceed to STATE_IDLE with current language.
    // Idle timeout → reset to default language and redraw.
    case STATE_WELCOME: {
      int action = getTouchedWelcome();
      if (action == 1 || action == 2) break;  // language selected — g_lang_th updated by getTouchedWelcome
      if (action == 0) {
        setState(STATE_IDLE);
        g_idleDrawn = false;
        idleAnimationUI();
        idleAnimation();
        drawIdleScreen();
        break;
      }
      // No touch — check welcome screen idle timeout
      if (elapsed > (unsigned long)(g_cfg_idle * 1000UL)) {
        g_lang_th      = g_lang_th_default;
        stateStartTime = millis();
        drawWelcomeScreen();
      }
      break;
    }

    // ── IDLE ───────────────────────────────────────────────────────────────────────
    case STATE_IDLE: {
      // Debug screen: hold bottom-left (x<80, y>400) for 5 seconds
      {
        static unsigned long debugHoldStart = 0;
        static bool          debugHolding   = false;
        touchReadOnce();  // R-150: single GT911 read shared with checkServiceGesture + getTouchedSlot
        if (_touch.isTouched &&
            _touch.points[0].x < 80 &&
            _touch.points[0].y > 400) {
          if (!debugHolding) {
            debugHolding   = true;
            debugHoldStart = millis();
          } else if (millis() - debugHoldStart > 5000) {
            debugHolding = false;
            drawDebugScreen();
            // Wait for tap to dismiss
            unsigned long dbgWait = millis();
            while (millis() - dbgWait < 15000) {
              _touch.read();
              if (_touch.isTouched) break;
              delay(50);
            }
            g_idleDrawn = false;
            drawIdleScreen();
          }
        } else {
          debugHolding = false;
        }
      }

      // Service mode: gesture check, then PIN if set
      if (checkServiceGesture()) {
        bool enter = _runPinEntryLoop(30000);
        if (enter) {
          g_service_tab = 0;
          setState(STATE_SERVICE);
          drawServiceScreen(g_service_tab);
        } else {
          g_idleDrawn = false;
          drawIdleScreen();
        }
        break;
      }

      int touched = getTouchedSlot();
      if (touched >= 0) {
        selectedSlot = touched;
        setState(STATE_PRODUCT_SELECTION);
        drawProductSelection(selectedSlot);
      }
      break;
    }

    // ── PRODUCT_SELECTION ──────────────────────────────────────────────────────────
    case STATE_PRODUCT_SELECTION: {
      int touched = getTouchedSlot();
      if (touched >= 0) {
        if (touched == selectedSlot) {
          setState(STATE_GIFT_OPTION);
          wantSacredWater = false;
          drawGiftOptionScreen(selectedSlot);
        } else {
          selectedSlot = touched;
          drawProductSelection(selectedSlot);
          stateStartTime = millis();
        }
      }
      if (elapsed > (unsigned long)(g_cfg_sel * 1000)) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── GIFT_OPTION ──────────────────────────────────────────────────────────────────
    case STATE_GIFT_OPTION: {
      if (elapsed < 500) break;   // R-148/R-151: 500ms entry guard + debounce reset in setState()
      int choice = getTouchedGiftOption();
      if (choice == 0) {
        wantSacredWater = false;
        setState(STATE_CONFIRMING);
        drawConfirmScreen(selectedSlot, wantSacredWater);
      } else if (choice == 1) {
        wantSacredWater = true;
        setState(STATE_CONFIRMING);
        drawConfirmScreen(selectedSlot, wantSacredWater);
      } else if (choice == 2) {  // R-153: Back → return to product selection
        setState(STATE_PRODUCT_SELECTION);
        drawProductSelection(selectedSlot);
      }
      if (elapsed > 10000) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── CONFIRMING ────────────────────────────────────────────────────────────────
    // R-153: order summary screen. createOrder() not called until Confirm is touched.
    case STATE_CONFIRMING: {
      if (elapsed < 300) break;  // entry guard — prevent carry-over touch from gift option
      int action = getTouchedConfirm();
      if (action == 1) {        // Confirm
        _proceedToPayment();
      } else if (action == -1) {  // Back
        setState(STATE_GIFT_OPTION);
        drawGiftOptionScreen(selectedSlot);
      }
      if (elapsed > 30000) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── AWAITING_PAYMENT ───────────────────────────────────────────────────────────
    case STATE_AWAITING_PAYMENT: {
      static unsigned long lastTimerUpdate = 0;
      if (millis() - lastTimerUpdate >= PAYMENT_TIMER_INTERVAL) {
        lastTimerUpdate = millis();
        int secsLeft = (int)((PAYMENT_TIMEOUT - (millis() - paymentStartMs)) / 1000);
        if (secsLeft < 0) secsLeft = 0;
        updateQrTimer(secsLeft);
      }
      if (elapsed > PAYMENT_TIMEOUT) {
        Serial.println("[STATE] Payment timeout");
        reportCompletion(currentOrderId, false, selectedSlot);
        setState(STATE_ERROR);
        drawErrorScreen("Payment timeout\nPlease try again");
        delay(3000);
        setState(STATE_IDLE);
        drawIdleScreen();
        break;
      }
      if (millis() - paymentPollTimer > PAYMENT_POLL_INTERVAL) {
        paymentPollTimer = millis();
        String status = checkPaymentStatus(currentOrderId);
        if (status == "paid") {
          Serial.println("[STATE] Payment confirmed via poll");
          _onPaymentConfirmed();
        }
      }
      break;
    }

    // ── VENDING ────────────────────────────────────────────────────────────────────────
    // R-128/R-129: vendProduct() runs synchronously — enters and exits this state within
    // _onPaymentConfirmed(). By the time loop() sees STATE_VENDING it is already done.
    case STATE_VENDING: {
      break;
    }

    // ── COMPLETING ─────────────────────────────────────────────────────────────────────
    case STATE_COMPLETING: {
      if (elapsed > 6000) {
        currentOrderId    = "";
        currentQrData     = "";
        currentAmount     = 0;
        selectedSlot      = -1;
        wantSacredWater   = false;
        setState(STATE_IDLE);
        idleAnimationUI();
        idleAnimation();
        drawIdleScreen();
      }
      break;
    }

    // ── SERVICE ────────────────────────────────────────────────────────────────────────
    case STATE_SERVICE: {
      if (checkServiceExit()) {
        setState(STATE_IDLE);
        g_idleDrawn = false;
        drawIdleScreen();
        break;
      }

      int newTab = getTouchedServiceTab();
      if (newTab >= 0 && newTab != g_service_tab) {
        g_service_tab = newTab;
        drawServiceScreen(g_service_tab);
        break;
      }

      int action = getTouchedServiceContent(g_service_tab);
      if (action > 0) {
        if (action == 401) {
          // Factory reset — must call backend first (R-74)
          svcLog("Calling backend factory-reset...");
          bool ok = factoryResetBackend();
          if (ok) {
            svcLog("Backend OK — wiping NVS...");
            delay(500);
            _clearNVS();
            delay(200);
            ESP.restart();
          } else {
            svcLog("Backend refused reset — NVS untouched");
          }
        } else if (action == 402) {
          // Toggle boot PIN
          Preferences prefs;
          prefs.begin("satu", false);
          bool cur = prefs.getBool("boot_pin", false);
          prefs.putBool("boot_pin", !cur);
          prefs.end();
          svcLog(String("Boot PIN: ") + (!cur ? "ENABLED" : "DISABLED"));
          delay(1000);
          drawServiceScreen(g_service_tab);
        } else if (action >= 301 && action <= 321) {
          int slotToTest = action - 301;
          svcLog("Testing slot " + String(slotToTest + 1) + "...");
          if (g_service_tab == TAB_SELFTEST) {
            vendProduct(slotToTest);
          } else if (g_service_tab == TAB_FREEPLAY) {
            bool ok = vendProduct(slotToTest);
            svcLog("Free play: slot " + String(slotToTest + 1) + " - " + (ok ? "OK" : "EMPTY"));
          }
        } else if (action == 500) {
          svcLog("Running Quick Test (10 items)...");
          _runSelfTest(SVC_BODY_X, false);
          drawServiceScreen(TAB_SELFTEST);
          svcLog("Quick Test complete");
        } else if (action == 501) {
          svcLog("Running Technical Test (14 items)...");
          _runSelfTest(SVC_BODY_X, true);
          drawServiceScreen(TAB_SELFTEST);
          svcLog("Technical Test complete");
        } else if (action == 502) {
          _stm = 0; _stN = 0;
          drawServiceScreen(TAB_SELFTEST);
          svcLog("Results cleared");
        } else if (action == 600) {
          svcLog("Testing backend...");
          sendHeartbeat();
          bool online = (WiFi.status() == WL_CONNECTED);
          svcLog(online ? "Backend OK" : "Backend UNREACHABLE");
        } else if (action >= 601 && action <= 612) {
          int relayNum = action - 600;  // 1-indexed
          _relState[relayNum] = !_relState[relayNum];
          setRelay(relayNum, _relState[relayNum]);
          svcLog("R" + String(relayNum) + (_relState[relayNum] ? " ON" : " OFF"));
          drawServiceScreen(TAB_DEVICES);
        } else if (action == 700) {
          Preferences prefs;
          prefs.begin("satu", false);
          int vol = prefs.getInt("vol", 50);
          vol = (vol + 10);
          if (vol > 100) vol = 0;
          prefs.putInt("vol", vol);
          prefs.end();
          svcLog("Volume: " + String(vol) + "%");
          drawServiceScreen(TAB_SETTINGS);
        } else if (action == 800) {
          {
            uint8_t mac[6];
            esp_read_mac(mac, ESP_MAC_WIFI_STA);
            Serial.println("[SVC] ===== DEVICE INFO =====");
            Serial.printf("[SVC] Device ID : %s\n",  g_deviceId.c_str());
            Serial.printf("[SVC] MAC       : %02X:%02X:%02X:%02X:%02X:%02X\n",
                          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            Serial.printf("[SVC] Firmware  : %s  Build: %s %s\n",
                          FW_VERSION, __DATE__, __TIME__);
            Serial.printf("[SVC] Free heap : %lu B  PSRAM: %lu B\n",
                          ESP.getFreeHeap(), ESP.getFreePsram());
            Serial.println("[SVC] ======================");
          }
          svcLog("Printed to serial");
        }
      }
      break;
    }

    // ── ERROR ─────────────────────────────────────────────────────────────────────────
    case STATE_ERROR: {
      if (elapsed > 10000) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── OFFLINE ─────────────────────────────────────────────────────────────────────
    case STATE_OFFLINE: {
      if (elapsed > 30000) {
        if (WiFi.status() == WL_CONNECTED) {
          setState(STATE_IDLE);
          drawIdleScreen();
        } else {
          stateStartTime = millis();
        }
      }
      break;
    }

    default:
      break;
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  HELPER: proceed to payment
// ════════════════════════════════════════════════════════════════════════════
void _proceedToPayment() {
  String qrUrl  = "";
  int    amount = 0;
  bool   ordered = createOrder(selectedSlot, wantSacredWater, qrUrl, amount, currentOrderId);

  if (!ordered || qrUrl.isEmpty()) {
    setState(STATE_ERROR);
    drawErrorScreen("Cannot create order\nPlease try again");
    delay(3000);
    setState(STATE_IDLE);
    drawIdleScreen();
    return;
  }

  currentQrData    = qrUrl;
  currentAmount    = amount;
  paymentPollTimer = millis();
  paymentStartMs   = millis();

  setState(STATE_AWAITING_PAYMENT);
  drawQrScreen(currentQrData, currentAmount, selectedSlot);
  Serial.printf("[STATE] Order %s — %d THB — water=%d\n",
                currentOrderId.c_str(), currentAmount, wantSacredWater);
}

// ════════════════════════════════════════════════════════════════════════════
//  HELPER: payment confirmed  (R-131: banner · R-128/R-129: synchronous vend)
// ════════════════════════════════════════════════════════════════════════════
void _onPaymentConfirmed() {
  showPaymentAccepted();           // R-131: 1.5s green banner on QR screen
  setState(STATE_VENDING);
  drawVendingScreen(selectedSlot);
  if (wantSacredWater) {
    setRelay(RELAY_PUMP, true);
    delay(3000);
    setRelay(RELAY_PUMP, false);
  }
  bool dropped = vendProduct(selectedSlot);  // R-128/R-129: synchronous, returns true=item detected
  if (dropped) _onItemDropped();
  else         _onLaneEmpty(selectedSlot);
}

// ════════════════════════════════════════════════════════════════════════════
//  HELPER: item dropped (sensor fired)  — R-128
// ════════════════════════════════════════════════════════════════════════════
void _onItemDropped() {
  int lucky = 10 + (int)(esp_random() % 90);  // 10-99, hardware RNG
  setState(STATE_COMPLETING);
  drawCompletionScreen(selectedSlot, lucky, wantSacredWater);
  reportCompletion(currentOrderId, true, selectedSlot);
  Serial.printf("[STATE] Complete — slot=%d lucky=%d water=%d\n",
                selectedSlot + 1, lucky, wantSacredWater);
}

// ════════════════════════════════════════════════════════════════════════════
//  HELPER: lane empty (sensor never fired, motor hit safety timeout)  — R-128
// ════════════════════════════════════════════════════════════════════════════
void _onLaneEmpty(int lane) {
  Serial.printf("[STATE] Lane %d EMPTY — disabling\n", lane + 1);
  if (lane >= 0 && lane < NUM_SLOTS) g_slots[lane].enabled = false;
  reportCompletion(currentOrderId, false, lane);
  drawErrorScreen("Slot " + String(lane + 1) + " is empty\nPlease contact staff");
  delay(4000);
  setState(STATE_IDLE);
  drawIdleScreen();
}

// ════════════════════════════════════════════════════════════════════════════
//  COMMAND HANDLER
// ════════════════════════════════════════════════════════════════════════════
void handleCommands() {
  CommandList cmds = pollCommands();

  for (int i = 0; i < cmds.count; i++) {
    String cmd = cmds.commands[i];
    Serial.printf("[CMD] %s\n", cmd.c_str());

    if (cmd == "payment_confirmed") {
      if (currentState == STATE_AWAITING_PAYMENT) {
        _onPaymentConfirmed();
      }
    }
    else if (cmd == "disable") {
      setState(STATE_ERROR);
      drawErrorScreen("Machine disabled\nContact admin");
    }
    else if (cmd == "enable") {
      if (currentState == STATE_ERROR || currentState == STATE_OFFLINE) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
    }
    else if (cmd == "reboot") {
      delay(500);
      ESP.restart();
    }
    else if (cmd == "nuke") {
      // Wipe NVS and restart (admin emergency command)
      Serial.println("[CMD] NUKE: wiping NVS");
      drawErrorScreen("Factory reset\nErasing device...");
      delay(1000);
      _clearNVS();
      delay(200);
      ESP.restart();
    }
    else if (cmd == "reload_slots") {
      JsonDocument doc;
      reloadHello(doc);
      if (doc.containsKey("slots")) {
        loadSlotsFromJson(doc["slots"].as<JsonArray>());
        g_idleDrawn = false;
        if (currentState == STATE_IDLE) drawIdleScreen();
      }
    }
    else {
      Serial.printf("[CMD] Unknown: %s\n", cmd.c_str());
    }
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  TIMERS
// ════════════════════════════════════════════════════════════════════════════
void runTimers() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (currentState != STATE_OFFLINE && currentState != STATE_SERVICE) {
      setState(STATE_OFFLINE);
      drawErrorScreen("No internet connection");
    }
    return;
  } else if (currentState == STATE_OFFLINE) {
    setState(STATE_IDLE);
    drawIdleScreen();
  }

  if (now - lastHeartbeatMs > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeatMs = now;
  }

  if (now - lastCommandPollMs > COMMAND_POLL_INTERVAL) {
    handleCommands();
    lastCommandPollMs = now;
  }
}
