# CC_PROMPT_fix_service_ux_v1.md
> Created by: Chat — 2026-06-22
> Session goal: Fix service menu UX — touch responsiveness + self test auto-run + result persistence + relay I2C hang
> Repo: Satu-Vending-Firmware
> Mode: Fix — firmware only
> Files touched: firmware/ui.h + firmware/ui_service.h ONLY
> Flash cycles: 1
> PR target: main
> Sequence: Prompt 1 of 1 — self-contained fix

---

## 1. CC INTRO

Read and execute: CC_PROMPT_fix_service_ux_v1.md
New session. Ignore all previous context from other projects.
Repo: https://github.com/Csmittee/Satu-Vending-Firmware

Read IN FULL before touching anything — state each filename aloud after reading:
1. CLAUDE.md
2. RULES.md
3. CC_SKILL.md
4. firmware/ui.h         ← read completely
5. firmware/ui_service.h ← read completely

State: "All 5 files read ✅" before writing a single line.

---

## 2. CONTEXT

Four UX problems confirmed via serial monitor evidence and owner QA on 2026-06-22.
Root causes are distinct — all fixes are in ui.h and ui_service.h only.
satu_vending.ino is NOT touched — action handlers 500-800 already correct.
hardware.h is R2 LOCKED — NOT touched under any circumstances.

### Problem 1 — Self test auto-runs on service mode entry (CONFIRMED via serial)
Serial log shows [SVC] FAIL: MCP1/MCP2 lines immediately on STATE 2→13 transition,
with no button tap by owner. _runSelfTest() is being called inside _drawSvcBody_SelfTest()
or triggered on entry. MCP hardware is not physically wired — I2C times out per item.
Each timeout = ~1 second × multiple items = multi-second CPU block on entry.
This is the PRIMARY cause of the frozen feeling on first entry.

### Problem 2 — Three raw _touch.read() calls per loop in STATE_SERVICE (CONFIRMED)
getTouchedServiceTab(), checkServiceExit(), getTouchedServiceContent() each call
_touch.read() independently. GT911 clears interrupt flag after first read — reads 2
and 3 see nothing. Touch landing on read 2 or 3 is silently consumed. Symptom:
random response — sometimes fast, sometimes requires double-tap. R-150 already
solved this for idle state with touchReadOnce(). Service state was never ported.

### Problem 3 — Self test results persist across service mode exit/re-entry (CONFIRMED)
_stm and _stN are file-scope statics — they survive tab switches and service mode
exit. Owner expects a clean slate on re-entry. Results should auto-clear when
leaving TAB_SELFTEST or exiting service mode entirely.

### Problem 4 — Relay toggle hangs when MCP not wired (CONFIRMED)
Action 601-612 calls setRelay() → MCP I2C write → hardware timeout if MCP not
connected. Same I2C block class as Problem 1. Devices tab button taps feel slow
for same reason. Guard needed: if MCP not connected, skip I2C write and log warning.

---

## 3. NEW FILES

NONE.

---

## 4. TASKS

### TASK 1 — Remove self test auto-run on tab entry (ui_service.h)

Find _drawSvcBody_SelfTest(int bodyX).
CC reads the live function to confirm whether _runSelfTest() is called inside it
or whether it is called from elsewhere on TAB_SELFTEST entry.

**Fix:** _drawSvcBody_SelfTest() must ONLY draw the buttons and results area.
It must NEVER call _runSelfTest() itself.
If _runSelfTest() is called anywhere other than action handlers 500/501 in
satu_vending.ino — remove that call. Do not touch satu_vending.ino.
After fix: entering service mode → Self Test tab draws "Tap Quick or Technical
Test to begin." prompt only. No I2C activity on entry.

### TASK 2 — Replace raw _touch.read() with touchReadOnce() in service touch functions (ui.h)

Find these three functions in ui.h:
- getTouchedServiceTab()
- checkServiceExit()
- getTouchedServiceContent()

Each currently calls _touch.read() independently.

**Fix:** Replace _touch.read() in ALL THREE functions with touchReadOnce().
Then read _touch.isTouched and _touch.points[0] directly — touchReadOnce()
populates the same _touch object, no other change needed.

Pattern (same as existing idle touch functions):
```cpp
// BEFORE:
_touch.read();
if (!_touch.isTouched) return -1;

// AFTER:
touchReadOnce();
if (!_touch.isTouched) return -1;
```

Apply this pattern to all three functions. Debounce guards (_lastSvcTabMs,
_lastSvcCntMs, _lastExitMs) stay unchanged.

### TASK 3 — Auto-clear self test results on tab leave or service exit (ui_service.h + ui.h)

_stm and _stN are the self test state variables (0=idle, 1=quick results, 2=tech).
They persist across tab switches and service mode exit.

**Fix part A:** In drawServiceScreen(int tab) in ui.h — add before the switch(tab):
```cpp
if (tab != TAB_SELFTEST && _stm != 0) {
  _stm = 0; _stN = 0;
}
```
This clears results when switching away from Self Test tab.

**Fix part B:** In satu_vending.ino STATE_SERVICE — checkServiceExit() fires and
calls setState(STATE_IDLE). CC must NOT touch satu_vending.ino.
Instead: expose a reset function in ui_service.h:
```cpp
void resetSelfTestResults() { _stm = 0; _stN = 0; }
```
Then call resetSelfTestResults() from drawServiceScreen() when tab == TAB_SELFTEST
is being drawn after service mode re-entry. The simplest approach: always reset
_stm/_stN when entering TAB_SELFTEST from a fresh service mode open.
CC reads the live code and determines the cleanest implementation that does not
touch satu_vending.ino.

### TASK 4 — Guard relay toggle against I2C hang when MCP not wired (ui.h)

In getTouchedServiceContent() — action 601-612 path — CC reads the live handler
in satu_vending.ino (read-only, do not modify).

The relay toggle action calls setRelay() via the action handler in satu_vending.ino.
CC cannot modify that handler. Instead: guard the MCP availability via _relState[]
behaviour — if g_mcp1_ok and g_mcp2_ok are both false, _svcLogPanel() a warning
instead of proceeding. This requires adding a check in _getTouchedServiceExtra()
in ui_service.h for relay action codes 601-612:

```cpp
// In _getTouchedServiceExtra(), relay cell hit detected:
if (!g_mcp1_ok && !g_mcp2_ok) {
  _svcLogPanel("MCP not connected — relay toggle skipped");
  return 0;  // suppress action — do not return 601-612
}
// else return relay action code as normal
```

This prevents the action code from reaching satu_vending.ino's setRelay() call
when hardware is not wired. When MCP is wired and g_mcp1_ok/g_mcp2_ok are true,
relay toggles work normally.

---

## 5. DO NOT TOUCH

- hardware.h — R2 LOCKED — never open, modify, or redeclare anything it owns
- satu_vending.ino — action handlers 500-800 already correct, do not touch
- state_machine.h — do not touch
- network.h — do not touch
- config.h — do not touch
- satu-system-tester.html — R-94 LOCKED never modify
- src/ backend files — do not touch
- wrangler.toml — do not touch
- schema.sql — do not touch
- PAYMENT_MODE — stays fake
- _runSelfTest() logic — do not change test items, labels, or pass/fail logic
- Debounce values (_lastSvcTabMs 150ms, _lastSvcCntMs 200ms, _lastExitMs 200ms) — keep as is

---

## 6. VERIFICATION

Before closing PR, CC confirms each item:

- [ ] Enter service mode → Self Test tab draws buttons only — NO [SVC] lines in Serial
- [ ] Tap Quick Test button → _runSelfTest() runs, results appear
- [ ] Switch to any other tab → results clear (_stm=0)
- [ ] Exit service mode and re-enter → Self Test tab shows "Tap to begin" — clean slate
- [ ] Tap sidebar tabs rapidly × 5 — all register within 1-2 taps, no 10-second freeze
- [ ] Devices tab relay cell tap with MCP not wired → [SVC] log shows "MCP not connected" — no hang
- [ ] getTouchedServiceTab() uses touchReadOnce() — confirmed in diff
- [ ] checkServiceExit() uses touchReadOnce() — confirmed in diff
- [ ] getTouchedServiceContent() uses touchReadOnce() — confirmed in diff
- [ ] hardware.h not in changed files list
- [ ] satu_vending.ino not in changed files list
- [ ] CI green before merge

---

## 7. MANDATORY CLOSING

1. Append CC_CHAT_LOG.md at TOP — format per CC_SKILL.md:
   - State all 4 fixes applied
   - State which touch functions were updated
   - Flag: owner must flash and QA — tap all 5 tabs, confirm no freeze, enter/exit service twice

2. Archive this prompt → docs/prompts/CC_PROMPT_fix_service_ux_v1.md
   stamped: ✅ COMPLETE — 2026-06-22 — service UX fix (touch/self-test/relay guard)

3. Append to RULES.md at TOP (next R-number after R-167) + version bump:
   ```
   R-168: SERVICE MODE TOUCH — all three service touch functions must use touchReadOnce()
   not raw _touch.read(). getTouchedServiceTab(), checkServiceExit(),
   getTouchedServiceContent() share one GT911 read per loop tick via touchReadOnce().
   Raw _touch.read() in service state consumes touch events randomly — root cause of
   intermittent double-tap requirement. Same principle as R-150 (idle state). (2026-06-22)

   R-169: SELF TEST MUST NEVER AUTO-RUN ON TAB ENTRY (2026-06-22).
   _drawSvcBody_SelfTest() draws UI only — never calls _runSelfTest().
   _runSelfTest() is called only from action handlers 500 (Quick) and 501 (Technical).
   Auto-run on entry causes I2C timeout block when MCP hardware not connected.
   Self test results auto-clear when leaving TAB_SELFTEST or re-entering service mode.

   R-170: MCP I2C GUARD IN SERVICE MODE (2026-06-22).
   _getTouchedServiceExtra() must check g_mcp1_ok / g_mcp2_ok before returning
   relay action codes 601-612. If both false: log warning, return 0 (suppress action).
   Prevents setRelay() I2C timeout hang when hardware not physically wired.
   ```

4. Update PROJECT_STATE.md — newest session at top + version bump

5. Bump version header on every file changed this session

6. Commit all in order → merge to main

7. Delete this prompt from repo root after archiving to docs/prompts/

---

## 8. PAYMENT MODE REMINDER

PAYMENT_MODE must remain = fake for this entire session.
Never suggest changing to live.
