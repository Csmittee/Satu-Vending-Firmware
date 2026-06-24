# CLAUDE.md — Satu Project Compass
> Version 1.9 — 2026-06-24
> Changes: Added SarabanSubset.h, ui_strings.h, ui_screens.h to Key Files (D-11 Thai language)
> Previous: v1.8 — 2026-06-21
<!-- max 35 lines · never grows · CC reads this on every session start -->

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
- `RULES.md` — 10 universal rules · read every session · domain rules in `.claude/rules/`
- `CC_SKILL.md` — CC session protocol + 6 skills · read every CC session
- `CC_CHAT_LOG.md` — CC→Chat log · Chat reads last 3 entries each session open
- `PROJECT_STATE.md` — phase status · roadmap · CC updates after every fix/PR
- `KNOWN_GOOD.md` — last confirmed test snapshot · updated by Chat/owner after each test session
- `hardware/HARDWARE_SPEC.md` — hardware single source of truth · read before touching pins, relays, sensors, or BOM · (firmware repo only)
- `UI_SPEC.md` — UI specification bible · read before touching any screen, font, layout, or service tab
- `SATU_ROADMAP.md` — product direction guide · read bullet summaries on every session open · read full section when architecture, commercial, or hardware model decision arises
- `SECURITY.md` — auth layers · ownership model · security gaps · read before any auth/ownership/reset code
- `KNOWLEDGE_MAP.md` — navigation guide · what to read for each task
- `public/satu-hw-trigger.html`       — HW Trigger standalone tool · Section C extracted from machine builder
- `public/satu-wiring.html`           — Wiring + BOM standalone reference · Section D extracted from machine builder
- `firmware/satu_vending.ino`         — main state machine loop · read before any logic change
- `firmware/hardware.h`               — R2 LOCKED · relay + sensor + flap control · read-only always
- `firmware/network.h`                — WiFi + API calls
- `firmware/ui.h`                     — R7: touch display + QR rendering + include chain (SarabanSubset.h → ui_strings.h → ...)
- `firmware/SarabanSubset.h`          — D-11: Thai GFXfont placeholder (12/18/24pt) · owner runs fontconvert with Sarabun.ttf
- `firmware/ui_strings.h`             — R2: EN/TH labels, g_lang_th, printThai(), all Thai string constants
- `firmware/ui_screens.h`             — R2: customer screens + welcome screen + getTouchedWelcome()
- `firmware/ui_service.h`             — service mode 5-tab body implementations + _getTouchedServiceExtra()

## Flashing Without Arduino IDE
1. GitHub → Actions tab → latest green run on main → Download artifact `satu-firmware-N`
2. Unzip → move all 3 .bin files to `~/satu-firmware/`
3. Find port: `ls /dev/cu.*`
4. Flash:
```
esptool --chip esp32s3 --port /dev/cu.usbserial-1420 --baud 460800 write-flash 0x0 ~/satu-firmware/satu_vending.ino.bootloader.bin 0x8000 ~/satu-firmware/satu_vending.ino.partitions.bin 0x10000 ~/satu-firmware/satu_vending.ino.bin
```
All 3 files required — missing any = black screen.
Port `/dev/cu.usbserial-1420` is confirmed device. Baud 460800 matches Arduino IDE setting.

## Repos
- Backend: `Csmittee/Satu-vending-backend`
- Firmware: `Csmittee/Satu-Vending-Firmware`
