// ============================================================
// SATU VENDING MACHINE — Main Firmware
// Board: ESP32-S3 (ESP32-8048S070C), 7" capacitive touch
// Flash: 16MB | PSRAM: 8MB
// ============================================================
// CHANGE LOG:
//   R2 — Fixed: variable declaration inside switch/case (compile error)
//         Added: full state machine with all transitions
//         Added: command polling every 30s
//         Added: heartbeat every 5min (HEARTBEAT_INTERVAL)
//         Added: NVS-based device_id + device_secret persistence
// ============================================================

#include "config.h"
#include "state_machine.h"
#include "hardware.h"
#include "network.h"
#include "ui.h"

// ── State machine globals (definitions, not just declarations) ──────────────
MachineState currentState   = STATE_STARTUP;
MachineState lastState      = STATE_STARTUP;
unsigned long stateStartTime = 0;
String currentOrderId       = "";
int selectedProduct         = -1;
String currentUserId        = "";
int laneErrorCount[10]      = {0};
bool laneDisabled[10]       = {false};

// ── QR / payment globals (shared with ui.h) ─────────────────────────────────
String currentQrData        = "";
int currentAmount           = 0;

// ── Timing ──────────────────────────────────────────────────────────────────
static unsigned long lastHeartbeatMs  = 0;
static unsigned long lastCommandPollMs = 0;
static unsigned long lastUiRefreshMs  = 0;
static unsigned long paymentPollTimer = 0;

#define COMMAND_POLL_INTERVAL 30000   // 30 seconds
#define UI_REFRESH_INTERVAL   1000    // UI redraws max 1/sec to avoid flicker
#define PAYMENT_POLL_INTERVAL 3000    // Poll payment status every 3s

// ── setState implementation ──────────────────────────────────────────────────
void setState(MachineState newState) {
  lastState      = currentState;
  currentState   = newState;
  stateStartTime = millis();
  Serial.printf("[STATE] %d → %d\n", (int)lastState, (int)newState);
}

// ── Forward declarations ─────────────────────────────────────────────────────
void handleCommands();
void runStateMachine();
void runTimers();

// ════════════════════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[SATU] Booting...");

  // 1. Hardware first (relays, sensors, LEDs)
  initHardware();
  Serial.println("[SATU] Hardware OK");

  // 2. Display — show boot screen while WiFi connects
  initUI();
  drawErrorScreen("Connecting to WiFi...");   // reuse error screen as status
  Serial.println("[SATU] Display OK");

  // 3. Network — connects WiFi, sends /hello, stores device_id + device_secret in NVS
  initWiFi();                                 // blocks until connected (or reboots after retries)
  Serial.println("[SATU] Network OK");

  // 4. Boot animation + idle
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
      // Waiting for user to touch the screen (product selection)
      // Touch is detected in getTouchedProduct() which reads TFT_eSPI touch
      // In this architecture, touching the screen while on the idle screen
      // transitions directly to PRODUCT_SELECTION.
      // The drawIdleScreen() is called once on state entry (see runTimers UI refresh).
      int touched = getTouchedProduct();
      if (touched >= 0) {
        selectedProduct = touched;
        setState(STATE_PRODUCT_SELECTION);
        drawProductSelection();
      }
      break;
    }

    // ── PRODUCT_SELECTION ────────────────────────────────────────────────
    // NOTE: On this board, the "idle screen" IS the product grid.
    // STATE_PRODUCT_SELECTION handles the confirmation / second tap.
    case STATE_PRODUCT_SELECTION: {
      int touched = getTouchedProduct();
      if (touched >= 0) {
        // Check if lane is disabled
        if (laneDisabled[touched]) {
          drawErrorScreen("Lane " + String(touched + 1) + " unavailable");
          delay(2000);
          setState(STATE_IDLE);
          drawIdleScreen();
          break;
        }
        selectedProduct = touched;
        setState(STATE_AWAITING_PAYMENT);

        // Create order on backend → returns QR code URL and amount
        String qrUrl  = "";
        int amount    = 0;
        bool ordered  = createOrder(selectedProduct, qrUrl, amount, currentOrderId);

        if (!ordered || qrUrl.isEmpty()) {
          Serial.println("[STATE] Order creation failed");
          setState(STATE_ERROR);
          drawErrorScreen("ไม่สามารถสร้าง QR ได้\nกรุณาลองใหม่");
          delay(3000);
          setState(STATE_IDLE);
          drawIdleScreen();
          break;
        }

        currentQrData  = qrUrl;
        currentAmount  = amount;
        paymentPollTimer = millis();
        drawQrScreen(currentQrData, currentAmount);
        Serial.printf("[STATE] Order %s created, amount %d THB\n",
                      currentOrderId.c_str(), currentAmount);
      }

      // Back-to-idle timeout (user walked away without selecting)
      if (elapsed > 60000) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── AWAITING_PAYMENT ─────────────────────────────────────────────────
    case STATE_AWAITING_PAYMENT: {
      // Payment confirmed via command poll (see handleCommands) OR
      // we poll /order/{id}/status here as a fallback.

      if (elapsed > PAYMENT_TIMEOUT) {
        Serial.println("[STATE] Payment timeout");
        setState(STATE_ERROR);
        drawErrorScreen("หมดเวลาชำระเงิน\nกรุณาเริ่มใหม่");
        delay(3000);
        setState(STATE_IDLE);
        drawIdleScreen();
        break;
      }

      // Poll payment status every 3 seconds (fallback to webhook/command path)
      if (millis() - paymentPollTimer > PAYMENT_POLL_INTERVAL) {
        paymentPollTimer = millis();
        String status = checkPaymentStatus(currentOrderId);
        if (status == "paid") {
          Serial.println("[STATE] Payment confirmed via status poll");
          setState(STATE_VENDING);
          drawVendingScreen(selectedProduct);
          vendProduct(selectedProduct);
        }
      }
      break;
    }

    // ── VENDING ──────────────────────────────────────────────────────────
    case STATE_VENDING: {
      // vendProduct() fires the relay and waits for the item to drop
      // Transition happens in STATE_WAITING_DROP
      setState(STATE_WAITING_DROP);
      break;
    }

    // ── WAITING_DROP ─────────────────────────────────────────────────────
    case STATE_WAITING_DROP: {
      // Wait for IR sensor to confirm item has dropped
      bool dropped = readSensor(selectedProduct);  // sensor triggers LOW when item passes
      if (!dropped) {
        // Item detected (sensor triggered)
        setState(STATE_DISPENSING);
        unlockDoor();
        Serial.println("[STATE] Item dropped, door unlocked");
      }

      if (elapsed > DROP_TIMEOUT) {
        laneErrorCount[selectedProduct]++;
        Serial.printf("[STATE] Drop timeout on lane %d (errors: %d)\n",
                      selectedProduct, laneErrorCount[selectedProduct]);
        if (laneErrorCount[selectedProduct] >= 3) {
          laneDisabled[selectedProduct] = true;
          Serial.printf("[STATE] Lane %d disabled after 3 errors\n", selectedProduct);
        }
        setState(STATE_ERROR);
        drawErrorScreen("กรุณาติดต่อเจ้าหน้าที่\nLane " + String(selectedProduct + 1) + " error");
        delay(3000);
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── DISPENSING ───────────────────────────────────────────────────────
    case STATE_DISPENSING: {
      // Door is open, waiting for user to take item
      setState(STATE_WAITING_REMOVAL);
      break;
    }

    // ── WAITING_REMOVAL ──────────────────────────────────────────────────
    case STATE_WAITING_REMOVAL: {
      // Wait for user to take item (IR clears) or timeout
      bool itemGone = readSensor(selectedProduct);  // HIGH = clear
      if (itemGone) {
        lockDoor();
        setState(STATE_COMPLETING);
        drawCompletionScreen();
        celebrationAnimation();
        reportCompletion(currentOrderId, true);
      }

      if (elapsed > REMOVAL_TIMEOUT) {
        lockDoor();
        Serial.println("[STATE] Removal timeout — locking door anyway");
        reportCompletion(currentOrderId, true);
        setState(STATE_COMPLETING);
        drawCompletionScreen();
      }
      break;
    }

    // ── COMPLETING ───────────────────────────────────────────────────────
    case STATE_COMPLETING: {
      // Show thank-you screen for 4 seconds then return to idle
      if (elapsed > 4000) {
        currentOrderId  = "";
        currentQrData   = "";
        currentAmount   = 0;
        selectedProduct = -1;
        setState(STATE_IDLE);
        idleAnimation();
        drawIdleScreen();
      }
      break;
    }

    // ── ERROR ─────────────────────────────────────────────────────────────
    case STATE_ERROR: {
      // Error screen shown by whoever entered this state.
      // Auto-recover after 10 seconds.
      if (elapsed > 10000) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
      break;
    }

    // ── OFFLINE ──────────────────────────────────────────────────────────
    case STATE_OFFLINE: {
      // Try reconnect every 30 seconds
      if (elapsed > 30000) {
        Serial.println("[STATE] Attempting WiFi reconnect...");
        initWiFi();
        if (WiFi.status() == WL_CONNECTED) {
          setState(STATE_IDLE);
          drawIdleScreen();
        } else {
          stateStartTime = millis();   // reset timer for next attempt
        }
      }
      break;
    }

    default:
      break;
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  COMMAND HANDLER
//  Called from runTimers() — processes commands from backend
// ════════════════════════════════════════════════════════════════════════════
void handleCommands() {
  CommandList cmds = pollCommands();

  for (int i = 0; i < cmds.count; i++) {
    String cmd = cmds.commands[i];
    Serial.printf("[CMD] Received: %s\n", cmd.c_str());

    if (cmd == "payment_confirmed") {
      // Backend tells us payment received (via webhook → command queue)
      if (currentState == STATE_AWAITING_PAYMENT) {
        Serial.println("[CMD] Payment confirmed via command");
        setState(STATE_VENDING);
        drawVendingScreen(selectedProduct);
        vendProduct(selectedProduct);
      }
    }
    else if (cmd == "disable") {
      Serial.println("[CMD] Machine disabled by admin");
      setState(STATE_ERROR);
      drawErrorScreen("เครื่องถูกปิดใช้งาน\nกรุณาติดต่อผู้ดูแล");
    }
    else if (cmd == "enable") {
      Serial.println("[CMD] Machine re-enabled by admin");
      if (currentState == STATE_ERROR || currentState == STATE_OFFLINE) {
        setState(STATE_IDLE);
        drawIdleScreen();
      }
    }
    else if (cmd == "reboot") {
      Serial.println("[CMD] Reboot command received");
      delay(1000);
      ESP.restart();
    }
    else if (cmd == "register") {
      // Machine has been claimed — update device status from NVS / local flag
      Serial.println("[CMD] Machine registered to temple owner");
      // Device is now 'active' — backend already updated status
      // No action needed; heartbeat will reflect new status
    }
    else {
      Serial.printf("[CMD] Unknown command: %s\n", cmd.c_str());
    }
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  TIMERS — heartbeat, command poll, WiFi watchdog
// ════════════════════════════════════════════════════════════════════════════
void runTimers() {
  unsigned long now = millis();

  // ── WiFi watchdog ─────────────────────────────────────────────────────
  if (WiFi.status() != WL_CONNECTED) {
    if (currentState != STATE_OFFLINE) {
      Serial.println("[NET] WiFi lost — entering OFFLINE state");
      setState(STATE_OFFLINE);
      drawErrorScreen("ไม่มีสัญญาณอินเตอร์เน็ต");
    }
    return;   // skip network calls while offline
  } else if (currentState == STATE_OFFLINE) {
    // Reconnected
    setState(STATE_IDLE);
    drawIdleScreen();
  }

  // ── Heartbeat ─────────────────────────────────────────────────────────
  if (now - lastHeartbeatMs > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeatMs = now;
  }

  // ── Command poll ──────────────────────────────────────────────────────
  if (now - lastCommandPollMs > COMMAND_POLL_INTERVAL) {
    handleCommands();
    lastCommandPollMs = now;
  }
}
