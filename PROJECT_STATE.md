# PROJECT_STATE.md — Satu 1.0 Live Status
<!-- CC updates phase status after Build sessions · Chat updates after design decisions locked -->
<!-- Last updated: 2026-05-29 — Post-pause restart + full chat history compiled -->

## Current Goal: First Real Board Test (Display + Touch)
Two ESP32-S3 boards arrived. Firmware written. Now validate LCD panel works before building hardware.

---

## Phase Status

| Phase | What | Status |
|-------|------|--------|
| P1 | Backend API | ✅ DONE — 14/14 tests pass |
| P2 | Payment Gateway (Omise test) | 🟡 TEST KEYS ACTIVE — KYC/bank pending, Omise slow to respond |
| P3 | Firmware (all .h files) | ✅ WRITTEN — not yet validated on real board |
| P4 | Hardware Build | 🔵 DESIGN DONE — components arrived, build not started |
| P5 | Temple Owner Dashboard | 🟡 PARTIAL — login/claim/index HTML built, backend patches not applied |
| P6 | Omise Live Keys | 🔴 BLOCKED — KYC incomplete |
| P7 | First Machine Field Test | ⬜ NOT STARTED |

---

## ⚠️ CRITICAL: Payment Gateway Variable Changed
```
OLD (delete from Cloudflare): PAYMENT_MODE = fake/live
NEW (set these in Cloudflare):
  PAYMENT_GATEWAY = fake_omise   ← use this for all dev/test
  PAYMENT_GATEWAY = omise_test   ← real Omise test keys
  PAYMENT_GATEWAY = omise_live   ← only when KYC done + real machine
  SYSTEM_MODE     = online
```
**Action needed: delete `PAYMENT_MODE` secret from Cloudflare, set `PAYMENT_GATEWAY` + `SYSTEM_MODE`**

---

## ⚠️ CRITICAL: Hardware Lane Count Changed
Old design: 10 lanes / 2× MCP23017
**New design: 21 lanes (7 cols × 3 rows) / 3× MCP23017 (0x20, 0x21, 0x22)**
- config.h and hardware.h may still reference 10 lanes — needs update before flashing
- BOM needs update: 21 motors, 21 IR sensors, 3× MCP23017, 3× 8-ch relay boards
- Wiring diagram needs revision (was designed for 12 relays, now needs 24)

---

## Backend — Endpoint Inventory (api.janishammer.com)

| Endpoint | Status | Notes |
|----------|--------|-------|
| POST /v1/machine/hello | ✅ | Device registration + NVS credential return |
| POST /v1/machine/heartbeat | ⚠️ | HTTP 500 — connection_logs column mismatch (not fixed) |
| GET /v1/machine/commands | ✅ | 30-sec poll by firmware |
| POST /v1/machine/command-ack | ✅ | Queue acknowledgment |
| POST /v1/machine/completion | ❌ | Missing — firmware calls it, returns 404 |
| POST /v1/order/create | ✅ | Creates order + PromptPay QR |
| GET /v1/order/:id/status | ✅ | Payment polling |
| POST /v1/webhook/omise | ✅ | Webhook handler (skips HMAC on fake_omise) |
| POST /v1/auth/login | ✅ | PBKDF2 auth, fixed Apr 2026 |
| POST /v1/auth/register | ✅ | With ALLOW_REGISTRATION gate |
| GET /v1/dashboard/* | 🟡 | HTML exists, /dashboard/orders endpoint missing |
| POST /v1/machine/claim | ✅ | Handler exists in machine.js, route wired |
| GET /[ADMIN_PATH] | ✅ | Protected admin dashboard |

**Open backend items:**
- `/v1/machine/completion` endpoint — missing (firmware calls it)
- Heartbeat HTTP 500 — column mismatch in machine.js
- Order expiry cron — was added in auth session, confirm it's deployed
- PDPA: Thai privacy notice page missing, donor delete endpoint missing
- Minimum charge 20 THB not enforced in order.js (10 THB product will fail at Omise)
- schema.sql seed MAC: `TEST:00:00:00:00:01` → should be `AA:BB:CC:DD:EE:00`

---

## Firmware — File Inventory (Satu-Vending-Firmware repo)

| File | Status | Notes |
|------|--------|-------|
| satu_vending.ino | ✅ R2 | Compile error fixed, full state machine |
| config.h | ⚠️ | Complete BUT lane count = 10 — needs update to 21 |
| state_machine.h | ✅ | All states defined |
| network.h | ✅ COMPLETE | /hello, /heartbeat, /commands, createOrder, checkPayment, reportCompletion |
| hardware.h | ⚠️ WRITTEN | Full MCP23017 + IR + LED + door logic — written for OLD 10-lane / 2-MCP design |
| ui.h | ✅ WRITTEN | Full TFT display, product grid, QR, water countdown, service mode |

**Next firmware action:** Validate display + touch on real board FIRST, then update config.h + hardware.h for 21-lane / 3-MCP layout.

---

## Hardware — Design Status (Satu-vending-hardware repo)

| Item | Status |
|------|--------|
| Wiring diagram | ⚠️ Needs revision (21-lane, 3-MCP, 24-relay) |
| BOM (bom_2025_04.csv) | ⚠️ Needs update for 21-lane counts |
| Frame spec | ✅ |
| Spring spec | ✅ LOCKED: 4.0×65×340×13 (wire/OD/length/coils) |
| Physical build | 🔵 Not started — components arrived |

**Key hardware decisions locked:**
- 7 cols × 3 rows = 21 lanes
- 3× MCP23017 at 0x20 / 0x21 / 0x22
- 3× 8-channel relay boards (21 motor + door + pump + spare)
- 12V separate supply for relays
- IR sensor: E18-D80NK, SENSOR_TRIGGERED = LOW, mount 5-8cm below shelf
- Board: ESP32-8048S070C, display driver EK9716, touch GT911 I2C

---

## Dashboard — Temple Owner (backend repo dashboard/)

| File | Status |
|------|--------|
| dashboard/login.html | ✅ Built — Thai mobile-first, correct API URL |
| dashboard/claim.html | ✅ Built — 3-step claim wizard |
| dashboard/index.html | ✅ Built — 4-tab dashboard |
| BACKEND_PATCH.js | ⚠️ Written but NOT applied to backend yet |

**Backend patches still needed before dashboard works:**
1. `ALTER TABLE users ADD COLUMN password_hash TEXT;` in D1 console
2. Add `signJWT()` to jwt.js
3. Add `/v1/dashboard/orders` route
4. Set `ALLOW_REGISTRATION=true`, create first user, set back to false

---

## Known Broken / Risks (priority order)

| Item | Severity | Owner |
|------|----------|-------|
| Omise KYC incomplete | 🔴 BLOCKS LAUNCH | You — pursue Omise |
| config.h / hardware.h lane count = 10 (not 21) | 🔴 MUST FIX before build | CC |
| Heartbeat HTTP 500 | 🟡 | CC |
| /v1/machine/completion missing | 🟡 | CC |
| PDPA consent incomplete | 🔴 Legal risk | Before any live install |
| Dashboard backend patches not applied | 🟡 | CC |
| IP (utility model) not filed | 🔴 File before public demo | You |
| PAYMENT_MODE secret not yet deleted from Cloudflare | 🟡 | You (1-min task) |

---

## Next 3 Actions (in order)
1. **Test LCD panel** — connect ESP32-S3 board, confirm display + touch work
2. **Fix config.h + hardware.h** — update from 10-lane to 21-lane / 3-MCP
3. **Cloudflare secret cleanup** — delete PAYMENT_MODE, set PAYMENT_GATEWAY=fake_omise, SYSTEM_MODE=online

---

## CC Prompt Archive
Stored in `docs/prompts/` — format `CC_PROMPT_001_description.md`
*(none yet — starting fresh)*

---

## Business Context
- P&L: ฿10,470 COGS per machine, break-even at 150 tx/mo = 6 months
- Revenue model: 15% revenue share (beats outright sale 5.4× over 3 years)
- Company registration: Thai Ltd. target May 2026 (may be delayed)
- BOI application: Category 4.1, target June 2026
- Omise: Test keys active, API testing ongoing, KYC meeting not yet held
