// ============================================================
// SATU VENDING MACHINE — Main Firmware
// Board: ESP32-S3 (ESP32-8048S070C), 7" capacitive touch
// Flash: 16MB | PSRAM: 8MB OPI — NEVER change PSRAM setting
// ============================================================
// CHANGE LOG:
//   R2 — Fixed compile error (variable declaration in switch/case)
//         Full state machine, command polling, heartbeat, NVS
//   R3 — ui.h rewrite: TFT_eSPI → Arduino_GFX
//         getTouchedProduct() → getTouchedSlot()
//         laneDisabled[] / laneErrorCount[] → NUM_SLOTS (21)
//         drawQrScreen() / drawVendingScreen() / drawCompletionScreen()
//           updated signatures
//         Added: STATE_GIFT_OPTION, STATE_ID_SCAN
//         Added: loadSlotsFromJson() called after /hello
//         Added: updateQrTimer() called every second in payment
//         sacredWater flag passed through to completion screen
//         Lucky number generated locally (random 10-99)
// ============================================================

#include "config.h"
#include "state_machine.h"
#include "hardware.h"
#include "network.h"
#include "ui.h"

// ── State machine globals ────────────────────────────────────────────────────
MachineState  currentState    = STATE_STARTUP;
MachineState  lastState       = STATE_STARTUP;
unsigned long stateStartTime  = 0;
String        currentOrderId  = "";
int           selectedSlot    = -1;      // R3: was selectedProduct
bool          wantSacredWater = false;   // R3: gift option flag
String        currentUserId   = "";

// ── Lane error tracking — now sized for NUM_SLOTS (max 21) ──────────────────
int  laneErrorCount[NUM_SLOTS] = {0};
bool laneDisabled[NUM_SLOTS]   = {false};

// ── QR / payment globals ─────────────────────────────────────────────────────
String currentQrData  = "";
int    currentAmount  = 0;

// ── Timing ──────────────────────────────────────────────────────────────────
static unsigned long lastHeartbeatMs   = 0;
static unsigned long lastCommandPollMs = 0;
static unsigned long paymentPollTimer  = 0;
static unsigned long paymentStartMs    = 0;   // for countdown timer

#define COMMAND_POLL_INTERVAL  30000   // 30 seconds
#define PAYMENT_POLL_INTERVAL   3000   // 3 seconds
#define PAYMENT_TIMER_INTERVAL  1000   // update countdown every 1s

// ── setState ─────────────────────────────────────────────────────────────────
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

// ════════════════════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[SATU] Booting R3...");

  // 1. Hardware (relays, sensors, LEDs)
  initHardware();
  Serial.println("[SATU] Hardware OK");

  // 2. Display — boot screen first, then WiFi
  initUI();
  drawBootScreen("Connecting to WiFi...");
  Serial.println("[SATU] Display OK");

  // 3. Network — WiFi connect + /hello (blocks until connected or reboots)
  //    initWiFi() populates g_deviceId, g_deviceSecret
  //    Returns JsonDocument with slots[] array from backend
  JsonDocument helloDoc;
  initWiFi(helloDoc);   // NOTE: network.h must pass hello response back

  // 4. Load slot config from /hello response
  if (helloDoc.containsKey("slots")) {
    loadSlotsFromJson(helloDoc["slots"].as<JsonArray>());
    Serial.println("[SATU] Slots loaded from backend");
  } else {
    Serial.println("[SATU] No slots in /hello — using defaults");
  }

  Serial.println("[SATU] Network OK");

  // 5. Boot animation + idle
  setState(STATE_IDLE);
  idleAnimation();
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

    // ── IDLE ─────────────────────────────────────────────────────────────
    case STATE_IDLE: {
      // Service mode: 3× tap top-right within 2 seconds
      if (checkServiceGesture()) {
        setState(STATE_SERVICE);
        // drawServiceScreen(); // implement in service mode session
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

    // ── PRODUCT_SELECTION ────────────────────────────────────────────────
    // First tap selects + highlights. Second tap confirms → gift option.
    // 3s timeout returns to idle.
    case STATE_PRODUCT_SELECTION: {
      int touched = getTouchedSlot();

      if (touched >= 0) {
        if (touched == selectedSlot) {
          // Second tap on same slot — confirm → gift option
          setState(STATE_GIFT_OPTION);
          wantSacredWater = false;
          drawGiftOptionScreen(selectedSlot);
        } else {
          // Tapped different slot — re-select
          selectedSlot = touched;
          drawProductSelection(selectedSlot);
          stateStartTime = millis();  // reset timeout
        }
      }

      // 5s idle → back to grid
      if (elapsed > 5000) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── GIFT_OPTION ───────────────────────────────────────────────────────
    // Donor chooses: Item Only (0) or +Sacred Water (1)
    case STATE_GIFT_OPTION: {
      int choice = getTouchedGiftOption();

      if (choice == 0) {
        // Item only → skip ID scan for now, go straight to payment
        wantSacredWater = false;
        Serial.println("[STATE] Gift: Item Only");
        _proceedToPayment();
      }
      else if (choice == 1) {
        // Sacred water → go to ID scan (or skip if no reader fitted)
        wantSacredWater = true;
        Serial.println("[STATE] Gift: +Sacred Water");
        _proceedToPayment();
      }

      // 10s timeout → back to idle
      if (elapsed > 10000) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── AWAITING_PAYMENT ──────────────────────────────────────────────────
    case STATE_AWAITING_PAYMENT: {
      // Update countdown timer every second
      static unsigned long lastTimerUpdate = 0;
      if (millis() - lastTimerUpdate >= PAYMENT_TIMER_INTERVAL) {
        lastTimerUpdate = millis();
        int secsLeft = (int)((PAYMENT_TIMEOUT - (millis() - paymentStartMs)) / 1000);
        if (secsLeft < 0) secsLeft = 0;
        updateQrTimer(secsLeft);
      }

      // Timeout
      if (elapsed > PAYMENT_TIMEOUT) {
        Serial.println("[STATE] Payment timeout");
        setState(STATE_ERROR);
        drawErrorScreen("Payment timeout\nPlease try again");
        delay(3000);
        setState(STATE_IDLE);
        drawIdleScreen();
        break;
      }

      // Poll backend every 3s as fallback (webhook/command is primary path)
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

    // ── VENDING ───────────────────────────────────────────────────────────
    case STATE_VENDING: {
      setState(STATE_WAITING_DROP);
      break;
    }

    // ── WAITING_DROP ──────────────────────────────────────────────────────
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
          // Also mark slot as disabled in g_slots so UI greys it out
          g_slots[selectedSlot].enabled = false;
          Serial.printf("[STATE] Slot %d disabled\n", selectedSlot);
        }
        setState(STATE_ERROR);
        drawErrorScreen("Please contact staff\nSlot " + String(selectedSlot + 1) + " error");
        delay(3000);
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── DISPENSING ────────────────────────────────────────────────────────
    case STATE_DISPENSING: {
      setState(STATE_WAITING_REMOVAL);
      break;
    }

    // ── WAITING_REMOVAL ───────────────────────────────────────────────────
    case STATE_WAITING_REMOVAL: {
      bool itemGone = readSensor(selectedSlot);
      if (itemGone) {
        lockDoor();
        _onItemRemoved();
      }
      if (elapsed > REMOVAL_TIMEOUT) {
        lockDoor();
        Serial.println("[STATE] Removal timeout — locking door");
        _onItemRemoved();
      }
      break;
    }

    // ── COMPLETING ────────────────────────────────────────────────────────
    case STATE_COMPLETING: {
      if (elapsed > 6000) {   // 6s on completion screen
        currentOrderId    = "";
        currentQrData     = "";
        currentAmount     = 0;
        selectedSlot      = -1;
        wantSacredWater   = false;
        setState(STATE_IDLE);
        idleAnimation();
        drawIdleScreen();
      }
      break;
    }

    // ── ERROR ─────────────────────────────────────────────────────────────
    case STATE_ERROR: {
      if (elapsed > 10000) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── OFFLINE ───────────────────────────────────────────────────────────
    case STATE_OFFLINE: {
      if (elapsed > 30000) {
        Serial.println("[STATE] Attempting reconnect...");
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
//  HELPER: proceed to payment (create order, show QR)
// ════════════════════════════════════════════════════════════════════════════
void _proceedToPayment() {
  String qrUrl = "";
  int    amount = 0;
  bool   ordered = createOrder(selectedSlot, wantSacredWater, qrUrl, amount, currentOrderId);

  if (!ordered || qrUrl.isEmpty()) {
    Serial.println("[STATE] Order creation failed");
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
//  HELPER: payment confirmed (from poll or command)
// ════════════════════════════════════════════════════════════════════════════
void _onPaymentConfirmed() {
  setState(STATE_VENDING);
  drawVendingScreen(selectedSlot);
  vendProduct(selectedSlot);
  if (wantSacredWater) {
    activateWaterPump();   // hardware.h — fires water pump relay
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  HELPER: item removed from tray
// ════════════════════════════════════════════════════════════════════════════
void _onItemRemoved() {
  int lucky = random(10, 100);   // lucky number 10-99
  setState(STATE_COMPLETING);
  drawCompletionScreen(selectedSlot, lucky, wantSacredWater);
  reportCompletion(currentOrderId, true);
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
        Serial.println("[CMD] Payment confirmed via command");
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
      Serial.println("[CMD] Rebooting...");
      delay(500);
      ESP.restart();
    }
    else if (cmd == "reload_slots") {
      // Backend has updated slot config — re-fetch via /hello
      Serial.println("[CMD] Reloading slot config...");
      JsonDocument doc;
      reloadHello(doc);   // network.h — re-calls /hello endpoint
      if (doc.containsKey("slots")) {
        loadSlotsFromJson(doc["slots"].as<JsonArray>());
        g_idleDrawn = false;   // force grid redraw
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

  // WiFi watchdog
  if (WiFi.status() != WL_CONNECTED) {
    if (currentState != STATE_OFFLINE) {
      setState(STATE_OFFLINE);
      drawErrorScreen("No internet connection");
    }
    return;
  } else if (currentState == STATE_OFFLINE) {
    setState(STATE_IDLE);
    drawIdleScreen();
  }

  // Heartbeat
  if (now - lastHeartbeatMs > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeatMs = now;
  }

  // Command poll
  if (now - lastCommandPollMs > COMMAND_POLL_INTERVAL) {
    handleCommands();
    lastCommandPollMs = now;
  }
}
