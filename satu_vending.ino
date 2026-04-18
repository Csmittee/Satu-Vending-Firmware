#include "config.h"
#include "state_machine.h"
#include "hardware.h"
#include "network.h"
#include "ui.h"

void setup() {
  Serial.begin(115200);
  
  initHardware();
  initWiFi();
  initUI();
  
  setState(STATE_IDLE);
  idleAnimation();
  
  Serial.println("[SYSTEM] Satu vending machine ready");
}

void loop() {
  // Run state machine
  switch(currentState) {
    case STATE_IDLE:
      // Check for ID card
      break;
    case STATE_PRODUCT_SELECTION:
      // Check touch
      int product = getTouchedProduct();
      if (product >= 0) {
        selectedProduct = product;
        setState(STATE_AUTHENTICATING);
      }
      break;
    // ... other states
  }
  
  // Update UI based on state
  updateUIBasedOnState();
  
  // Send heartbeat periodically
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
  
  delay(10);
}

void updateUIBasedOnState() {
  switch(currentState) {
    case STATE_IDLE:
      drawIdleScreen();
      break;
    case STATE_PRODUCT_SELECTION:
      drawProductSelection();
      break;
    case STATE_AWAITING_PAYMENT:
      drawQrScreen(currentQrData, 10);
      break;
    case STATE_VENDING:
      drawVendingScreen(selectedProduct);
      break;
    case STATE_COMPLETING:
      drawCompletionScreen();
      break;
    case STATE_ERROR:
      drawErrorScreen("Transaction failed");
      break;
  }
}
