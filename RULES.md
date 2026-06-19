# RULES.md — Satu 1.0 Universal Rules
> Version 1.6 — 2026-06-19
> Changes: Added R-150 (idle touch single-read), R-151 (gift debounce reset), R-152 (PRODUCT_SELECTION_TIMEOUT)
> Previous: v1.5 — 2026-06-19
> For domain rules: load `.claude/rules/RULES-[domain].md`
> Domain files: workflow · backend · firmware · hardware · security

---

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
