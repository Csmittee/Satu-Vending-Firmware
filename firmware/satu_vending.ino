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
// ============================================================

#include "config.h"
#include "state_machine.h"
#include "hardware.h"
#include "network.h"
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
}

// ── Forward declarations ──────────────────────────────────────────────────────
void handleCommands();
void runStateMachine();
void runTimers();
void _proceedToPayment();
void _onPaymentConfirmed();
void _onItemRemoved();

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
  Serial.println("\n[SATU] Booting R4...");

  initHardware();
  Serial.println("[SATU] Hardware OK");

  initUI();
  drawBootScreen("Connecting to WiFi...");
  Serial.println("[SATU] Display OK");

  JsonDocument helloDoc;
  initWiFi(helloDoc);

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

  // Ready
  setState(STATE_IDLE);
  idleAnimationUI();   // screen gold flash
  idleAnimation();     // LED breathing (hardware.h)
  drawIdleScreen();

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

    // ── IDLE ───────────────────────────────────────────────────────────────────────
    case STATE_IDLE: {
      // Debug screen: hold bottom-left (x<80, y>400) for 5 seconds
      {
        static unsigned long debugHoldStart = 0;
        static bool          debugHolding   = false;
        _touch.read();
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
      int choice = getTouchedGiftOption();
      if (choice == 0) {
        wantSacredWater = false;
        _proceedToPayment();
      } else if (choice == 1) {
        wantSacredWater = true;
        _proceedToPayment();
      }
      if (elapsed > 10000) {
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
    case STATE_VENDING: {
      setState(STATE_WAITING_DROP);
      break;
    }

    // ── WAITING_DROP ───────────────────────────────────────────────────────────────────
    case STATE_WAITING_DROP: {
      bool dropped = readSensor(selectedSlot);
      if (!dropped) {
        setState(STATE_DISPENSING);
        unlockDoor();
        Serial.println("[STATE] Item dropped, door unlocked");
      }
      if (elapsed > DROP_TIMEOUT) {
        laneErrorCount[selectedSlot]++;
        Serial.printf("[STATE] Drop timeout lane %d (errors: %d)\n",
                      selectedSlot, laneErrorCount[selectedSlot]);
        if (laneErrorCount[selectedSlot] >= 3) {
          laneDisabled[selectedSlot] = true;
          g_slots[selectedSlot].enabled = false;
        }
        reportCompletion(currentOrderId, false, selectedSlot);
        setState(STATE_ERROR);
        drawErrorScreen("Please contact staff\nSlot " + String(selectedSlot + 1) + " error");
        delay(3000);
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── DISPENSING ─────────────────────────────────────────────────────────────────────
    case STATE_DISPENSING: {
      setState(STATE_WAITING_REMOVAL);
      break;
    }

    // ── WAITING_REMOVAL ───────────────────────────────────────────────────────────────
    case STATE_WAITING_REMOVAL: {
      bool itemGone = readSensor(selectedSlot);
      if (itemGone) {
        lockDoor();
        _onItemRemoved();
      }
      if (elapsed > REMOVAL_TIMEOUT) {
        lockDoor();
        _onItemRemoved();
      }
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
            vendProduct(slotToTest);
          }
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
//  HELPER: payment confirmed
// ════════════════════════════════════════════════════════════════════════════
void _onPaymentConfirmed() {
  setState(STATE_VENDING);
  drawVendingScreen(selectedSlot);
  vendProduct(selectedSlot);
  if (wantSacredWater) {
    setRelay(RELAY_PUMP, true);
    delay(3000);
    setRelay(RELAY_PUMP, false);
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  HELPER: item removed from tray
// ════════════════════════════════════════════════════════════════════════════
void _onItemRemoved() {
  int lucky = 10 + (int)(esp_random() % 90);  // 10-99, hardware RNG
  setState(STATE_COMPLETING);
  drawCompletionScreen(selectedSlot, lucky, wantSacredWater);
  reportCompletion(currentOrderId, true, selectedSlot);
  Serial.printf("[STATE] Complete — lucky=%d water=%d\n", lucky, wantSacredWater);
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
