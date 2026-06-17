# CC_BUILD_PROMPT_firmware_ux_r128_REVISED.md
> Created by: Chat (Claude) — 2026-06-17 (REVISED — replaces CC_BUILD_PROMPT_firmware_ux_r128.md)
> Session goal: Motor stop logic + pin-lock flap + UX font rules + payment accepted banner
> Repo: Satu-Vending-Firmware
> Mode: Firmware Mode
> Files changed: hardware.h (authorized), config.h, satu_vending.ino, ui.h, state_machine.h
> Flash cycles: 1 expected
> PR target: main
> Sequence: Prompt 1 of 2 — run BEFORE CC_BUILD_PROMPT_service_menu_v1.md

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
4. hardware.h    ← SPECIAL: partially unlocked for this prompt — see authorized changes below
5. satu_vending.ino
6. ui.h
7. state_machine.h

State the name of every file you read before writing a single line.
Then execute: CC_BUILD_PROMPT_firmware_ux_r128_REVISED.md
```

---

## CONTEXT

This prompt implements confirmed architectural decisions from owner QA session 2026-06-17.
All decisions are FINAL — do not reopen architecture questions.
Do not invent alternatives. Implement exactly as specified.

hardware.h is normally R2 LOCKED.
This prompt authorizes changes to hardware.h for these functions ONLY:
- vendProduct() — full rewrite (R-128)
- unlockFlap() — new function (R-129)
- lockFlap() — new function (R-129)
- Remove: unlockDoor(), lockDoor() (replaced by unlockFlap/lockFlap)
- Rename constant: RELAY_DOOR_LOCK → RELAY_FLAP
No other hardware.h changes permitted.

---

## ARCHITECTURE REFERENCE — READ BEFORE WRITING ANY CODE

### Flap mechanism (R-129 FINAL — pin-lock solenoid design)

The exit flap uses a solenoid PIN LOCK, not a magnetic lock and not a spring pulse.

```
Relay 12 HIGH = pin RETRACTED = flap UNLOCKED
Relay 12 LOW  = pin EXTENDED  = flap LOCKED (fail-secure, spring-held)
```

Fail-secure behaviour:
- Power OFF at any time → relay de-energizes → pin extends → flap locks
- Mid-vend power cut: pin extends but flap is past one-way mechanical stop →
  item still falls through → safe. Pin cannot re-engage until flap returns to rest.
- This is a HARDWARE guarantee, not a firmware concern.

Flap sequence during vend:
1. Payment confirmed → motor ON + unlockFlap() called SIMULTANEOUSLY (same line, same moment)
2. Flap stays unlocked for ENTIRE vend duration — it does NOT re-lock during spin
3. Item falls off coil → pushes flap open → flap springs back to rest position
4. IR sensor detects item passing → motor OFF (R-128)
5. Wait for proximity switch CLOSED (flap at rest) → lockFlap() immediately
6. Safety: if proximity never fires within FLAP_RELOCK_TIMEOUT → lockFlap() anyway + log warning
7. vendProduct() returns true

Proximity switch:
- Wired to MCP2 GPA2–GPA7 — exact pin TBD, defined as FLAP_PROXIMITY_MCP_PIN in config.h
- FLAP_PROXIMITY_MCP_PIN = -1 until owner assigns — stub safely (treat as never-fires → use timeout)
- When FLAP_PROXIMITY_MCP_PIN >= 0: read via mcp2.digitalRead(FLAP_PROXIMITY_MCP_PIN)
- CLOSED = LOW (same logic as IR sensors, INPUT_PULLUP)

One-way mechanical stop:
- Flap cannot be pushed back up from below
- Customer cannot force entry through exit door even when flap is unlocked
- This is purely mechanical — no firmware involvement

### Motor stop logic (R-128 FINAL)

Motor stops on IR sensor trigger. Never on a timer.
VEND_MAX_SPIN_MS = 30000ms safety cutoff only (not normal stop).
IR sensor reads every SENSOR_POLL_MS = 10ms during spin.
Lane empty = motor ran full 30s with no sensor → disable lane → POST error to backend.

### Font rule (R-137 NEW — applies to ALL text in ALL files)

Arduino_GFX 1.4.9 setTextSize() scales pixel fonts — produces blocky ugly output.
FreeSans fonts are proven working on SATU-4R473R hardware as of 2026-06-16.
Font files are already in firmware/ folder: FreeSansBold24pt7b.h, FreeSansBold18pt7b.h, FreeSansBold12pt7b.h

MANDATORY font usage table — apply to every screen, every new function:

| Text role | Font | setTextSize |
|-----------|------|-------------|
| Hero numbers (amounts, lucky number) | FreeSansBold24pt7b | 1 or 2 |
| Screen titles (large) | FreeSansBold18pt7b | 1 |
| Section headings, slot names | FreeSansBold12pt7b | 1 |
| Body text, labels, status | NULL (default pixel font) | 1 only |
| Never use | NULL font | > 1 ← FORBIDDEN except as absolute last resort |

After every setFont(&FreeSansXXX) call, reset with setFont(NULL) before body text.
Every screen in ui.h must follow this table from now on — not just newly written ones.
Check all existing screens and apply the table. If a screen uses setTextSize(2+) with
NULL font on visible text, replace with the appropriate FreeSans font at size 1.

Exception: single-char symbols (✓ ✗ icons drawn as text) may use NULL + setTextSize(2)
if no FreeSans alternative exists and the char is non-Latin.

---

## FIX 1 — config.h: replace timer constants, add flap constants

Remove from config.h:
```cpp
#define VEND_PULSE_MS    800
#define DROP_TIMEOUT    5000
#define REMOVAL_TIMEOUT 30000
```

Add to config.h:
```cpp
// ── Vend + Flap timing (R-128, R-129) ────────────────────────────────────
#define VEND_MAX_SPIN_MS      30000  // R-128: motor safety cutoff (30s = ~10 turns)
#define SENSOR_POLL_MS            10  // R-128: IR sensor read interval during spin
#define FLAP_RELOCK_TIMEOUT     3000  // R-129: max wait for proximity before force-lock

// ── Flap proximity switch (R-129) ─────────────────────────────────────────
// Wired to MCP2 GPA2–GPA7. Assign pin number (0–5 = GPA2–GPA7) when wired.
// -1 = not yet assigned. Firmware stubs safely: uses FLAP_RELOCK_TIMEOUT only.
#define FLAP_PROXIMITY_MCP_PIN   -1   // TODO: assign MCP2 GPA pin when wired

// ── Speaker (R-134) ────────────────────────────────────────────────────────
// TODO: assign GPIO when speaker is wired. -1 = not yet assigned.
#define SPEAKER_PIN              -1
```

Rename in config.h constants section:
```cpp
// Remove:
#define RELAY_DOOR_LOCK  12

// Add:
#define RELAY_FLAP       12   // R-129: solenoid pin lock. HIGH=unlocked, LOW=locked (fail-secure)
```

---

## FIX 2 — hardware.h: vendProduct() rewrite + flap functions (R-128, R-129)

### AUTHORIZED hardware.h changes — these functions ONLY:

#### Remove entirely:
```cpp
void unlockDoor() { ... }
void lockDoor()   { ... }
```

#### Add unlockFlap() and lockFlap():
```cpp
// unlockFlap() — R-129 solenoid pin lock
// HIGH = pin retracted = UNLOCKED. Call at same moment motor starts.
// Flap stays unlocked for entire vend — do NOT call lockFlap() until proximity fires.
void unlockFlap() {
  setRelay(RELAY_FLAP, true);
  Serial.println("[HW] Flap UNLOCKED — pin retracted");
  setLEDColor(ZONE_DOOR_START, ZONE_DOOR_END, CRGB::Green);
}

// lockFlap() — R-129 solenoid pin lock
// LOW = pin extended = LOCKED (spring-held, fail-secure).
// Call ONLY when proximity switch confirms flap is at rest, OR on FLAP_RELOCK_TIMEOUT.
void lockFlap() {
  setRelay(RELAY_FLAP, false);
  Serial.println("[HW] Flap LOCKED — pin extended");
  setLEDColor(ZONE_DOOR_START, ZONE_DOOR_END, CRGB(80, 56, 0));
}
```

#### Replace vendProduct() entirely:
```cpp
// vendProduct() — R-128 sensor-driven motor stop + R-129 pin-lock flap
//
// Sequence:
//   1. unlockFlap() + motor ON — simultaneous
//   2. Spin loop: read IR sensor every SENSOR_POLL_MS
//   3. Sensor fires → motor OFF
//   4. Wait for proximity switch CLOSED → lockFlap()
//   5. Safety: FLAP_RELOCK_TIMEOUT → lockFlap() if proximity never fires
//
// Returns: true  = item confirmed dropped (sensor triggered)
//          false = lane empty (VEND_MAX_SPIN_MS elapsed, no sensor)
//
// RELAY POLARITY NOTE:
//   Motor relays 1–10: HIGH=ON  LOW=OFF
//   RELAY_FLAP (12):   HIGH=UNLOCKED  LOW=LOCKED  ← INVERTED — do not confuse
bool vendProduct(int lane) {
  if (lane < 0 || lane > 9) {
    Serial.printf("[HW] vendProduct: invalid lane %d\n", lane);
    return false;
  }

  int relayNum = lane + 1;

  vendingAnimation(lane);

  // Step 1: unlock flap + start motor simultaneously
  unlockFlap();
  setRelay(relayNum, true);
  Serial.printf("[HW] Relay %d ON — motor SPINNING + flap UNLOCKED\n", relayNum);

  // Step 2: spin loop — sensor poll every SENSOR_POLL_MS
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

  // Step 3: stop motor
  setRelay(relayNum, false);
  if (sensorFired) {
    Serial.printf("[HW] Relay %d OFF — motor stopped (sensor)\n", relayNum);
  } else {
    Serial.printf("[HW] Relay %d OFF — SAFETY CUTOFF %dms — lane %d EMPTY\n",
                  relayNum, VEND_MAX_SPIN_MS, lane + 1);
  }

  // Step 4: wait for proximity switch → lockFlap()
  // Stub safely when FLAP_PROXIMITY_MCP_PIN = -1 (not yet wired)
  unsigned long flapStart = millis();
  bool proxFired = false;

#if FLAP_PROXIMITY_MCP_PIN >= 0
  while (millis() - flapStart < FLAP_RELOCK_TIMEOUT) {
    // CLOSED = LOW (INPUT_PULLUP, same as IR sensors)
    if (mcp2.digitalRead(FLAP_PROXIMITY_MCP_PIN) == LOW) {
      proxFired = true;
      Serial.printf("[HW] Proximity CLOSED — flap at rest after %lums\n",
                    millis() - flapStart);
      break;
    }
    delay(10);
  }
#endif

  // Step 5: lock flap — on proximity or timeout
  lockFlap();
  if (!proxFired) {
    Serial.printf("[HW] Flap re-locked via TIMEOUT (%dms) — proximity not wired or stuck\n",
                  FLAP_RELOCK_TIMEOUT);
  }

  return sensorFired;
}
```

#### Rename constant reference in hardware.h:
Any remaining reference to `RELAY_DOOR_LOCK` → replace with `RELAY_FLAP`.
Confirm the boot sequence in initMCP23017() sets RELAY_FLAP LOW (locked) on boot:
```cpp
// On boot: ensure flap is LOCKED (pin extended)
mcp2.digitalWrite(RELAY_MAP[RELAY_FLAP - 1].pin, RELAY_OFF);
Serial.println("[HW] MCP2 OK — flap LOCKED on boot");
```
Replace any old "Door locked" boot message with above.

---

## FIX 3 — state_machine.h: remove dead states

Remove from MachineState enum:
```cpp
STATE_WAITING_DROP,
STATE_DISPENSING,
STATE_WAITING_REMOVAL,
```

These states are eliminated. The entire dispense cycle now runs synchronously
inside vendProduct(). No state machine polling needed during vend.

---

## FIX 4 — satu_vending.ino: state machine + helpers rewrite

### Remove from runStateMachine() switch:
- case STATE_WAITING_DROP
- case STATE_DISPENSING
- case STATE_WAITING_REMOVAL

### Replace STATE_VENDING case:
```cpp
case STATE_VENDING: {
  // vendProduct() runs synchronously — motor + flap handled inside
  // This case is entered but exits immediately — _onPaymentConfirmed handles all
  break;
}
```

### Replace _onPaymentConfirmed():
```cpp
void _onPaymentConfirmed() {
  showPaymentAccepted();    // R-131: green banner 1.5s on QR screen
  setState(STATE_VENDING);
  drawVendingScreen(selectedSlot);

  // Sacred water runs BEFORE vend so pump doesn't compete with motor current
  if (wantSacredWater) {
    setRelay(RELAY_PUMP, true);
    delay(3000);
    setRelay(RELAY_PUMP, false);
  }

  // R-128 + R-129: sensor-driven motor + pin-lock flap — fully synchronous
  bool dropped = vendProduct(selectedSlot);

  if (dropped) {
    _onItemDropped();
  } else {
    _onLaneEmpty(selectedSlot);
  }
}
```

### Add _onItemDropped():
```cpp
// _onItemDropped() — R-128/R-129
// Sensor confirmed item passed IR beam. Flap already re-locked inside vendProduct().
// No STATE_WAITING_REMOVAL. Customer retrieves from exit — not our concern.
void _onItemDropped() {
  int lucky = 10 + (int)(esp_random() % 90);
  setState(STATE_COMPLETING);
  drawCompletionScreen(selectedSlot, lucky, wantSacredWater);
  reportCompletion(currentOrderId, true, selectedSlot);
  Serial.printf("[STATE] Complete — slot=%d lucky=%d water=%d\n",
                selectedSlot + 1, lucky, wantSacredWater);
}
```

### Add _onLaneEmpty():
```cpp
// _onLaneEmpty() — R-128
// Motor ran VEND_MAX_SPIN_MS with no sensor trigger → lane is empty.
// Disables lane in runtime config, reports to backend, shows error.
void _onLaneEmpty(int lane) {
  Serial.printf("[STATE] Lane %d EMPTY — disabling\n", lane + 1);

  if (lane >= 0 && lane < NUM_SLOTS) {
    g_slots[lane].enabled = false;
  }

  reportCompletion(currentOrderId, false, lane);

  drawErrorScreen("Slot " + String(lane + 1) + " is empty\nPlease contact staff");
  delay(4000);
  setState(STATE_IDLE);
  drawIdleScreen();
}
```

### Add forward declarations near top of satu_vending.ino (before setup()):
```cpp
void _onPaymentConfirmed();
void _onItemDropped();
void _onLaneEmpty(int lane);
```

---

## FIX 5 — ui.h: showPaymentAccepted() + font audit (R-131, R-137)

### Add showPaymentAccepted():
```cpp
// showPaymentAccepted() — R-131
// Called on QR screen immediately when payment confirmed.
// Shows green banner for 1500ms before transitioning to vending screen.
// Must not block touch — uses millis() loop.
void showPaymentAccepted() {
  // Draw green banner over bottom third of QR screen
  int bY = SCR_H - 120;
  gfx->fillRect(0, bY, SCR_W, 120, gfx->color565(10, 80, 20));
  gfx->drawRect(0, bY, SCR_W, 120, C_GREEN);

  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_GREEN); gfx->setTextSize(1);
  const char* msg = "Payment Accepted";
  gfx->setCursor(SCR_W/2 - 140, bY + 52);
  gfx->print(msg);
  gfx->setFont(NULL);

  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  const char* sub = "Dispensing your item...";
  gfx->setCursor(SCR_W/2 - strlen(sub)*6/2, bY + 82);
  gfx->print(sub);

  // R-132: non-blocking wait — poll touch every 16ms
  unsigned long t0 = millis();
  while (millis() - t0 < 1500) {
    _touch.read();
    delay(16);
  }
  Serial.println("[UI] Payment accepted banner shown");
}
```

### Font audit — apply R-137 to ALL existing screens:

Read every drawXxxScreen() function in ui.h.
For each function, identify every gfx->setTextSize() call:
- If setTextSize(2) or higher with NULL font on visible Latin text → replace with FreeSans
- Apply the font table from ARCHITECTURE REFERENCE above
- Reset to setFont(NULL) after every FreeSans section

Screens that were already updated in R-126 (FreeSansBold confirmed working):
- drawBootScreen() — FreeSansBold24pt7b ✅
- drawCompletionScreen() lucky number — FreeSansBold24pt7b ✅
- drawVendingScreen() title — FreeSansBold18pt7b ✅
- drawCompletionScreen() heading — FreeSansBold12pt7b ✅

Screens to audit and update in this prompt:
- drawIdleScreen() — check all text sizes
- drawQrScreen() — check amount display
- drawSelectScreen() — check product name display
- drawErrorScreen() — check title text
- drawSetupCodeScreen() — check code display
- drawWifiSetupScreen() — check any scaled text
- Any other screen in ui.h not listed above

Do NOT change text positions unless the font change makes text overflow screen bounds.
If repositioning is needed, adjust x/y to maintain visual alignment. Document changes.

---

## DO NOT TOUCH

- hardware.h: only the 4 authorized changes (vendProduct, unlockFlap, lockFlap, boot message)
- config.h: only the constants listed in FIX 1
- network.h: no changes
- NUM_SLOTS: defined in config.h only — never redefine in ui.h or anywhere else
- idleAnimation(): owned by hardware.h — do not merge with idleAnimationUI()
- FreeSansBold*.h files: do not modify — they are the font bitmaps
- PAYMENT_MODE: stays fake — never change

---

## VERIFICATION STEPS (CC confirms before declaring done)

**Serial event sequence for Test A (normal dispense) — must appear in this exact order:**
```
[HW] Relay N ON — motor SPINNING + flap UNLOCKED
[HW] Sensor N TRIGGERED — item detected after Xms
[HW] Relay N OFF — motor stopped (sensor)
[HW] Proximity CLOSED — flap at rest after Xms    ← shows when pin wired
     OR
[HW] Flap re-locked via TIMEOUT (3000ms)           ← shows when pin = -1 (stub)
[HW] Flap LOCKED — pin extended
[STATE] Complete — slot=N lucky=NN water=0
```

**Test B — Lane empty:**
Motor runs → SAFETY CUTOFF after 30000ms → Relay OFF → Flap LOCKED via TIMEOUT → error screen → lane disabled in grid.

**Test C — Idle animation touch:**
Single tap responds within 16ms. No double-tap needed.

**Test D — Font audit:**
No setTextSize(2+) with NULL font on any Latin text on any screen.
FreeSans used for all titles and hero numbers.

**Pre-flight checklist:**
- [ ] RELAY_DOOR_LOCK renamed to RELAY_FLAP everywhere (hardware.h, config.h)
- [ ] unlockDoor() / lockDoor() deleted, unlockFlap() / lockFlap() added
- [ ] vendProduct() returns bool
- [ ] RELAY polarity comment in vendProduct() is present — HIGH=UNLOCKED, LOW=LOCKED
- [ ] FLAP_PROXIMITY_MCP_PIN = -1 stubs safely (no crash, uses timeout)
- [ ] STATE_WAITING_DROP, STATE_DISPENSING, STATE_WAITING_REMOVAL removed from enum
- [ ] showPaymentAccepted() uses millis() loop (not delay)
- [ ] Font table applied to all screens — no NULL+setTextSize(2+) on Latin text
- [ ] setFont(NULL) reset called after every FreeSans block
- [ ] GitHub Actions compile GREEN before merge

---

## RULES TO ADD (append to RULES.md at TOP — use next available R-number after R-132)

```
R-137: Font rule — UNIVERSAL and PERMANENT (2026-06-17)
       Never use NULL font with setTextSize > 1 on any Latin text on any screen.
       FreeSansBold24pt7b = hero numbers + large titles (setTextSize 1 or 2)
       FreeSansBold18pt7b = screen titles (setTextSize 1)
       FreeSansBold12pt7b = section headings, slot names (setTextSize 1)
       NULL font setTextSize 1 = body text, labels, status lines only
       After every setFont(&FreeSansXXX) call, reset with setFont(NULL).
       Font files live in firmware/ — FreeSansBold24/18/12pt7b.h
       Proven on SATU-4R473R hardware 2026-06-16. Do not replace or question.

R-136: Flap re-lock trigger = proximity switch CLOSED → lockFlap() immediately.
       No timer-based re-lock in normal operation.
       FLAP_RELOCK_TIMEOUT = 3000ms safety only (proximity not wired or flap stuck).
       Log warning if timeout fires — means proximity pin needs wiring or flap is jammed.

R-135: Flap unlock and motor start are SIMULTANEOUS — same moment, same call block.
       Flap stays unlocked for entire vend duration.
       Do NOT re-lock flap during motor spin. Do NOT unlock flap after motor stops.

R-134: SPEAKER_PIN = -1 in config.h until GPIO assigned. NVS key "vol" saved regardless.
       Volume UI stubs gracefully when SPEAKER_PIN < 0.

R-129 REVISED (2026-06-17): Relay 12 = solenoid pin lock. INVERTED polarity vs motor relays.
       HIGH = pin retracted = UNLOCKED. LOW = pin extended = LOCKED (fail-secure, spring-held).
       unlockFlap() at payment confirmed. lockFlap() on proximity CLOSED or FLAP_RELOCK_TIMEOUT.
       Flap stays unlocked entire vend. One-way mechanical stop prevents forced re-entry.
       Power cut = fail-locked (safe — mechanical stop protects mid-vend state).
       FLAP_PROXIMITY_MCP_PIN on MCP2 GPA2-7 — TBD — stub with -1 until assigned.

R-128 CONFIRMED (2026-06-17): Motor stop = IR sensor triggered ONLY.
       VEND_MAX_SPIN_MS = 30000ms safety cutoff. SENSOR_POLL_MS = 10ms.
       vendProduct() returns bool. Lane empty → disable + log.
       VEND_PULSE_MS, DROP_TIMEOUT, REMOVAL_TIMEOUT deleted permanently.
       NON-NEGOTIABLE. Timer-based motor stop is permanently prohibited.
```

---

## MANDATORY END OF SESSION (R-84)

1. Archive this prompt → `docs/prompts/` stamped:
   `✅ COMPLETE — 2026-06-17 — R-128 sensor motor + R-129 pin-lock flap + R-131 banner + R-137 fonts`

2. Append rules R-128 through R-137 to RULES.md at TOP (newest first)

3. Update PROJECT_STATE.md — session log + mark firmware motor/flap/UX COMPLETE

4. Update KNOWN_GOOD.md at TOP:
   `2026-06-17 — Firmware R-128-137 — sensor motor + pin-lock flap + font audit + payment banner`

5. Commit: `feat: sensor motor R-128 + pin-lock flap R-129 + font audit R-137 + payment banner R-131`

6. Merge to main after GitHub Actions compile GREEN

**CHAT_HANDOFF.md is Chat's responsibility — CC must never write it.**

---

## PAYMENT MODE REMINDER
PAYMENT_MODE stays fake. Never suggest changing to live.
This prompt makes zero backend changes.

---
*Prompt 1 of 2 — run BEFORE CC_BUILD_PROMPT_service_menu_v1.md*
*Next: CC_BUILD_PROMPT_service_menu_v1.md — service mode 5 tabs*
