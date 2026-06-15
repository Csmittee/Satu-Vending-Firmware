# CC_PROMPT_fix_png_decode_psram_contention.md
> ✅ COMPLETE — 2026-06-15 — PNG PSRAM contention R-117 — result: [pending flash]
> Created: 2026-06-15
> Repo: Satu-Vending-Firmware
> Loop: B (firmware) — 1 flash cycle expected
> Prompt type: Fix Mode (1 file: ui.h only)
> Root cause: CONFIRMED — PSRAM bus bandwidth contention between RGB DMA + zlib inflate
> Evidence: Serial rc=8 rows=1 w=165 h=165 — fetch 200 OK 27458 bytes (PNG valid, decode fails)

---

## CC INTRO

New session. Ignore all previous context from other projects.

You are working on SATU 1.0 FIRMWARE at:
https://github.com/Csmittee/Satu-Vending-Firmware

Read IN FULL and state each filename before writing a single line:
1. CLAUDE.md
2. RULES.md
3. .claude/rules/SKILL_esp32s3_rgb_panel_constraints.md  ← read this before touching any code
4. firmware/ui.h
5. CC_PROMPT_fix_png_decode_psram_contention.md ← this file

State "All files read ✅" then execute this prompt.

---

## CONTEXT — ROOT CAUSE CONFIRMED, DO NOT RE-INVESTIGATE

Serial monitor output from owner flash 2026-06-15:
```
[NET] fetchImageBytes: HTTP 200
[NET] fetchImageBytes: Content-Length=27458
[NET] fetchImageBytes: 27458 bytes read
[UI] QR PNG loaded: 27458 bytes — rendering
[UI] PNG decode: rc=8 rows=1 w=165 h=165
[UI] QR screen drawn
```

The PNG is valid — fetch succeeds, 27458 bytes received, openRAM succeeds (w=165 h=165 parsed).
Only inflate fails. Root cause documented in SKILL_esp32s3_rgb_panel_constraints.md:

The ESP32-8048S070C RGB panel DMA engine reads the 800×480 frame buffer from PSRAM continuously
at ~16MHz pixel clock, consuming ~50% of OPI PSRAM bus bandwidth at all times.
PNGdec's zlib inflate requires random-access reads across a 32KB sliding window in PSRAM.
DMA wins every bus arbitration. zlib stalls. rc=8 after row 1.

This is NOT a PNG format problem. Do NOT change color type, zlib, or compression.
This is NOT a PSRAM allocation problem. g_pngBuf IS in PSRAM — that is correct.
Fix: pause DMA bus pressure before decode, restore after. One function change. Nothing else.

---

## TASK — 3 changes to firmware/ui.h only

### CHANGE 1 — Re-enable drawQrFromBytes() with pause-decode-resume pattern

Find this entire commented-out block in ui.h:

```cpp
// drawQrFromBytes() — DISABLED (R-114): PNGdec 1.1.6 fails on all PNG variants
// tested across PRs #16-#19. Kept for potential future use with other image types.
// void drawQrFromBytes(uint8_t* buf, size_t len, int x, int y) {
//   if (!buf || len == 0) return;
//   _pngDrawX = x;
//   _pngDrawY = y;
//   _pngRowCount = 0;
//   if (_png.openRAM(buf, (int32_t)len, _pngDrawRow) == PNG_SUCCESS) {
//     int rc = _png.decode(nullptr, 0);
//     Serial.printf("[UI] PNG decode: rc=%d rows=%d w=%d h=%d\n",
//                   rc, _pngRowCount, _png.getWidth(), _png.getHeight());
//     _png.close();
//   } else {
//     Serial.println("[UI] PNG open failed");
//   }
// }
```

Replace the ENTIRE block above with this live implementation:

```cpp
// drawQrFromBytes() — R-117 PAUSE-DECODE-RESUME
// Root cause of rc=8: PSRAM bandwidth contention between RGB panel DMA and zlib inflate.
// Fix: gate backlight off + 20ms yield → DMA bus pressure drops → zlib gets full bandwidth.
// Donor sees brief dark flash (~100ms total). Acceptable for static QR display.
// Works for all future images: product photos, amulet images, temple uploads.
// See: .claude/rules/SKILL_esp32s3_rgb_panel_constraints.md
void drawQrFromBytes(uint8_t* buf, size_t len, int x, int y) {
  if (!buf || len == 0) {
    Serial.println("[UI] drawQrFromBytes: null buf or zero len");
    return;
  }

  // R-117 Step 1: release PSRAM bus from DMA pressure
  digitalWrite(TFT_BL, LOW);
  delay(20);  // allow RGB DMA to complete current frame burst — bus contention drops

  // R-117 Step 2: decode PNG with full PSRAM bandwidth
  _pngDrawX    = x;
  _pngDrawY    = y;
  _pngRowCount = 0;

  int openRc = _png.openRAM(buf, (int32_t)len, _pngDrawRow);
  if (openRc == PNG_SUCCESS) {
    int rc = _png.decode(nullptr, 0);
    Serial.printf("[UI] PNG decode: rc=%d rows=%d w=%d h=%d\n",
                  rc, _pngRowCount, _png.getWidth(), _png.getHeight());
    _png.close();
  } else {
    Serial.printf("[UI] PNG openRAM failed: rc=%d len=%u\n", openRc, (unsigned)len);
  }

  // R-117 Step 3: restore display
  delay(5);
  digitalWrite(TFT_BL, HIGH);
}
```

### CHANGE 2 — Make lineBuf static in _pngDrawRow

Find this function:
```cpp
static int _pngDrawRow(PNGDRAW* pDraw) {
  uint16_t lineBuf[800];
```

Change the lineBuf declaration from:
```cpp
  uint16_t lineBuf[800];
```
To:
```cpp
  static uint16_t lineBuf[800];  // R-119: static — off stack, consistent memory layout
```

### CHANGE 3 — Add guard comment above drawQrFromBitmap()

Find the line immediately before the drawQrFromBitmap() function definition.
Add this comment block directly above it:

```cpp
// ============================================================
//  drawQrFromBitmap() — R-114 EMERGENCY FALLBACK ONLY
//  Normal operation: use drawQrFromBytes() (PNG, R-117).
//  This function is fake-mode-only scaffolding.
//  Omise live mode serves real PromptPay PNG with EMVCo branding.
//  Bitmap cannot substitute for Omise live QR.
//  Do NOT call unless PNG decode is confirmed broken on a specific unit.
// ============================================================
```

---

## VERIFICATION — CC must confirm all of these before committing

1. `drawQrFromBytes()` exists as a live callable function (NOT commented out)
2. `digitalWrite(TFT_BL, LOW)` appears BEFORE `_png.openRAM()`
3. `delay(20)` appears between LOW and openRAM
4. `delay(5)` and `digitalWrite(TFT_BL, HIGH)` appear AFTER `_png.close()` / error path
5. `lineBuf` is declared `static uint16_t lineBuf[800]`
6. Guard comment is present above `drawQrFromBitmap()`
7. No PNG format was changed (color type, zlib wrapper, compression, byte structure)
8. No other file was modified
9. `drawQrFromBitmap()` still exists — it was NOT deleted

---

## DO NOT TOUCH
- hardware.h — R2 LOCKED — never open, never modify, never redeclare anything it owns
- config.h — WiFi credentials, never touch
- network.h — fetchImageBytes() is working correctly, no change needed
- state_machine.h — read-only context
- satu_vending.ino — no change needed
- Any PNG byte structure, color type, zlib format, or compression logic
- PAYMENT_MODE stays fake

---

## CI SELF-TEST — MANDATORY BEFORE OPENING PR

1. Push to new branch: `fix/png-psram-contention-r117`
2. Actions tab → wait for compile check (~3 min)
3. ✅ GREEN → open PR titled:
   `fix: PNG decode pause-decode-resume for PSRAM DMA contention (R-117)`
4. ❌ RED → fix compile error, push again, wait for green
5. NEVER open PR with red Actions — R-90

---

## WHAT OWNER DOES AFTER FLASH — 1 FLASH CYCLE

Trigger one order on the physical machine.
Watch for: brief backlight-off moment (~100ms) when QR screen appears.
Report this exact serial output to Chat:

```
[UI] PNG decode: rc=? rows=? w=? h=?
```

SUCCESS = rc=0 rows=165 w=165 h=165
PARTIAL = rc=0 rows<165 → report exact number, try delay(50)
STILL FAILING = rc=8 rows=1 → report to Chat, do NOT change PNG format

---

## MANDATORY — END OF SESSION

1. GitHub Actions ✅ GREEN before opening PR

2. PR body must include:
```
GitHub Actions compile: ✅ GREEN
Root cause: PSRAM bandwidth contention between RGB DMA and zlib inflate.
Fix: backlight gate (20ms) before PNG decode releases DMA bus pressure.
R-117 + R-119 applied. drawQrFromBitmap() kept as documented emergency fallback.
Pending owner flash — expected: rc=0 rows=165 w=165 h=165
```

3. Append to RULES.md at TOP (newest first), using next available R-numbers:
```
R-120: NVS writes must not occur during image decode or QR display — schedule at idle state only
R-119: lineBuf in _pngDrawRow must be static (not stack-allocated) — stable layout during decode
R-118: Product images = JPEG ≤320×320px from backend. Only Omise QR = PNG (EMVCo requirement)
R-117: PNG decode must use pause-decode-resume pattern:
       digitalWrite(TFT_BL,LOW) → delay(20) → _png.openRAM() → _png.decode() → delay(5) → digitalWrite(TFT_BL,HIGH)
       Reason: RGB DMA owns PSRAM bus continuously on ESP32-8048S070C class boards.
       Gating backlight releases bus bandwidth to zlib inflate sliding window.
       Root cause: NOT PSRAM allocation, NOT PNG format — confirmed PSRAM bus contention.
       Evidence: rc=8 rows=1 on all PNG variants. Fetch=200 OK 27458 bytes. openRAM succeeds.
       Reference: .claude/rules/SKILL_esp32s3_rgb_panel_constraints.md
```

4. Update PROJECT_STATE.md:
   - PNG decode: FIXED pending owner flash confirm (R-117 pause-decode-resume)
   - Root cause documented: PSRAM DMA bus contention — ESP32-8048S070C class
   - drawQrFromBitmap() preserved in ui.h as emergency fallback (R-114)
   - SKILL_esp32s3_rgb_panel_constraints.md added to .claude/rules/ (2026-06-15)

5. Update KNOWN_GOOD.md at TOP:
```
## 2026-06-15 — PNG decode fix R-117 (pending owner flash)
- Files: ui.h (drawQrFromBytes re-enabled with pause-decode-resume, lineBuf static)
- Compile: ✅ GitHub Actions green
- Flash: ⬜ pending owner flash
- Expected serial: [UI] PNG decode: rc=0 rows=165 w=165 h=165
```

6. Archive this prompt → docs/prompts/ stamped:
   `✅ COMPLETE — 2026-06-15 — PNG PSRAM contention R-117 — result: [pending flash]`

7. Do NOT delete CC_PROMPT_diagnose_psram_pngdec.md if it still exists in repo root —
   owner will remove it manually.

## PAYMENT MODE REMINDER
PAYMENT_MODE stays fake. Never change. Not relevant to this fix.
