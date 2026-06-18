# RULES.md — Satu 1.0 Universal Rules
> Version 1.3 — 2026-06-18
> Changes: R-138 to R-141 added (CC_CHAT_LOG, CC_SKILL, HTML size limit, doc versioning)
> Previous: v1.2 — 2026-06-17
> For domain rules: load `.claude/rules/RULES-[domain].md`
> Domain files: workflow · backend · firmware · hardware · security

---

- **R-141: DOCUMENT VERSIONING — PERMANENT (2026-06-18):**
  Every .md file must carry a version header:
    > Version X.Y — YYYY-MM-DD
    > Changes: [one line summary]
    > Previous: vX.Y — YYYY-MM-DD
  Increment rule: X.Y→X.Y+1 = detail change only. X.0→X+1.0 = new section or structure.
  Whoever last edited the file applies the bump — Chat, CC, or Owner.
  No file is committed without a version bump if content changed.

- **R-140: HTML FILE SIZE LIMIT — PERMANENT (2026-06-18):**
  Any HTML file in public/ that exceeds 1000 lines must be flagged immediately
  by Chat or CC. Chat proposes a split plan to owner before next CC session.
  CC executes the split only after owner confirms.
  Independent sections must become independent files — each self-contained,
  no shared JS between files.

- **R-139: CC_SKILL.md MANDATORY READ — PERMANENT (2026-06-18):**
  CC reads CC_SKILL.md at the start of every session alongside CLAUDE.md and RULES.md.
  CC_SKILL.md lives at repo root in both repos.
  It contains: session protocol, 6 skills, CC_CHAT_LOG write format, closing checklist.
  Never remove CC_SKILL.md from the mandatory read list.

- **R-138: CC_CHAT_LOG PROTOCOL — PERMANENT (2026-06-18):**
  CC appends one entry to CC_CHAT_LOG.md at the TOP after every session.
  Format defined in CC_SKILL.md. Max 10 lines per entry. Newest at top always.
  Chat reads last 3 entries at every session open.
  If CC_CHAT_LOG is missing or unreadable — Chat tells owner before proceeding.
  CC never deletes old entries.

- **R-137: Font rule — UNIVERSAL and PERMANENT (2026-06-17).**
  Never use NULL font with setTextSize > 1 on any Latin text on any screen.
  FreeSansBold24pt7b = hero numbers + large titles (setTextSize 1 or 2).
  FreeSansBold18pt7b = screen titles (setTextSize 1).
  FreeSansBold12pt7b = section headings, slot names (setTextSize 1).
  NULL font setTextSize 1 = body text, labels, status lines only.
  After every setFont(&FreeSansXXX) call, reset with setFont(NULL).
  Font files live in firmware/ — FreeSansBold24/18/12pt7b.h.
  Proven on SATU-4R473R hardware 2026-06-16. Do not replace or question.

- **R-136: Flap re-lock trigger = proximity switch CLOSED → lockFlap() immediately (2026-06-17).**
  No timer-based re-lock in normal operation.
  FLAP_RELOCK_TIMEOUT = 3000ms safety only (proximity not wired or flap stuck).
  Log warning if timeout fires — means proximity pin needs wiring or flap is jammed.

- **R-135: Flap unlock and motor start are SIMULTANEOUS — same moment, same call block (2026-06-17).**
  Flap stays unlocked for entire vend duration.
  Do NOT re-lock flap during motor spin. Do NOT unlock flap after motor stops.

- **R-134: SPEAKER_PIN = -1 in config.h until GPIO assigned (2026-06-17). NVS key "vol" saved regardless.**
  Volume UI stubs gracefully when SPEAKER_PIN < 0.

- **R-129 REVISED (2026-06-17): Relay 12 = solenoid pin lock. INVERTED polarity vs motor relays.**
  HIGH = pin retracted = UNLOCKED. LOW = pin extended = LOCKED (fail-secure, spring-held).
  unlockFlap() at payment confirmed. lockFlap() on proximity CLOSED or FLAP_RELOCK_TIMEOUT.
  Flap stays unlocked entire vend. One-way mechanical stop prevents forced re-entry.
  Power cut = fail-locked (safe — mechanical stop protects mid-vend state).
  FLAP_PROXIMITY_MCP_PIN on MCP2 GPA2-7 — TBD — stub with -1 until assigned.

- **R-128 CONFIRMED (2026-06-17): Motor stop = IR sensor triggered ONLY.**
  VEND_MAX_SPIN_MS = 30000ms safety cutoff. SENSOR_POLL_MS = 10ms.
  vendProduct() returns bool. Lane empty → disable + log.
  VEND_PULSE_MS, DROP_TIMEOUT, REMOVAL_TIMEOUT deleted permanently.
  NON-NEGOTIABLE. Timer-based motor stop is permanently prohibited.

- **R-127: satu_observer.ino is a standalone diagnostic tool — must live in `tools/satu_observer/`, NEVER in `firmware/` (2026-06-16).**
  Arduino IDE compiles ALL .ino files in a sketch folder together.
  Two .ino files with setup()/loop() = duplicate symbol error = CI failure.
  Each standalone tool must have its own directory matching its .ino filename.

- **R-126: Idle animation must NEVER block touch polling (2026-06-16).**
  Touch check must be called inside the animation frame loop, not after it.
  Maximum touch latency = 1 frame (~16ms). Double-tap requirement = animation bug.
  Any future animation added to idle screen must follow this same pattern.
  Font rule: never scale small bitmap fonts for large display sizes.
  Use native Adafruit GFX FreeFonts at their designed size instead.

- **R-123: CALLBACK RETURN VALUES — for any library using callbacks, document what each return value means in LIBRARY_xxx.md BEFORE writing project code (2026-06-15).**
  Wrong return value = silent failure that mimics hardware bugs.
  Example: PNGdec return 0 = stop decode. return 1 = continue. (48hr lesson — R-121 prevents recurrence)
  See: `.claude/rules/SKILL_library_onboarding.md`

- **R-122: LIBRARY EXAMPLE FIRST — before writing project-specific library code, run the designer's own simplest example on target hardware (2026-06-15).**
  Confirm it works. "Library broken" is never the first hypothesis.
  See: `.claude/rules/SKILL_library_onboarding.md`

- **R-121: LIBRARY ONBOARDING — when any new library is added to firmware or backend (2026-06-15):**
  Chat or CC visits designer's GitHub, reads README + releases + /examples/,
  creates `.claude/rules/LIBRARY_[name].md` BEFORE writing any code.
  Commit the LIBRARY file first. Code second. No exceptions.
  See: `.claude/rules/SKILL_library_onboarding.md` for full process.

- **R-120: NVS writes must not occur during image decode or QR display — schedule at idle state only (2026-06-15)**
- **R-119: lineBuf in _pngDrawRow must be static (not stack-allocated) — stable layout during decode (2026-06-15)**
- **R-118: Product images = JPEG ≤320×320px served from backend. Only Omise QR = PNG (EMVCo requirement) (2026-06-15)**
- **R-117: PNG decode CONFIRMED WORKING — root cause was return 0 in callback (2026-06-15 CORRECTED):**
  Root cause confirmed on hardware 2026-06-15 16:41:32: `_pngDrawRow()` returned `0` = PNGdec stop-early (v1.1.4 feature).
  Fix: `return 1` in callback. One character. rc=0 rows=165 w=165 h=165 confirmed.
  pause-decode-resume pattern (TFT_BL gate) was tested alongside but was NOT the root cause.
  PSRAM bus contention is a real constraint on this board class and remains documented for future reference.
  Reference: `.claude/rules/LIBRARY_pngdec.md` · `.claude/rules/SKILL_esp32s3_rgb_panel_constraints.md`
- **R-116 PNGDEC ROOT CAUSE CONFIRMED — PSRAM BANDWIDTH CONTENTION (2026-06-15 update):**
  Root cause = RGB DMA engine reads 800×480 frame buffer from PSRAM continuously at ~16MHz,
  consuming ~50% of OPI PSRAM bus bandwidth at all times. zlib inflate needs 32KB sliding
  window with random PSRAM reads — DMA wins every bus arbitration. Fix = pause-decode-resume (R-117).
  CLOSED: do not run further format/allocation diagnostics on this issue.
- **R-116 PNGDEC INVESTIGATION STATUS — (2026-06-14) [SUPERSEDED by above 2026-06-15]:**
  PNGdec 1.1.6 openRAM() returns rc=8 rows=1 for all PNG variants tested.
  The library is NOT confirmed broken — it works for thousands of ESP32
  projects. Root cause NOT yet identified.
  Next diagnostic: esp_ptr_in_psram(g_pngBuf) immediately after ps_malloc
  in initUI(). If PSRAM=NO, zlib inflate fails due to insufficient 
  sliding window in internal RAM. This is the most likely root cause.
  Do not change PNG format or architecture again until this is measured.
  Bitmap branch preserved as fallback only.
- **R-115 CRITICAL FIX ESCALATION PROTOCOL — PERMANENT (2026-06-14):**
  When any fix attempt exceeds 2 loops without solving the root cause,
  STOP ALL CODE CHANGES immediately. Do not create a workaround that
  only works in one mode. Do not change architecture to avoid a library
  bug without first confirming the library is actually broken.

  Instead follow these steps in order:

  Step 1 — BIG PICTURE REVIEW
    List every file touched in this fix sequence.
    List all assumptions that have never been verified on hardware.
    State the original problem in one sentence.
    Ask: are we still solving the original problem or a new one?

  Step 2 — DEEP DIAGNOSTIC SPY
    Add serial output at the exact failure point before changing code.
    Never guess what data looks like — measure it.
    Report raw values (rc=, bytes=, addr=, PSRAM=YES/NO).
    Do not remove diagnostic output until root cause is confirmed.

  Step 3 — RESEARCH GLOBAL KNOWLEDGE
    Search Arduino forums, GitHub issues, and library release notes
    for the exact error code and library version.
    If the community has already solved it — use their fix.
    Do not invent a new architecture to avoid a known solvable bug.

  Step 4 — FIX MUST WORK IN ALL MODES
    Any fix must work for both fake mode AND live Omise PNG mode.
    A fix that only works in fake mode is not a fix — it is a 
    workaround that creates future problems.
    Image rendering (PNG/JPEG) is core to this product — QR codes,
    amulet photos, Buddha images, temple owner uploads all depend on it.

  Applied lesson: QR PNG bitmap experiment — PRs #16-#20, 5 flash 
  cycles, 125K firmware tokens, problem confirmed but not root-caused.
  Bitmap workaround works in fake mode only. PNGdec investigation 
  continues next session with esp_ptr_in_psram() diagnostic.
- **R-114 FIRMWARE QR BITMAP APPROACH — (2026-06-14) [PR #17 MERGED to firmware main 2026-06-14 — backend /bitmap endpoint REVERTED 2026-06-15 via PR #21 — owner reflashing R5.3 from backup — firmware main has bitmap code but backend has no /bitmap — investigation continues per R-116]:**
  drawQrScreen() fetches /bitmap endpoint, not PNG.
  drawQrFromBitmap() draws direct with gfx->fillRect() — no PNGdec.
  drawQrFromBytes() (PNGdec) remains in ui.h commented out — do not delete.
  PNGdec 1.1.6 fails on all PNG variants tested (PRs #16–#19): rc=8 or rc=2.
  PNGdec is NOT confirmed permanently broken — see R-116 for next diagnostic.
  Bitmap confirmed working on hardware (owner flash 2026-06-14) in fake mode only.
  Cannot use bitmap with live Omise — real PromptPay PNG cannot be re-served as bitmap.

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
- **R-85 NO HARDCODED CREDENTIALS — PERMANENT RULE (2026-06-12):**
  WiFi credentials NEVER in source files or git — NVS only (nvs_ssid / nvs_pass).
  config.h WIFI_SSID and WIFI_PASSWORD MUST remain empty strings ("") permanently.
  Credentials entered via drawWifiSetupScreen() touchscreen → saveWifiAndReboot() → NVS.
  Do NOT suggest filling in config.h WiFi fields in any session, ever.
- **R-86 config.h WORKFLOW — PERMANENT RULE (2026-06-12):**
  config.h = gitignored local file for pin constants and build config only.
  config.h.example = tracked template in git — WIFI_SSID="" WIFI_PASSWORD="" intentional.
  On new machine: copy config.h.example → config.h, leave WiFi empty, flash, enter on screen.
- **R-87 config.h IS the repo file — NO .example pattern (2026-06-12):**
  firmware/config.h is tracked in git with empty WIFI_SSID="" WIFI_PASSWORD="" permanently.
  .gitignore entry remains as the accidental-credential safety net — this is sufficient.
  No config.h.example needed. Arduino IDE reads config.h directly without any copy step.
  Supersedes R-86: there is no .example file. config.h is the repo file. .gitignore protects it.
- **R-88 SHARED GLOBALS — extern in network.h, define in ui.h (2026-06-12, corrected):**
  g_grid_rows, g_grid_cols, g_cfg_idle, g_cfg_sel, g_cfg_water, g_cfg_lucky:
    network.h → `extern int g_grid_rows;`   (declaration, no value — network.h included first)
    ui.h      → `int g_grid_rows = 2;`      (definition with default, no static keyword)
  network.h is included before ui.h in satu_vending.ino — extern is required so network.h
  functions can reference the variables before ui.h is parsed.
  NEVER use `static` on shared globals — static makes them translation-unit-private, breaking extern.
- **R-105 BLOCKING READBYTES FOR HTTP STREAMS — PERMANENT (2026-06-13):**
  stream->available() returns 0 between TCP packets on ESP32 — never use it
  as the sole read condition for HTTP streams.
  Use blocking readBytes() instead: readBytes() blocks until data arrives,
  the stream closes, or the per-read timeout fires.
  Confirmed 2026-06-13: available()-based idle timer fired at 497 bytes
  on a 3KB+ QR PNG from api.qrserver.com (chunked transfer, Content-Length=-1).
- **R-103 CHUNKED HTTP READ — PERMANENT (2026-06-13):**
  fetchImageBytes() MUST handle chunked transfer encoding (Content-Length=-1).
  Use !http.connected() to detect stream EOF — do NOT rely on Content-Length.
  Per-packet idle timeout = 5000ms. Global wall-clock timeout via http.setTimeout(15000).
  api.qrserver.com returns chunked — this is permanent behaviour, not a bug.
  Root cause confirmed: 502 bytes truncated = chunked EOF detection missing.
  WiFiClientSecure + setInsecure() remains correct (R-97).
- **R-102 QR PAYMENT TIMEOUT — PERMANENT (2026-06-13):**
  QR screen payment wait timeout = 30 seconds (was 120s).
  30s is sufficient for PromptPay scan. 120s causes poor UX at temple kiosk.
  Reference: firmware/config.h PAYMENT_TIMEOUT constant.
- **R-97 WIFI CLIENT SECURE FOR EXTERNAL HTTPS — PERMANENT (2026-06-13):**
  fetchImageBytes() MUST use WiFiClientSecure with setInsecure() for external HTTPS URLs
  (e.g. api.qrserver.com). Plain HTTPClient.begin(url) silently fails on ESP32 for external
  HTTPS — no cert chain available. setInsecure() is acceptable for QR image fetch only —
  not for sensitive auth data. Always add http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS).
- **R-96 ARDUINO-CLI SKETCH FOLDER — CI RULE (2026-06-13):**
  arduino-cli REQUIRES the sketch folder name to match the .ino filename exactly.
  If repo folder is `firmware/` but sketch is `satu_vending.ino`, CI must:
    `cp -r firmware satu_vending` then compile from `satu_vending/`
  Passing the .ino file path directly does NOT bypass this — arduino-cli still
  uses the parent folder name. Always compile from a correctly named folder.
- **R-95 ARDUINO-CLI LIBRARY INDEX — CI RULE (2026-06-13):**
  `arduino-cli core update-index` only refreshes board packages — NOT libraries.
  Always run `arduino-cli lib update-index` separately before any `lib install`.
  Omitting this causes "Library not found" even for valid library names.
- **R-94 ARDUINO-CLI LIBRARY REGISTRY NAMES — CI RULE (2026-06-13):**
  arduino-cli lib install uses the `Name:` field from library.properties — NOT
  the GitHub folder name, class name, or Arduino IDE display name. Verified names:
    "GFX Library for Arduino"          ← NOT Arduino_GFX_Library
    "PNGdec"
    "TAMC_GT911"
    "ArduinoJson"
    "Adafruit MCP23017 Arduino Library" ← NOT Adafruit MCP23X17, NOT git URL
    "FastLED"
  If a library is not found by name, check Arduino Library Registry — do NOT use
  --git-url as a first resort. The name is almost always a spelling difference.
- **R-93 ARDUINO-CLI INSTALL METHOD — CI RULE (2026-06-13):**
  Use `arduino/setup-arduino-cli@v2` GitHub Action — NOT the `curl | sh` method.
  The curl script installs to the workspace ./bin (CWD), not $HOME/bin, so the
  PATH export fails and all subsequent steps see "arduino-cli: command not found".
  The official action handles install + PATH correctly with no extra steps.
- **R-92 KNOWN_GOOD.md SCOPE — PERMANENT RULE (2026-06-13):**
  KNOWN_GOOD.md = firmware only (compile + flash status). PROJECT_STATE.md = everything else.
  Chat includes UPDATE KNOWN_GOOD.md block in every firmware CC prompt.
  CC appends to TOP of KNOWN_GOOD.md as instructed — never overwrites existing entries.
  Owner and Chat read only — never write to KNOWN_GOOD.md manually.
- **R-91 CI CONFIG — PERMANENT RULE (2026-06-13):**
  GitHub Actions creates its own config.h at compile time from the locked template in the workflow.
  Never store real credentials in the workflow file.
  The CI config.h is identical to the repo config.h (empty creds) — compile only, never flashed.
- **R-90 GITHUB ACTIONS COMPILE — PERMANENT RULE (2026-06-13):**
  Every firmware push and PR triggers automatic compile check via .github/workflows/compile-check.yml.
  CC must NEVER open a firmware PR without waiting for the GitHub Actions check to go green first.
  If it goes red: fix the error, push to same branch, wait for green, THEN notify owner the PR is ready.
  Owner should never see a red compile on main.
- **R-89 PNGdec v1.1.6 callback returns int NOT void — ALWAYS return 1 (2026-06-12, corrected 2026-06-15):**
  PNG_DRAW_CALLBACK signature: `int (*)(PNGDRAW*)` — return type is int, not void.
  Always write: `static int _pngDrawRow(PNGDRAW* pDraw) { ... return 1; }` ← MUST BE 1
  `return 0` = STOP decode early (PNGdec v1.1.4 feature) — root cause of 48hr rc=8 investigation.
  Passing a void callback to PNG::openRAM() is a compile error (-fpermissive).
  See: `.claude/rules/LIBRARY_pngdec.md`

---

## Domain Rules Index

| Task | Load |
|------|------|
| Session structure · CC prompt · handoff | `.claude/rules/RULES-workflow.md` |
| Backend API · payment · D1 · rate limit | `.claude/rules/RULES-backend.md` |
| Firmware · Arduino · NVS · compile · UI | `.claude/rules/RULES-firmware.md` |
| Hardware · wiring · relays · power | `.claude/rules/RULES-hardware.md` |
| Auth · secrets · ownership · legal | `.claude/rules/RULES-security.md` |
