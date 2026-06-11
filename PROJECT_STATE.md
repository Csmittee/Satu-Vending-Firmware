# PROJECT STATE — Satu 1.0 Vending Machine
> Last updated: 2026-06-11
> Compiled by: Chat S15 (chaijohn-personal session — first proper STATE doc for Satu)
> Status: Phase 1 active — ~45% complete

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
- [x] State machine architecture (state_machine.h) — IDLE, PRODUCT_SELECTION, AWAITING_PAYMENT, VENDING, COMPLETING, ERROR
- [x] config.h — pin constants, NUM_SLOTS, timeouts, NVS key constants
- [x] hardware.h R2 — MCP23017, relays, IR sensors, LED breathing, idleAnimation() — LOCKED, never modify
- [x] network.h — WiFi, NVS, /hello, /order, /completion, /factory-reset, fetchImageBytes()
- [x] satu_vending.ino — main loop, state machine branches, boot PIN, slot loading
- [ ] ui.h — service mode 5 tabs NOT COMPLETE — last CC build attempted, status unclear ⚠️
- [ ] Full end-to-end test on real hardware — BLOCKED (hardware arriving)
- [ ] OTA firmware update — explicitly deferred (not in Phase 1 scope)

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
