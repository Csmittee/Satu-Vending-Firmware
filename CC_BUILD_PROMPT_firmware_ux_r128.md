# CC_BUILD_PROMPT_firmware_ux_r128.md
> Created by: Chat (Claude) — 2026-06-17
> Session goal: Motor stop logic rewrite + UX fixes + flap logic
> Repo: Satu-Vending-Firmware
> Mode: Firmware Mode — hardware.h + satu_vending.ino + ui.h
> Flash cycles: 1 expected
> PR target: main
> Sequence: Run AFTER wiring tab (CC_BUILD_PROMPT_wiring_tab_v2.md) is merged

---

## CC INTRO — PASTE THIS TO CC

```
New session. Ignore all previous context from other projects.

You are working on SATU 1.0 firmware at:
https://github.com/Csmittee/Satu-Vending-Firmware

Before doing anything else, read IN FULL:
1. CLAUDE.md
2. RULES.md
3. config.h
4. hardware.h    ← SPECIAL: partially unlocked for this prompt — see DO NOT TOUCH
5. satu_vending.ino
6. ui.h
7. state_machine.h

State the name of every file you read before writing a single line.
Then execute: CC_BUILD_PROMPT_firmware_ux_r128.md
```

---

## CONTEXT

This prompt implements 5 confirmed fixes from QA session 2026-06-17.
All decisions are final — do not reopen architecture questions.

**Critical rule for this prompt:**
hardware.h is normally R2 LOCKED. This prompt explicitly authorizes specific
changes to hardware.h only as listed in FIX 1 and FIX 2 below.
No other changes to hardware.h are permitted.

---

## FIX 1 — Motor stop logic rewrite (R-128)
**File: hardware.h**
**Authorized hardware.h change: vendProduct() function only**

### Current (WRONG — timer-based stop):
```cpp
void vendProduct(int lane) {
  vendingAnimation(lane);
  setRelay(relayNum, true);
  delay(VEND_PULSE_MS);        // ← timer stop — WRONG
  setRelay(relayNum, false);
  Serial.printf("[HW] Vend pulse complete: relay %d OFF\n", relayNum);
}
```

### Replace with (sensor-triggered stop):
```cpp
// vendProduct() — R-128 sensor-driven motor stop
// Motor runs until IR sensor detects item drop OR safety limit reached.
// Returns: true = item dropped (sensor triggered)
//          false = lane empty (safety limit reached, lane disabled)
bool vendProduct(int lane) {
  if (lane < 0 || lane > 9) {
    Serial.printf("[HW] vendProduct: invalid lane %d\n", lane);
    return false;
  }

  int relayNum = lane + 1;
  Serial.printf("[HW] Vending lane %d (relay %d) — sensor-driven\n", lane, relayNum);

  vendingAnimation(lane);

  setRelay(relayNum, true);
  Serial.printf("[HW] Relay %d ON — motor SPINNING\n", relayNum);

  unsigned long spinStart = millis();
  bool sensorFired = false;

  while (millis() - spinStart < VEND_MAX_SPIN_MS) {
    if (readSensor(lane)) {
      sensorFired = true;
      Serial.printf("[HW] Sensor %d TRIGGERED — item detected after %lums\n",
                    lane + 1, millis() - spinStart);
      break;
    }
    delay(SENSOR_POLL_MS);
  }

  setRelay(relayNum, false);

  if (sensorFired) {
    Serial.printf("[HW] Relay %d OFF — motor stopped (sensor)\n", relayNum);
    return true;
  } else {
    Serial.printf("[HW] Relay %d OFF — SAFETY CUTOFF after %dms — lane %d EMPTY\n",
                  relayNum, VEND_MAX_SPIN_MS, lane + 1);
    return false;
  }
}
```

### Add to config.h (replace VEND_PULSE_MS):
Remove:
```cpp
#define VEND_PULSE_MS  800
```

Add:
```cpp
#define VEND_MAX_SPIN_MS  30000   // R-128: motor safety cutoff (30s = ~10 turns at 90-180deg/sec)
#define SENSOR_POLL_MS        10  // R-128: how often sensor is read during motor spin
#define FLAP_PULSE_MS        300  // R-129: spring flap open duration (Relay 12)
```

Also remove from config.h:
```cpp
#define DROP_TIMEOUT    5000    // no longer needed — sensor fires during spin
#define REMOVAL_TIMEOUT 30000   // no longer needed — spring flap, no wait
```

---

## FIX 2 — Spring flap replaces door lock logic (R-129)
**File: hardware.h**
**Authorized hardware.h change: unlockDoor() / lockDoor() functions only**

### Remove both existing functions entirely:
```cpp
void unlockDoor() { ... }
void lockDoor()   { ... }
```

### Replace with:
```cpp
// openFlap() — R-129 spring flap
// Energizes Relay 12 briefly. Spring closes it automatically.
// Call only AFTER vendProduct() returns true (sensor confirmed drop).
void openFlap() {
  setRelay(RELAY_FLAP, true);
  Serial.println("[HW] Flap OPEN — relay 12 ON");
  setLEDColor(ZONE_DOOR_START, ZONE_DOOR_END, CRGB::Green);
  delay(FLAP_PULSE_MS);
  setRelay(RELAY_FLAP, false);
  Serial.println("[HW] Flap CLOSED — spring return");
  setLEDColor(ZONE_DOOR_START, ZONE_DOOR_END, CRGB(80, 56, 0));
}
```

### In hardware.h constants section, rename:
```cpp
// Remove:
#define RELAY_DOOR_LOCK   12

// Add:
#define RELAY_FLAP        12   // R-129: spring flap (pulse to open, spring closes)
```

---

## FIX 3 — State machine update for new motor + flap logic (R-128, R-129)
**File: satu_vending.ino**

### Update _onPaymentConfirmed():

Remove:
```cpp
void _onPaymentConfirmed() {
  setState(STATE_VENDING);
  drawVendingScreen(selectedSlot);
  vendProduct(selectedSlot);          // ← returns void, timer-based
  if (wantSacredWater) { ... }
}
```

Replace with:
```cpp
void _onPaymentConfirmed() {
  setState(STATE_VENDING);
  drawVendingScreen(selectedSlot);

  if (wantSacredWater) {
    setRelay(RELAY_PUMP, true);
    delay(3000);
    setRelay(RELAY_PUMP, false);
  }

  bool dropped = vendProduct(selectedSlot);  // R-128: sensor-driven, returns bool

  if (dropped) {
    // Item confirmed dropped — open flap briefly
    openFlap();                               // R-129: spring flap pulse
    _onItemDropped();
  } else {
    // Lane empty — disable and alert
    _onLaneEmpty(selectedSlot);
  }
}
```

### Add new helper _onItemDropped() (replaces _onItemRemoved()):
```cpp
// _onItemDropped() — R-129
// Called after sensor confirms drop AND flap has opened+closed.
// No waiting for item removal — flap handles containment, pickup door is passive.
void _onItemDropped() {
  int lucky = 10 + (int)(esp_random() % 90);
  setState(STATE_COMPLETING);
  drawCompletionScreen(selectedSlot, lucky, wantSacredWater);
  reportCompletion(currentOrderId, true, selectedSlot);
  Serial.printf("[STATE] Complete — lucky=%d water=%d\n", lucky, wantSacredWater);
}
```

### Add new helper _onLaneEmpty():
```cpp
// _onLaneEmpty() — R-128
// Called when motor ran VEND_MAX_SPIN_MS with no sensor trigger.
// Disables lane, reports to backend, shows error screen.
void _onLaneEmpty(int lane) {
  Serial.printf("[STATE] Lane %d EMPTY — disabling\n", lane + 1);

  // Disable in slot config
  if (lane >= 0 && lane < NUM_SLOTS) {
    g_slots[lane].enabled = false;
    laneDisabled[lane] = true;
  }

  // Report to backend
  reportError(currentOrderId, lane, "lane_empty");

  // Show error screen
  drawErrorScreen("Lane " + String(lane + 1) + " empty\nPlease contact staff");
  delay(4000);

  // Return to idle — lane will be greyed out in grid
  reportCompletion(currentOrderId, false, lane);
  setState(STATE_IDLE);
  drawIdleScreen();
}
```

### Remove STATE_WAITING_DROP, STATE_DISPENSING, STATE_WAITING_REMOVAL from state machine loop:
These states no longer exist in the dispense path. The entire dispense cycle
(motor spin + sensor wait + flap pulse) now happens synchronously inside
`_onPaymentConfirmed()` → `vendProduct()` → `openFlap()`.

In the runStateMachine() switch:
- Remove cases: STATE_WAITING_DROP, STATE_DISPENSING, STATE_WAITING_REMOVAL
- STATE_VENDING case now just calls setState(STATE_COMPLETING) — it transitions
  immediately because _onPaymentConfirmed() handles everything synchronously

### Add forward declaration for new helpers (near top of satu_vending.ino):
```cpp
void _onItemDropped();
void _onLaneEmpty(int lane);
```

### Add reportError() function (near reportCompletion):
```cpp
// reportError() — R-128 lane empty + relay stuck alerts
void reportError(String orderId, int lane, String errorType) {
  if (WiFi.status() != WL_CONNECTED) return;
  DynamicJsonDocument doc(256);
  doc["device_id"]   = g_deviceId;
  doc["order_id"]    = orderId;
  doc["lane"]        = lane + 1;
  doc["error_type"]  = errorType;
  String body;
  serializeJson(doc, body);
  Serial.printf("[NET] POST /v1/machine/error — %s lane %d\n",
                errorType.c_str(), lane + 1);
  // Use same HTTP pattern as reportCompletion() in network.h
  httpPost("/v1/machine/error", body);
}
```

### Update state_machine.h — remove dead states:
Remove from MachineState enum:
```cpp
STATE_WAITING_DROP,
STATE_DISPENSING,
STATE_WAITING_REMOVAL,
```
These states are no longer used. Removing keeps enum clean and prevents
accidental use in future code.

---

## FIX 4 — "Payment accepted" text on QR screen (R-131)
**File: ui.h**
**Function: updateQrTimer()**

The QR screen already has updateQrTimer() which patches the timer display.
Add a payment-accepted overlay drawn once when payment is confirmed.

Add new function to ui.h:
```cpp
// showPaymentAccepted() — R-131
// Called by _onPaymentConfirmed() BEFORE drawVendingScreen().
// Draws green "Payment Accepted" text over existing QR screen for 1.5s visual confirmation.
// Does NOT clear the screen — patches only the bottom instruction area.
void showPaymentAccepted() {
  // Draw green banner over the instruction area (below QR code)
  int bx = 20, by = SCR_H - 80, bw = SCR_W - 40, bh = 60;
  gfx->fillRoundRect(bx, by, bw, bh, 8, gfx->color565(0, 60, 10));
  gfx->drawRoundRect(bx, by, bw, bh, 8, C_GREEN);

  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_GREEN);
  gfx->setTextSize(1);
  const char* msg = "Payment Accepted";
  // Center text — FreeSansBold18pt7b ~12px per char at size 1
  gfx->setCursor(SCR_W/2 - 96, by + 40);
  gfx->print(msg);
  gfx->setFont(NULL);

  Serial.println("[UI] Payment accepted banner shown");
  delay(1500);   // Hold 1.5s so donor sees confirmation
}
```

In satu_vending.ino, update _onPaymentConfirmed() — add showPaymentAccepted()
as FIRST line before drawVendingScreen():
```cpp
void _onPaymentConfirmed() {
  showPaymentAccepted();          // R-131 — green banner on QR screen for 1.5s
  setState(STATE_VENDING);
  drawVendingScreen(selectedSlot);
  // ... rest of function
}
```

---

## FIX 5 — Idle touch wake-up fix (R-132)
**File: ui.h**
**Function: idleAnimationUI()**

### Problem:
Idle screen requires multiple taps to register. The animation loop blocks touch
polling. PR #25 fixed the animation `delay()` but idleAnimationUI() still has
internal timing that can miss a fast touch.

### Root cause to verify first:
Read idleAnimationUI() in ui.h. If it calls any `delay()` > 16ms anywhere
inside the animation loop — that is the blocker. Report what you find.

### Fix — non-blocking touch check pattern (R-126 extended):
Every delay() inside idleAnimationUI() must be replaced with a millis() loop
that checks touch every 16ms and returns immediately if touched.

Use this pattern for every wait segment:
```cpp
// Replace: delay(N);
// With:
unsigned long _t = millis();
while (millis() - _t < N) {
  if (_touch.read()) return;   // touch detected — exit animation immediately
  delay(16);
}
```

Apply this replacement to EVERY delay() call inside idleAnimationUI() and
any animation helper it calls (idleAnimation() in hardware.h is separate —
do not touch it unless it is called synchronously from idleAnimationUI()).

### Additional touch sensitivity fix:
In the STATE_IDLE case of runStateMachine(), ensure getTouchedSlot() is called
on every loop iteration with no blocking calls before it. If there is any
delay() or millis() gate before the touch check, remove it.

---

## SERIAL LOG IMPROVEMENTS (minor — part of R-130)
**File: hardware.h and satu_vending.ino**

These small additions improve visibility for future QA sessions.
Add these Serial.printf lines where noted:

1. In readSensor() — add after return:
   (No change needed — sensor trigger is now logged inside vendProduct() loop)

2. In openFlap() — already included in FIX 2 above. ✅

3. In _onPaymentConfirmed() — already logged via showPaymentAccepted() and
   vendProduct() internal logs. ✅

4. In reportError() — already logged. ✅

No other serial log changes needed.

---

## DO NOT TOUCH

- hardware.h — only vendProduct(), unlockDoor()/lockDoor() replacement, and
  RELAY_DOOR_LOCK → RELAY_FLAP rename are authorized. Nothing else.
- config.h — only the constant changes listed in FIX 1. Nothing else.
- network.h — do not modify. httpPost() must already exist. If it does not,
  use the same pattern as the existing POST calls in network.h.
- state_machine.h — only remove the 3 dead state enums listed in FIX 3.
- All screen draw functions not mentioned above — do not touch.
- PAYMENT_MODE — stays fake.
- wrangler.toml, schema.sql, backend files — wrong repo.

---

## VERIFICATION BEFORE PR

CC self-checks:
1. vendProduct() returns bool — not void
2. No reference to VEND_PULSE_MS anywhere in any file
3. No reference to DROP_TIMEOUT or REMOVAL_TIMEOUT anywhere
4. No reference to unlockDoor() or lockDoor() anywhere — replaced by openFlap()
5. No reference to RELAY_DOOR_LOCK — replaced by RELAY_FLAP
6. STATE_WAITING_DROP, STATE_DISPENSING, STATE_WAITING_REMOVAL removed from enum
7. _onItemRemoved() removed — replaced by _onItemDropped()
8. reportError() function exists and is called from _onLaneEmpty()
9. showPaymentAccepted() exists in ui.h and is called first in _onPaymentConfirmed()
10. No delay() > 16ms remains inside idleAnimationUI()
11. GitHub Actions compile check goes GREEN before PR is opened

---

## OWNER QA AFTER FLASH — WHAT TO CHECK

After flashing, run these tests and report serial output to Chat:

**Test A — Normal vend:**
1. Select product → payment → trigger paid
2. Expected serial: Relay ON → Sensor TRIGGERED → Relay OFF → Flap OPEN → Flap CLOSED
3. Expected screen: green "Payment Accepted" banner → Dispensing → Completion

**Test B — Lane empty simulation:**
1. Remove all items from one lane
2. Trigger purchase on that lane
3. Expected: motor runs ~30s → SAFETY CUTOFF log → lane greyed out on grid
4. Confirm lane stays greyed after return to idle

**Test C — Idle touch:**
1. Let machine idle for 30s (animation running)
2. Single tap on product
3. Expected: responds immediately on first tap, no double-tap needed

**Test D — Serial event sequence (must see all 4 in order):**
```
[HW] Relay N ON — motor SPINNING
[HW] Sensor N TRIGGERED — item detected after Xms
[HW] Relay N OFF — motor stopped (sensor)
[HW] Flap OPEN — relay 12 ON
[HW] Flap CLOSED — spring return
```

---

## MANDATORY END OF SESSION (R-84)

1. Archive this prompt → `docs/prompts/` stamped:
   `✅ COMPLETE — 2026-06-17 — firmware motor R-128 + flap R-129 + UX R-131 R-132`

2. Append to RULES.md at TOP:
   ```
   R-132: Idle animation must never block touch. Every delay() in idleAnimationUI()
          replaced with 16ms millis() loop that returns on touch. (2026-06-17)
   R-131: QR screen shows green "Payment Accepted" banner for 1.5s before
          transitioning to vending screen. showPaymentAccepted() in ui.h. (2026-06-17)
   R-130: Serial log must show sensor trigger as separate event line during motor spin.
          vendProduct() logs sensor trigger with timestamp. (2026-06-17)
   R-129: Spring flap replaces door lock. openFlap() pulses Relay 12 for FLAP_PULSE_MS.
          No STATE_WAITING_REMOVAL. No REMOVAL_TIMEOUT. Pickup door is passive. (2026-06-17)
   R-128: Motor stop is sensor-driven. vendProduct() returns bool.
          Motor runs until IR sensor triggers OR VEND_MAX_SPIN_MS exceeded.
          VEND_PULSE_MS deleted. DROP_TIMEOUT deleted. Lane empty → disable + POST error.
          Non-negotiable. Timer-based motor stop is permanently prohibited. (2026-06-17)
   ```

3. Update PROJECT_STATE.md — session log + mark firmware UX fixes COMPLETE

4. Update KNOWN_GOOD.md at TOP:
   `2026-06-17 — Firmware R-128-132 — sensor-driven motor + spring flap + UX fixes`

5. Commit: `feat: sensor-driven motor R-128 + spring flap R-129 + UX R-131-132`

6. Merge to main after GitHub Actions compile check GREEN

**CHAT_HANDOFF.md is Chat's responsibility — CC must never write it.**

---

## PAYMENT MODE REMINDER
PAYMENT_MODE stays fake. Never suggest changing to live.
This prompt makes zero backend changes.
