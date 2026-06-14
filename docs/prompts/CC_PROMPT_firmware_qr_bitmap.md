# CC_PROMPT_firmware_qr_bitmap.md
> ✅ COMPLETE — 2026-06-14 — Firmware QR bitmap draw (R-114)
> Executed by: CC (Claude Code)
> Result: firmware/ui.h — drawQrFromBitmap() added, drawQrFromBytes() commented out,
>          drawQrScreen() switched to /bitmap endpoint
> Branch: claude/cool-hopper-6owumd
> CI: ⬜ pending GitHub Actions compile
> PR: ⬜ pending CI green (R-90)

---

# CC_PROMPT_firmware_qr_bitmap.md
> Created by: Chat (Claude) — 2026-06-14
> Session goal: Replace PNGdec QR rendering with direct bitmap draw
> Repo: Satu-Vending-Firmware
> Loop: B (firmware) — requires owner compile + flash
> Flash cycles expected: 1
> Prompt: 2 of 2 (backend must be deployed first — CC_PROMPT_fix_qr_bitmap.md)

---

## CC INTRO

New session. Ignore all previous context from other projects.

You are working on SATU 1.0 FIRMWARE at:
https://github.com/Csmittee/Satu-Vending-Firmware

CC_PROMPT files are always at the repository ROOT level.
docs/prompts/ is archive only — never execute from there.

Read IN FULL and state each filename aloud before writing a single line:
1. CLAUDE.md
2. RULES.md
3. firmware/ui.h              ← drawQrScreen() and drawQrFromBytes() live here
4. firmware/network.h         ← fetchImageBytes() already working
5. firmware/state_machine.h   ← read for context only — do NOT modify
6. CC_PROMPT_firmware_qr_bitmap.md ← this file

State "All files read ✅" then execute this prompt.

---

## CONTEXT — WHY THIS CHANGE

PNGdec 1.1.6 has been tested across 4 PNG variants (PRs #16–#19).
Every variant fails: rc=8 rows=1 or rc=2 rows=0.
The library is broken for this hardware/version combination.

The backend now serves a raw bitmap at:
`GET /v1/qr/:charge_id/bitmap`

Response format:
```
Byte 0–1:  width  uint16 big-endian
Byte 2–3:  height uint16 big-endian
Byte 4+:   pixels 0x00=black 0xFF=white, row by row, 1 byte per pixel
```

Typical size: ~42KB for a 205×205 QR. Well within the 200KB g_pngBuf buffer.

fetchImageBytes() already works correctly (confirmed HTTP 200, correct byte count).
The only change needed is in ui.h — replace the PNGdec decode call with
a direct pixel-by-pixel draw loop.

The qr_code_url from the backend order response currently points to:
`https://api.janishammer.com/v1/qr/<charge_id>`  (PNG endpoint)

We need the firmware to call the bitmap endpoint instead:
`https://api.janishammer.com/v1/qr/<charge_id>/bitmap`

---

## TASK 1 — ui.h: add drawQrFromBitmap()

Add this new function to ui.h BEFORE drawQrScreen():

```cpp
// ============================================================
//  DRAW QR FROM RAW BITMAP BYTES (R-113)
//  Format: 4-byte header (width uint16 BE + height uint16 BE)
//          then 1 byte per pixel: 0x00=black 0xFF=white
//  No decoder library — direct gfx->fillRect() per black pixel.
// ============================================================
void drawQrFromBitmap(const uint8_t* buf, size_t len, int destX, int destY, int destW, int destH) {
    if (!buf || len < 5) {
        Serial.println("[UI] drawQrFromBitmap: buffer too small");
        return;
    }
    ...
}
```

---

## TASK 2 — ui.h: update drawQrScreen() to use bitmap endpoint

In drawQrScreen(), append /bitmap to qrUrl and call drawQrFromBitmap().
Comment out the old PNGdec call — do NOT delete drawQrFromBytes().

---

## TASK 3 — ui.h: verify g_pngBuf is large enough

Confirm g_pngBuf allocated with ps_malloc(200*1024). No change needed if already 200KB.

---

## DO NOT TOUCH
- hardware.h — R2 LOCKED
- config.h
- state_machine.h
- network.h
- satu_vending.ino

---

## NEW RULE — append to RULES.md at TOP

R-114: FIRMWARE QR USES BITMAP NOT PNG — PERMANENT (2026-06-14)

---

## MANDATORY (end of session)

1. Wait for GitHub Actions ✅ GREEN before opening PR
2. Append R-114 to RULES.md
3. Update KNOWN_GOOD.md at TOP
4. Update PROJECT_STATE.md — QR bitmap firmware: ⬜ pending flash
5. Archive this prompt → docs/prompts/
6. Commit and merge to main
