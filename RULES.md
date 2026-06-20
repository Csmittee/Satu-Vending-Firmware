# RULES.md — Satu 1.0 Universal Rules
> Version 1.9 — 2026-06-20
> Changes: Added R-158 (UI PR checklist) and R-157 (CI artifact upload rule)
> Previous: v1.8 — 2026-06-20
> For domain rules: load `.claude/rules/RULES-[domain].md`
> Domain files: workflow · backend · firmware · hardware · security

---

- **R-158: UI PR CHECKLIST — mandatory before any PR touching a .h UI file (2026-06-20).**
  Before opening PR: (1) confirm all Y positions use _BDY-relative expressions, not hardcoded literals;
  (2) confirm no content drawn past x = SVC_BODY_X + _LOG_X_OFF (=670) in any service tab;
  (3) confirm _svcLogDraw() called at end of every _drawSvcBody_* function;
  (4) confirm all hit-tests in _getTouchedServiceExtra() match exact draw Y positions;
  (5) confirm lineH in result lists matches badge height (never leave clipped/overlapping rows).
  Failure to complete this checklist was the cause of 13 visual issues caught in owner photo QA session 2026-06-20.

- **R-157: CI ARTIFACT — GitHub Actions saves compiled .bin as downloadable artifact (2026-06-20).**
  compile-check.yml: `--output-dir ./build` routes .bin to known path (without it: unpredictable /tmp/arduino-sketch-*/),
  then `actions/upload-artifact@v4` saves artifact `satu-firmware-N` (N = run number), retention 7 days.
  Flash: `esptool.py --chip esp32s3 --port /dev/cu.XXXX --baud 921600 write_flash 0x0 satu_vending.ino.bin`
  Never change FQBN, board config, or locked library versions in compile-check.yml.

- **R-156: SERVICE MODE DEVICES TAB — relay R12 display (2026-06-19).**
  Relay 12 (RELAY_FLAP) is the solenoid pin lock. Display shows LOCKED / UNLOCKED, NOT ON / OFF.
  LOCKED = relay OFF (pin extended, fail-secure on power loss) — cell colour red.
  UNLOCKED = relay ON (pin retracted, door can open) — cell colour green.
  Never show ON/OFF for R12 in any service UI.

- **R-155: SELF TEST TWO MODES (2026-06-19).**
  Quick (10 items): MCP1, MCP2, IR 1-10(sim), Relay 1-6(sim), Relay 7-12(sim), Flap(sim), Pump(sim), LEDs(sim), WiFi, Heap≥100KB.
  Technical (14 items): adds WS2812B GPIO5(sim), Display(sim), GT911(sim), NVS(sim), live backend heartbeat.
  Items marked (sim) = simulated pass — hardware not actually actuated during test.
  Live items: MCP1/MCP2 (real begin_I2C), WiFi (WL_CONNECTED), Heap (getFreeHeap), heartbeat (sendHeartbeat + WiFi proxy).
  Results stored in file-scope statics _stP[]/\_stN — persist across tab redraws until next run or action 502 (clear).

- **R-154: SERVICE MODE ACTION CODES RESERVED (2026-06-19).**
  500=Quick Self Test · 501=Technical Self Test · 502=Clear Results
  600=Test Backend · 601-612=Toggle Relay 1-12 · 700=Volume cycle · 800=Print to Serial
  Never reuse these codes for any other purpose.

- **R-152: PRODUCT_SELECTION_TIMEOUT in config.h.example (2026-06-19).**
  Default: 15 seconds. Controls product selection screen timeout before return to idle.
  ui.h g_cfg_sel initialises from this constant. Operator can override via NVS key cfg_sel.
  Never hardcode the value in ui.h.

- **R-151: STATE_GIFT_OPTION carry-over touch fix — two-layer defence (2026-06-19).**
  Layer 1: entry guard raised to 500ms (was 250ms). Layer 2: setState() calls
  resetGiftTouchDebounce() on STATE_GIFT_OPTION entry, setting _lastGiftTouchMs=millis().
  Without the reset, the static debounce stays at zero/old and passes immediately on re-entry.
  _lastGiftTouchMs lives at module scope in ui.h — NOT as a static local inside getTouchedGiftOption().

- **R-150: One GT911 touch read per idle tick — touchReadOnce() pattern (2026-06-19).**
  GT911 clears its interrupt flag after each I2C read. STATE_IDLE had 3 reads per tick:
  debug hold, checkServiceGesture(), getTouchedSlot(). First read consumed the tap; later
  reads returned isTouched=false → first 2 taps dropped, 3rd tap happened to be the only read.
  Fix: touchReadOnce() caches the read for 4ms. All idle-path functions call touchReadOnce()
  not _touch.read(). Functions outside the idle path (animation loops, keyboard) keep _touch.read().

- **R-149: vendProduct() spin loop polls backend commands every 500ms (2026-06-19).**
  sensor_triggered command = treated as real IR sensor fire → sensorFired=true → motor stop.
  Poll cadence: 500ms (50 × SENSOR_POLL_MS). Real sensor still polled every 10ms — unchanged.
  pollCommands() called from hardware.h spin loop only — not from other hardware.h functions.
  Root cause this solves: vendProduct() blocks loop() for up to 30s — commands queue but never drain.

- **R-148: STATE_GIFT_OPTION entry guard — ignore all touches for first 250ms after state entry (2026-06-19).**
  Prevents carry-over touch from STATE_PRODUCT_SELECTION confirm tap triggering gift card immediately.
  Implementation: `if (elapsed < 250) break;` — first line inside STATE_GIFT_OPTION case.
  elapsed = millis() - stateStartTime, already computed at top of runStateMachine().

- **R-147: THREE-FILE MACHINE BUILDER ARCHITECTURE — PERMANENT (2026-06-19):**
  satu-machine-builder.html = Section A (Single Flow) + Section B (Fleet)
  satu-hw-trigger.html      = Section C (HW Trigger) — standalone test tool
  satu-wiring.html          = Section D (Wiring + BOM) — standalone reference
  All 3 files: self-contained CSS+JS, gold/dark theme, sidebar nav with `<a href>` cross-links.
  Never merge these files back into one.
  satu-hw-trigger.html → /satu-hw-trigger · satu-wiring.html → /satu-wiring

- **R-146: DOCUMENT VERSIONING — PERMANENT (2026-06-18):**
  Every .md file must carry: `> Version X.Y — YYYY-MM-DD / Changes: [summary] / Previous: vX.Y`
  X.Y+1 = detail change · X+1.0 = new section or structure.
  Whoever last edited applies the bump. No file committed without a version bump if changed.

- **R-145: HTML FILE SIZE LIMIT — PERMANENT (2026-06-18):**
  Any HTML file in public/ exceeding 1000 lines: flag immediately, Chat proposes split plan,
  CC executes only after owner confirms. Each section → independent self-contained file.

- **R-144: CC_SKILL.md MANDATORY READ — PERMANENT (2026-06-18):**
  CC reads CC_SKILL.md every session alongside CLAUDE.md and RULES.md.
  CC_SKILL.md = session protocol + 6 skills + CC_CHAT_LOG write format + closing checklist.

- **R-143: CC_CHAT_LOG PROTOCOL — PERMANENT (2026-06-18):**
  CC appends one entry to CC_CHAT_LOG.md at TOP after every session. Max 10 lines per entry.
  Chat reads last 3 entries each session open. CC never deletes old entries.

---

## Universal — Apply to Every Session

1. **Never hardcode secrets** — always Cloudflare secrets manager
2. **Security = non-negotiable** — real money at religious institutions · flag immediately
3. **Full files only** — never partial snippets for critical files · complete replacement always
4. **Run 14-test suite** (satu-system-tester.html) after any backend change · all 14 must pass
5. **Document every decision** — handoff-ready at all times
6. **hardware.h R2 LOCKED** — never open, modify, or redeclare anything it owns
7. **PAYMENT_GATEWAY=fake_omise** for all dev/test — never suggest live without physical hardware
8. **Three-repo system** — read all three repos before any decision (detail → RULES-workflow R-83)
9. **Session closing** — archive → RULES.md → PROJECT_STATE.md → commit (detail → RULES-workflow R-84)
10. **No ghost devices** — only SATU-TEST001 (AA:BB:CC:DD:EE:00) + SATU-SIM01 (AA:BB:CC:DD:EE:01)

---

## Domain Rules Index

| Task | Load |
|------|------|
| Session structure · CC prompt · handoff | `.claude/rules/RULES-workflow.md` |
| Backend API · payment · D1 · rate limit | `.claude/rules/RULES-backend.md` |
| Firmware · Arduino · NVS · compile | `.claude/rules/RULES-firmware.md` |
| Hardware · wiring · relays · power | `.claude/rules/RULES-hardware.md` |
| Auth · secrets · ownership · legal | `.claude/rules/RULES-security.md` |
