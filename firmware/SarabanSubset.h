// ============================================================
// SarabanSubset.h — Thai GFXfont placeholder for Satu D-11
// ============================================================
// Version: R1 — 2026-06-24
// Purpose: Thai bitmap font (Sarabun-family subset) for sale sequence.
//          Three sizes: 12pt (status bar), 18pt (body), 24pt (headings).
//
// PLACEHOLDER STATUS: All glyph bitmaps are zero bytes.
//   Thai text areas display blank until owner generates real bitmaps.
//   To generate real bitmaps:
//     1. Install fontconvert from Adafruit GFX library tools
//     2. Download Sarabun-Regular.ttf (https://fonts.google.com/specimen/Sarabun)
//     3. Run: fontconvert Sarabun-Regular.ttf 12 0x0E01 0x0E4C > SarabanSubset_12pt_bitmaps.h
//        (repeat for 18 and 24)
//     4. Replace bitmap arrays and GFXglyph arrays below with fontconvert output
//     5. Keep the GFXfont struct definitions (name, first, last, yAdvance)
//
// COMBINING MARKS (xAdvance=0, overlay preceding consonant):
//   U+0E31 ั U+0E34-0E37 ิ ี ึ ื U+0E38-0E39 ุ ู
//   U+0E3A ฺ U+0E47-0E4C ็ ่ ้ ๊ ๋ ์
//
// RENDERING: Use printThai(x, y, utf8, color, font) in ui_strings.h.
//   gfx->print() does NOT handle UTF-8 Thai — always use printThai().
//
// Range: U+0E01 (ก) to U+0E4C (์) = 76 glyphs
// Estimated size with real bitmaps: ~6KB for all three sizes (PROGMEM)
// ============================================================

#pragma once

#include <Arduino_GFX_Library.h>

// ── Shared zero bitmap (placeholder — all glyphs invisible until fontconvert) ──
// 64 bytes: enough for a 24pt glyph worst case (17×24 = 408 bits = 51 bytes).
static const uint8_t _SarabanZeroBitmaps[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// ── Glyph index helpers ────────────────────────────────────────────────
// All 76 glyphs share bitmapOffset=0 (all zero — same placeholder bitmap).
// NC = non-combining consonant/vowel, CA = combining above, CB = combining below, EM = empty

// 12pt glyph variants
#define _NC12  { 0,  10, 13, 12, -1, -13 }
#define _CA12  { 0,  10,  5,  0, -10, -18 }
#define _CB12  { 0,  10,  4,  0, -10,  -2 }
#define _EM12  { 0,   0,  0,  0,   0,   0 }

// 18pt glyph variants
#define _NC18  { 0,  13, 18, 15, -1, -18 }
#define _CA18  { 0,  13,  7,  0, -13, -25 }
#define _CB18  { 0,  13,  6,  0, -13,  -2 }
#define _EM18  { 0,   0,  0,  0,   0,   0 }

// 24pt glyph variants
#define _NC24  { 0,  17, 24, 19, -1, -24 }
#define _CA24  { 0,  17,  9,  0, -17, -32 }
#define _CB24  { 0,  17,  8,  0, -17,  -3 }
#define _EM24  { 0,   0,  0,  0,   0,   0 }

// ════════════════════════════════════════════════════════════════════════
//  12pt — STATUS BAR
// ════════════════════════════════════════════════════════════════════════

static const GFXglyph SarabanSubset_12ptGlyphs[] PROGMEM = {
  // idx  Unicode  char  type
  _NC12,  // 0    0E01  ก   NC
  _NC12,  // 1    0E02  ข   NC
  _NC12,  // 2    0E03  ฃ   NC
  _NC12,  // 3    0E04  ค   NC
  _NC12,  // 4    0E05  ฅ   NC
  _NC12,  // 5    0E06  ฆ   NC
  _NC12,  // 6    0E07  ง   NC
  _NC12,  // 7    0E08  จ   NC
  _NC12,  // 8    0E09  ฉ   NC
  _NC12,  // 9    0E0A  ช   NC
  _NC12,  // 10   0E0B  ซ   NC
  _NC12,  // 11   0E0C  ฌ   NC
  _NC12,  // 12   0E0D  ญ   NC
  _NC12,  // 13   0E0E  ฎ   NC
  _NC12,  // 14   0E0F  ฏ   NC
  _NC12,  // 15   0E10  ฐ   NC
  _NC12,  // 16   0E11  ฑ   NC
  _NC12,  // 17   0E12  ฒ   NC
  _NC12,  // 18   0E13  ณ   NC
  _NC12,  // 19   0E14  ด   NC
  _NC12,  // 20   0E15  ต   NC
  _NC12,  // 21   0E16  ถ   NC
  _NC12,  // 22   0E17  ท   NC
  _NC12,  // 23   0E18  ธ   NC
  _NC12,  // 24   0E19  น   NC
  _NC12,  // 25   0E1A  บ   NC
  _NC12,  // 26   0E1B  ป   NC
  _NC12,  // 27   0E1C  ผ   NC
  _NC12,  // 28   0E1D  ฝ   NC
  _NC12,  // 29   0E1E  พ   NC
  _NC12,  // 30   0E1F  ฟ   NC
  _NC12,  // 31   0E20  ภ   NC
  _NC12,  // 32   0E21  ม   NC
  _NC12,  // 33   0E22  ย   NC
  _NC12,  // 34   0E23  ร   NC
  _NC12,  // 35   0E24  ฤ   NC
  _NC12,  // 36   0E25  ล   NC
  _NC12,  // 37   0E26  ฦ   NC
  _NC12,  // 38   0E27  ว   NC
  _NC12,  // 39   0E28  ศ   NC
  _NC12,  // 40   0E29  ษ   NC
  _NC12,  // 41   0E2A  ส   NC
  _NC12,  // 42   0E2B  ห   NC
  _NC12,  // 43   0E2C  ฬ   NC
  _NC12,  // 44   0E2D  อ   NC
  _NC12,  // 45   0E2E  ฮ   NC
  _NC12,  // 46   0E2F  ๆ   NC
  _NC12,  // 47   0E30  ะ   NC
  _CA12,  // 48   0E31  ั   CA
  _NC12,  // 49   0E32  า   NC
  _NC12,  // 50   0E33  ำ   NC
  _CA12,  // 51   0E34  ิ   CA
  _CA12,  // 52   0E35  ี   CA
  _CA12,  // 53   0E36  ึ   CA
  _CA12,  // 54   0E37  ื   CA
  _CB12,  // 55   0E38  ุ   CB
  _CB12,  // 56   0E39  ู   CB
  _CA12,  // 57   0E3A  ฺ   CA
  _EM12,  // 58   0E3B  --  EM
  _EM12,  // 59   0E3C  --  EM
  _EM12,  // 60   0E3D  --  EM
  _EM12,  // 61   0E3E  --  EM
  _NC12,  // 62   0E3F  ฿   NC
  _NC12,  // 63   0E40  เ   NC
  _NC12,  // 64   0E41  แ   NC
  _NC12,  // 65   0E42  โ   NC
  _NC12,  // 66   0E43  ใ   NC
  _NC12,  // 67   0E44  ไ   NC
  _NC12,  // 68   0E45  ๅ   NC
  _NC12,  // 69   0E46  ๆ   NC
  _CA12,  // 70   0E47  ็   CA
  _CA12,  // 71   0E48  ่   CA
  _CA12,  // 72   0E49  ้   CA
  _CA12,  // 73   0E4A  ๊   CA
  _CA12,  // 74   0E4B  ๋   CA
  _CA12,  // 75   0E4C  ์   CA
};

const GFXfont SarabanSubset_12pt PROGMEM = {
  (uint8_t*)_SarabanZeroBitmaps,
  (GFXglyph*)SarabanSubset_12ptGlyphs,
  0,   // first = glyph index 0 (GFXfont.first is uint8_t — Unicode base 0x0E01 handled in printThai)
  75,  // last  = glyph index 75 (76 glyphs: U+0E01..U+0E4C)
  18   // yAdvance
};

// ════════════════════════════════════════════════════════════════════════
//  18pt — BODY TEXT
// ════════════════════════════════════════════════════════════════════════

static const GFXglyph SarabanSubset_18ptGlyphs[] PROGMEM = {
  _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18,  // 0-7  ก-จ
  _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18,  // 8-15 ฉ-ฐ
  _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18,  // 16-23 ฑ-ธ
  _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18,  // 24-31 น-ภ
  _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18,  // 32-39 ม-ศ
  _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18,  // 40-47 ษ-ะ
  _CA18,                                                     // 48    ั
  _NC18, _NC18,                                              // 49-50 า ำ
  _CA18, _CA18, _CA18, _CA18,                               // 51-54 ิ ี ึ ื
  _CB18, _CB18,                                              // 55-56 ุ ู
  _CA18,                                                     // 57    ฺ
  _EM18, _EM18, _EM18, _EM18,                               // 58-61 undefined
  _NC18,                                                     // 62    ฿
  _NC18, _NC18, _NC18, _NC18, _NC18, _NC18, _NC18,         // 63-69 เ-ๆ
  _CA18, _CA18, _CA18, _CA18, _CA18, _CA18,                 // 70-75 ็ ่ ้ ๊ ๋ ์
};

const GFXfont SarabanSubset_18pt PROGMEM = {
  (uint8_t*)_SarabanZeroBitmaps,
  (GFXglyph*)SarabanSubset_18ptGlyphs,
  0,   // first = glyph index 0 (GFXfont.first is uint8_t — Unicode base 0x0E01 handled in printThai)
  75,  // last  = glyph index 75
  26   // yAdvance
};

// ════════════════════════════════════════════════════════════════════════
//  24pt — HEADINGS
// ════════════════════════════════════════════════════════════════════════

static const GFXglyph SarabanSubset_24ptGlyphs[] PROGMEM = {
  _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24,  // 0-7
  _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24,  // 8-15
  _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24,  // 16-23
  _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24,  // 24-31
  _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24,  // 32-39
  _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24,  // 40-47
  _CA24,                                                     // 48
  _NC24, _NC24,                                              // 49-50
  _CA24, _CA24, _CA24, _CA24,                               // 51-54
  _CB24, _CB24,                                              // 55-56
  _CA24,                                                     // 57
  _EM24, _EM24, _EM24, _EM24,                               // 58-61
  _NC24,                                                     // 62
  _NC24, _NC24, _NC24, _NC24, _NC24, _NC24, _NC24,         // 63-69
  _CA24, _CA24, _CA24, _CA24, _CA24, _CA24,                 // 70-75
};

const GFXfont SarabanSubset_24pt PROGMEM = {
  (uint8_t*)_SarabanZeroBitmaps,
  (GFXglyph*)SarabanSubset_24ptGlyphs,
  0,   // first = glyph index 0 (GFXfont.first is uint8_t — Unicode base 0x0E01 handled in printThai)
  75,  // last  = glyph index 75
  34   // yAdvance
};

// Undefine macros to avoid pollution
#undef _NC12
#undef _CA12
#undef _CB12
#undef _EM12
#undef _NC18
#undef _CA18
#undef _CB18
#undef _EM18
#undef _NC24
#undef _CA24
#undef _CB24
#undef _EM24
