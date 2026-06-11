# CLAUDE.md — Satu Project Compass
<!-- max 30 lines · never grows · CC reads this on every session start -->

## Stack
- **Backend**: Cloudflare Workers + D1 (SQLite) · `api.janishammer.com`
- **Payment**: Omise PromptPay · PAYMENT_MODE secret = `fake` (dev) / `live` (real machine only)
- **Hardware**: ESP32-S3 (ESP32-8048S070C) · MCP23017 ×2 · relays · IR sensors (E18-D80NK)
- **Firmware**: Arduino/C++ · satu_vending.ino + .h headers
- **Frontend**: Vanilla HTML/JS · Cloudflare Pages
- **Firmware IDE**: Arduino 1.8.19 · ESP32 core **2.0.17 ONLY** · GFX lib **1.4.9 ONLY** · PNGdec 1.1.6
- **Arduino IDE settings**: Board=ESP32S3 Dev Module · Flash=16MB · Partition=16M(3MB APP/9.9MB FATFS)
  PSRAM=**OPI PSRAM** (CRITICAL — never change) · Upload=460800 · Port=/dev/cu.usbserial-1420
- **TFT_eSPI**: REMOVE if installed — incompatible with RGB panel
- **hardware.h**: R2 LOCKED — never open, modify, or redeclare anything it owns

## 5 Rules (non-negotiable)
1. **Never hardcode secrets** — always Cloudflare secrets manager
2. **Security = non-negotiable** — real money at religious institutions · flag issues immediately
3. **Full files only** — never partial snippets for critical files
4. **Run the 14-test suite** (satu-system-tester.html) after any backend change
5. **Document every decision** — this must be handoff-ready at all times

## Key Files (read before touching anything)
- `RULES.md` — lessons learned · read every session
- `PROJECT_STATE.md` — phase status · roadmap · what's broken
- `CHAT_HANDOFF.md` — last session summary · read at session start · overwrite each session
- `WORKFLOW_SKILL.md` — how Chat + CC + Owner work together · Loop A (cloud) + Loop B (firmware)
- `UI_SPEC.md` — screen inventory · grid system · service mode tabs · NVS key table · read before any ui.h change
- `SECURITY.md` — auth layers · ownership model · security gaps · read before any auth/ownership/reset code
- `KNOWLEDGE_MAP.md` (backend repo) — navigation guide · what to read for each task
- - `KNOWN_GOOD.md` — timestamped test snapshots, last confirmed working state

## Repos
- Backend: `Csmittee/Satu-vending-backend`
- Firmware: `Csmittee/Satu-Vending-Firmware`
- Hardware: `Csmittee/Satu-vending-hardware` — wiring diagrams, BOM — read before any hardware decision
