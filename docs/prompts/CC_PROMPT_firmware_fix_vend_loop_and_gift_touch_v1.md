# CC_PROMPT_firmware_fix_vend_loop_and_gift_touch_v1.md
> ✅ COMPLETE — 2026-06-19 — R-148 gift guard + R-149 vend loop poll
> Fix Mode — 2 bugs, 2 files only
> Repo: https://github.com/Csmittee/Satu-Vending-Firmware
> Created: 2026-06-19
> Flash cycles owner should expect: 1

---

## 1. CC INTRO

New session. Ignore all previous context from other projects.
Repo: https://github.com/Csmittee/Satu-Vending-Firmware

Read IN FULL before touching anything — state each filename aloud after reading:
1. CLAUDE.md
2. RULES.md
3. CC_SKILL.md
4. hardware.h         ← read entire file
5. satu_vending.ino   ← read entire file

State: "All 5 files read ✅" before writing a single line of code.

---

## 2. CONTEXT

Two firmware bugs confirmed from serial monitor evidence on SATU-4R473R (2026-06-19).

**Bug 1 — Gift option screen triggered by carry-over touch from product selection**
Serial evidence:
```
[UI] Touch: slot 0 (Small Amulet 100THB)
[UI] Product selection: slot 0
[STATE] 5 → 6          ← only 33ms later
[UI] Gift option screen: slot 0
```
Root cause: `getTouchedSlot()` has its own debounce (`lastTouchMs` static var).
`getTouchedGiftOption()` has a separate independent debounce (`lastGiftTouchMs` static var).
When the loop transitions STATE_PRODUCT_SELECTION → STATE_GIFT_OPTION in the same
loop() tick where the confirm tap registered, `lastGiftTouchMs` has NOT been set yet —
so the first poll of `getTouchedGiftOption()` in STATE_GIFT_OPTION accepts the still-active
finger immediately as a gift card tap. Screen jumps past gift option without user choice.

**Bug 2 — IR Sensor Triggered / dispense buttons have no effect during vend**
Serial evidence:
```
[HW] Relay 1 ON — motor SPINNING + flap UNLOCKED
[HW] Relay 1 OFF — SAFETY CUTOFF 30000ms — lane 1 EMPTY  ← 30s later
[CMD] sensor_triggered   ← arrived AFTER vend finished, too late
[CMD] Unknown: sensor_triggered  ← repeated 4 times
```
Root cause: `vendProduct()` in hardware.h runs a tight `while` spin loop using
only `delay(SENSOR_POLL_MS)`. The main `loop()` never runs during this 30-second block.
`handleCommands()` / `pollCommands()` is never called — all backend commands queue up
and drain only after `vendProduct()` returns. The `sensor_triggered` command was never
processed in time to stop the motor.

Consequence of Bug 2: motor hits VEND_MAX_SPIN_MS safety cutoff → `_onLaneEmpty()` fires
→ lane disabled → error screen shown. Lane is not actually empty. This is correct hardware
behaviour for an empty lane but wrong when sensor_triggered was queued but ignored.

---

## 3. NEW FILES

NONE.

---

## 4. TASKS

### TASK 1 — satu_vending.ino: Fix Bug 1 — gift screen entry guard

**File:** `satu_vending.ino`
**Location:** `STATE_GIFT_OPTION` case in `runStateMachine()`

**Root cause:** No entry guard. Touch detection begins the first loop() tick after state entry.
The same finger that confirmed the product is still making contact.

**Fix:** Add a minimum state-entry delay before accepting any gift option touch.
Use the existing `stateStartTime` (already set by `setState()`) and `elapsed` variable
(already computed at top of `runStateMachine()`).

**Current code (read from live file — do not assume line numbers):**
```cpp
case STATE_GIFT_OPTION: {
  int choice = getTouchedGiftOption();
  // ... rest of handler
```

**Replace with:**
```cpp
case STATE_GIFT_OPTION: {
  if (elapsed < 250) break;   // R-148: entry guard — ignore carry-over touch from product selection
  int choice = getTouchedGiftOption();
  // ... rest of handler — copy exactly, zero other changes
```

**Exact rule to add to RULES.md (see Section 7):**
```
R-148: STATE_GIFT_OPTION entry guard — ignore all touches for first 250ms after state entry.
       Prevents carry-over touch from STATE_PRODUCT_SELECTION confirm tap triggering gift card immediately.
       Implementation: if (elapsed < 250) break; — first line inside STATE_GIFT_OPTION case.
       elapsed = millis() - stateStartTime, already computed at top of runStateMachine().
```

**Verify after fix:**
Serial must show a visible gap (≥250ms) between:
`[UI] Product selection: slot N` and `[UI] Gift touch: Item Only` or `[UI] Gift touch: +Sacred Water`

---

### TASK 2 — hardware.h: Fix Bug 2 — poll commands inside vend spin loop

**File:** `hardware.h`
**Location:** `vendProduct()` — the `while (millis() - spinStart < VEND_MAX_SPIN_MS)` spin loop

**Root cause:** Spin loop calls only `readSensor()` and `delay(SENSOR_POLL_MS)`.
`pollCommands()` / `handleCommands()` are never called. Backend `sensor_triggered` command
is queued but not processed until after `vendProduct()` returns — up to 30 seconds later.

**Fix:** Add a periodic command poll inside the spin loop.
Poll every ~500ms (every 50 iterations of 10ms delay).
If any command string equals `"sensor_triggered"` — treat it identically to a real sensor fire:
set `sensorFired = true` and break.

**pollCommands() note:** `pollCommands()` is defined in `network.h` and returns a `CommandList`
struct. CC must verify the exact signature and return type from the live `network.h` file before
writing this code. Do NOT assume the function signature — read it first.

**Current spin loop structure (read from live file — do not assume line numbers):**
```cpp
while (millis() - spinStart < VEND_MAX_SPIN_MS) {
  if (readSensor(lane)) {
    sensorFired = true;
    Serial.printf("[HW] Sensor %d TRIGGERED ...", ...);
    break;
  }
  delay(SENSOR_POLL_MS);
}
```

**Replace with (adapt to exact signature CC reads from live files):**
```cpp
static unsigned long lastCmdPollMs = 0;

while (millis() - spinStart < VEND_MAX_SPIN_MS) {

  // Primary: real IR sensor
  if (readSensor(lane)) {
    sensorFired = true;
    Serial.printf("[HW] Sensor %d TRIGGERED — item detected after %lums\n",
                  lane + 1, millis() - spinStart);
    break;
  }

  // Secondary: backend sensor_triggered command (R-149)
  // Poll every 500ms to avoid I2C bus contention with sensor reads
  if (millis() - lastCmdPollMs >= 500) {
    lastCmdPollMs = millis();
    CommandList cmds = pollCommands();
    for (int i = 0; i < cmds.count; i++) {
      if (cmds.commands[i] == "sensor_triggered") {
        sensorFired = true;
        Serial.printf("[HW] sensor_triggered command received — stopping motor after %lums\n",
                      millis() - spinStart);
        break;
      }
    }
    if (sensorFired) break;
  }

  delay(SENSOR_POLL_MS);
}
```

**Critical constraints CC must respect:**
- `CommandList` struct and `pollCommands()` — read exact definition from `network.h` live file
- `cmds.commands[i]` — use whatever field name exists in the live `CommandList` struct
- Do NOT change `readSensor()` call — it is the primary stop condition
- Do NOT change `SENSOR_POLL_MS` — keep 10ms poll cadence for real sensor
- `lastCmdPollMs` must be `static` — persists across loop iterations, resets correctly per-call
  because it is function-local static (initialized to 0 on first vendProduct() call only)
- The `break` inside the for loop breaks the for loop — need outer `if (sensorFired) break` to
  also break the while loop. CC must verify the break-chain is correct before committing.
- Do NOT add `pollCommands()` anywhere else in hardware.h — only inside this spin loop
- hardware.h is R2 LOCKED for structure — this change adds code inside an existing function body
  only. No new functions, no new includes, no changes to function signatures.

**Exact rule to add to RULES.md (see Section 7):**
```
R-149: vendProduct() spin loop polls backend commands every 500ms (R-148: 2026-06-19).
       sensor_triggered command = treated as real IR sensor fire → sensorFired=true → motor stop.
       Poll cadence: 500ms (50 × SENSOR_POLL_MS). Real sensor still polled every 10ms — unchanged.
       pollCommands() called from hardware.h spin loop only — not from other hardware.h functions.
       Root cause this solves: vendProduct() blocks loop() for up to 30s — commands queue but never drain.
```

**Verify after fix:**
Serial must show motor stopping within ~1 second of clicking IR Sensor Triggered button:
```
[HW] Relay N ON — motor SPINNING + flap UNLOCKED
[HW] sensor_triggered command received — stopping motor after ~XXXms
[HW] Relay N OFF — motor stopped (sensor)
[HW] Flap re-locked via TIMEOUT (3000ms) ...
[HW] Flap LOCKED — pin extended
[STATE] Complete — slot=N ...
```
NOT the 30-second safety cutoff path.

---

## 5. DO NOT TOUCH

- `hardware.h` function signatures — read-only except the vendProduct() spin loop body (Task 2)
- `hardware.h` R2 LOCK applies to all other functions — only the spin loop body is modified
- `network.h` — read only (to confirm pollCommands() signature)
- `ui.h` — do not touch
- `config.h` — do not touch. RELAY_FLAP, VEND_MAX_SPIN_MS, SENSOR_POLL_MS unchanged
- `state_machine.h` — do not touch
- All states in `runStateMachine()` EXCEPT `STATE_GIFT_OPTION` case
- `getTouchedGiftOption()` in `ui.h` — do not modify the function itself
- `getTouchedSlot()` in `ui.h` — do not touch
- `handleCommands()` in `satu_vending.ino` — do not touch
- `_onLaneEmpty()` — do not touch
- `_onItemDropped()` — do not touch
- PAYMENT_MODE stays `fake`
- `satu-system-tester.html` — not a firmware file but explicitly: never touch

---

## 6. VERIFICATION

Before opening PR, CC must confirm:

1. **Bug 1 fix in place:** `if (elapsed < 250) break;` is the first executable line inside
   `STATE_GIFT_OPTION` case, before `getTouchedGiftOption()` is called ✅

2. **Bug 2 fix in place:** `pollCommands()` is called inside the `while` spin loop in
   `vendProduct()`, every 500ms, checking for `"sensor_triggered"` ✅

3. **Break-chain correct:** `sensorFired = true` inside for loop → outer `if (sensorFired) break`
   exits the while loop — verify both breaks are present ✅

4. **`lastCmdPollMs` is `static`** — not `unsigned long` on the stack ✅

5. **No changes outside the two specified locations** — diff must show only:
   - `satu_vending.ino`: one line added inside `STATE_GIFT_OPTION`
   - `hardware.h`: spin loop body inside `vendProduct()` expanded

6. **GitHub Actions compile check ✅ GREEN** before opening PR (R-90)
   If red: fix, push to same branch, wait for green — do NOT open PR on red

7. **PR title:** `fix: vend loop command poll (R-149) + gift screen entry guard (R-148)`

---

## 7. MANDATORY CLOSING

1. **CC_CHAT_LOG.md** — append entry at TOP, format per CC_SKILL.md:
   - State both bugs fixed: R-148 (gift guard) + R-149 (vend loop poll)
   - State files changed: satu_vending.ino + hardware.h only
   - State CI result: ✅ GREEN or ❌ RED (do not close without green)
   - State flash cycles expected: 1
   - Pending Chat verify: serial output confirms motor stops within ~1s of IR Trigger button

2. **RULES.md** — append at TOP (R-148 first, then R-149):
   ```
   R-149: vendProduct() spin loop polls backend commands every 500ms ...
   R-148: STATE_GIFT_OPTION entry guard — ignore touches for first 250ms ...
   ```
   (Use exact text from Task 1 and Task 2 rule blocks above.)
   Bump RULES.md version header.

3. **KNOWN_GOOD.md** — append at TOP:
   ```
   ## 2026-06-19 — Firmware R7 (R-148/R-149) — CI ⬜ pending
   - satu_vending.ino R7: STATE_GIFT_OPTION entry guard (R-148) — 250ms touch ignore on entry
   - hardware.h R7: vendProduct() spin loop polls pollCommands() every 500ms (R-149)
   - Files changed: satu_vending.ino, hardware.h only
   - CI: ⬜ GitHub Actions pending
   - Flash: ⬜ 1 cycle required — owner flash after CI green
   - Expected serial (IR trigger test):
     [HW] Relay N ON — motor SPINNING + flap UNLOCKED
     [HW] sensor_triggered command received — stopping motor after ~XXXms
     [HW] Relay N OFF — motor stopped (sensor)
     [HW] Flap re-locked via TIMEOUT (3000ms)
     [HW] Flap LOCKED — pin extended
     [STATE] Complete — slot=N ...
   ```

4. **PROJECT_STATE.md** — add session log entry (newest at top):
   ```
   ### 2026-06-19 — R-148 gift guard + R-149 vend loop command poll
   - Bug 1 fixed: carry-over touch from product selection no longer auto-selects gift option.
     Entry guard (250ms) added to STATE_GIFT_OPTION in satu_vending.ino.
   - Bug 2 fixed: sensor_triggered backend command now stops motor within ~1s.
     pollCommands() called every 500ms inside vendProduct() spin loop in hardware.h.
   - Files: satu_vending.ino, hardware.h
   - Rules: R-148, R-149
   - CI: pending. Flash: 1 cycle required.
   ```
   Bump PROJECT_STATE.md version header.

5. **Archive this prompt** → `docs/prompts/CC_PROMPT_firmware_fix_vend_loop_and_gift_touch_v1.md`
   stamped: `✅ COMPLETE — [date] — R-148 gift guard + R-149 vend loop poll`

6. Bump version header on every file changed.

7. Commit all docs + source → merge to main.

---

## 8. PAYMENT MODE REMINDER

PAYMENT_MODE must remain = `fake` for this entire session.
No backend files are touched in this session — firmware only.
Never suggest changing PAYMENT_MODE to live.
