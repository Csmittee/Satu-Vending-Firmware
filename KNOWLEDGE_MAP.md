# SATU — Knowledge Architecture Guide
<!-- How to find anything in this project quickly -->
<!-- Read this first in any new chat session -->
<!-- Last updated: 2026-05-31 -->

## Document Map — What to Read for What Task

| Task | Read first | Then read |
|------|-----------|-----------|
| Starting new chat session | CHAT_HANDOFF.md | PROJECT_STATE.md |
| Any firmware change | UI_SPEC.md | CHAT_HANDOFF.md file touch rules |
| Any backend change | PROJECT_STATE.md endpoint table | SECURITY.md auth layers |
| Any auth / ownership code | SECURITY.md | schema.sql |
| Service mode UI | UI_SPEC.md (tabs 1-5 section) | simulator.html |
| Grid / slot layout | UI_SPEC.md (grid system section) | machine.js _loadSlots() |
| Payment integration | PROJECT_STATE.md Omise section | SECURITY.md payment modes |
| CC build session | CC_BUILD_PROMPT_*.md | Upload local .h files first |
| Security review | SECURITY.md | PROJECT_STATE.md known risks |
| Hardware wiring | satu_wiring_diagram.html | hardware.h pin arrays |
| Business / legal | satu-business-model.html | work_instruction.txt |

---

## File Locations

### Firmware (local Arduino folder — always upload fresh, do NOT use repo)
```
satu_vending.ino   — main state machine, setup(), loop()
config.h           — pin constants, timeouts, NUM_SLOTS
hardware.h         — MCP23017, relays, IR, LEDs, idleAnimation()   ← NEVER REPLACE
network.h          — WiFi, NVS, /hello, /order, /completion
ui.h               — all screen drawing, touch detection, service mode
state_machine.h    — enum MachineState, extern declarations
```

### Backend (GitHub repo → Cloudflare auto-deploy)
```
src/index.js            — route table, CORS, auth middleware
src/handlers/machine.js — /hello, /heartbeat, /commands, /slots, /completion
src/handlers/order.js   — /order, /order/:id/status
src/handlers/webhook.js — Omise webhook handler
src/handlers/admin.js   — admin device management
src/handlers/dashboard.js — temple owner dashboard routes
src/middleware/auth.js  — JWT + device secret auth
src/middleware/rateLimit.js — D1-backed rate limiting
schema.sql              — authoritative D1 schema
wrangler.toml           — Cloudflare config, routes, cron
public/                 — static HTML files (simulator, testers, admin)
```

### Project knowledge docs (this folder)
```
SECURITY.md         — auth layers, ownership model, gaps
UI_SPEC.md          — screen inventory, grid system, service tabs, NVS keys
KNOWLEDGE_MAP.md    — this file
PROJECT_STATE.md    — endpoint status, known bugs, next actions
CHAT_HANDOFF.md     — session summary, what broke, what to tell next chat
CC_BUILD_PROMPT_*.md — CC session opening prompt
```

---

## Critical Rules (memorise these)

1. **hardware.h is R2 — NEVER replace or modify it**
2. **NUM_SLOTS defined in config.h only** — ui.h reads it, never redefines
3. **idleAnimation() lives in hardware.h** (LED) — ui.h has idleAnimationUI() (screen flash) — different functions
4. **Local Arduino files = source of truth** — not project knowledge, not GitHub
5. **Factory reset requires backend call first** — see SECURITY.md
6. **NVS keys ≤15 chars each** — see UI_SPEC.md NVS table for approved keys only
7. **config.h is .gitignored** — WiFi creds never in git
8. **PAYMENT_MODE=fake for all dev/testing** — never switch to live without physical hardware

---

## Architecture Decisions (locked — do not revisit without good reason)

| Decision | Choice | Reason |
|----------|--------|--------|
| Backend hosting | Cloudflare Workers + D1 | Edge latency, free tier sufficient |
| Payment | Omise PromptPay | Thai market standard, good API |
| Display lib | Arduino_GFX (moononournation v1.4.9) | Only lib supporting EK9716 RGB panel |
| Identity model | MAC → backend assigns device_id | MAC spoofable, backend ID is canonical |
| Ownership model | AirTag-style binding | Single owner, admin override via nuke |
| Grid system | R×Out with side tabs when R≥3 | Works on 800×480, maps to physical shelves |
| QR display | PNGdec + HTTP fetch from Omise URL | Show exactly what Omise sends, no rebuild |
| Domain | janishammer.com = company, satu-th.com = product | Alphabet/Google model |
| WiFi config | NVS primary, config.h fallback | Owner changes WiFi without reflash |
| Language | EN now, TH font in R5 | ASCII font only in R4, Thai bitmap later |

---

## Endpoint Status Quick Reference

| Endpoint | Status | Notes |
|----------|--------|-------|
| POST /v1/machine/hello | ✅ Working | Returns slots[], status, setup_code |
| POST /v1/machine/heartbeat | ✅ Working | |
| GET /v1/machine/commands | ✅ Working | 30s poll |
| POST /v1/machine/completion | ❌ Missing | Returns 404 — R4 build |
| POST /v1/machine/factory-reset | ❌ Missing | R4 build |
| POST /v1/machine/claim | ✅ Working | |
| POST /v1/order | ✅ Working | |
| GET /v1/order/:id/status | ✅ Working | |
| POST /v1/webhook/omise | ✅ Working | |
| POST /v1/auth/login | ✅ Working | |
| POST /v1/auth/register | ✅ Working | ALLOW_REGISTRATION gated |
| GET /v1/dashboard/slots | ✅ Working | |
| PUT /v1/dashboard/slots | ✅ Working | |
| GET /v1/dashboard/orders | ❌ Missing | R4 build |
| GET /v1/admin-data/:table | ❌ Missing | R4 build |
| GET /health | ✅ Working | Returns payment_mode |

---

## Variable Code Name Reference (use in discussions)

| Code | What it is | Where set | NVS key |
|------|-----------|-----------|---------|
| DEVID | Device ID (SATU-XXXXXX) | Backend assigned | device_id |
| DEVSEC | Device secret | Backend assigned | dev_secret |
| SETUPC | 6-digit setup code | Backend generated | shown on screen |
| SVC_PIN | Service mode PIN | Machine Settings | svc_pin |
| SVC_PIN_EN | Require PIN to enter service | Machine Settings | svc_pin_en |
| BOOT_PIN | Require PIN on daily startup | Machine Settings | boot_pin |
| IDLE_TO | Idle screen timeout | Both (backend → NVS) | cfg_idle |
| SEL_TO | Selection confirm timeout | Both (backend → NVS) | cfg_sel |
| WATER_EN | Sacred water feature on/off | Both (backend → NVS) | cfg_water |
| LUCKY_EN | Lucky number feature on/off | Both (backend → NVS) | cfg_lucky |
| THEME | Screen colour theme | Machine Settings | scr_theme |
| LANG | Language EN/TH | Machine Settings | lang |
| GRID | Grid rows × cols | Backend → /hello | nvs_grow, nvs_gcol |
| PAY_MODE | Payment gateway mode | Cloudflare secret | — |
| VPULSE | Relay pulse duration | config.h hardcoded | — |

---

## How to Start a CC Session

1. Open new CC conversation
2. Paste CC_BUILD_PROMPT_*.md as first message
3. Upload local Arduino files when CC asks (never use project knowledge versions)
4. Do NOT upload hardware.h — tell CC it is R2, locked, do not touch
5. CC reads all backend files from repo automatically
6. While CC is running: install PNGdec library in Arduino IDE (only new lib needed)
