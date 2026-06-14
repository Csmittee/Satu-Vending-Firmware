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

    uint16_t bmpW = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t bmpH = ((uint16_t)buf[2] << 8) | buf[3];

    Serial.printf("[UI] drawQrFromBitmap: w=%u h=%u dest=(%d,%d) size=%dx%d\n",
                  bmpW, bmpH, destX, destY, destW, destH);

    if (bmpW == 0 || bmpH == 0 || len < (size_t)(4 + bmpW * bmpH)) {
        Serial.println("[UI] drawQrFromBitmap: invalid dimensions or truncated buffer");
        return;
    }

    // Scale to fit destW x destH, maintain aspect ratio
    // Use integer math — avoid float on inner loop
    // Each source pixel maps to a rectangle in destination
    // Simple approach: scale factor = destW / bmpW (integer, floor)
    int scale = destW / bmpW;
    if (scale < 1) scale = 1;
    int drawW = bmpW * scale;
    int drawH = bmpH * scale;

    // Centre in destination area
    int offX = destX + (destW - drawW) / 2;
    int offY = destY + (destH - drawH) / 2;

    // White background for the draw area
    gfx->fillRect(destX, destY, destW, destH, 0xFFFF);

    const uint8_t* pixels = buf + 4;
    for (int y = 0; y < bmpH; y++) {
        for (int x = 0; x < bmpW; x++) {
            if (pixels[y * bmpW + x] == 0x00) {
                gfx->fillRect(offX + x * scale, offY + y * scale, scale, scale, 0x0000);
            }
        }
    }

    Serial.println("[UI] drawQrFromBitmap: done");
}
```

---

## TASK 2 — ui.h: update drawQrScreen() to use bitmap endpoint

In drawQrScreen(), find the section that builds the QR URL and calls fetchImageBytes().

Currently the code calls:
```cpp
size_t pngLen = fetchImageBytes(qrUrl, g_pngBuf, 200 * 1024);
```
where `qrUrl` comes from the order response field `qr_code_url`.

The `qr_code_url` in the order response is the PNG URL:
`https://api.janishammer.com/v1/qr/<charge_id>`

We need to append `/bitmap` to get the bitmap URL.

**Find where `qrUrl` (or `g_currentQrUrl` or equivalent) is used in drawQrScreen().**
Read the actual variable name from the live file — do not assume.

Add bitmap URL construction BEFORE the fetchImageBytes call:

```cpp
// R-113: Use raw bitmap endpoint — PNGdec is broken for this hardware
// Append /bitmap to the qr_code_url (which points to the PNG endpoint)
String bitmapUrl = qrUrl;
if (!bitmapUrl.endsWith("/bitmap")) {
    bitmapUrl += "/bitmap";
}
Serial.printf("[UI] QR bitmap URL: %s\n", bitmapUrl.c_str());

size_t bmpLen = fetchImageBytes(bitmapUrl, g_pngBuf, 200 * 1024);
if (bmpLen > 4) {
    Serial.printf("[UI] QR bitmap loaded: %u bytes — rendering\n", bmpLen);
    gfx->fillRect(qrAreaX, qrAreaY, 244, 244, 0xFFFF);
    drawQrFromBitmap(g_pngBuf, bmpLen, qrAreaX + 2, qrAreaY + 2, 240, 240);
} else {
    Serial.println("[UI] QR bitmap failed — showing fallback");
    gfx->fillRect(qrAreaX, qrAreaY, 244, 244, 0xFFFF);
    gfx->setTextColor(0x8410);  // C_GREY equivalent
    gfx->setTextSize(1);
    gfx->setCursor(qrAreaX + 50, qrAreaY + 118);
    gfx->print("QR unavailable");
}
```

**Remove or comment out** the old PNGdec call (`drawQrFromBytes()` / `png.openRAM()` etc).
Do NOT delete the drawQrFromBytes() function itself — leave it in place commented out
in case it is needed for other image types later.

---

## TASK 3 — ui.h: verify g_pngBuf is large enough

Confirm `g_pngBuf` is allocated with at least 200*1024 bytes in initUI().
Bitmap for a 205x205 QR = 4 + 205*205 = ~42KB — well within 200KB.
No change needed if already 200KB. Just confirm in the read.

---

## DO NOT TOUCH
- hardware.h — R2 LOCKED — never open, never modify
- config.h — NUM_SLOTS, WiFi credentials
- state_machine.h — read only
- network.h — fetchImageBytes() is correct and working — do not touch
- satu_vending.ino — do not touch
- Payment mode — stays fake
- Any backend files

---

## EXPECTED SERIAL OUTPUT AFTER FLASH

```
[NET] fetchImageBytes: url=https://api.janishammer.com/v1/qr/fake_chg_xxxxx/bitmap
[NET] fetchImageBytes: HTTP 200
[NET] fetchImageBytes: Content-Length=42029
[NET] fetchImageBytes: 42029 bytes read
[UI] QR bitmap loaded: 42029 bytes — rendering
[UI] drawQrFromBitmap: w=205 h=205 dest=(x,y) size=240x240
[UI] drawQrFromBitmap: done
[UI] QR screen drawn
```

If this appears → QR is visible on screen → milestone complete.

---

## CI SELF-TEST — MANDATORY BEFORE OPENING PR

After pushing changes to branch:
1. Go to Actions tab in Csmittee/Satu-Vending-Firmware
2. Wait for compile-check workflow (~3-5 min)
3. ✅ GREEN → open PR, write "GitHub Actions compile: ✅ GREEN" in PR body
4. ❌ RED → fix in same branch, push again, wait for green
5. NEVER open a PR with red Actions

---

## NEW RULE — append to RULES.md at TOP

```
R-114: FIRMWARE QR USES BITMAP NOT PNG — PERMANENT (2026-06-14)
drawQrScreen() fetches /bitmap endpoint (R-113), not PNG.
drawQrFromBitmap() draws direct with gfx->fillRect() — no PNGdec.
drawQrFromBytes() (PNGdec) remains in ui.h commented out — do not delete.
Never re-enable PNGdec for QR on this hardware.
```

---

## MANDATORY (end of session)

1. Wait for GitHub Actions ✅ GREEN before opening PR
2. State "GitHub Actions compile: ✅ GREEN" in PR body
3. Append R-114 to RULES.md (newest at TOP)
4. Update KNOWN_GOOD.md at TOP:
```
## 2026-06-14 — QR bitmap draw (pending owner flash)
- Files: ui.h (drawQrFromBitmap + drawQrScreen bitmap fetch)
- Compile: ✅ GitHub Actions green
- Flash: ⬜ pending owner flash
- Expected serial: drawQrFromBitmap: w=205 h=205 ... done
```
5. Update PROJECT_STATE.md — QR bitmap firmware: ⬜ pending flash
6. Archive this prompt → docs/prompts/:
   `✅ COMPLETE — 2026-06-14 — Firmware QR bitmap draw (R-114)`
7. Commit and merge to main

## PAYMENT MODE REMINDER
PAYMENT_MODE must remain = fake for this entire session.
Never suggest changing to live.
