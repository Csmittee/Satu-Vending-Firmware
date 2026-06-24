# RULES.md — Satu 1.0 Universal Rules
> Version 2.7 — 2026-06-24
> Changes: R-172 to R-175 (D-11 Thai language support)
> Previous: v2.6 — 2026-06-22
> For domain rules: load `.claude/rules/RULES-[domain].md`
> Domain files: workflow · backend · firmware · hardware · security

---

- **R-175: STATE_WELCOME BOOT FLOW (2026-06-24 — D-11).**
  STATE_WELCOME is the boot entry state (after STATE_STARTUP and optional STATE_WIFI_SETUP).
  On boot: g_lang_th = g_lang_th_default → setState(STATE_WELCOME) → drawWelcomeScreen().
  In STATE_WELCOME: tap EN button → g_lang_th=false + redraw buttons (stay in STATE_WELCOME).
  Tap TH button → g_lang_th=true + redraw buttons (stay in STATE_WELCOME).
  Tap anywhere else → enter STATE_IDLE with current language.
  Idle timeout → reset g_lang_th=g_lang_th_default + redraw welcome screen.

- **R-174: printThai() HARDCODED RANGE — GFXfont.first/last ARE uint8_t (2026-06-24 — D-11).**
  Thai Unicode U+0E01–U+0E4C (values 3585–3660) overflow uint8_t (max 255).
  SarabanSubset.h GFXfont structs use first=0/last=75 (glyph array indices, not codepoints).
  printThai() hardcodes Thai Unicode range 0x0E01–0x0E4C — NEVER read from font->first/last.
  Glyph index = cp - 0x0E01. Combining marks (ั ิ ี etc.) have xAdvance=0 → overlay preceding consonant.

- **R-173: SarabanSubset.h INCLUDE POSITION (2026-06-24 — D-11).**
  SarabanSubset.h must be the FIRST include in ui.h include chain — before ui_strings.h.
  ui_strings.h has printThai() which references SarabanSubset_12/18/24pt font objects.
  Full chain (NON-NEGOTIABLE, R-171 updated): SarabanSubset.h → ui_strings.h → ui_keyboard.h → ui_screens.h → ui_service.h.
  All included at bottom of ui.h, after gfx and hardware objects are defined.

- **R-172: g_lang_th / g_lang_th_default OWNERSHIP (2026-06-24 — D-11).**
  Authoritative definitions live in ui_strings.h (bool g_lang_th = false; bool g_lang_th_default = false;).
  g_lang_th_default is extern'd in network.h and loaded by loadConfigFromNVS() from NVS key "lang".
  DO NOT redeclare either variable in ui.h — duplicate definition compile error.
  g_lang_th = session language flag (reset on each welcome screen). g_lang_th_default = NVS-persisted operator default.

---

- **R-171: UI.H INCLUDE CHAIN ORDER (2026-06-22 — D-10 ui.h split).**
  ui.h R6 include order is NON-NEGOTIABLE: ui_strings.h → ui_keyboard.h → ui_screens.h → ui_service.h.
  _fillRoundRect/_drawRoundRect defined in ui.h (before chain) — ui_keyboard.h depends on these.
  No file in the chain may #include ui.h. satu_vending.ino still only includes ui.h.
  Service orchestration (drawServiceScreen, getTouchedServiceTab, checkServiceExit, getTouchedServiceContent)
  lives in ui.h AFTER #include "ui_service.h" — depends on _stm/_stN/resetSelfTestResults() from ui_service.h.

---

- **R-170: MCP I2C GUARD IN SERVICE MODE (2026-06-22).**
  _getTouchedServiceExtra() must check g_mcp1_ok / g_mcp2_ok before returning relay action codes 601-612.
  If both false: log warning via _svcLogPanel(), return 0 (suppress action).
  Prevents setRelay() I2C timeout hang (~1s per item) when hardware not physically wired.

- **R-169: SELF TEST MUST NEVER AUTO-RUN ON TAB ENTRY (2026-06-22).**
  _drawSvcBody_SelfTest() draws UI only — never calls _runSelfTest().
  _runSelfTest() is called only from action handlers 500 (Quick) and 501 (Technical) in satu_vending.ino.
  Auto-run on entry causes I2C timeout block when MCP hardware is not connected.
  Self test results (_stm, _stN) auto-clear when leaving TAB_SELFTEST or re-entering service mode.
  _svcFreshEntry flag pattern: set in checkServiceExit(), cleared in drawServiceScreen() which calls resetSelfTestResults().

- **R-168: SERVICE MODE TOUCH — touchReadOnce() required (2026-06-22).**
  getTouchedServiceTab(), checkServiceExit(), getTouchedServiceContent() must all use touchReadOnce(),
  not raw _touch.read(). GT911 clears its interrupt flag after each I2C read — three independent reads
  per loop tick silently consume touches landing on reads 2 and 3.
  Same principle as R-150 (idle state). Debounce guards unchanged (150ms/200ms/200ms).

---

- **R-166: ONE PR OPEN AT A TIME (2026-06-21).**
  Never open a new CC session while a previous PR is unmerged.
  Sequence: CI green → owner merges → then next CC session starts.
  CC_CHAT_LOG, PROJECT_STATE, RULES are written every session without exception.
  Stacking PRs creates merge conflicts and splits doc history. One thread at a time.

- **R-167: CI FQBN must match owner Arduino IDE Tools menu exactly — last verified 2026-06-21 (2026-06-21).**
  CDCOnBoot=default (not cdc) — board uses hardware UART, Serial Monitor at /dev/cu.usbserial-1420 at 115200 baud.
  CDCOnBoot=cdc routes Serial to USB CDC virtual port → /dev/cu.usbmodem never appears → Serial Monitor silent.
  UploadProtocol is NOT a valid FQBN compile option — arduino-cli compile rejects it with "invalid option". Omit it.
  Any IDE board setting change → update compile-check.yml FQBN to match before next PR.
- **R-165: HARDWARE_SPEC.md v1.2 is hardware wiring source of truth from 2026-06-21 (2026-06-21).**
  Relay 12 = magnetic pin-lock solenoid (NOT spring flap). Two physical locks parallel-wired on relay 12.
  FLAP_PROXIMITY_MCP_PIN = 2 (MCP2 GPA2 — roller microswitch, CLOSED=LOW, INPUT_PULLUP).
  SPEAKER_PIN = 1 (GPIO1 — passive speaker PWM).
  E18-D80NK drop sensor is placeholder — confirm type before bracket fabrication.
  Model naming: 5×2=10 lanes, 5×3=15 lanes, 7×3=21 lanes (col×row format, never reversed).
  MCP3 address for 5×3/7×3 = TBD — owner confirms A0/A1/A2 jumper when parts arrive.

- **R-164: drawServiceScreen() must NEVER call fillScreen() — PSRAM bus contention on ESP32-8048S070C (2026-06-21).**
  fillScreen() writes 800×480 = 384,000 pixels into PSRAM frame buffer while LCD DMA reads the same buffer simultaneously.
  Body area clear only: fillRect(SVC_BODY_X, STATUS_H, SCR_W - SVC_BODY_X, SCR_H - STATUS_H, C_BG).
  Root cause: same class as R-117. See SKILL_esp32s3_rgb_panel_constraints.md.

- **R-163: DEVICES TAB GRID — driven by MACHINE_LANES in config.h (2026-06-20).**
  MACHINE_LANES = lane count for current hardware build: 10 (5×2), 15 (5×3), or 21 (7×3).
  Never hardcode relay or IR sensor counts in ui_service.h. Lane relays R1–MACHINE_LANES
  shown in grid (cols=5 if ≤10 lanes, cols=7 if 21 lanes). R11 (pump) and R12 (flap)
  always shown as fixed special relay cells below the lane relay grid, never in the lane grid.

- **R-162: SATU_ROADMAP.md IS THE PRODUCT DIRECTION SOURCE OF TRUTH (2026-06-20).**
  This file answers "where are we heading" — PROJECT_STATE.md answers "where are we now".
  They must mirror each other: roadmap sets direction, project state tracks position.
  Chat reads SATU_ROADMAP.md bullet summaries at every session open (mandatory).
  Full read required when: new firmware architecture, new screen design, commercial
  decision, SaaS direction, hardware model choice, or new repo created.
  CC updates SATU_ROADMAP.md when owner confirms a strategic decision.
  Never add status columns, progress tracking, or completion icons to SATU_ROADMAP.md.
  That belongs in PROJECT_STATE.md.

- **R-161: UI_SPEC.md IS THE UI SOURCE OF TRUTH (2026-06-20).**
  All font decisions, layout rules, screen inventory, service tab specs, and NVS keys
  live here. Any UI decision made in a Chat session must trigger a UI_SPEC.md update
  in the same CC PR as the UI code change. CC reads UI_SPEC.md before any ui.h or
  ui_service.h change. Chat flags owner when a UI decision is made in conversation
  without updating UI_SPEC.md — never silently skip the update.

- **R-160: HARDWARE_SPEC.md IS THE HARDWARE SOURCE OF TRUTH (2026-06-20).**
  Renamed from HARDWARE_TRUTH.md. Lives at hardware/HARDWARE_SPEC.md in firmware repo.
  All pin assignments, relay logic, sensor logic, BOM, and wiring decisions live here.
  Any hardware change must update this file in the same PR.
  CC reads hardware/HARDWARE_SPEC.md before any hardware.h or config.h read.
  Chat flags owner if a hardware decision is made in conversation without updating
  HARDWARE_SPEC.md.

- **R-158: UI PR CHECKLIST — mandatory before any PR touching a .h UI file (2026-06-20).**
  Before opening PR: (1) confirm all Y positions use _BDY-relative expressions, not hardcoded literals;
  (2) confirm no content drawn past x = SVC_BODY_X + _LOG_X_OFF (=670) in any service tab;
  (3) confirm _svcLogDraw() called at end of every _drawSvcBody_* function;
  (4) confirm all hit-tests in _getTouchedServiceExtra() match exact draw Y positions;
  (5) confirm lineH in result lists matches badge height (never leave clipped/overlapping rows).
  Failure to complete this checklist was the cause of 13 visual issues caught in owner photo QA session 2026-06-20.

- **R-157: CI ARTIFACT — 3 files required for full flash from scratch (updated 2026-06-21).**
  compile-check.yml: `--output-dir ./build` routes all build outputs to known path.
  `ls -la ./build/` step in CI confirms exact filenames in the run log.
  Artifact `satu-firmware-N` contains: `satu_vending.ino.bootloader.bin` + `satu_vending.ino.partitions.bin` + `satu_vending.ino.bin`.
  All 3 must be flashed — missing any = black screen. Port confirmed: /dev/cu.usbserial-1420. Baud: 460800.
  `esptool --chip esp32s3 --port /dev/cu.usbserial-1420 --baud 460800 write-flash 0x0 ~/satu-firmware/satu_vending.ino.bootloader.bin 0x8000 ~/satu-firmware/satu_vending.ino.partitions.bin 0x10000 ~/satu-firmware/satu_vending.ino.bin`
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
  Results stored in file-scope statics _stP[]/_stN — persist across tab redraws until next run or action 502 (clear).

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
