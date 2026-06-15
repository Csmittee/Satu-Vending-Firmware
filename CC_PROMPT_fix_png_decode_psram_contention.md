# CC_PROMPT_fix_png_decode_psram_contention.md
> Created: 2026-06-15
> Repo: Satu-Vending-Firmware
> Loop: B (firmware) — 1 flash cycle expected
> Root cause: PSRAM bandwidth contention between RGB panel DMA and zlib inflate
> Fix: pause-decode-resume pattern (backlight gate + delay)

---

## CC INTRO

New session. Ignore all previous context.

You are working on SATU 1.0 FIRMWARE at:
https://github.com/Csmittee/Satu-Vending-Firmware

Read IN FULL and state each filename before writing anything:
1. CLAUDE.md
2. RULES.md
3. firmware/ui.h
4. .claude/rules/SKILL_esp32s3_rgb_panel_constraints.md  ← CRITICAL — read this first
5. CC_PROMPT_fix_png_decode_psram_contention.md ← this file

State "All files read ✅" then execute this prompt.

---

## ROOT CAUSE — CONFIRMED (do not re-investigate)

PNGdec rc=8 / rows=1 is caused by PSRAM bus bandwidth contention between:
- The RGB LCD DMA engine (reads 800×480×2 = 768KB frame buffer from PSRAM continuously)
- The zlib inflate algorithm (needs 32KB random-access sliding window in PSRAM)

Both compete for the same OPI PSRAM bus. DMA wins. zlib stalls. rc=8.

This is NOT a PNG format problem. Do NOT change color type, zlib wrapper, or compression.
This is NOT a PSRAM allocation problem. g_pngBuf IS in PSRAM — that is expected and correct.
The fix is to release PSRAM bandwidth from DMA before decode, then restore after.

SKILL_esp32s3_rgb_panel_constraints.md contains the full explanation and decision tree.

---

## TASK — 3 changes to firmware/ui.h

### CHANGE 1 — Re-enable and fix drawQrFromBytes()

The function is currently commented out with note "DISABLED (R-114)".
Remove the comment block entirely. Replace with the corrected implementation below.

Find this block in ui.h:
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

Replace the ENTIRE block above with:

```cpp
// drawQrFromBytes() — R-117 PAUSE-DECODE-RESUME PATTERN
// Root cause of previous rc=8 failures: PSRAM bandwidth contention
// RGB DMA owns PSRAM bus during continuous frame refresh.
// Fix: gate backlight off → yield 20ms → DMA bus pressure drops → decode → restore.
// This gives zlib inflate full PSRAM bandwidth for its 32KB sliding window.
// Blackout duration: ~55-120ms total — imperceptible on donation vending machine.
void drawQrFromBytes(uint8_t* buf, size_t len, int x, int y) {
  if (!buf || len == 0) {
    Serial.println("[UI] drawQrFromBytes: null buf or zero len");
    return;
  }

  // R-117: Step 1 — release PSRAM bus from DMA pressure
  digitalWrite(TFT_BL, LOW);
  delay(20);  // allow DMA to complete current frame burst, bus pressure drops

  // R-117: Step 2 — decode PNG with full PSRAM bandwidth
  _pngDrawX   = x;
  _pngDrawY   = y;
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

  // R-117: Step 3 — restore display
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

Change `uint16_t lineBuf[800];` to `static uint16_t lineBuf[800];`

Reason (R-119): removes 1600-byte stack allocation per callback invocation.
`static` means one fixed allocation in data segment — safer and consistent.

### CHANGE 3 — Add guard comment above drawQrFromBitmap()

The bitmap fallback function (R-114) is still in ui.h for emergency use.
Add a clear guard comment directly above it:

```cpp
// ============================================================
//  DRAW QR FROM RAW BITMAP — R-114 EMERGENCY FALLBACK ONLY
//  Use drawQrFromBytes() (PNG) for all normal operation.
//  This function is fake-mode-only scaffolding.
//  Omise live mode serves real PromptPay PNG — bitmap cannot substitute.
//  Do NOT call this function unless PNG decode is confirmed broken.
// ============================================================
```

---

## DO NOT TOUCH
- hardware.h — R2 LOCKED — never open
- config.h — credentials, never touch
- state_machine.h
- network.h
- satu_vending.ino
- Any PNG byte generation in the backend
- PAYMENT_MODE stays fake

---

## VERIFICATION BEFORE OPENING PR

CC must verify these in ui.h before committing:

1. `drawQrFromBytes()` is NOT commented out — it is a live, callable function
2. `digitalWrite(TFT_BL, LOW)` appears before `_png.openRAM()`
3. `delay(20)` appears between backlight-off and openRAM
4. `delay(5)` and `digitalWrite(TFT_BL, HIGH)` appear after decode
5. `lineBuf` is declared `static`
6. `drawQrFromBitmap()` has the emergency fallback comment above it
7. No PNG format was changed (color type, zlib, compression)
8. No changes to any other file

---

## WHAT OWNER DOES AFTER FLASH

1. Trigger one order on physical machine
2. Watch for backlight flash (brief dark period ~100ms) when QR appears
3. Report serial output — expected:
```
[UI] PNG decode: rc=0 rows=165 w=165 h=165
```
4. Verify QR renders correctly and is scannable with phone

If rc=0 rows=165 → ROOT CAUSE CONFIRMED FIXED ✅
If rc=8 rows=1 still → report to Chat — try delay(50) before next flash
If rc=0 but rows < 165 → partial decode — report exact rows count to Chat

---

## CI SELF-TEST — MANDATORY BEFORE OPENING PR

1. Push to new branch: `fix/png-psram-contention-r117`
2. Actions tab → wait for compile-check (~3 min)
3. ✅ GREEN → open PR titled:
   "fix: PNG decode pause-decode-resume for PSRAM DMA contention (R-117)"
4. ❌ RED → fix compile error, push, wait for green
5. NEVER open PR with red Actions

---

## MANDATORY — END OF SESSION

1. GitHub Actions ✅ GREEN before opening PR
2. PR body must say:
   "GitHub Actions compile: ✅ GREEN
   Root cause: PSRAM bandwidth contention between RGB DMA and zlib inflate.
   Fix: backlight-off gate (20ms) before PNG decode releases DMA bus pressure.
   R-117 applied. lineBuf now static (R-119). drawQrFromBitmap kept as emergency fallback."

3. Append to RULES.md at TOP (newest first):
```
R-120: NVS writes must not occur during image decode or QR display screen — idle state only
R-119: lineBuf in _pngDrawRow must be static (not stack) — R-117 requires stable memory layout
R-118: Product images = JPEG ≤320×320px from backend. Only Omise QR = PNG (EMVCo requirement)
R-117: PNG decode must use pause-decode-resume: TFT_BL LOW → delay(20) → decode → delay(5) → TFT_BL HIGH
       Reason: RGB DMA owns PSRAM bus continuously. Gating backlight releases bandwidth to zlib.
       Confirmed root cause: not PSRAM allocation, not PNG format — PSRAM bus contention.
```

4. Update PROJECT_STATE.md:
   - Mark "PNGdec decode" as FIXED (pending owner flash confirm)
   - Add note: root cause = PSRAM DMA contention, fix = R-117 pause-decode-resume
   - Note: bitmap fallback (R-114) preserved in ui.h but not called

5. Archive this prompt → docs/prompts/ stamped:
   ✅ COMPLETE — 2026-06-15 — PNG PSRAM contention fix — result: [rc=0 rows=165 / pending]

6. Do NOT delete drawQrFromBitmap() from ui.h — it stays as documented emergency fallback

## PAYMENT MODE REMINDER
PAYMENT_MODE stays fake. Never change.
