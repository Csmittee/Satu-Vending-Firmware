// ============================================================
// state_machine.h — Satu Vending Machine State Definitions
// ============================================================
// CHANGE LOG:
//   R3 — Added: STATE_GIFT_OPTION, STATE_SERVICE
//        Changed: laneErrorCount/laneDisabled arrays → NUM_SLOTS
//        Changed: selectedProduct extern → selectedSlot
//        Added: wantSacredWater extern
// ============================================================

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "config.h"   // for NUM_SLOTS

enum MachineState {
  STATE_STARTUP,
  STATE_IDLE,
  STATE_ID_SCANNING,        // waiting for ID card insert
  STATE_AUTHENTICATING,     // reading ID card
  STATE_PRODUCT_SELECTION,  // slot highlighted, waiting for confirm tap
  STATE_GIFT_OPTION,        // Item Only vs +Sacred Water choice   ← R3
  STATE_AWAITING_PAYMENT,   // QR shown, waiting for PromptPay
  STATE_VENDING,            // relay fired
  STATE_WAITING_DROP,       // IR waiting for item to drop
  STATE_DISPENSING,         // door unlocked, item in tray
  STATE_WAITING_REMOVAL,    // waiting for user to take item
  STATE_COMPLETING,         // lucky number + thank you screen
  STATE_ERROR,
  STATE_OFFLINE,
  STATE_SERVICE             // service / technician mode           ← R3
};

// ── Extern declarations (defined in satu_vending.ino) ───────────────────────
extern MachineState   currentState;
extern MachineState   lastState;
extern unsigned long  stateStartTime;
extern String         currentOrderId;
extern int            selectedSlot;       // R3: was selectedProduct
extern bool           wantSacredWater;    // R3: gift option flag
extern String         currentUserId;


// ── State transition helper (defined in satu_vending.ino) ────────────────────
void setState(MachineState newState);

#endif // STATE_MACHINE_H
