// ui.h — Satu 1.0 — All screen drawing, touch, service mode
// R-70: hardware.h is R2 LOCKED — never open, modify, or redeclare anything it owns
// R-72: NUM_SLOTS defined in config.h only — ui.h reads it, never redefines
// R-71: idleAnimationUI() = screen gold flash — idleAnimation() = LED breathing (hardware.h)

#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <PNGdec.h>
#include "config.h"
#include "hardware.h"
#include "network.h"
#include "state_machine.h"

// ============================================================
//  DISPLAY DIMENSIONS
// ============================================================
#define SCR_W   800
#define SCR_H   480
#define STATUS_H 44

// ============================================================
//  COLOUR PALETTE
// ============================================================
#define C_BG           0x0861   // dark grey-blue
#define C_GOLD         0xFEA0
#define C_GOLD_DARK    0xB480
#define C_WHITE        0xFFFF
#define C_BLACK        0x0000
#define C_GREY         0x8410
#define C_GREY_DARK    0x4208
#define C_GREEN        0x07E0
#define C_GREEN_DARK   0x0320
#define C_RED          0xF800
#define C_BLUE         0x001F
#define C_CYAN         0x07FF
#define C_AMBER        0xFCA0
#define C_ORANGE       0xFD20
#define C_TEAL         0x0438
#define C_PURPLE       0x8010
#define C_PINK         0xF81F
#define C_YELLOW       0xFFE0
#define C_DEEPORANGE   0xFA20

// Slot price colours
#define C_PRICE_TEAL       0x0438
#define C_PRICE_GOLD       0xFEA0
#define C_PRICE_AMBER      0xFCA0
#define C_PRICE_ORANGE     0xFD20
#define C_PRICE_DEEPORANGE 0xFA20

// ============================================================
//  GLOBALS
// ============================================================
extern Arduino_GFX* gfx;

// Grid layout from /hello config (R-77)
int g_grid_rows = 2;
int g_grid_cols = 5;

// Idle screen config from /hello
int g_cfg_idle  = 30;
int g_cfg_sel   = 60;
int g_cfg_water = 0;
int g_cfg_lucky = 0;

// PSRAM buffer for image fetches (R-76)
static uint8_t*  g_pngBuf = nullptr;
static size_t    g_pngBufLen = 0;

// Active tab for multi-row grids (A=0, B=1, C=2) (R-78)
static int g_activeTab = 0;

// ============================================================
//  SLOT DATA
// ============================================================
struct SlotInfo {
  String name_en;
  String name_th;
  int    price;
  bool   enabled;
  int    stock;         // -1 = unknown
  int    relay_index;   // 1-based relay number
};

extern SlotInfo g_slots[NUM_SLOTS];
SlotInfo g_slots[NUM_SLOTS];

// ============================================================
//  FONT / TEXT HELPERS
// ============================================================
static void _setFont(int size, uint16_t color) {
  gfx->setTextSize(size);
  gfx->setTextColor(color);
}

// ============================================================
//  PNG DECODER (PNGdec) — instance + draw state
//  drawQrFromBytes() DISABLED — R-114: PNGdec broken on this hardware
// ============================================================
static PNG      _png;
static int      _pngDrawX = 0;
static int      _pngDrawY = 0;
static int      _pngRowCount = 0;

static int _pngDrawRow(PNGDRAW* pDraw) {
  uint16_t lineBuf[800];
  _png.getLineAsRGB565(pDraw, lineBuf, PNG_RGB565_LITTLE_ENDIAN, 0xFFFFFFFF);
  gfx->draw16bitRGBBitmap(_pngDrawX, _pngDrawY + pDraw->y, lineBuf, pDraw->iWidth, 1);
  _pngRowCount++;
  return 0;
}

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

// ============================================================
//  DRAW QR FROM RAW BITMAP BYTES (R-114)
//  Format: 4-byte header (width uint16 BE + height uint16 BE)
//          then 1 byte per pixel: 0x00=black 0xFF=white, row by row.
//  No decoder library — direct gfx->fillRect() per black pixel.
//  Backend endpoint: GET /v1/qr/:charge_id/bitmap
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

    // Scale to fit destW x destH, integer math — avoid float on inner loop
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

// ============================================================
//  HELPERS
// ============================================================
static uint16_t _priceColor(int price) {
  if (price <= 50)  return C_PRICE_TEAL;
  if (price <= 100) return C_PRICE_GOLD;
  if (price <= 200) return C_PRICE_AMBER;
  if (price <= 300) return C_PRICE_ORANGE;
  return C_PRICE_DEEPORANGE;
}

static inline void _fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
  gfx->fillRoundRect(x, y, w, h, r, c);
}
static inline void _drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
  gfx->drawRoundRect(x, y, w, h, r, c);
}

// ============================================================
//  STATUS BAR
// ============================================================
void drawStatusBar(const char* label, uint16_t bgColor = C_GOLD_DARK) {
  gfx->fillRect(0, 0, SCR_W, STATUS_H, bgColor);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(12, 12);
  gfx->print(label);
}

// ============================================================
//  IDLE SCREEN
// ============================================================
void idleAnimationUI() {
  // Brief gold flash on screen — distinct from hardware LED breathing (idleAnimation)
  gfx->fillScreen(C_GOLD);
  delay(80);
  gfx->fillScreen(C_BG);
}

// Build slot label from row/col within the current tab
static String _slotLabel(int tabIdx, int slotIdx) {
  // tabIdx: 0=A, 1=B, 2=C; slotIdx 0-based within tab
  char tabChar = 'A' + tabIdx;
  String lbl = String(tabChar) + String(slotIdx + 1);
  return lbl;
}

void drawIdleScreen() {
  gfx->fillScreen(C_BG);
  drawStatusBar("SATU  |  Select a product");

  int rows = g_grid_rows;
  int cols = g_grid_cols;
  int totalSlots = rows * cols;
  bool hasTabs = (rows >= 3);  // R-78: tabs A/B/C only when 3+ rows

  int tabBarH = hasTabs ? 40 : 0;
  int gridTop = STATUS_H + tabBarH + 8;
  int gridH   = SCR_H - gridTop - 8;
  int cellW   = SCR_W / cols;
  int cellH   = gridH / (hasTabs ? 2 : rows);  // 2 rows per tab if tabs present

  // Draw tab bar if needed
  if (hasTabs) {
    const char* tabNames[] = {"A", "B", "C"};
    int numTabs = rows / 2 + (rows % 2);  // ceiling
    if (numTabs > 3) numTabs = 3;
    int tabW = SCR_W / numTabs;
    for (int t = 0; t < numTabs; t++) {
      uint16_t tabBg = (t == g_activeTab) ? C_GOLD : C_GREY_DARK;
      gfx->fillRect(t * tabW, STATUS_H, tabW, tabBarH, tabBg);
      gfx->setTextColor(C_WHITE);
      gfx->setTextSize(2);
      gfx->setCursor(t * tabW + tabW / 2 - 6, STATUS_H + 10);
      gfx->print(tabNames[t]);
    }
  }

  // Draw grid cells
  int startSlot = g_activeTab * cols * 2;
  int displayRows = hasTabs ? 2 : rows;
  for (int r = 0; r < displayRows; r++) {
    for (int c = 0; c < cols; c++) {
      int sIdx = startSlot + r * cols + c;
      if (sIdx >= NUM_SLOTS) break;
      SlotInfo& s = g_slots[sIdx];

      int cx = c * cellW;
      int cy = gridTop + r * cellH;

      uint16_t bg = s.enabled ? C_GREY_DARK : C_BG;
      _fillRoundRect(cx + 4, cy + 4, cellW - 8, cellH - 8, 6, bg);
      _drawRoundRect(cx + 4, cy + 4, cellW - 8, cellH - 8, 6, C_GREY);

      if (s.enabled) {
        // Price badge
        uint16_t pc = _priceColor(s.price);
        _fillRoundRect(cx + 8, cy + 8, 54, 20, 4, pc);
        gfx->setTextColor(C_WHITE);
        gfx->setTextSize(1);
        gfx->setCursor(cx + 12, cy + 12);
        gfx->printf("%d THB", s.price);

        // Slot label (A1, A2 etc)
        String lbl = hasTabs ? _slotLabel(g_activeTab, sIdx - startSlot)
                             : String(sIdx + 1);
        gfx->setTextColor(C_GREY);
        gfx->setTextSize(1);
        gfx->setCursor(cx + cellW - 28, cy + 10);
        gfx->print(lbl);

        // Name
        gfx->setTextColor(C_WHITE);
        gfx->setTextSize(1);
        gfx->setCursor(cx + 8, cy + 34);
        gfx->print(s.name_en.substring(0, 18));
      } else {
        gfx->setTextColor(C_GREY);
        gfx->setTextSize(1);
        gfx->setCursor(cx + 14, cy + cellH / 2 - 6);
        gfx->print("EMPTY");
      }
    }
  }
}

// ============================================================
//  PRODUCT SELECTION SCREEN
// ============================================================
void drawSelectionScreen(int slotIdx) {
  if (slotIdx < 0 || slotIdx >= NUM_SLOTS) return;
  SlotInfo& s = g_slots[slotIdx];

  gfx->fillScreen(C_BG);
  drawStatusBar("SATU  |  Product details");

  // Large price
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(4);
  gfx->setCursor(40, 80);
  gfx->printf("%d THB", s.price);

  // Product name
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(40, 140);
  gfx->print(s.name_en);
  gfx->setCursor(40, 168);
  gfx->print(s.name_th);

  // Stock
  if (s.stock >= 0) {
    gfx->setTextColor(s.stock > 0 ? C_GREEN : C_RED);
    gfx->setTextSize(1);
    gfx->setCursor(40, 200);
    gfx->printf("Stock: %d", s.stock);
  }

  // BACK button
  _fillRoundRect(40, SCR_H - 80, 160, 50, 8, C_GREY_DARK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(76, SCR_H - 62);
  gfx->print("BACK");

  // CONFIRM button
  _fillRoundRect(SCR_W - 200, SCR_H - 80, 160, 50, 8, C_GREEN_DARK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(SCR_W - 176, SCR_H - 62);
  gfx->print("SELECT");
}

// ============================================================
//  GIFT OPTION SCREEN
// ============================================================
void drawGiftOptionScreen(int slotIdx) {
  gfx->fillScreen(C_BG);
  drawStatusBar("SATU  |  Gift option");

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(40, 90);
  gfx->print("Dedicate this offering to someone?");

  // Item Only
  _fillRoundRect(60, 160, 300, 80, 10, C_GREY_DARK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(120, 192);
  gfx->print("Item Only");

  // Gift / Dedicate
  _fillRoundRect(440, 160, 300, 80, 10, C_GOLD_DARK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(490, 192);
  gfx->print("Dedicate");

  // Back
  _fillRoundRect(40, SCR_H - 80, 120, 50, 8, C_GREY_DARK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(62, SCR_H - 62);
  gfx->print("BACK");
}

// ============================================================
//  PROCESSING SCREEN
// ============================================================
void drawProcessingScreen(const char* msg = "Processing...") {
  gfx->fillScreen(C_BG);
  drawStatusBar("SATU  |  Please wait");
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(3);
  gfx->setCursor(100, SCR_H / 2 - 20);
  gfx->print(msg);
}

// ============================================================
//  QR SCREEN  (R4: fetches image · R-114: switched to raw bitmap endpoint)
// ============================================================
void drawQrScreen(String qrUrl, int amount, int slotIdx) {
  gfx->fillScreen(C_BG);
  drawStatusBar("SATU  |  Scan to pay");

  // Left panel — amount + instructions
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(3);
  gfx->setCursor(40, 80);
  gfx->printf("%d THB", amount);

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(40, 130);
  gfx->print("Open your banking app");
  gfx->setCursor(40, 158);
  gfx->print("and scan the QR code.");

  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  gfx->setCursor(40, 200);
  gfx->print("Payment via PromptPay");
  gfx->setCursor(40, 216);
  gfx->print("Times out in 30 seconds.");

  // QR area (right panel)
  int qrAreaX = SCR_W - 264;
  int qrAreaY = STATUS_H + 10;

  // Placeholder white box
  gfx->fillRect(qrAreaX, qrAreaY, 244, 244, C_WHITE);
  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  gfx->setCursor(qrAreaX + 60, qrAreaY + 118);
  gfx->print("Loading QR...");

  // Fetch and render bitmap (R-114) — PNGdec abandoned, raw bitmap endpoint used
  if (g_pngBuf && !qrUrl.isEmpty()) {
    // Append /bitmap to PNG URL to get raw pixel data — bypasses PNGdec entirely
    String bitmapUrl = qrUrl;
    if (!bitmapUrl.endsWith("/bitmap")) {
      bitmapUrl += "/bitmap";
    }
    Serial.printf("[UI] QR bitmap URL: %s\n", bitmapUrl.c_str());

    size_t bmpLen = fetchImageBytes(bitmapUrl, g_pngBuf, 200 * 1024);
    if (bmpLen > 4) {
      Serial.printf("[UI] QR bitmap loaded: %u bytes — rendering\n", bmpLen);
      gfx->fillRect(qrAreaX, qrAreaY, 244, 244, C_WHITE);
      drawQrFromBitmap(g_pngBuf, bmpLen, qrAreaX + 2, qrAreaY + 2, 240, 240);
    } else {
      Serial.println("[UI] QR bitmap failed — showing fallback");
      gfx->fillRect(qrAreaX, qrAreaY, 244, 244, C_WHITE);
      gfx->setTextColor(C_GREY);
      gfx->setTextSize(1);
      gfx->setCursor(qrAreaX + 50, qrAreaY + 118);
      gfx->print("QR unavailable");
    }
  }

  // Cancel button
  _fillRoundRect(40, SCR_H - 80, 160, 50, 8, C_RED);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(68, SCR_H - 62);
  gfx->print("CANCEL");

  Serial.println("[UI] QR screen drawn");
}

// ============================================================
//  COMPLETION SCREEN
// ============================================================
void drawCompletionScreen(int slotIdx) {
  gfx->fillScreen(C_BG);
  drawStatusBar("SATU  |  Thank you!");

  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(3);
  gfx->setCursor(60, 100);
  gfx->print("Payment complete!");

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(60, 160);
  gfx->print("Please collect your item.");

  if (slotIdx >= 0 && slotIdx < NUM_SLOTS) {
    gfx->setTextColor(C_GREY);
    gfx->setTextSize(1);
    gfx->setCursor(60, 210);
    gfx->printf("Slot %d — %s", slotIdx + 1, g_slots[slotIdx].name_en.c_str());
  }

  // Returning to main screen message
  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  gfx->setCursor(60, 400);
  gfx->print("Returning to main screen in 5 seconds...");
}

// ============================================================
//  TIMEOUT SCREEN
// ============================================================
void drawTimeoutScreen() {
  gfx->fillScreen(C_BG);
  drawStatusBar("SATU  |  Timed out");

  gfx->setTextColor(C_RED);
  gfx->setTextSize(3);
  gfx->setCursor(60, 120);
  gfx->print("QR code expired.");

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(60, 200);
  gfx->print("Please try again.");

  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  gfx->setCursor(60, 380);
  gfx->print("Returning to main screen...");
}

// ============================================================
//  ERROR SCREEN
// ============================================================
void drawErrorScreen(const char* msg) {
  gfx->fillScreen(C_BG);
  drawStatusBar("SATU  |  Error", C_RED);

  gfx->setTextColor(C_RED);
  gfx->setTextSize(2);
  gfx->setCursor(40, 100);
  gfx->print("An error occurred:");

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(40, 140);
  gfx->print(msg);

  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  gfx->setCursor(40, 380);
  gfx->print("Returning to main screen...");
}

// ============================================================
//  WIFI SETUP SCREEN  (R5: NVS provisioning — no hardcoded creds)
// ============================================================

// Keyboard layout
static const char* _kbRows[] = {
  "QWERTYUIOP",
  "ASDFGHJKL",
  "ZXCVBNM"
};
static const char* _kbRowsSpecial[] = {
  "1234567890",
  "-_.@",
  ""
};

enum WifiSetupField { FIELD_SSID, FIELD_PASS };
static WifiSetupField _wfField = FIELD_SSID;
static String _wfSSID = "";
static String _wfPass = "";
static bool   _wfShowPass = false;

// Key touch zones (populated by drawWifiSetupScreen)
struct KeyZone { int x, y, w, h; char ch; };
static KeyZone  _kbZones[60];
static int      _kbZoneCount = 0;

static void _drawKey(int x, int y, int w, int h, const char* label, bool highlight = false) {
  uint16_t bg = highlight ? C_GOLD_DARK : C_GREY_DARK;
  _fillRoundRect(x + 2, y + 2, w - 4, h - 4, 4, bg);
  _drawRoundRect(x + 2, y + 2, w - 4, h - 4, 4, C_GREY);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  int tx = x + w / 2 - 3;
  int ty = y + h / 2 - 4;
  gfx->setCursor(tx, ty);
  gfx->print(label);
}

void drawWifiSetupScreen() {
  gfx->fillScreen(C_BG);
  drawStatusBar("SATU  |  WiFi Setup");

  // SSID field
  _fillRoundRect(20, 54, SCR_W / 2 - 30, 36, 6,
                 _wfField == FIELD_SSID ? C_GOLD_DARK : C_GREY_DARK);
  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  gfx->setCursor(24, 58);
  gfx->print("SSID");
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(24, 70);
  gfx->print(_wfSSID.substring(0, 30));

  // Password field
  _fillRoundRect(SCR_W / 2, 54, SCR_W / 2 - 20, 36, 6,
                 _wfField == FIELD_PASS ? C_GOLD_DARK : C_GREY_DARK);
  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  gfx->setCursor(SCR_W / 2 + 4, 58);
  gfx->print("Password");
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(SCR_W / 2 + 4, 70);
  String displayPass = _wfShowPass ? _wfPass : String(_wfPass.length(), '*');
  gfx->print(displayPass.substring(0, 30));

  // Keyboard rows
  _kbZoneCount = 0;
  int kbTop = 100;
  int keyH  = 42;
  int keyW  = 36;

  for (int row = 0; row < 3; row++) {
    const char* rowStr = _kbRows[row];
    int n = strlen(rowStr);
    int rowW = n * keyW;
    int startX = (SCR_W - rowW) / 2;
    for (int i = 0; i < n; i++) {
      char ch = rowStr[i];
      int kx = startX + i * keyW;
      int ky = kbTop + row * (keyH + 4);
      char label[2] = {ch, 0};
      _drawKey(kx, ky, keyW, keyH, label);
      if (_kbZoneCount < 60) {
        _kbZones[_kbZoneCount++] = {kx, ky, keyW, keyH, ch};
      }
    }
  }

  // Special chars row
  {
    const char* spec = _kbRowsSpecial[0];  // digits
    int n = strlen(spec);
    int rowW = n * keyW;
    int startX = (SCR_W - rowW) / 2;
    int ky = kbTop + 3 * (keyH + 4);
    for (int i = 0; i < n; i++) {
      char ch = spec[i];
      int kx = startX + i * keyW;
      char label[2] = {ch, 0};
      _drawKey(kx, ky, keyW, keyH, label);
      if (_kbZoneCount < 60) {
        _kbZones[_kbZoneCount++] = {kx, ky, keyW, keyH, ch};
      }
    }
  }
  {
    const char* spec = _kbRowsSpecial[1];  // . @ - _
    int n = strlen(spec);
    int startX = 20;
    int ky = kbTop + 4 * (keyH + 4);
    for (int i = 0; i < n; i++) {
      char ch = spec[i];
      int kx = startX + i * (keyW + 4);
      char label[2] = {ch, 0};
      _drawKey(kx, ky, keyW, keyH, label);
      if (_kbZoneCount < 60) {
        _kbZones[_kbZoneCount++] = {kx, ky, keyW, keyH, ch};
      }
    }
  }

  // Action buttons
  int btnY = kbTop + 5 * (keyH + 4) + 4;

  // BACKSPACE
  _fillRoundRect(20, btnY, 100, 38, 6, C_GREY_DARK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(32, btnY + 14);
  gfx->print("<BACK");

  // SPACE
  _fillRoundRect(130, btnY, 200, 38, 6, C_GREY_DARK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(210, btnY + 14);
  gfx->print("SPACE");

  // CONNECT
  _fillRoundRect(SCR_W - 160, btnY, 140, 38, 6, C_GREEN_DARK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(SCR_W - 136, btnY + 14);
  gfx->print("CONNECT");
}

// Touch handler for WiFi setup — returns true when CONNECT pressed
bool handleWifiSetupTouch(int tx, int ty) {
  int kbTop = 100;
  int keyH  = 42;
  int btnY  = kbTop + 5 * (keyH + 4) + 4;

  // SSID / Password field select
  if (ty >= 54 && ty <= 90) {
    if (tx < SCR_W / 2) _wfField = FIELD_SSID;
    else                _wfField = FIELD_PASS;
    drawWifiSetupScreen();
    return false;
  }

  // BACKSPACE
  if (tx >= 20 && tx <= 120 && ty >= btnY && ty <= btnY + 38) {
    if (_wfField == FIELD_SSID && _wfSSID.length() > 0)
      _wfSSID.remove(_wfSSID.length() - 1);
    else if (_wfField == FIELD_PASS && _wfPass.length() > 0)
      _wfPass.remove(_wfPass.length() - 1);
    drawWifiSetupScreen();
    return false;
  }

  // SPACE
  if (tx >= 130 && tx <= 330 && ty >= btnY && ty <= btnY + 38) {
    if (_wfField == FIELD_SSID) _wfSSID += ' ';
    else                        _wfPass  += ' ';
    drawWifiSetupScreen();
    return false;
  }

  // CONNECT
  if (tx >= SCR_W - 160 && ty >= btnY && ty <= btnY + 38) {
    if (_wfSSID.length() > 0) {
      saveWifiAndReboot(_wfSSID.c_str(), _wfPass.c_str());
    }
    return false;  // saveWifiAndReboot() doesn't return
  }

  // Keyboard key zones
  for (int i = 0; i < _kbZoneCount; i++) {
    KeyZone& z = _kbZones[i];
    if (tx >= z.x && tx <= z.x + z.w && ty >= z.y && ty <= z.y + z.h) {
      if (_wfField == FIELD_SSID) _wfSSID += z.ch;
      else                        _wfPass  += z.ch;
      drawWifiSetupScreen();
      return false;
    }
  }

  return false;
}

// ============================================================
//  SERVICE MODE
// ============================================================

enum ServiceTab {
  SVC_SLOTS   = 0,
  SVC_SENSORS = 1,
  SVC_RELAYS  = 2,
  SVC_SYSTEM  = 3,
  SVC_SETTINGS = 4
};
static ServiceTab _svcTab = SVC_SLOTS;

static const char* _svcTabNames[] = {
  "Slots", "Sensors", "Relays", "System", "Settings"
};

// Draw the 5-tab service mode shell
static void _drawSvcShell() {
  gfx->fillScreen(C_BG);
  drawStatusBar("SERVICE MODE", C_PURPLE);

  // Tab bar
  int tabW = SCR_W / 5;
  for (int t = 0; t < 5; t++) {
    uint16_t bg = (t == _svcTab) ? C_GOLD_DARK : C_GREY_DARK;
    gfx->fillRect(t * tabW, STATUS_H, tabW, 36, bg);
    gfx->setTextColor(C_WHITE);
    gfx->setTextSize(1);
    int lx = t * tabW + tabW / 2 - strlen(_svcTabNames[t]) * 3;
    gfx->setCursor(lx, STATUS_H + 12);
    gfx->print(_svcTabNames[t]);
  }
}

// Tab: Slots
static void _drawSvcSlots() {
  int top = STATUS_H + 44;
  int rowH = 34;
  int cols[5] = {10, 60, 200, 340, 480};

  // Header
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(1);
  gfx->setCursor(cols[0], top);
  gfx->print("#");
  gfx->setCursor(cols[1], top);
  gfx->print("Name");
  gfx->setCursor(cols[2], top);
  gfx->print("Price (THB)");
  gfx->setCursor(cols[3], top);
  gfx->print("Stock");
  gfx->setCursor(cols[4], top);
  gfx->print("Status");

  for (int i = 0; i < NUM_SLOTS; i++) {
    SlotInfo& s = g_slots[i];
    int y = top + (i + 1) * rowH;
    if (y > SCR_H - 10) break;  // clip

    uint16_t rowBg = (i % 2 == 0) ? C_GREY_DARK : C_BG;
    gfx->fillRect(0, y - 4, SCR_W, rowH, rowBg);

    gfx->setTextColor(C_GREY);
    gfx->setTextSize(1);
    gfx->setCursor(cols[0], y + 8);
    gfx->print(i + 1);

    gfx->setTextColor(C_WHITE);
    gfx->setCursor(cols[1], y + 8);
    gfx->print(s.name_en.substring(0, 16));

    // Price — read-only from g_slots[] per R-79
    gfx->setTextColor(_priceColor(s.price));
    gfx->setCursor(cols[2], y + 8);
    gfx->printf("%d", s.price);

    gfx->setTextColor(C_WHITE);
    gfx->setCursor(cols[3], y + 8);
    if (s.stock >= 0) gfx->printf("%d", s.stock);
    else              gfx->print("?");

    gfx->setTextColor(s.enabled ? C_GREEN : C_RED);
    gfx->setCursor(cols[4], y + 8);
    gfx->print(s.enabled ? "ON" : "OFF");
  }
}

// Tab: Sensors
static void _drawSvcSensors() {
  int top = STATUS_H + 52;
  int rowH = 36;

  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(1);
  gfx->setCursor(20, top);
  gfx->print("IR Sensor Status (live)");

  for (int i = 0; i < 10; i++) {
    int y = top + (i + 1) * rowH;
    bool triggered = readSensor(i + 1);  // hardware.h
    gfx->fillRect(0, y - 2, SCR_W, rowH - 2,
                  (i % 2 == 0) ? C_GREY_DARK : C_BG);
    gfx->setTextColor(C_WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(20, y + 8);
    gfx->printf("Sensor %2d:", i + 1);
    gfx->setTextColor(triggered ? C_RED : C_GREEN);
    gfx->setCursor(140, y + 8);
    gfx->print(triggered ? "TRIGGERED" : "CLEAR");
  }
}

// Tab: Relays
static void _drawSvcRelays() {
  int top = STATUS_H + 52;
  int rowH = 36;

  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(1);
  gfx->setCursor(20, top);
  gfx->print("Relay Control (tap to toggle)");

  for (int i = 0; i < 12; i++) {
    int y = top + (i + 1) * rowH;
    gfx->fillRect(0, y - 2, SCR_W, rowH - 2,
                  (i % 2 == 0) ? C_GREY_DARK : C_BG);
    gfx->setTextColor(C_WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(20, y + 8);
    if (i == 10) gfx->print("R11 — PUMP");
    else if (i == 11) gfx->print("R12 — DOOR LOCK");
    else gfx->printf("Relay %2d", i + 1);

    // Pulse button
    _fillRoundRect(SCR_W - 120, y, 100, 28, 6, C_AMBER);
    gfx->setTextColor(C_BLACK);
    gfx->setCursor(SCR_W - 100, y + 8);
    gfx->print("PULSE");
  }
}

// Tab: System info
static void _drawSvcSystem() {
  int top = STATUS_H + 52;
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(1);
  gfx->setCursor(20, top);
  gfx->print("System Info");

  struct SysLine { const char* label; String value; };
  SysLine lines[] = {
    {"Free heap",     String(ESP.getFreeHeap()) + " bytes"},
    {"PSRAM free",    String(ESP.getFreePsram()) + " bytes"},
    {"Uptime",        String(millis() / 1000) + " sec"},
    {"WiFi IP",       WiFi.localIP().toString()},
    {"WiFi RSSI",     String(WiFi.RSSI()) + " dBm"},
    {"Core temp",     String((int)temperatureRead()) + " C"},
    {"Grid",          String(g_grid_rows) + "x" + String(g_grid_cols)},
    {"g_pngBuf",      g_pngBuf ? "allocated" : "NULL"},
  };
  int n = sizeof(lines) / sizeof(lines[0]);
  for (int i = 0; i < n; i++) {
    int y = top + 24 + i * 28;
    gfx->fillRect(0, y - 2, SCR_W, 28,
                  (i % 2 == 0) ? C_GREY_DARK : C_BG);
    gfx->setTextColor(C_GREY);
    gfx->setCursor(20, y + 6);
    gfx->print(lines[i].label);
    gfx->setCursor(180, y + 6);
    gfx->setTextColor(C_WHITE);
    gfx->print(lines[i].value);
  }

  // Factory reset button
  _fillRoundRect(SCR_W - 200, SCR_H - 70, 180, 44, 8, C_RED);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(SCR_W - 172, SCR_H - 52);
  gfx->print("FACTORY RESET");
}

// Tab: Settings
static void _drawSvcSettings() {
  int top = STATUS_H + 52;
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(1);
  gfx->setCursor(20, top);
  gfx->print("Settings");

  // Volume slider placeholder (R-80)
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(20, top + 30);
  gfx->print("Volume:");
  // Slot prices read-only (R-79)
  gfx->setTextColor(C_GREY);
  gfx->setCursor(20, top + 60);
  gfx->print("Lane prices: edit via dashboard only");
}

void drawServiceModeScreen() {
  _drawSvcShell();
  switch (_svcTab) {
    case SVC_SLOTS:    _drawSvcSlots();    break;
    case SVC_SENSORS:  _drawSvcSensors();  break;
    case SVC_RELAYS:   _drawSvcRelays();   break;
    case SVC_SYSTEM:   _drawSvcSystem();   break;
    case SVC_SETTINGS: _drawSvcSettings(); break;
  }
}

bool handleServiceModeTouch(int tx, int ty) {
  int tabW = SCR_W / 5;
  // Tab bar touch
  if (ty >= STATUS_H && ty <= STATUS_H + 36) {
    int t = tx / tabW;
    if (t >= 0 && t < 5) {
      _svcTab = (ServiceTab)t;
      drawServiceModeScreen();
    }
    return false;
  }

  // Factory reset touch zone (System tab)
  if (_svcTab == SVC_SYSTEM) {
    if (tx >= SCR_W - 200 && ty >= SCR_H - 70) {
      drawProcessingScreen("Contacting server...");
      requestFactoryReset();
      return false;
    }
  }

  // Relay pulse (Relay tab)
  if (_svcTab == SVC_RELAYS) {
    int top = STATUS_H + 52;
    int rowH = 36;
    for (int i = 0; i < 12; i++) {
      int y = top + (i + 1) * rowH;
      if (ty >= y && ty <= y + 28 && tx >= SCR_W - 120) {
        activateRelay(i + 1, 500);  // hardware.h — 500ms pulse
        return false;
      }
    }
  }

  return false;
}

// ============================================================
//  INIT UI — allocate PSRAM image buffer
// ============================================================
void initUI() {
  g_pngBuf = (uint8_t*)ps_malloc(200 * 1024);  // 200KB in PSRAM
  if (g_pngBuf) {
    g_pngBufLen = 200 * 1024;
    Serial.println("[UI] PSRAM image buffer: 200KB allocated");
  } else {
    Serial.println("[UI] PSRAM image buffer: FAILED — QR fetch disabled");
  }
}
