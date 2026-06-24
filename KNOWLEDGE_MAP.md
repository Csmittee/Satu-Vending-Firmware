# SATU — Knowledge Architecture Guide (Firmware)
> Version 1.6 — 2026-06-24
> Changes: Added SarabanSubset.h (D-11 Thai font placeholder) to Firmware file list
> Previous: v1.5 — 2026-06-22

## Document Map — What to Read for What Task

| Task | Read first | Then read |
|------|-----------|----------|
| Starting new chat session | CHAT_HANDOFF.md (proj folder) | CC_CHAT_LOG.md (last 3 entries) |
| CC session start | CC_SKILL.md | CLAUDE.md + RULES.md |
| Any firmware change | UI_SPEC.md | CHAT_HANDOFF.md file touch rules |
| Any backend change | PROJECT_STATE.md endpoint table | SECURITY.md auth layers |
| Any auth / ownership code | SECURITY.md | schema.sql |
| Service mode UI | UI_SPEC.md (tabs 1-5 section) | simulator.html |
| Grid / slot layout | UI_SPEC.md (grid system section) | machine.js _loadSlots() |
| Payment integration | PROJECT_STATE.md Omise section | SECURITY.md payment modes |
| CC build session | CC_BUILD_PROMPT_*.md (repo root) | CC_CHAT_LOG.md last 3 entries |
| Security review | SECURITY.md | PROJECT_STATE.md known risks |
| Hardware wiring / BOM | satu-machine-builder.html (Wiring tab) | hardware.h pin arrays |
| Workflow / session modes | .claude/claude_project/WORKFLOW_SKILL.md | CLAUDE.md |
| Business / legal | satu-business-model.html | work_instruction.txt |
| Flashing firmware | CLAUDE.md "Flashing Without Arduino IDE" | .github/workflows/compile-check.yml |
| Hardware change (pins/relays/sensors/BOM) | hardware/HARDWARE_SPEC.md | hardware.h, config.h |
| UI change (screen/font/layout/service tab) | UI_SPEC.md | firmware/ui.h, ui_screens.h, ui_service.h |
| Architecture or product direction question | SATU_ROADMAP.md | PROJECT_STATE.md |

---

## File Locations

### Firmware (Arduino sketch folder — pull fresh from repo before compiling)
```
satu_vending.ino   — main state machine, setup(), loop()
config.h           — pin constants, timeouts, NUM_SLOTS
hardware.h         — MCP23017, relays, IR, LEDs, idleAnimation()   ← NEVER REPLACE
network.h          — WiFi, NVS, /hello, /order, /completion
ui.h               — R7: HW primitives, PNG/QR, initUI(), include chain, service orchestration
SarabanSubset.h    — D-11: Thai GFXfont placeholder (12/18/24pt). first=0/last=75 (glyph indices). Owner must fontconvert Sarabun.ttf to populate real bitmaps.
ui_strings.h       — R2: StatusBarState, EN/TH labels, g_lang_th/g_lang_th_default, printThai(), Thai string constants
ui_keyboard.h      — R1: PIN numpad (_drawNumpad, getTouchedNumpad), WiFi keyboard (drawWifiSetupScreen)
ui_screens.h       — R2: all customer-facing screen draw/touch + welcome screen (drawWelcomeScreen, getTouchedWelcome)
ui_service.h       — LOCKED: service mode 5-tab body (_drawSvcBody_* + _getTouchedServiceExtra())
state_machine.h    — enum MachineState + STATE_WELCOME, extern declarations
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

### CI / Flashing
```
.github/workflows/compile-check.yml — auto-compile on push/PR to firmware/**
  └ Produces artifact: satu-firmware-N (N = run number) — downloadable from Actions tab
  └ Binary: satu_vending.ino.bin — flash with esptool.py (see CLAUDE.md for command)
  └ Retention: 7 days per run — download before artifact expires
  └ Board: ESP32S3 Dev Module · Core 2.0.17 LOCKED · Libraries: 6 at locked versions
```

### Repo root docs
```
CLAUDE.md           — project compass · CC reads every session
RULES.md            — universal rules + domain index · CC reads every session
CC_SKILL.md         — CC session protocol, 6 skills, CC_CHAT_LOG format · CC reads every session
CC_CHAT_LOG.md      — CC→Chat session log · CC writes · Chat reads last 3
PROJECT_STATE.md    — firmware phase status, known bugs, next actions
KNOWN_GOOD.md       — firmware compile + flash snapshots (append only)
UI_SPEC.md          — UI specification bible · screen inventory, grid system, service tabs, NVS keys
SATU_ROADMAP.md     — product vision and commercial direction · both repos
SECURITY.md         — auth layers, ownership model, gaps
CC_BUILD_PROMPT_*.md — CC session prompts (active at root; archived to docs/prompts/ after use)
hardware/HARDWARE_SPEC.md — hardware single source of truth (renamed from HARDWARE_TRUTH.md) · firmware repo only
```

### .claude/claude_project/ (reference copies — Chat reads, CC rarely needs)
```
WORKFLOW_SKILL.md   — v2.2 governance master reference
CHAT_RULE.md        — Chat non-negotiables reference
```

### .claude/rules/ (domain rules — CC loads by task)
```
RULES-workflow.md   — session structure, CC prompts, handoff
RULES-backend.md    — API, payment, D1, rate limiting
RULES-firmware.md   — Arduino, NVS, compile, UI
RULES-hardware.md   — wiring, relays, power
RULES-security.md   — auth, secrets, ownership, legal
SKILL_*.md          — KT problem solving, library onboarding, ESP32 constraints
LIBRARY_*.md        — library onboarding docs (PNGdec, etc.)
```

---

## Critical Rules (memorise these)

1. **hardware.h is R2 — NEVER replace or modify it**
2. **NUM_SLOTS defined in config.h only** — ui.h reads it, never redefines
3. **idleAnimation() lives in hardware.h** (LED) — ui.h has idleAnimationUI() (screen flash) — different functions
4. **config.h is .gitignored** — WiFi creds never in git
5. **Factory reset requires backend call first** — see SECURITY.md
6. **NVS keys ≤15 chars each** — see UI_SPEC.md NVS table for approved keys only
7. **PAYMENT_MODE=fake for all dev/testing** — never switch to live without physical hardware
8. **Two-repo system** — read both repos (backend + firmware) before any decision — hardware repo was deleted

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

## Variable Code Name Reference

| Code | What it is | Where set | NVS key |
|------|-----------|-----------|----------|
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
