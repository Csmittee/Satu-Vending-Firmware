#ifndef UI_H
#define UI_H

// ui.h — Version R7 — 2026-06-24
// D-10: ui.h split — HW primitives + include chain + service orchestration only.
// D-11: R7 — SarabanSubset.h added to include chain (Thai GFXfont placeholder).
//            g_lang_th moved to ui_strings.h (authoritative definition — removed from here).
// Screens → ui_screens.h  Keyboard → ui_keyboard.h  Strings → ui_strings.h
// satu_vending.ino still only includes ui.h — include chain makes all symbols visible.
// Previous: R6 — 2026-06-22 (D-10 split)

#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <TAMC_GT911.h>
#include <ArduinoJson.h>
#include <PNGdec.h>
#include "FreeSansBold24pt7b.h"
#include "FreeSansBold18pt7b.h"
#include "FreeSansBold12pt7b.h"
#include "config.h"

// ============================================================
//  DISPLAY + TOUCH OBJECTS
// ============================================================
Arduino_ESP32RGBPanel *_bus = new Arduino_ESP32RGBPanel(
  41, 40, 39, 42,
  14, 21, 47, 48, 45,
  9, 46, 3, 8, 16, 1,
  15, 7, 6, 5, 4,
  0, 8, 2, 43,
  0, 8, 2, 12,
  1, 16000000
);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(800, 480, _bus, 0, true);
TAMC_GT911 _touch(19, 20, -1, -1, 800, 480);

// R-150: Touch read cache — one GT911 I2C read per idle tick prevents event consumption.
// GT911 clears its interrupt flag after each read(); a second read in the same tick
// returns isTouched=false even if finger is still down. touchReadOnce() shares one read
// across all checks within a 4ms window (idle tick is ~10ms with delay(10) in loop()).
static unsigned long _touchCacheMs = 0;
static inline void touchReadOnce() {
  unsigned long _now = millis();
  if (_now - _touchCacheMs > 4) { _touch.read(); _touchCacheMs = _now; }
}

// R-151: Gift option debounce reset — module-level so setState() can reset it on entry.
// Static local inside getTouchedGiftOption() would persist across state transitions and
// allow carry-over touches to fire immediately on re-entry.
static unsigned long _lastGiftTouchMs    = 0;
static unsigned long _lastConfirmTouchMs = 0;
static void resetGiftTouchDebounce() { _lastGiftTouchMs = millis(); }

// ============================================================
//  COLOUR PALETTE
// ============================================================
static uint16_t C_BLACK, C_WHITE, C_BG, C_BGCELL, C_BGCELL_SEL;
static uint16_t C_GOLD, C_DARKGOLD, C_GOLD_DIM;
static uint16_t C_GREY, C_DARKGREY, C_MIDGREY;
static uint16_t C_GREEN, C_RED, C_ORANGE;
static uint16_t C_PRICE_TEAL, C_PRICE_GOLD, C_PRICE_AMBER;
static uint16_t C_PRICE_ORANGE, C_PRICE_DEEPORANGE;

// ============================================================
//  SCREEN DIMENSIONS
// ============================================================
#define SCR_W     800
#define SCR_H     480
#define STATUS_H   44

// ============================================================
//  RUNTIME GRID  (set by recalcGrid(), values from /hello config)
//  R4: no compile-time GRID_COLS / GRID_ROWS / CELL_W / CELL_H macros
// ============================================================
#define GRID_PAD    6
#define GRID_X      6
#define GRID_Y     (STATUS_H + 4)
#define SIDE_TAB_W  40   // width of A/B/C row-selector strip when rows>=3

int g_grid_rows  = 2;    // plain globals — shared with network.h via link-time resolution
int g_grid_cols  = 5;
int CELL_W       = 152;  // recalculated by recalcGrid()
int CELL_H       = 212;
static int g_active_tab = 0;  // ui.h-private

// Config globals (populated from NVS by loadConfigFromNVS())
int  g_cfg_idle    = 60;
int  g_cfg_sel     = PRODUCT_SELECTION_TIMEOUT;
bool g_cfg_water   = true;
bool g_cfg_lucky   = true;
// g_lang_th moved to ui_strings.h (D-11 R7) — authoritative definition is there

void recalcGrid() {
  int gridAreaW = SCR_W - GRID_X * 2;
  if (g_grid_rows >= 3) gridAreaW -= SIDE_TAB_W;  // reserve left strip
  CELL_W = (gridAreaW - GRID_PAD * (g_grid_cols - 1)) / g_grid_cols;
  CELL_H = (SCR_H - GRID_Y - GRID_PAD * (g_grid_rows - 1)) / g_grid_rows;
  Serial.printf("[UI] recalcGrid: %dx%d  CELL %dx%d\n",
                g_grid_cols, g_grid_rows, CELL_W, CELL_H);
}

// ============================================================
//  SERVICE TAB CONSTANTS
// ============================================================
#define TAB_SELFTEST  0
#define TAB_FREEPLAY  1
#define TAB_DEVICES   2
#define TAB_SETTINGS  3
#define TAB_FIRMWARE  4

// ============================================================
//  SLOT CONFIG STRUCT
// ============================================================
struct SlotConfig {
  char  name_th[32];
  char  name_en[32];
  int   price;
  bool  enabled;
  bool  configured;
};

static SlotConfig g_slots[NUM_SLOTS];

static void _initDefaultSlots() {
  for (int i = 0; i < NUM_SLOTS; i++) {
    snprintf(g_slots[i].name_en, 32, "Slot %d", i + 1);
    g_slots[i].name_th[0] = '\0';
    g_slots[i].price      = 0;
    g_slots[i].enabled    = false;
    g_slots[i].configured = false;
  }
}

void loadSlotsFromJson(JsonArray arr) {
  for (JsonObject obj : arr) {
    int idx = (int)obj["slot"] - 1;
    if (idx < 0 || idx >= NUM_SLOTS) continue;
    strlcpy(g_slots[idx].name_th, obj["name_th"] | "", 32);
    strlcpy(g_slots[idx].name_en, obj["name_en"] | "", 32);
    g_slots[idx].price      = obj["price"]   | 0;
    g_slots[idx].enabled    = obj["enabled"] | false;
    g_slots[idx].configured = true;
    Serial.printf("[UI] Slot %d: %s %d THB en=%d\n",
                  idx+1, g_slots[idx].name_en, g_slots[idx].price, g_slots[idx].enabled);
  }
}

// ============================================================
//  STATE TRACKING
// ============================================================
static int  g_selectedSlot  = -1;
static bool g_idleDrawn     = false;
static int  g_qrSecondsLeft = 120;

extern String g_deviceId;

// ============================================================
//  PNG / QR SUPPORT  (PNGdec by bitbank2)
// ============================================================
static PNG      _png;
static uint8_t* g_pngBuf   = nullptr;
static int      _pngDrawX  = 0;
static int      _pngDrawY  = 0;
static int      _pngRowCount = 0;
static uint16_t g_qrFrameBuf[165 * 165];

static int _pngDrawRow(PNGDRAW* pDraw) {
  static uint16_t lineBuf[800];  // R-119: static — off stack, consistent memory layout
  _png.getLineAsRGB565(pDraw, lineBuf, PNG_RGB565_LITTLE_ENDIAN, 0xFFFFFFFF);
  memcpy(&g_qrFrameBuf[pDraw->y * pDraw->iWidth], lineBuf, pDraw->iWidth * 2);
  _pngRowCount++;
  return 1;
}

// drawQrFromBytes() — confirmed working 2026-06-15 (rc=0 rows=165 on hardware)
// Root cause of rc=8: _pngDrawRow() was returning 0 = PNGdec stop-early signal (v1.1.4 feature)
// Fix: return 1 in callback. pause-decode-resume gate kept as defensive measure.
// See: .claude/rules/LIBRARY_pngdec.md + .claude/rules/SKILL_esp32s3_rgb_panel_constraints.md
void drawQrFromBytes(uint8_t* buf, size_t len, int x, int y) {
  if (!buf || len == 0) {
    Serial.println("[UI] drawQrFromBytes: null buf or zero len");
    return;
  }

  // R-117 Step 1: release PSRAM bus from DMA pressure
  digitalWrite(TFT_BL, LOW);
  delay(100);  // R-117b: 800x480 @ 16MHz = 24ms per frame. 100ms = 4+ frames to drain DMA burst

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
    if (rc == 0 && _pngRowCount == 165) {
      gfx->draw16bitRGBBitmap(_pngDrawX, _pngDrawY, g_qrFrameBuf, 165, 165);
    }
  } else {
    Serial.printf("[UI] PNG openRAM failed: rc=%d len=%u\n", openRc, (unsigned)len);
  }

  // R-117 Step 3: restore display
  delay(5);
  digitalWrite(TFT_BL, HIGH);
}

// ============================================================
//  drawQrFromBitmap() — R-114 EMERGENCY FALLBACK ONLY
//  Normal operation: use drawQrFromBytes() (PNG, R-117).
//  This function is fake-mode-only scaffolding.
//  Omise live mode serves real PromptPay PNG with EMVCo branding.
//  Bitmap cannot substitute for Omise live QR.
//  Do NOT call unless PNG decode is confirmed broken on a specific unit.
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
//  DRAW HELPERS  (defined before include chain — ui_keyboard.h depends on these)
// ============================================================
static inline void _fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
  gfx->fillRoundRect(x, y, w, h, r, c);
}
static inline void _drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
  gfx->drawRoundRect(x, y, w, h, r, c);
}

// ============================================================
//  INIT UI
// ============================================================
void initUI() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(0x0000);

  Wire.begin(I2C_SDA, I2C_SCL);
  _touch.begin();
  _touch.setRotation(ROTATION_INVERTED);

  C_BLACK           = gfx->color565(0,   0,   0);
  C_WHITE           = gfx->color565(255, 255, 255);
  C_BG              = gfx->color565(10,  8,   18);
  C_BGCELL          = gfx->color565(15,  14,  26);
  C_BGCELL_SEL      = gfx->color565(31,  21,  0);
  C_GOLD            = gfx->color565(201, 168, 76);
  C_DARKGOLD        = gfx->color565(120, 90,  20);
  C_GOLD_DIM        = gfx->color565(139, 105, 20);
  C_GREY            = gfx->color565(85,  85,  85);
  C_DARKGREY        = gfx->color565(25,  20,  35);
  C_MIDGREY         = gfx->color565(85,  85,  100);
  C_GREEN           = gfx->color565(76,  175, 80);
  C_RED             = gfx->color565(244, 67,  54);
  C_ORANGE          = gfx->color565(255, 140, 0);
  C_PRICE_TEAL      = gfx->color565(79,  195, 247);
  C_PRICE_GOLD      = gfx->color565(201, 168, 76);
  C_PRICE_AMBER     = gfx->color565(240, 165, 0);
  C_PRICE_ORANGE    = gfx->color565(255, 140, 0);
  C_PRICE_DEEPORANGE= gfx->color565(255, 87,  34);

  _initDefaultSlots();

  // Allocate PSRAM buffer for PNG QR decoding
  g_pngBuf = (uint8_t*)malloc(50 * 1024);
  if (!g_pngBuf) Serial.println("[UI] WARN: PNG buffer alloc failed");

  Serial.println("[UI] Display init OK — Arduino_GFX R4");
}

// ============================================================
//  SERVICE SCREEN LAYOUT CONSTANTS
//  (needed by ui_screens.h _drawSvcTabBar + service orchestration below)
// ============================================================
#define SVC_TAB_W  110
#define SVC_BODY_X (SVC_TAB_W + 4)

// ============================================================
//  INCLUDE CHAIN  (order is non-negotiable — R-171)
//  SarabanSubset.h — Thai GFXfont placeholder (3 sizes: 12/18/24pt)
//  ui_strings.h   — StatusBarState enum + string literals + printThai()
//  ui_keyboard.h  — PIN numpad + WiFi setup keyboard
//  ui_screens.h   — all customer-facing screen draw/touch functions
//  ui_service.h   — LOCKED: 5-tab service body + _getTouchedServiceExtra()
// ============================================================
#include "SarabanSubset.h"
#include "ui_strings.h"
#include "ui_keyboard.h"
#include "ui_screens.h"
#include "ui_service.h"

// ============================================================
//  SERVICE ORCHESTRATION  (depends on symbols from ui_service.h above)
// ============================================================
static bool _svcFreshEntry = false;

void drawServiceScreen(int tab) {
  if (_svcFreshEntry) { resetSelfTestResults(); _svcFreshEntry = false; }
  if (tab != TAB_SELFTEST && _stm != 0) { _stm = 0; _stN = 0; }

  gfx->fillRect(0, 0, SCR_W, STATUS_H, C_DARKGREY);
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_ORANGE); gfx->setTextSize(1);
  gfx->setCursor(10, 32);
  gfx->print("SERVICE");
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  gfx->setCursor(SCR_W - 90, 12);
  gfx->print("[EXIT]");

  _drawSvcTabBar(tab);

  int bodyX = SVC_BODY_X;
  gfx->fillRect(SVC_BODY_X, STATUS_H, SCR_W - SVC_BODY_X, SCR_H - STATUS_H, C_BG);

  switch (tab) {
    case TAB_SELFTEST:  _drawSvcBody_SelfTest(bodyX);  break;
    case TAB_FREEPLAY:  _drawSvcBody_FreePlay(bodyX);  break;
    case TAB_DEVICES:   _drawSvcBody_Devices(bodyX);   break;
    case TAB_SETTINGS:  _drawSvcBody_Settings(bodyX);  break;
    case TAB_FIRMWARE:  _drawSvcBody_Firmware(bodyX);  break;
  }
}

// ============================================================
//  SERVICE TOUCH HELPERS
// ============================================================
int getTouchedServiceTab() {
  touchReadOnce();
  if (!_touch.isTouched) return -1;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  if (tx > SVC_TAB_W) return -1;
  if (ty < STATUS_H)  return -1;
  int tabH = (SCR_H - STATUS_H) / 5;
  int tab  = (ty - STATUS_H) / tabH;
  if (tab < 0 || tab > 4) return -1;
  static unsigned long _lastSvcTabMs = 0;
  if (millis() - _lastSvcTabMs < 150) return -1;
  _lastSvcTabMs = millis();
  return tab;
}

bool checkServiceExit() {
  touchReadOnce();
  if (!_touch.isTouched) return false;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  // EXIT label top-right of status bar
  if (tx > SCR_W - 100 && ty < STATUS_H) {
    static unsigned long _lastExitMs = 0;
    if (millis() - _lastExitMs < 200) return false;
    _lastExitMs = millis();
    _svcFreshEntry = true;
    return true;
  }
  return false;
}

// Forward declaration — full definition in ui_service.h (included above)
int _getTouchedServiceExtra(int tab, int tx, int ty);

// Returns action code: 301-321 slot tap, 401 factory reset, 402 boot PIN,
//   500-502 self-test, 600-612 devices, 700 volume, 800 print. 0=none.
int getTouchedServiceContent(int tab) {
  touchReadOnce();
  if (!_touch.isTouched) return 0;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  if (tx <= SVC_TAB_W) return 0;

  static unsigned long _lastSvcCntMs = 0;
  if (millis() - _lastSvcCntMs < 200) return 0;

  if (tab == TAB_SETTINGS) {
    int bodyX = SVC_BODY_X;
    // y-positions match _S_Y401/_S_Y402 defines in ui_service.h
    const int y401 = 340; // _S_Y401
    const int y402 = 156; // _S_Y402
    if (ty >= y401 && ty <= y401 + 34 && tx >= bodyX + 16 && tx <= bodyX + 276) {
      _lastSvcCntMs = millis();
      return 401;
    }
    if (ty >= y402 && ty <= y402 + 36 && tx >= bodyX + 16 && tx <= bodyX + 236) {
      _lastSvcCntMs = millis();
      return 402;
    }
  }

  if (tab == TAB_FREEPLAY) {
    int bodyX = SVC_BODY_X;
    int cols  = g_grid_cols > 0 ? g_grid_cols : 5;
    const int cw = 44, ch = 44;
    for (int i = 0; i < NUM_SLOTS && i < 21; i++) {
      int col = i % cols;
      int row = i / cols;
      int cx = bodyX + 16 + col * (cw + 2);
      int cy = STATUS_H + 68 + row * (ch + 2);
      if (tx >= cx && tx <= cx + cw && ty >= cy && ty <= cy + ch) {
        _lastSvcCntMs = millis();
        return 300 + i + 1;  // 301 = slot 1, 302 = slot 2...
      }
    }
  }

  // New action codes 500-800 handled by helper in ui_service.h
  int extra = _getTouchedServiceExtra(tab, tx, ty);
  if (extra > 0) {
    _lastSvcCntMs = millis();
    return extra;
  }

  return 0;
}

#endif // UI_H
