# PROJECT STATE — Satu 1.0 Vending Machine
> Last updated: 2026-06-17
> Compiled by: Chat S15 (chaijohn-personal session — first proper STATE doc for Satu)
> Status: Phase 1 active — ~55% complete

## SESSION LOG (newest first)

### 2026-06-17 — Firmware R6: sensor motor + pin-lock flap + payment banner + font audit (CC_BUILD_PROMPT_firmware_ux_r128_REVISED)
- **R-128 (Sensor-driven motor stop):** `vendProduct(int lane)` rewritten to return `bool`.
  Motor stops on IR sensor trigger only — `VEND_MAX_SPIN_MS=30000` is safety cutoff, NOT primary stop.
  `SENSOR_POLL_MS=10` — IR sensor read interval during spin. `VEND_PULSE_MS`, `DROP_TIMEOUT`,
  `REMOVAL_TIMEOUT` deleted permanently from config.h and all usage points.
- **R-129 (Pin-lock solenoid flap):** `RELAY_DOOR_LOCK` renamed `RELAY_FLAP=12` in config.h.
  `HIGH=UNLOCKED` (pin retracted), `LOW=LOCKED` (fail-secure on power loss).
  `unlockFlap()` + `lockFlap()` added to hardware.h. MCP2 relay 12 wired to solenoid pin lock.
  `FLAP_PROXIMITY_MCP_PIN=-1` stubs safely — uses `FLAP_RELOCK_TIMEOUT=3000` when not wired.
  Motor + flap unlocked simultaneously on vend start. Flap locked after motor stop.
- **R-131 (Payment accepted banner):** `showPaymentAccepted()` added to ui.h.
  1.5s green overlay on QR screen before vend. Called in `_onPaymentConfirmed()` in satu_vending.ino.
- **R-137 (Font audit):** NEVER use NULL font with `setTextSize>1` on Latin text.
  Rule: `FreeSansBold24pt7b`=hero numbers, `FreeSansBold18pt7b`=screen titles,
  `FreeSansBold12pt7b`=section headings, NULL size 1=body text only.
  All screens in ui.h audited and corrected.
- **State machine cleanup:** `STATE_WAITING_DROP`, `STATE_DISPENSING`, `STATE_WAITING_REMOVAL`
  removed from state_machine.h enum. Vend flow is now synchronous via `_onPaymentConfirmed()`.
- **Session closing:** RULES.md updated (R-128 through R-137 prepended), KNOWN_GOOD.md snapshot
  prepended, CC_BUILD_PROMPT archived to docs/prompts/ with ✅ COMPLETE stamp.
- **Files changed:** firmware/config.h (R6), firmware/hardware.h (R6), firmware/state_machine.h,
  firmware/satu_vending.ino (R6), firmware/ui.h (R6)
- **CI:** ⬜ PENDING — waiting for GitHub Actions compile check
- **Flash:** ⬜ PENDING — owner to flash and confirm vend + flap + banner on SATU-4R473R
- **Branch:** claude/magical-feynman-rrkais · PR pending

### 2026-06-16 — CI fixes + workflow docs (no CC_PROMPT — inline session)
- **CI Fix 1: FreeFonts not found** — arduino-cli only adds a library's include path when it
  detects a direct `#include <LibraryName.h>`. Installing "Adafruit GFX Library" did nothing
  because `<Adafruit_GFX.h>` was never explicitly included. Fix: bundled 3 FreeSans font headers
  directly in `firmware/` (stripped `#include <Adafruit_GFX.h>`), changed ui.h to local includes.
  Files added: `firmware/FreeSansBold24pt7b.h`, `FreeSansBold18pt7b.h`, `FreeSansBold12pt7b.h`
- **CI Fix 2: duplicate setup()/loop()** — `satu_observer.ino` was in `firmware/`. Arduino
  compiles all .ino files in sketch folder together → duplicate symbol error. Fix: moved to
  `tools/satu_observer/satu_observer.ino` (proper Arduino standalone sketch structure).
- **R-127 added to RULES.md** — standalone tools must never live in firmware/
- **WORKFLOW_SKILL.md + RULES-workflow.md (both repos)** — removed CHAT_HANDOFF.md from CC
  session closing checklist. CHAT_HANDOFF.md = Chat's responsibility only (R-05/R-84 corrected).
  Backend PR #27 created and merged.
- **CI:** ✅ Runs 95 (push) + 96 (PR) GREEN — commit 9cbfd29
- **Flash:** ✅ Owner flashed SATU-4R473R — 2026-06-16

### 2026-06-16 — Firmware UX fixes post first hardware test (CC_PROMPT_firmware_ux_fixes)
- **First full end-to-end hardware test PASSED on SATU-4R473R**
  Payment → relay → door → completion → idle — confirmed working 2026-06-16
- **Fix 1 (R-126): Touch delay on idle** — `idleAnimationUI()` blocking `delay()` replaced
  with millis-based loops that poll `_touch.read()` every ~16ms and return on touch.
  Max touch latency reduced from 400ms → 16ms. Double-tap requirement eliminated.
- **Fix 2 (R-126): Font quality** — replaced `setTextSize(4-8)` scaled bitmap fonts with
  Adafruit FreeFonts on large text screens:
  - Lucky number hero: `FreeSansBold24pt7b` size 2
  - Amount display (QR screen): `FreeSansBold24pt7b` size 1
  - "Dispensing..." title: `FreeSansBold18pt7b` size 1
  - "Your Merit Lucky Number": `FreeSansBold12pt7b` size 1
  - Boot "SATU": `FreeSansBold24pt7b` size 1
- Items #2 and #3 (completion HTTP 400 + long dispense time) fixed in backend session — not firmware
- **R-126 added to RULES.md**
- **CI:** ✅ GREEN — commit 9cbfd29 (CI fixed in subsequent inline session — see above)
- **Flash:** ✅ Owner flashed SATU-4R473R — 2026-06-16

## OPEN ITEMS
- [ ] PAYMENT_TIMEOUT_MS — return to 120000 before temple deployment
      Currently set for HW testing. Direct edit in config.h by owner.
- [ ] Service mode firmware — ui.h 5 tabs full build (stubs only currently)
      Next firmware CC session after this one.
- [ ] FLAP_PROXIMITY_MCP_PIN — assign MCP2 GPA pin when solenoid wired (-1 = stub, safe)

## SESSION LOG (newest first)

### 2026-06-15 — PNG victory cleanup (CC_PROMPT_png_victory_cleanup)
- **PNG QR decode: ✅ FIXED AND CONFIRMED ON HARDWARE 2026-06-15 16:41:32**
  Root cause: `_pngDrawRow()` returned `0` = PNGdec v1.1.4 stop-early signal
  Fix: `return 1` in callback — one character — rc=0 rows=165 w=165 h=165
- **ui.h:** spy diagnostic Serial.printf lines removed from `_pngDrawRow()`
- **Skills added:** `.claude/rules/SKILL_library_onboarding.md` (new) · `.claude/rules/LIBRARY_pngdec.md` (new)
- **Skills updated:** `SKILL_problem_solving_kt.md` v1.1 · `SKILL_esp32s3_rgb_panel_constraints.md` v1.1 (final resolution appended)
- **RULES.md:** R-121/122/123 added · R-117 corrected (actual root cause) · R-89 corrected (return 0→1)
- **WORKFLOW_SKILL.md:** intervention levels + library onboarding + KT framework sections added
- **CI:** ✅ GitHub Actions Run #76 GREEN — commit 79090e3

### 2026-06-15 — PNG decode fix R-117 (pause-decode-resume for PSRAM DMA contention)
- **ROOT CAUSE CONFIRMED:** PSRAM bus bandwidth contention between RGB DMA and zlib inflate
  Evidence: rc=8 rows=1 on all PNG variants. Fetch=200 OK 27458 bytes. openRAM succeeds.
  DMA reads 800×480 frame buffer from PSRAM non-stop at ~16MHz — zlib loses every bus arbitration.
  Full analysis: `.claude/rules/SKILL_esp32s3_rgb_panel_constraints.md`
- **FIX APPLIED:** `ui.h drawQrFromBytes()` re-enabled with pause-decode-resume pattern (R-117)
  `digitalWrite(TFT_BL, LOW)` → `delay(20)` → `_png.decode()` → `delay(5)` → `digitalWrite(TFT_BL, HIGH)`
  Backlight gate stops DMA bus pressure — zlib inflate gets full PSRAM bandwidth
- **FIX APPLIED:** `ui.h _pngDrawRow()` — `lineBuf` made `static` (R-119) — off stack permanently
- **FIX APPLIED:** `ui.h drawQrScreen()` — switched back from bitmap URL to PNG URL — calls `drawQrFromBytes()`
  Backend /bitmap was reverted — PNG path is the correct production path
- **EMERGENCY FALLBACK:** `drawQrFromBitmap()` preserved in ui.h with guard comment — R-114
- **RULES.md:** R-117, R-118, R-119, R-120 prepended at TOP · R-116 status updated to CLOSED
- **CI:** Pending — waiting for GitHub Actions compile check
- **Flash:** ⬜ PENDING owner flash — expected: `[UI] PNG decode: rc=0 rows=165 w=165 h=165`

### 2026-06-15 — Bitmap experiment revert + rules update
- **DISCOVERY:** Firmware PR #17 WAS merged to main on 2026-06-14 (not closed without merge as previously stated)
  Owner confirmed: bitmap ui.h on firmware main, QR bitmap flashed and working on hardware ✅
- **BACKEND REVERTED (PR #21):** `/v1/qr/:charge_id/bitmap` endpoint removed from backend main
  `src/handlers/qr.js` restored to _zlibStore() RFC 1950 state (PR #17 backend state)
  `src/index.js` restored: no bitmap import, version=R4, no bitmap route
- **REASON FOR REVERT:** Live Omise serves real PromptPay PNG with EMVCo payload — cannot re-serve as bitmap.
  Bitmap is fake-mode-only. PNGdec must be fixed for live mode. Backend cleaned while investigation continues.
- **RULES:** R-115 (Critical Fix Escalation Protocol) + R-116 (PNGdec Investigation Status) added to RULES.md in BOTH repos
- **RULES:** R-114 annotation corrected to reflect actual state (PR #17 merged, backend reverted)
- **INCONSISTENCY STATE:** Firmware main has bitmap code, backend has no /bitmap endpoint
  Owner reflashing hardware with R5.3 (pre-bitmap) from Mac trash backup
- **NEXT SESSION:** Add esp_ptr_in_psram(g_pngBuf) diagnostic to initUI() — measure PSRAM allocation

### 2026-06-14 — QR bitmap draw firmware (CC_PROMPT_firmware_qr_bitmap)
- **PNGdec STILL FAILING (R-114 context):** 4 PNG variants tested PRs #16-#19, all fail (rc=8 or rc=2)
  NOTE (2026-06-15): PNGdec NOT confirmed permanently broken — root cause unknown — see R-116
- **FIX:** `ui.h drawQrFromBitmap()` — direct gfx->fillRect() pixel draw from raw bitmap
  No decode library. Backend /bitmap endpoint: 4-byte header + 1 byte/pixel 0x00=black 0xFF=white
- **FIX:** `ui.h drawQrScreen()` — appends /bitmap to qrUrl, calls drawQrFromBitmap()
  drawQrFromBytes() (PNGdec) commented out — kept for future investigation
- **RULES.md:** R-114 prepended at top
- **CI:** ✅ GitHub Actions Run #46 GREEN
- **Flash status:** ✅ CONFIRMED — owner flashed, QR bitmap rendered on screen
- **PR #17:** ✅ MERGED to firmware main 2026-06-14
- **Backend bitmap:** ❌ REVERTED from backend main 2026-06-15 (PR #21)

### 2026-06-13 — QR blocking read fix (CC_PROMPT_firmware_qr_blocking_read)
- **PR #13 CONFIRMED FLASH:** HTTP 200 ✅, Content-Length=-1, idle timeout at 497 bytes — available() root cause confirmed
- **ROOT CAUSE (R-105):** stream->available()=0 between TCP packets on ESP32 → 5s idle timer fires at 497 bytes
  QR PNG from api.qrserver.com is ~3KB+. available()-based loop always exits too early.
- **FIX:** `network.h fetchImageBytes()` — R5.3 — blocking readBytes() with 10s per-read timeout
  available() poll + idle timer removed entirely. readBytes() blocks until data or stream close.
- **RULES.md:** R-105 appended at TOP
- **KNOWN_GOOD.md:** PR #13 confirmed flash snapshot appended at TOP
- **Files:** firmware/network.h only

### 2026-06-13 — QR Chunked Read Fix + Timeout (CC_PROMPT_firmware_qr_chunked_fix)
- **PR #12 CONFIRMED FLASH:** HTTP 200 ✅ but 502 bytes truncated — chunked stream exits early
- **ROOT CAUSE:** stream.available()=0 between TCP packets → 15s wall-clock timer triggers early
  Content-Length=-1 (chunked encoding) — stream close detection was missing
- **FIX:** `network.h fetchImageBytes()` — chunked-safe loop: !http.connected() detects EOF
  Per-packet idle timeout 5000ms replaces broken global 15s wall-clock
- **FIX:** `config.h PAYMENT_TIMEOUT`: 120000 → 30000 (30s, was 2 min — R-102)
- **RULES.md:** R-102 (QR timeout 30s) + R-103 (chunked HTTP read) appended
- **KNOWN_GOOD.md:** PR #12 confirmed flash snapshot + R5.2 snapshot both appended at TOP
- **Flash status:** PENDING — owner to flash, look for "stream closed — transfer complete" + 2000+ bytes
- **Branch:** claude/vibrant-cray-cqp2em · CI pending
- **Next if still fails:** report byte count — PNG decode investigation

### 2026-06-13 — QR PNG Fetch Fix (CC_PROMPT_firmware_qr_png_fetch)
- **ROOT CAUSE:** Plain `HTTPClient.begin(url)` silently fails on ESP32 for external HTTPS — no cert chain
- **FIX:** `network.h fetchImageBytes()` — replaced with `WiFiClientSecure + setInsecure()` + `HTTPC_STRICT_FOLLOW_REDIRECTS`
- **FIX:** `ui.h drawQrScreen()` — added `[UI] QR PNG loaded` and `[UI] QR PNG failed` serial logs on both paths
- **RULES.md:** R-97 appended — WiFiClientSecure for external HTTPS is now a permanent rule
- **Flash status:** PENDING — owner to flash and verify serial shows HTTP 200 + PNG bytes
- **Branch:** claude/vibrant-cray-cqp2em · CI pending
- **Next if still fails:** PNG decode investigation (pngLen > 0 but no render = _pngDrawRow issue)

### 2026-06-13 — GitHub Actions Compile Check (CC_BUILD_PROMPT_github_actions_compile)
- **CREATED:** .github/workflows/compile-check.yml — auto-compile on every push + every PR to main
- **Board FQBN:** esp32:esp32:esp32s3 with CDCOnBoot=cdc, FlashSize=16M, PartitionScheme=app3M_fat9M_16MB, PSRAM=opi, UploadSpeed=460800
- **Core locked:** ESP32 2.0.17 · Libraries locked: Arduino_GFX_Library@1.4.9, PNGdec@1.1.6, TAMC_GT911, ArduinoJson, Adafruit MCP23X17, FastLED
- **Est. CI time:** 3-5 min per run (vs 10+ min local Arduino IDE)
- **RULES.md:** R-90 (GitHub Actions compile) + R-91 (CI config) + R-92 (KNOWN_GOOD.md scope) appended
- **WORKFLOW_SKILL.md:** CC firmware loop updated with CI check step; KNOWN_GOOD.md update block rule added
- **Cleaned up:** CC_BUILD_PROMPT_R4.md + CC_BUILD_PROMPT_github_actions_compile.md archived to docs/prompts/ · removed from repo root
- **CI status:** ✅ GREEN — runs #15 and #16 passed (commit 8c21d78)
- **Fix loops:** 6 iterations to resolve workflow config errors (all CI infra — no firmware bugs)
- **Lessons → RULES.md:** R-93 (arduino-cli install), R-94 (library registry names), R-95 (lib update-index), R-96 (sketch folder convention)
- **Branch:** claude/pensive-heisenberg-1sf4c1 · PR #10 ✅ ready to merge

### 2026-06-12 — CC Compile Error Fix (CC_PROMPT_fix_compile_errors)
- **FIXED:** 7 compile errors from first R5 build attempt — R5 now ready to flash
- **ui.h:** g_grid_rows, g_grid_cols, g_cfg_idle, g_cfg_sel, g_cfg_water, g_cfg_lucky —
  removed `static` keyword (now plain globals, resolved at link time)
- **network.h:** removed 6 `extern` re-declarations — not needed in Arduino sketch compilation
- **ui.h:** `_pngDrawRow` return type changed `void` → `int` (PNGdec v1.1.6 requires int callback)
- **RULES.md:** R-88 (shared globals pattern) + R-89 (PNGdec callback type) appended
- **Branch:** claude/loving-bohr-t3n3yf · pushed to PR #4

### 2026-06-12 — CC R5 WiFi Provisioning (CC_PROMPT_fix_wifi_credentials)
- **RESOLVED:** WiFi credential security risk — credentials no longer in source files or git
- **network.h R5:** initWiFi() NVS-first (nvs_ssid/nvs_pass) → config.h fallback → setup screen
- **network.h R5:** saveWifiAndReboot() — saves credentials to NVS, calls ESP.restart()
- **ui.h R5:** drawWifiSetupScreen() — blocking QWERTY touchscreen keyboard, SSID + masked password, CONNECT button
- **state_machine.h R5:** STATE_WIFI_SETUP added between STATE_STARTUP and STATE_IDLE
- **satu_vending.ino R5:** WiFi.status() check after initWiFi() → STATE_WIFI_SETUP if not connected
- **config.h.example:** Created — WIFI_SSID="" WIFI_PASSWORD="" intentionally empty, R5 NVS note
- **RULES.md:** R-85 + R-86 appended (permanent: no hardcoded credentials, config.h workflow)
- **Branch:** claude/loving-bohr-t3n3yf · PR created
- **Self-check:** hardware.h untouched ✅ · NVS keys nvs_ssid/nvs_pass approved ✅ · FW_VERSION=v1.0.0-r5 ✅

---

## IDENTITY

Satu is a Thai temple donation vending machine system. Donors select a product
(amulet, blessing card, sacred water), scan a PromptPay QR code, and the machine
dispenses the item. Built by a solo founder. Revenue share model (15%) targeting
Thai temples.

**Three generations planned:**
- Satu 1.0 — Physical vending machine (IN DEVELOPMENT — this repo)
- Satu 2.0 — National digital donation platform (CONCEPT)
- Satu 3.0 — AI Dharma guidance system (VISION)

**Business model:** Revenue share 15% per transaction. Break-even at 150 tx/month
= 6 months. Full P&L and pricing model documented in satu-business-model.html.

---

## REPOS

| Repo | Purpose | CC-ready? |
|---|---|---|
| `Csmittee/Satu-vending-backend` | Cloudflare Worker + D1 + all API | ✅ Yes |
| `Csmittee/Satu-Vending-Firmware` | Arduino/C++ ESP32 firmware | ⚠️ Partial (upload constraint) |
| `Csmittee/Satu-vending-hardware` | Wiring diagrams, BOM | ❌ Reference only |

---

## TECH STACK

| Layer | Technology | Notes |
|---|---|---|
| Backend | Cloudflare Workers + D1 (SQLite) | api.janishammer.com |
| Payment | Omise PromptPay | PAYMENT_MODE=fake (dev) / live (real hardware only) |
| Storage | D1 (14 tables) | schema.sql is source of truth |
| Static files | Cloudflare Pages [assets] in Worker | public/ folder served automatically |
| Cron | Cloudflare Cron Trigger (*/30 min) | rate limit cleanup |
| Hardware | ESP32-S3 (ESP32-8048S070C) | 16MB flash, 8MB OPI PSRAM |
| Display | Arduino_GFX RGB panel 800×480 | backlight pin=2 |
| Touch | TAMC_GT911 | SDA=19, SCL=20 |
| IO expander | MCP23017 ×2 | MCP1 0x20 (sensors 1-8, relays 1-6) · MCP2 0x21 (sensors 9-10, relays 7-12) |
| Firmware IDE | Arduino 1.8.19 | ESP32 core 2.0.17 ONLY — 3.x breaks WiFi |
| GFX library | moononournation v1.4.9 ONLY | 1.6.5 requires core 3.x |
| QR display | PNGdec ✅ FIXED 2026-06-15 | return 1 in callback — rc=0 rows=165 confirmed on hardware |

---

## PHASE STATUS

### Phase 1 — Prototyping (~55% complete) ← CURRENT

#### Backend & API ✅ COMPLETE
- [x] Cloudflare Workers + D1 backend (index.js, machine.js, order.js, webhook.js, admin.js)
- [x] Omise PromptPay integration — test keys working, real QR confirmed via curl
- [x] Device authentication — X-Device-Secret header, mode-aware bypass
- [x] Webhook idempotency + HMAC verification — race condition guard, Omise signature verified
- [x] 14-test automated test suite (satu-system-tester.html) — 14/14 passing
- [x] Admin dashboard — ADMIN_SECRET + ADMIN_PATH secrets, XSS protected, DB browser
- [x] Rate limiting — D1-backed counter (fixed April 2026, was broken in-memory Map)
- [x] Auth login endpoint (/v1/auth/login) — JWT working, login.html URL fixed
- [x] D1 database cleanup — ghost devices purged, test device MACs locked (R-14)
- [x] /v1/machine/completion endpoint — added, tested
- [x] /v1/machine/factory-reset endpoint — backend side implemented
- [x] /v1/qr/:charge_id PNG endpoint — _zlibStore() RFC 1950, synchronous, confirmed working
- [❌] /v1/qr/:charge_id/bitmap — REVERTED from backend main 2026-06-15 (PR #21)
  Preserved on branch revert/qr-bitmap-experiment and claude/cool-hopper-6owumd

#### Backend — Pending / Known Issues
- [ ] Order expiry / QR timeout — pending orders never expire — needs Cron Trigger ⚠️
- [ ] PDPA consent flow — partially coded in order.js — legal review required before launch 🔴
- [ ] Temple owner claim flow (setup code UI) — owners cannot self-onboard yet
- [ ] Omise KYC/bank registration — not yet complete, cannot go live with real payments
- [ ] satu-admin.html CORS/401 — /v1/admin-data/:table route missing (JWT admin version, not X-Admin-Token)

#### Firmware ⚠️ IN PROGRESS
- [x] State machine architecture (state_machine.h) — R6: removed STATE_WAITING_DROP/STATE_DISPENSING/STATE_WAITING_REMOVAL
- [x] config.h R6 — RELAY_FLAP=12, VEND_MAX_SPIN_MS=30000, SENSOR_POLL_MS=10, FLAP_RELOCK_TIMEOUT=3000
      VEND_PULSE_MS / DROP_TIMEOUT / REMOVAL_TIMEOUT deleted permanently (R-128)
- [x] config.h.example — tracked template, WIFI_SSID="" WIFI_PASSWORD="" intentional (R5)
- [x] hardware.h R6 — 4 authorized R6 changes complete:
      unlockFlap() + lockFlap() added (R-129) · vendProduct() returns bool + sensor-driven (R-128)
      Boot message: "[HW] MCP2 OK — flap LOCKED on boot" · Relay 12 comment updated
      ALL OTHER hardware.h content R2 LOCKED — never modify
- [x] network.h R5 — NVS-first WiFi, saveWifiAndReboot(), /hello, /order, /completion, /factory-reset, fetchImageBytes()
- [x] satu_vending.ino R6 — _onPaymentConfirmed(), _onItemDropped(), _onLaneEmpty() · sync vend flow
- [x] ui.h R6 — showPaymentAccepted() 1.5s green banner (R-131) · font audit complete (R-137)
      _pngDrawRow returns 1 (R-89/R-117) · static lineBuf (R-119) · pause-decode-resume (R-117)
- [x] ui.h PNG decode — ✅ CONFIRMED ON HARDWARE 2026-06-15 16:41:32 — rc=0 rows=165 w=165 h=165
- [ ] ui.h — service mode 5 tabs NOT COMPLETE — stubs only, next firmware CC session
- [ ] FLAP_PROXIMITY_MCP_PIN — assign MCP2 GPA pin when solenoid wired (-1 = stub, safe)
- [ ] Full end-to-end test on real hardware (R6 vend + flap + banner) — PENDING owner flash ⬜
- [ ] OTA firmware update — explicitly deferred (not in Phase 1 scope)

#### Infrastructure ✅ ADDED 2026-06-13
- [x] GitHub Actions compile check: ACTIVE — .github/workflows/compile-check.yml
- Triggers on: all pushes to any branch + all PRs to main
- Est. compile time: 3-5 minutes (vs 10+ min local Arduino IDE)
- Board: ESP32S3 | Core: 2.0.17 LOCKED | Libraries: 6 at locked versions
- CC waits for green ✅ before opening PR (R-90)

#### Hardware ⚠️ IN PROGRESS
- [x] ESP32-S3 display board — ARRIVED · first hardware test 2026-06-16 ✅
- [ ] Relay modules — sourcing
- [ ] IR sensors (E18-D80NK) — sourcing (R-128 requires sensors for sensor-driven motor stop)
- [ ] Solenoid pin-lock flap — sourcing (R-129: RELAY_FLAP=12, HIGH=UNLOCKED)
- [ ] Physical build and wiring — blocked on components

#### Business / Legal IN PROGRESS
- [x] P&L model and pricing strategy — complete (satu-business-model.html)
- [x] Revenue share model confirmed (15%)
- [x] Thai/English MOU template drafted
- [x] PDPA compliance audit — gaps identified
- [ ] Omise sales meeting / KYC — scheduled, not complete
- [ ] Utility model filing (อนุสิทธิบัตร) at ipthailand.go.th — P0, time-sensitive ⚠️
- [ ] Company registration (Thai Ltd.) — in progress
- [ ] BOI application — Category 4.1 Digital/IoT, target June 2026

### Phase 2 — Production (NOT STARTED)
- Multi-machine deployment
- Temple owner portal
- Analytics dashboard
- Omise live mode with real bank account

### Phase 3 — Scale (VISION)
- Satu 2.0 national platform
- Satu 3.0 AI Dharma system

---

## CONFIRMED WORKING — DO NOT BREAK

- 14-test suite: 14/14 passing — run after EVERY backend change before closing
- PAYMENT_MODE=fake — never change to live without physical ESP32 connected
- Test device MACs: SATU-TEST001 (AA:BB:CC:DD:EE:00) + SATU-SIM01 (AA:BB:CC:DD:EE:01) ONLY
- D1-backed rate limiting — rateLimit.js — fixed, do not revert to in-memory
- hardware.h R2 — NEVER modify beyond 4 authorized R6 changes (vendProduct/unlockFlap/lockFlap/boot msg)
- NUM_SLOTS defined in config.h ONLY — ui.h reads it, never redefines
- idleAnimation() = LED breathing in hardware.h · idleAnimationUI() = screen flash in ui.h — TWO different functions
- RELAY_FLAP=12 HIGH=UNLOCKED LOW=LOCKED (fail-secure) — never invert

---

## KNOWN RISKS / SECURITY GAPS

| Risk | Severity | Status |
|---|---|---|
| PDPA consent flow incomplete | 🔴 Legal | Not complete — legal review required before any live donor data |
| Omise live keys not active | 🟡 Business | KYC not done — test only |
| Utility model not filed | 🔴 IP | File BEFORE any public demo or temple visit |
| PIN brute-force no lockout | 🟡 Security | Acceptable for single-machine, not for multi-tenant |
| Factory reset: offline wipe blocked | 🟢 Mitigated | R-74: must call backend first, only wipe NVS on HTTP 200 |
| D1 database_id in wrangler.toml | 🟢 Low | Public repo — low risk, worth noting |
| /v1/admin-data/:table CORS/401 | 🟡 UX | satu-admin.html JWT admin route missing |
| WiFi credentials in config.h | 🟢 RESOLVED | R5 2026-06-12: NVS provisioning screen eliminates this permanently |
| PNGdec rc=8 root cause | 🟢 RESOLVED | Root cause: return 0 in callback = PNGdec stop-early (v1.1.4). Fix: return 1. Confirmed 2026-06-15. |
| Firmware main has bitmap, backend has no /bitmap | 🟢 RESOLVED | PNG path restored in drawQrScreen(). drawQrFromBitmap() kept as emergency fallback. |
| RELAY_FLAP fail-secure | 🟢 By design | LOW=LOCKED on power loss — pin extends, flap stays closed (R-129) |

---

## FILE INVENTORY

### Backend (Csmittee/Satu-vending-backend)
```
src/
├── index.js              — route table, CORS, auth middleware, cron handler
├── db/
│   └── schema.sql        — authoritative D1 schema (14 tables) — source of truth
├── handlers/
│   ├── machine.js        — /hello, /heartbeat, /commands, /slots, /completion, /factory-reset
│   ├── order.js          — /order, /order/:id/status, PDPA (incomplete)
│   ├── webhook.js        — Omise webhook, HMAC, idempotency
│   ├── admin.js          — device management (disable/enable/reassign/reboot/factory-reset)
│   ├── dashboard.js      — temple owner dashboard routes, JWT-protected
│   └── qr.js             — /v1/qr/:charge_id PNG only — _zlibStore() RFC 1950
├── auth/
│   └── jwt.js            — verifyJWT + signJWT
├── middleware/
│   ├── auth.js           — JWT + device-secret validation helpers
│   └── rateLimit.js      — D1-backed counter (fixed, multi-instance safe)
├── commands/
│   └── queue.js          — command queue helpers
└── utils/
    └── setupCode.js      — 6-digit setup code generator (crypto.getRandomValues)
wrangler.toml             — Workers config, D1 binding, [assets], cron, routes
public/
├── satu-system-tester.html  — 14-test automated suite ← run after every backend change
├── satu-machine-tester.html — manual machine simulation tool
├── satu-preflight.html      — firmware pre-flash checklist tool
├── simulator.html           — service mode UI reference spec
└── satu-admin.html          — admin dashboard (JWT version — CORS fix pending)
```

### Firmware (Csmittee/Satu-Vending-Firmware)
```
satu_vending.ino    — main: setup(), loop(), state machine, _onPaymentConfirmed/Dropped/LaneEmpty
config.h            — ALL constants: pins, NUM_SLOTS, timeouts, NVS key names, RELAY_FLAP=12
                      ← IN .gitignore — WiFi credentials NEVER in git
config.h.example    — template for new dev environment setup
hardware.h          — MCP23017, relays, IR, LEDs, idleAnimation(), unlockFlap(), lockFlap(),
                      vendProduct() bool (sensor-driven) ← R6 changes complete, rest LOCKED R2
network.h           — WiFi, NVS, /hello, /order, /completion, /factory-reset, fetchImageBytes()
ui.h                — all screen drawing, touch detection, showPaymentAccepted() (R-131),
                      FreeSansBold fonts (R-137), PNG pause-decode-resume (R-117)
                      5-tab service mode — STUBS ONLY — next CC session
state_machine.h     — enum MachineState (R6: DROP/DISPENSING/REMOVAL states removed)
```

### Project Knowledge Docs (project folder)
```
CLAUDE.md           — project compass, stack, 5 rules, key files, repos (30 lines max)
RULES.md            — lessons learned R-85 to R-137 (newest at top)
PROJECT_STATE.md    — this file
CHAT_HANDOFF.md     — last session summary (overwrite each session, never append)
KNOWLEDGE_MAP.md    — what to read for what task (navigation guide)
UI_SPEC.md          — screen inventory, grid system, 5-tab service mode, NVS key table
SECURITY.md         — auth layers, ownership model, payment modes, security gaps
```

---

## ENDPOINT STATUS TABLE

| Method | Endpoint | Status | Notes |
|---|---|---|---|
| POST | /v1/machine/hello | ✅ Working | Returns config + slots |
| GET | /v1/machine/commands | ✅ Working | Command poll |
| POST | /v1/machine/heartbeat | ✅ Working | connection_logs fixed |
| POST | /v1/machine/completion | ✅ Working | R4 added slotIdx |
| POST | /v1/machine/factory-reset | ✅ Working | R-74 compliant |
| POST | /v1/order | ✅ Working | fake mode |
| GET | /v1/order/:id/status | ✅ Working | |
| POST | /v1/order/webhook | ✅ Working | HMAC + idempotency |
| POST | /v1/auth/login | ✅ Working | JWT, signJWT added |
| POST | /v1/auth/register | ✅ Working | ALLOW_REGISTRATION secret |
| POST | /v1/admin/device/* | ✅ Working | X-Admin-Token auth |
| GET | /v1/dashboard/* | ✅ Working | JWT auth |
| GET | /v1/qr/:charge_id | ✅ Working | PNG — _zlibStore() RFC 1950 |
| GET | /v1/qr/:charge_id/bitmap | ❌ REVERTED | Removed from backend main 2026-06-15 (PR #21). Branch preserved. |
| GET | /v1/admin-data/:table | ❌ Missing | CORS/401 in satu-admin.html |

---

## PAYMENT MODES

| Mode | What it does | When to use |
|---|---|---|
| PAYMENT_MODE=fake | Skips Omise entirely, auto-confirms | All dev, all CC sessions, all testing |
| PAYMENT_MODE=live | Real Omise API call, real PromptPay QR | Physical ESP32 only, KYC complete |

**NEVER set PAYMENT_MODE=live without physical hardware connected and Omise KYC complete.**

---

## ARDUINO IDE SETTINGS (CRITICAL — never change)

```
Board:         ESP32S3 Dev Module
Flash:         16MB
Partition:     16M Flash (3MB APP/9.9MB FATFS)
PSRAM:         OPI PSRAM ← CRITICAL — never disable
Upload speed:  460800
Port:          /dev/cu.usbserial-1420 (varies by machine)
Core:          ESP32 2.0.17 ONLY (3.x breaks WiFi)
GFX Library:   moononournation v1.4.9 ONLY
PNGdec:        v1.1.6 — return 1 in callback confirmed working (R-89/R-117)
TFT_eSPI:      REMOVE if installed — incompatible with RGB panel
```

---

## IMMEDIATE NEXT ACTIONS (priority order)

### P0 — Do before any public demo
1. File utility model (อนุสิทธิบัตร) at ipthailand.go.th
2. Complete Omise KYC / bank account registration
3. Complete PDPA consent flow + legal review

### P1 — Post R6 flash (next firmware session)
4. Owner flashes R6 (claude/magical-feynman-rrkais → main after CI green + PR merged)
   Expected: motor stops on IR trigger · flap unlocks/locks · 1.5s payment banner
5. Wire IR sensors (E18-D80NK) + solenoid pin-lock flap to relay 12 (MCP2)
   Assign FLAP_PROXIMITY_MCP_PIN in config.h when wired
6. Complete ui.h service mode (5 tabs) — verify against simulator.html spec
7. Fix /v1/admin-data/:table CORS/401 (add JWT admin route to index.js)

### P2 — Polish
8. Implement order expiry (Cron Trigger already configured in wrangler.toml)
9. Build temple owner claim/onboarding flow (setup code UI)
10. Temple owner dashboard — complete missing features

### P3 — Phase 2 prep
11. Multi-machine architecture review
12. Analytics and reconciliation reporting
