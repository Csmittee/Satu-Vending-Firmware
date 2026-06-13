# RULES.md — Satu 1.0 Universal Rules
> For domain rules: load `.claude/rules/RULES-[domain].md`
> Domain files: workflow · backend · firmware · hardware · security
> Last updated: 2026-06-12

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
- **R-89 PNGdec v1.1.6 callback returns int NOT void (2026-06-12):**
  PNG_DRAW_CALLBACK signature: `int (*)(PNGDRAW*)` — return type is int, not void.
  Always write: `static int _pngDrawRow(PNGDRAW* pDraw) { ... return 0; }`
  Passing a void callback to PNG::openRAM() is a compile error (-fpermissive).

---

## Domain Rules Index

| Task | Load |
|------|------|
| Session structure · CC prompt · handoff | `.claude/rules/RULES-workflow.md` |
| Backend API · payment · D1 · rate limit | `.claude/rules/RULES-backend.md` |
| Firmware · Arduino · NVS · compile · UI | `.claude/rules/RULES-firmware.md` |
| Hardware · wiring · relays · power | `.claude/rules/RULES-hardware.md` |
| Auth · secrets · ownership · legal | `.claude/rules/RULES-security.md` |
