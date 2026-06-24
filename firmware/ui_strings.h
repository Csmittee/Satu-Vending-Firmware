#ifndef UI_STRINGS_H
#define UI_STRINGS_H

// ui_strings.h — Version R2 — 2026-06-24
// D-11: Thai language support added.
//   - g_lang_th / g_lang_th_default moved here from ui.h (authoritative definition)
//   - _stateLabels_TH[] + _sl() accessor for bilingual status bar
//   - Thai string constants for all customer-facing sale screens
//   - printThai() custom UTF-8→Thai GFXglyph renderer (Arduino_GFX cannot handle Thai Unicode)
//   - SarabanSubset.h font objects available via ui.h include (SarabanSubset.h included before this)
// Previous: R1 — 2026-06-22 (EN only, g_lang_th stub)

// ============================================================
//  LANGUAGE FLAGS
//  g_lang_th         — session flag: true = Thai customer UI this session
//  g_lang_th_default — NVS-persisted operator default (loaded in network.h loadConfigFromNVS)
// ============================================================
#define LANG_TH
bool g_lang_th         = false;
bool g_lang_th_default = false;

// ============================================================
//  STATUS BAR STATE
// ============================================================
enum StatusBarState { SB_IDLE=0, SB_CONFIRM, SB_PAYMENT, SB_DISPENSING, SB_CONNECTING };

static const char* _stateLabels[] = {
  "Select Item",
  "Confirm",
  "Payment",
  "Dispensing",
  "Connecting..."
};

static const char* _stateLabels_TH[] = {
  "\xe0\xb9\x80\xe0\xb8\xa5\xe0\xb8\xb7\xe0\xb8\xad\xe0\xb8\x81\xe0\xb8\xaa\xe0\xb8\xb4\xe0\xb8\x99\xe0\xb8\x84\xe0\xb9\x89\xe0\xb8\xb2",  // เลือกสินค้า
  "\xe0\xb8\xa2\xe0\xb8\xb7\xe0\xb8\x99\xe0\xb8\xa2\xe0\xb8\xb1\xe0\xb8\x99",                                                                  // ยืนยัน
  "\xe0\xb8\x8a\xe0\xb8\xb3\xe0\xb8\xa3\xe0\xb8\xb0\xe0\xb9\x80\xe0\xb8\x87\xe0\xb8\xb4\xe0\xb8\x99",                                         // ชำระเงิน
  "\xe0\xb8\x81\xe0\xb8\xb3\xe0\xb8\xa5\xe0\xb8\xb1\xe0\xb8\x87\xe0\xb8\x88\xe0\xb9\x88\xe0\xb8\xb2\xe0\xb8\xa2",                            // กำลังจ่าย
  "\xe0\xb8\x81\xe0\xb8\xb3\xe0\xb8\xa5\xe0\xb8\xb1\xe0\xb8\x87\xe0\xb9\x80\xe0\xb8\x8a\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb8\xa1\xe0\xb8\x95\xe0\xb9\x88\xe0\xb8\xad..."  // กำลังเชื่อมต่อ...
};

static inline const char* _sl(StatusBarState state) {
  if (g_lang_th) return _stateLabels_TH[state];
  return _stateLabels[state];
}

// ============================================================
//  SERVICE TAB LABELS
// ============================================================
static const char* _svcTabL1[5] = { "Self",  "Free", "Devices", "Settings", "Firmware" };
static const char* _svcTabL2[5] = { "Test",  "Play", "",        "",         "" };

// ============================================================
//  WELCOME SCREEN STRINGS
// ============================================================
#define STR_WEL_TITLE_EN   "SATU"
#define STR_WEL_SUB_EN     "Blessed Vending"
#define STR_WEL_SUB_TH     "\xe0\xb9\x80\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb8\xb7\xe0\xb9\x88\xe0\xb8\xad\xe0\xb8\x87\xe0\xb8\x88\xe0\xb8\xb3\xe0\xb8\xab\xe0\xb8\x99\xe0\xb9\x88\xe0\xb8\xb2\xe0\xb8\xa2\xe0\xb8\x9e\xe0\xb8\xa3"  // เครื่องจำหน่ายพร
#define STR_WEL_PROMPT_EN  "Tap language to begin"
#define STR_WEL_PROMPT_TH  "\xe0\xb8\x81\xe0\xb8\xa3\xe0\xb8\xb8\xe0\xb8\x93\xe0\xb8\xb2\xe0\xb9\x80\xe0\xb8\xa5\xe0\xb8\xb7\xe0\xb8\xad\xe0\xb8\x81\xe0\xb8\xa0\xe0\xb8\xb2\xe0\xb8\xa9\xe0\xb8\xb2"  // กรุณาเลือกภาษา

// ============================================================
//  GIFT OPTION SCREEN STRINGS
// ============================================================
#define STR_GIFT_TITLE_EN  "Choose your blessing"
#define STR_GIFT_TITLE_TH  "\xe0\xb9\x80\xe0\xb8\xa5\xe0\xb8\xb7\xe0\xb8\xad\xe0\xb8\x81\xe0\xb8\x82\xe0\xb8\xad\xe0\xb8\x87\xe0\xb8\x97\xe0\xb8\xb5\xe0\xb9\x88\xe0\xb8\xa3\xe0\xb8\xb0\xe0\xb8\xa5\xe0\xb8\xb6\xe0\xb8\x81"  // เลือกของที่ระลึก
#define STR_GIFT_ITEM_EN   "Item Only"
#define STR_GIFT_ITEM_TH   "\xe0\xb8\xaa\xe0\xb8\xb4\xe0\xb8\x99\xe0\xb8\x84\xe0\xb9\x89\xe0\xb8\xb2\xe0\xb9\x80\xe0\xb8\x97\xe0\xb9\x88\xe0\xb8\xb2\xe0\xb8\x99\xe0\xb8\xb1\xe0\xb9\x89\xe0\xb8\x99"  // สินค้าเท่านั้น
#define STR_GIFT_WATER_EN  "+Sacred Water"
#define STR_GIFT_WATER_TH  "+\xe0\xb8\x99\xe0\xb9\x89\xe0\xb8\xb3\xe0\xb8\xa1\xe0\xb8\x99\xe0\xb8\x95\xe0\xb9\x8c"  // +น้ำมนต์
#define STR_BACK_EN        "< Back"
#define STR_BACK_TH        "< \xe0\xb8\xa2\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\x99\xe0\xb8\x81\xe0\xb8\xa5\xe0\xb8\xb1\xe0\xb8\x9a"  // < ย้อนกลับ

// ============================================================
//  CONFIRM SCREEN STRINGS
// ============================================================
#define STR_CONF_TITLE_EN  "Confirm your order"
#define STR_CONF_TITLE_TH  "\xe0\xb8\xa2\xe0\xb8\xb7\xe0\xb8\x99\xe0\xb8\xa2\xe0\xb8\xb1\xe0\xb8\x99\xe0\xb8\x84\xe0\xb8\xb3\xe0\xb8\xaa\xe0\xb8\xb1\xe0\xb9\x88\xe0\xb8\x87\xe0\xb8\x8b\xe0\xb8\xb7\xe0\xb9\x89\xe0\xb8\xad"  // ยืนยันคำสั่งซื้อ
#define STR_CONF_BTN_EN    "Confirm >"
#define STR_CONF_BTN_TH    "\xe0\xb8\xa2\xe0\xb8\xb7\xe0\xb8\x99\xe0\xb8\xa2\xe0\xb8\xb1\xe0\xb8\x99 >"  // ยืนยัน >

// ============================================================
//  VENDING SCREEN STRINGS
// ============================================================
#define STR_VEND_TITLE_EN  "Dispensing..."
#define STR_VEND_TITLE_TH  "\xe0\xb8\x81\xe0\xb8\xb3\xe0\xb8\xa5\xe0\xb8\xb1\xe0\xb8\x87\xe0\xb8\x88\xe0\xb9\x88\xe0\xb8\xb2\xe0\xb8\xa2\xe0\xb8\xaa\xe0\xb8\xb4\xe0\xb8\x99\xe0\xb8\x84\xe0\xb9\x89\xe0\xb8\xb2..."  // กำลังจ่ายสินค้า...
#define STR_VEND_WAIT_EN   "Please wait..."
#define STR_VEND_WAIT_TH   "\xe0\xb8\x81\xe0\xb8\xa3\xe0\xb8\xb8\xe0\xb8\x93\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\xad\xe0\xb8\xaa\xe0\xb8\xb1\xe0\xb8\x81\xe0\xb8\x84\xe0\xb8\xa3\xe0\xb8\xb9\xe0\xb9\x88..."  // กรุณารอสักครู่...

// ============================================================
//  COMPLETION SCREEN STRINGS
// ============================================================
#define STR_DONE_LUCKY_EN  "Your Merit Lucky Number"
#define STR_DONE_LUCKY_TH  "\xe0\xb8\xab\xe0\xb8\xa1\xe0\xb8\xb2\xe0\xb8\xa2\xe0\xb9\x80\xe0\xb8\xa5\xe0\xb8\x82\xe0\xb8\xa1\xe0\xb8\x87\xe0\xb8\x84\xe0\xb8\xa5"  // หมายเลขมงคล
#define STR_DONE_BLESS_EN  "May blessings be upon you"
#define STR_DONE_BLESS_TH  "\xe0\xb8\x82\xe0\xb8\xad\xe0\xb9\x83\xe0\xb8\xab\xe0\xb9\x89\xe0\xb9\x82\xe0\xb8\x8a\xe0\xb8\x84\xe0\xb8\x94\xe0\xb8\xb5\xe0\xb8\xa1\xe0\xb8\xb5\xe0\xb8\xaa\xe0\xb8\xb8\xe0\xb8\x82"  // ขอให้โชคดีมีสุข
#define STR_DONE_THANKS_EN "Thank you for your donation"
#define STR_DONE_THANKS_TH "\xe0\xb8\x82\xe0\xb8\xad\xe0\xb8\x9a\xe0\xb8\x84\xe0\xb8\xb8\xe0\xb8\x93\xe0\xb8\xaa\xe0\xb8\xb3\xe0\xb8\xab\xe0\xb8\xa3\xe0\xb8\xb1\xe0\xb8\x9a\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb8\x9a\xe0\xb8\xa3\xe0\xb8\xb4\xe0\xb8\x88\xe0\xb8\xb2\xe0\xb8\x84"  // ขอบคุณสำหรับการบริจาค

// ============================================================
//  printThai() — custom UTF-8 Thai renderer
//
//  Arduino_GFX 1.4.9 drawChar() takes unsigned char (8-bit only).
//  Thai Unicode U+0E01–U+0E4C cannot be rendered via gfx->print("...").
//  This function decodes 3-byte UTF-8 sequences to codepoints,
//  maps them to SarabanSubset glyph indices, and renders via
//  pgm_read_byte() pixel loop directly from GFXglyph bitmap data.
//
//  GFXfont.first/last are uint8_t — Thai Unicode range (0x0E01–0x0E4C)
//  is hardcoded here, NOT read from font->first/last.
//
//  Combining marks (ั ิ ี ึ ื ุ ู ฺ ็ ่ ้ ๊ ๋ ์) have xAdvance=0 in
//  SarabanSubset — cursor does NOT advance, mark overlays preceding consonant.
//
//  Call: printThai(x, baseline_y, utf8_string, fg_color, &SarabanSubset_18pt)
// ============================================================
static void printThai(int16_t x, int16_t y, const char* utf8, uint16_t fg, const GFXfont* font) {
  if (!font || !utf8) return;
  int16_t cx = x;
  const uint8_t* p = (const uint8_t*)utf8;
  while (*p) {
    uint32_t cp = 0;
    uint8_t  skip = 1;
    if ((p[0] & 0xF0) == 0xE0 && p[1] && p[2]) {
      cp   = ((uint32_t)(p[0] & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
      skip = 3;
    } else if (*p < 0x80) {
      cp   = *p;
      skip = 1;
    }
    p += skip;

    // Thai Unicode block: U+0E01 ก — U+0E4C ์ (76 glyphs)
    if (cp < 0x0E01 || cp > 0x0E4C) continue;
    uint16_t gi = (uint16_t)(cp - 0x0E01);

    GFXglyph* g = &((GFXglyph*)(font->glyph))[gi];
    if (g->width && g->height) {
      const uint8_t* bmp = font->bitmap + g->bitmapOffset;
      int16_t bx = cx + g->xOffset;
      int16_t by = y  + g->yOffset;
      for (uint8_t row = 0; row < g->height; row++) {
        uint8_t bits = 0, bit = 0;
        for (uint8_t col = 0; col < g->width; col++) {
          if (!bit) { bits = pgm_read_byte(bmp++); }
          if (bits & 0x80) gfx->drawPixel(bx + col, by + row, fg);
          bits <<= 1;
          if (++bit >= 8) bit = 0;
        }
      }
    }
    cx += g->xAdvance;  // 0 for combining marks → overlays on same x
  }
}

#endif // UI_STRINGS_H
