# PROJECT STATE — Satu 1.0 Vending Machine
> Last updated: 2026-06-13
> Compiled by: Chat S15 (chaijohn-personal session — first proper STATE doc for Satu)
> Status: Phase 1 active — ~48% complete

## SESSION LOG (newest first)

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
| QR display | PNGdec v1.1.6 (Larry Bank) | buffer in PSRAM via ps_malloc(200*1024) |

---

## PHASE STATUS

### Phase 1 — Prototyping (~45% complete) ← CURRENT

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

#### Backend — Pending / Known Issues
- [ ] Order expiry / QR timeout — pending orders never expire — needs Cron Trigger ⚠️
- [ ] PDPA consent flow — partially coded in order.js — legal review required before launch 🔴
- [ ] Temple owner claim flow (setup code UI) — owners cannot self-onboard yet
- [ ] Omise KYC/bank registration — not yet complete, cannot go live with real payments
- [ ] satu-admin.html CORS/401 — /v1/admin-data/:table route missing (JWT admin version, not X-Admin-Token)

#### Firmware ⚠️ IN PROGRESS
- [x] State machine architecture (state_machine.h) — all states incl. STATE_WIFI_SETUP (R5)
- [x] config.h — pin constants, NUM_SLOTS, timeouts, NVS key constants (gitignored)
- [x] config.h.example — tracked template, WIFI_SSID="" WIFI_PASSWORD="" intentional (R5)
- [x] hardware.h R2 — MCP23017, relays, IR sensors, LED breathing, idleAnimation() — LOCKED, never modify
- [x] network.h R5 — NVS-first WiFi, saveWifiAndReboot(), /hello, /order, /completion, /factory-reset, fetchImageBytes()
- [x] satu_vending.ino R5 — STATE_WIFI_SETUP guard, WiFi.status() check after initWiFi()
- [x] ui.h R5 — drawWifiSetupScreen() QWERTY keyboard (blocking, restarts on CONNECT)
- [ ] ui.h — service mode 5 tabs NOT COMPLETE — last CC build attempted, status unclear ⚠️
- [ ] Full end-to-end test on real hardware — BLOCKED (hardware arriving)
- [ ] OTA firmware update — explicitly deferred (not in Phase 1 scope)

#### Infrastructure ✅ ADDED 2026-06-13
- [x] GitHub Actions compile check: ACTIVE — .github/workflows/compile-check.yml
- Triggers on: all pushes to any branch + all PRs to main
- Est. compile time: 3-5 minutes (vs 10+ min local Arduino IDE)
- Board: ESP32S3 | Core: 2.0.17 LOCKED | Libraries: 6 at locked versions
- CC waits for green ✅ before opening PR (R-90)

#### Hardware ⚠️ PENDING
- [ ] ESP32-S3 display board — in transit from China
- [ ] Relay modules — sourcing
- [ ] IR sensors (E18-D80NK) — sourcing
- [ ] Push spring set — sourcing
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
- hardware.h R2 — NEVER modify, NEVER replace
- NUM_SLOTS defined in config.h ONLY — ui.h reads it, never redefines
- idleAnimation() = LED breathing in hardware.h · idleAnimationUI() = screen flash in ui.h — TWO different functions

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
│   └── dashboard.js      — temple owner dashboard routes, JWT-protected
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
satu_vending.ino    — main: setup(), loop(), state machine, slot loading
config.h            — ALL constants: pins, NUM_SLOTS, timeouts, NVS key names
                      ← IN .gitignore — WiFi credentials NEVER in git
config.h.example    — template for new dev environment setup
hardware.h          — MCP23017, relays, IR, LEDs, idleAnimation() ← R2 LOCKED
network.h           — WiFi, NVS, /hello, /order, /completion, /factory-reset, fetchImageBytes()
ui.h                — all screen drawing, touch detection, 5-tab service mode ← IN PROGRESS
state_machine.h     — enum MachineState, extern declarations
```

### Project Knowledge Docs (project folder)
```
CLAUDE.md           — project compass, stack, 5 rules, key files, repos (30 lines max)
RULES.md            — lessons learned R-01 to R-82+ (newest at top)
PROJECT_STATE.md    — this file
CHAT_HANDOFF.md     — last session summary (overwrite each session, never append)
KNOWLEDGE_MAP.md    — what to read for what task (navigation guide)
UI_SPEC.md          — screen inventory, grid system, 5-tab service mode, NVS key table
SECURITY.md         — auth layers, ownership model, payment modes, security gaps
CC_BUILD_PROMPT_SUNDAY.md — last CC build prompt (archive after execution)
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
PNGdec:        v1.1.6 (Larry Bank) — pin this version
TFT_eSPI:      REMOVE if installed — incompatible with RGB panel
```

---

## IMMEDIATE NEXT ACTIONS (priority order)

### P0 — Do before any public demo
1. File utility model (อนุสิทธิบัตร) at ipthailand.go.th
2. Complete Omise KYC / bank account registration
3. Complete PDPA consent flow + legal review

### P1 — Unblock hardware test
4. Complete ui.h service mode (5 tabs) — verify against simulator.html spec
5. Verify full end-to-end on real hardware when components arrive
6. Fix /v1/admin-data/:table CORS/401 (add JWT admin route to index.js)

### P2 — Polish
7. Implement order expiry (Cron Trigger already configured in wrangler.toml)
8. Build temple owner claim/onboarding flow (setup code UI)
9. Temple owner dashboard — complete missing features

### P3 — Phase 2 prep
10. Multi-machine architecture review
11. Analytics and reconciliation reporting
