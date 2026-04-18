#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

enum MachineState {
  STATE_STARTUP,
  STATE_IDLE,
  STATE_ID_SCANNING,
  STATE_AUTHENTICATING,
  STATE_PRODUCT_SELECTION,
  STATE_AWAITING_PAYMENT,
  STATE_VENDING,
  STATE_WAITING_DROP,
  STATE_DISPENSING,
  STATE_WAITING_REMOVAL,
  STATE_COMPLETING,
  STATE_ERROR,
  STATE_OFFLINE
};

extern MachineState currentState;
extern MachineState lastState;
extern unsigned long stateStartTime;
extern String currentOrderId;
extern int selectedProduct;
extern String currentUserId;
extern int laneErrorCount[10];
extern bool laneDisabled[10];

// State transition helper
void setState(MachineState newState);

#endif
