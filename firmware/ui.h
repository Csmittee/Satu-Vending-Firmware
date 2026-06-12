// ============================================================
// ui.h — Satu Vending Machine Display Layer  R4
// Board  : ESP32-8048S070C — 7" 800×480 capacitive touch
// Driver : Arduino_GFX (RGB panel, EK9716)
// Touch  : TAMC_GT911  SDA=19 SCL=20 ROTATION_INVERTED
// ============================================================
// CHANGE LOG:
//   R3   — Full rewrite: TFT_eSPI → Arduino_GFX
//   R3.2 — Fixed grid 5×2, idleAnimationUI() sync
//   R4   — Runtime grid: removed compile-time GRID_COLS/ROWS macros
//          recalcGrid() recalculates CELL_W/CELL_H from /hello config
//          PNGdec QR: fetchImageBytes() + drawQrFromBytes() via PSRAM buf
//          Service mode: 5-tab drawServiceScreen()
//          PIN screens: drawBootPinScreen(), drawPinOverlay(), getTouchedNumpad()
//          Setup code screen: drawSetupCodeScreen()
//          Debug screen: drawDebugScreen()
//          Side tabs: _drawSideTabs() for 3-row grid
//          getTouchedSlotXY() using runtime grid vars
// ============================================================

#ifndef UI_H
#define UI_H

#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <TAMC_GT911.h>
#include <ArduinoJson.h>
#include <PNGdec.h>
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
int  g_cfg_sel     = 15;
bool g_cfg_water   = true;
bool g_cfg_lucky   = true;
static bool g_lang_th = false;  // ui.h-private

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

static int _pngDrawRow(PNGDRAW* pDraw) {
  uint16_t lineBuf[800];
  _png.getLineAsRGB565(pDraw, lineBuf, PNG_RGB565_LITTLE_ENDIAN, 0xFFFFFFFF);
  gfx->draw16bitRGBBitmap(_pngDrawX, _pngDrawY + pDraw->y, lineBuf, pDraw->iWidth, 1);
  return 0;
}

void drawQrFromBytes(uint8_t* buf, size_t len, int x, int y) {
  if (!buf || len == 0) return;
  _pngDrawX = x;
  _pngDrawY = y;
  if (_png.openRAM(buf, (int32_t)len, _pngDrawRow) == PNG_SUCCESS) {
    _png.decode(nullptr, 0);
    _png.close();
  } else {
    Serial.println("[UI] PNG open failed");
  }
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
static const char* _stateLabels[] = {
  "Select Item",
  "Confirm",
  "Payment",
  "Dispensing",
  "Connecting..."
};
enum StatusBarState { SB_IDLE=0, SB_CONFIRM, SB_PAYMENT, SB_DISPENSING, SB_CONNECTING };

static void _drawStatusBar(StatusBarState state) {
  gfx->fillRect(0, 0, SCR_W, STATUS_H, C_DARKGOLD);
  gfx->fillRect(0, STATUS_H - 3, SCR_W, 3, C_GOLD);

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(3);
  gfx->setCursor(10, 8);
  gfx->print("SATU");

  const char* label = _stateLabels[state];
  gfx->setTextSize(2);
  int lw = strlen(label) * 12;
  gfx->setCursor(SCR_W/2 - lw/2, 12);
  gfx->setTextColor(C_WHITE);
  gfx->print(label);

  gfx->setTextSize(1);
  String devLabel = g_deviceId.isEmpty() ? "No ID" : g_deviceId;
  gfx->setTextColor(C_GOLD_DIM);
  gfx->setCursor(SCR_W - devLabel.length()*6 - 4, 6);
  gfx->print(devLabel.c_str());

  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  gfx->setTextColor(wifiOk ? C_GREEN : C_RED);
  const char* wifiStr = wifiOk ? "WiFi OK" : "No WiFi";
  gfx->setCursor(SCR_W - strlen(wifiStr)*6 - 4, 20);
  gfx->print(wifiStr);

  gfx->setTextColor(C_WHITE);
}

// ============================================================
//  SIDE TABS (A/B/C when g_grid_rows >= 3)
// ============================================================
static void _drawSideTabs() {
  if (g_grid_rows < 3) return;
  const char* labels[] = {"A", "B", "C"};
  int tabH = (SCR_H - GRID_Y) / 3;
  for (int i = 0; i < 3; i++) {
    int ty = GRID_Y + i * tabH;
    uint16_t bg = (i == g_active_tab) ? C_GOLD : C_DARKGREY;
    uint16_t fg = (i == g_active_tab) ? C_BLACK : C_GREY;
    gfx->fillRect(0, ty, SIDE_TAB_W - 2, tabH - 2, bg);
    gfx->drawRect(0, ty, SIDE_TAB_W - 2, tabH - 2, C_MIDGREY);
    gfx->setTextColor(fg);
    gfx->setTextSize(2);
    gfx->setCursor(12, ty + tabH/2 - 8);
    gfx->print(labels[i]);
  }
}

// ============================================================
//  SINGLE PRODUCT CELL
// ============================================================
static void _drawCell(int idx, bool selected) {
  int col = idx % g_grid_cols;
  int row = idx / g_grid_cols;
  int xOff = (g_grid_rows >= 3) ? SIDE_TAB_W : GRID_X;
  int x   = xOff + col * (CELL_W + GRID_PAD);
  int y   = GRID_Y + row * (CELL_H + GRID_PAD);

  SlotConfig& s   = g_slots[idx];
  bool disabled   = !s.enabled || !s.configured;

  uint16_t bgColor  = disabled ? C_DARKGREY
                    : selected ? C_BGCELL_SEL
                    : C_BGCELL;
  uint16_t bdrColor = disabled ? C_MIDGREY
                    : selected ? C_GOLD
                    : gfx->color565(42, 31, 74);
  int bdrThick = selected ? 2 : 1;

  _fillRoundRect(x, y, CELL_W, CELL_H, 8, bgColor);
  for (int t = 0; t < bdrThick; t++) {
    _drawRoundRect(x+t, y+t, CELL_W-t*2, CELL_H-t*2, 8-t, bdrColor);
  }
  if (selected) {
    _drawRoundRect(x-1, y-1, CELL_W+2, CELL_H+2, 9,
                   gfx->color565(100, 80, 20));
  }

  if (disabled) {
    char slotLabel[8];
    snprintf(slotLabel, 8, "%d", idx + 1);
    gfx->setTextSize(3);
    gfx->setTextColor(C_MIDGREY);
    int lw = strlen(slotLabel) * 18;
    gfx->setCursor(x + CELL_W/2 - lw/2, y + CELL_H/2 - 12);
    gfx->print(slotLabel);
    return;
  }

  int badgeX = x + CELL_W/2;
  int badgeY = y + 16;
  gfx->fillCircle(badgeX, badgeY, 11,
                  selected ? C_GOLD : gfx->color565(30, 22, 55));
  gfx->setTextColor(selected ? C_BLACK : C_MIDGREY);
  gfx->setTextSize(1);
  char numBuf[4];
  snprintf(numBuf, 4, "%d", idx + 1);
  int nw = strlen(numBuf) * 6;
  gfx->setCursor(badgeX - nw/2, badgeY - 4);
  gfx->print(numBuf);

  uint16_t priceColor = selected ? C_WHITE : _priceColor(s.price);
  char priceBuf[10];
  snprintf(priceBuf, 10, "%d", s.price);
  gfx->setTextSize(4);
  gfx->setTextColor(priceColor);
  int pw  = strlen(priceBuf) * 24;
  int pY  = y + CELL_H/2 - 16;
  gfx->setCursor(x + CELL_W/2 - pw/2, pY);
  gfx->print(priceBuf);

  gfx->setTextSize(1);
  gfx->setTextColor(selected ? C_GOLD_DIM : C_GREY);
  gfx->setCursor(x + CELL_W/2 - 9, pY + 34);
  gfx->print("THB");

  gfx->setTextSize(1);
  gfx->setTextColor(selected ? C_GOLD : C_GREY);
  char nameDisp[26];
  strncpy(nameDisp, s.name_en, 25);
  nameDisp[25] = '\0';
  int nameW = strlen(nameDisp) * 6;
  gfx->setCursor(x + CELL_W/2 - nameW/2, y + CELL_H - 20);
  gfx->print(nameDisp);

  gfx->setTextColor(C_WHITE);
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
  g_pngBuf = (uint8_t*)ps_malloc(200 * 1024);
  if (!g_pngBuf) Serial.println("[UI] WARN: PNG buffer alloc failed");

  Serial.println("[UI] Display init OK — Arduino_GFX R4");
}

// ============================================================
//  DRAW BOOT SCREEN
// ============================================================
void drawBootScreen(String status) {
  gfx->fillScreen(C_BG);

  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(5);
  int lw = 4 * 30;
  gfx->setCursor(SCR_W/2 - lw/2, SCR_H/2 - 60);
  gfx->print("SATU");

  gfx->setTextColor(C_GOLD_DIM);
  gfx->setTextSize(2);
  const char* tag = "Merit Donation System";
  int tw = strlen(tag) * 12;
  gfx->setCursor(SCR_W/2 - tw/2, SCR_H/2 - 10);
  gfx->print(tag);

  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  int sw = status.length() * 6;
  gfx->setCursor(SCR_W/2 - sw/2, SCR_H/2 + 40);
  gfx->print(status.c_str());

  gfx->drawRect(SCR_W/2 - 140, SCR_H/2 + 60, 280, 4, C_DARKGOLD);

  Serial.printf("[UI] Boot: %s\n", status.c_str());
}

// ============================================================
//  DRAW SETUP CODE SCREEN  (pending device awaiting registration)
// ============================================================
void drawSetupCodeScreen(String code, String countdown) {
  gfx->fillScreen(C_BG);

  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(3);
  gfx->setCursor(10, 8);
  gfx->print("SATU");
  gfx->setTextColor(C_ORANGE);
  gfx->setTextSize(1);
  gfx->setCursor(SCR_W/2 - 60, 8);
  gfx->print("SETUP MODE");

  int midY = SCR_H / 2 - 60;

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  const char* prompt = "Enter this code in the admin panel:";
  int pw = strlen(prompt) * 12;
  gfx->setCursor(SCR_W/2 - pw/2, midY);
  gfx->print(prompt);

  // Code hero
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(6);
  int cw = code.length() * 36;
  gfx->setCursor(SCR_W/2 - cw/2, midY + 36);
  gfx->print(code.c_str());

  // Countdown
  gfx->setTextColor(C_GREY);
  gfx->setTextSize(2);
  String cdLabel = "Retrying in " + countdown;
  int cdw = cdLabel.length() * 12;
  gfx->setCursor(SCR_W/2 - cdw/2, midY + 108);
  gfx->print(cdLabel.c_str());

  // Device ID footer
  extern String g_deviceId;
  gfx->setTextColor(C_MIDGREY);
  gfx->setTextSize(1);
  String idLine = "Device: " + g_deviceId;
  int idw = idLine.length() * 6;
  gfx->setCursor(SCR_W/2 - idw/2, SCR_H - 24);
  gfx->print(idLine.c_str());
}

// ============================================================
//  DRAW DEBUG SCREEN  (no PIN needed — read-only info)
// ============================================================
void drawDebugScreen() {
  gfx->fillScreen(C_BG);
  gfx->fillRect(0, 0, SCR_W, STATUS_H, C_DARKGREY);
  gfx->setTextColor(C_ORANGE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 12);
  gfx->print("DEBUG");

  extern String g_deviceId;
  extern String g_setupCode;

  int y = STATUS_H + 16;
  int lh = 24;

  auto dbgLine = [&](const char* label, String val, uint16_t col = 0) {
    gfx->setTextColor(C_GREY);
    gfx->setTextSize(1);
    gfx->setCursor(20, y);
    gfx->print(label);
    gfx->setTextColor(col ? col : C_WHITE);
    gfx->setCursor(160, y);
    gfx->print(val.c_str());
    y += lh;
  };

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macStr[18];
  snprintf(macStr, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  dbgLine("MAC:",       String(macStr));
  dbgLine("Device ID:", g_deviceId.isEmpty() ? "(none)" : g_deviceId);
  dbgLine("Setup Code:",g_setupCode.isEmpty() ? "(none)" : g_setupCode, C_GOLD);
  dbgLine("Firmware:",  FW_VERSION);
  dbgLine("IP:",        WiFi.localIP().toString());
  dbgLine("RSSI:",      String(WiFi.RSSI()) + " dBm");
  dbgLine("Grid:",      String(g_grid_cols) + "x" + String(g_grid_rows));
  dbgLine("Idle TO:",   String(g_cfg_idle) + "s");
  dbgLine("Select TO:", String(g_cfg_sel) + "s");

  gfx->setTextColor(C_DARKGOLD);
  gfx->setTextSize(1);
  gfx->setCursor(20, SCR_H - 24);
  gfx->print("Tap anywhere to dismiss");
}

// ============================================================
//  NUMPAD HELPER  (used by PIN screens)
// ============================================================
static const char* _numpadKeys[12] = {
  "1","2","3","4","5","6","7","8","9","<","0","OK"
};
#define NP_COLS  3
#define NP_ROWS  4
#define NP_KEY_W 80
#define NP_KEY_H 60
#define NP_GAP   6

static void _drawNumpad(int ox, int oy, bool showOK = true) {
  for (int i = 0; i < 12; i++) {
    int col = i % NP_COLS;
    int row = i / NP_COLS;
    int kx  = ox + col * (NP_KEY_W + NP_GAP);
    int ky  = oy + row * (NP_KEY_H + NP_GAP);
    bool isOK  = (i == 11);
    bool isDel = (i == 9);
    uint16_t bg = isOK  ? C_GREEN
                : isDel ? C_RED
                : gfx->color565(30, 22, 55);
    _fillRoundRect(kx, ky, NP_KEY_W, NP_KEY_H, 8, bg);
    _drawRoundRect(kx, ky, NP_KEY_W, NP_KEY_H, 8, C_GOLD);
    gfx->setTextColor(C_WHITE);
    gfx->setTextSize(isOK ? 2 : 3);
    int lw = strlen(_numpadKeys[i]) * (isOK ? 12 : 18);
    gfx->setCursor(kx + NP_KEY_W/2 - lw/2,
                   ky + NP_KEY_H/2 - (isOK ? 8 : 12));
    gfx->print(_numpadKeys[i]);
  }
}

// Returns 0-9 for digit, 10 for DEL, 11 for OK, -1 for none
int getTouchedNumpad(int ox, int oy) {
  _touch.read();
  if (!_touch.isTouched) return -1;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  for (int i = 0; i < 12; i++) {
    int col = i % NP_COLS;
    int row = i / NP_COLS;
    int kx  = ox + col * (NP_KEY_W + NP_GAP);
    int ky  = oy + row * (NP_KEY_H + NP_GAP);
    if (tx >= kx && tx <= kx + NP_KEY_W &&
        ty >= ky && ty <= ky + NP_KEY_H) {
      static unsigned long _lastNpMs = 0;
      if (millis() - _lastNpMs < 120) return -1;
      _lastNpMs = millis();
      return i == 9 ? 10 : (i == 11 ? 11 : (i < 9 ? i + 1 : 0));
    }
  }
  return -1;
}

// ============================================================
//  BOOT PIN SCREEN  (blocks until correct PIN entered or skipped)
// ============================================================
void drawBootPinScreen() {
  gfx->fillScreen(C_BG);
  gfx->fillRect(0, 0, SCR_W, STATUS_H, C_DARKGOLD);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(3);
  gfx->setCursor(10, 8);
  gfx->print("SATU");
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(2);
  gfx->setCursor(SCR_W/2 - 80, 12);
  gfx->print("Enter PIN");

  int npX = SCR_W/2 - (NP_COLS * (NP_KEY_W + NP_GAP))/2 + 60;
  int npY = STATUS_H + 40;
  _drawNumpad(npX, npY);

  // PIN display box
  gfx->fillRect(npX, npY - 36, NP_COLS * (NP_KEY_W + NP_GAP) - NP_GAP, 28, gfx->color565(20,16,30));
  gfx->drawRect(npX, npY - 36, NP_COLS * (NP_KEY_W + NP_GAP) - NP_GAP, 28, C_GOLD);
}

// Overlay: draws PIN entry box over current screen. Returns entered PIN string or "" if cancelled.
// Caller loops until getTouchedNumpad() returns OK (11) then validates.
String drawPinOverlay() {
  int boxW = 360, boxH = 340;
  int boxX = SCR_W/2 - boxW/2;
  int boxY = SCR_H/2 - boxH/2;
  _fillRoundRect(boxX, boxY, boxW, boxH, 12, gfx->color565(10, 8, 18));
  _drawRoundRect(boxX, boxY, boxW, boxH, 12, C_GOLD);
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(2);
  const char* title = "Service PIN";
  gfx->setCursor(boxX + boxW/2 - strlen(title)*12/2, boxY + 12);
  gfx->print(title);

  int npX = boxX + 20;
  int npY = boxY + 50;
  // PIN display
  gfx->fillRect(npX, npY - 4, boxW - 40, 24, gfx->color565(20,16,30));
  gfx->drawRect(npX, npY - 4, boxW - 40, 24, C_GOLD);
  _drawNumpad(npX, npY + 28);
  return "";  // caller drives the loop via getTouchedNumpad
}

// ============================================================
//  DRAW IDLE SCREEN (product grid)
// ============================================================
void drawIdleScreen() {
  if (g_idleDrawn) return;

  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_IDLE);
  if (g_grid_rows >= 3) _drawSideTabs();

  int numSlotsVisible = g_grid_cols * g_grid_rows;
  if (numSlotsVisible > NUM_SLOTS) numSlotsVisible = NUM_SLOTS;
  for (int i = 0; i < numSlotsVisible; i++) {
    _drawCell(i, false);
  }

  g_idleDrawn    = true;
  g_selectedSlot = -1;
  Serial.printf("[UI] Idle screen drawn (%dx%d grid)\n", g_grid_cols, g_grid_rows);
}

// ============================================================
//  DRAW PRODUCT SELECTION
// ============================================================
void drawProductSelection(int slotIdx) {
  g_idleDrawn    = false;
  g_selectedSlot = slotIdx;

  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_CONFIRM);
  if (g_grid_rows >= 3) _drawSideTabs();

  int numSlotsVisible = g_grid_cols * g_grid_rows;
  if (numSlotsVisible > NUM_SLOTS) numSlotsVisible = NUM_SLOTS;
  for (int i = 0; i < numSlotsVisible; i++) {
    _drawCell(i, i == slotIdx);
  }

  int barH = 44, barY = SCR_H - barH;
  gfx->fillRect(0, barY, SCR_W, barH, C_DARKGOLD);
  gfx->drawRect(0, barY, SCR_W, barH, C_GOLD);
  gfx->setTextColor(C_BLACK);
  gfx->setTextSize(2);
  const char* msg = "Tap again to confirm  |  Wait to cancel";
  int mw = strlen(msg) * 12;
  gfx->setCursor(SCR_W/2 - mw/2, barY + 13);
  gfx->print(msg);

  Serial.printf("[UI] Product selection: slot %d\n", slotIdx);
}

// ============================================================
//  DRAW GIFT OPTION SCREEN
// ============================================================
void drawGiftOptionScreen(int slotIdx) {
  g_idleDrawn = false;
  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_CONFIRM);

  SlotConfig& s = g_slots[slotIdx];

  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(2);
  const char* title = "Choose your blessing";
  int tw = strlen(title) * 12;
  gfx->setCursor(SCR_W/2 - tw/2, STATUS_H + 18);
  gfx->print(title);

  int cardW = 280, cardH = 200;
  int cardY = STATUS_H + 60;
  int cardAX = SCR_W/2 - cardW - 30;
  int cardBX = SCR_W/2 + 30;

  _fillRoundRect(cardAX, cardY, cardW, cardH, 12, gfx->color565(20, 18, 35));
  _drawRoundRect(cardAX, cardY, cardW, cardH, 12, gfx->color565(50, 40, 80));
  gfx->fillRect(cardAX + cardW/2 - 20, cardY + 30, 40, 40, gfx->color565(140, 90, 30));
  gfx->drawRect(cardAX + cardW/2 - 20, cardY + 30, 40, 40, C_GOLD);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(2);
  const char* labelA = "Item Only";
  gfx->setCursor(cardAX + cardW/2 - strlen(labelA)*12/2, cardY + 90);
  gfx->print(labelA);
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  const char* subA = "Receive your donation item";
  gfx->setCursor(cardAX + cardW/2 - strlen(subA)*6/2, cardY + 116);
  gfx->print(subA);
  char priceA[16]; snprintf(priceA, 16, "%d THB", s.price);
  gfx->setTextColor(_priceColor(s.price)); gfx->setTextSize(2);
  gfx->setCursor(cardAX + cardW/2 - strlen(priceA)*12/2, cardY + 148);
  gfx->print(priceA);

  if (g_cfg_water) {
    _fillRoundRect(cardBX, cardY, cardW, cardH, 12, gfx->color565(10, 18, 35));
    for (int t = 0; t < 2; t++)
      _drawRoundRect(cardBX+t, cardY+t, cardW-t*2, cardH-t*2, 12-t, C_GOLD);
    _drawRoundRect(cardBX-1, cardY-1, cardW+2, cardH+2, 13, gfx->color565(100, 80, 20));
    int wx = cardBX + cardW/2;
    gfx->fillCircle(wx - 8, cardY + 46, 10, gfx->color565(79, 195, 247));
    gfx->fillCircle(wx + 8, cardY + 52, 12, gfx->color565(33, 150, 243));
    gfx->setTextColor(C_GOLD); gfx->setTextSize(2);
    const char* labelB = "+Sacred Water";
    gfx->setCursor(cardBX + cardW/2 - strlen(labelB)*12/2, cardY + 90);
    gfx->print(labelB);
    gfx->setTextColor(C_GREY); gfx->setTextSize(1);
    const char* subB = "Add sacred water blessing";
    gfx->setCursor(cardBX + cardW/2 - strlen(subB)*6/2, cardY + 116);
    gfx->print(subB);
    char priceB[20]; snprintf(priceB, 20, "%d+20 THB", s.price);
    gfx->setTextColor(C_PRICE_ORANGE); gfx->setTextSize(2);
    gfx->setCursor(cardBX + cardW/2 - strlen(priceB)*12/2, cardY + 148);
    gfx->print(priceB);
  }

  Serial.printf("[UI] Gift option screen: slot %d\n", slotIdx);
}

// ============================================================
//  DRAW QR SCREEN  (R4: fetches PNG via fetchImageBytes + PNGdec)
// ============================================================
void drawQrScreen(String qrUrl, int amount, int slotIdx) {
  g_idleDrawn     = false;
  g_qrSecondsLeft = 120;

  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_PAYMENT);

  SlotConfig& s = (slotIdx >= 0 && slotIdx < NUM_SLOTS)
                  ? g_slots[slotIdx] : g_slots[0];

  int lx = 30;

  gfx->setTextColor(C_GOLD); gfx->setTextSize(5);
  char amtBuf[12]; snprintf(amtBuf, 12, "B%d", amount);
  gfx->setCursor(lx, STATUS_H + 18);
  gfx->print(amtBuf);

  gfx->setTextColor(C_MIDGREY); gfx->setTextSize(2);
  gfx->setCursor(lx, STATUS_H + 82);
  gfx->print(s.name_en);

  gfx->setTextSize(1); gfx->setTextColor(C_GREY);
  const char* ins[] = {
    "1. Open your banking app",
    "2. Select PromptPay QR scan",
    "3. Scan the code and confirm"
  };
  int iy = STATUS_H + 116;
  for (int i = 0; i < 3; i++) {
    gfx->fillRect(lx, iy, 2, 12, C_DARKGOLD);
    gfx->setCursor(lx + 8, iy);
    gfx->print(ins[i]);
    iy += 20;
  }

  gfx->setTextColor(C_ORANGE); gfx->setTextSize(2);
  gfx->setCursor(lx, iy + 16);
  gfx->print("2:00");

  // QR area — draw placeholder first
  int qrAreaX = SCR_W - 264;
  int qrAreaY = STATUS_H + 10;
  gfx->fillRect(qrAreaX, qrAreaY, 244, 244, C_WHITE);
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  gfx->setCursor(qrAreaX + 80, qrAreaY + 118);
  gfx->print("Loading QR...");

  // Fetch and render PNG
  if (g_pngBuf && !qrUrl.isEmpty()) {
    size_t pngLen = fetchImageBytes(qrUrl, g_pngBuf, 200 * 1024);
    if (pngLen > 0) {
      gfx->fillRect(qrAreaX, qrAreaY, 244, 244, C_WHITE);
      drawQrFromBytes(g_pngBuf, pngLen, qrAreaX + 2, qrAreaY + 2);
    } else {
      gfx->setCursor(qrAreaX + 50, qrAreaY + 118);
      gfx->setTextColor(C_RED);
      gfx->print("QR fetch failed");
    }
  }

  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  const char* ppLabel = "PromptPay";
  gfx->setCursor(qrAreaX + (244 - strlen(ppLabel)*6)/2, qrAreaY + 248);
  gfx->print(ppLabel);

  gfx->fillRect(0, SCR_H - 4, SCR_W, 4, C_DARKGREY);
  gfx->fillRect(0, SCR_H - 4, SCR_W, 4, C_GOLD);

  Serial.println("[UI] QR screen drawn");
}

// ============================================================
//  UPDATE QR TIMER
// ============================================================
void updateQrTimer(int secondsLeft) {
  g_qrSecondsLeft = secondsLeft;
  if (secondsLeft < 0) secondsLeft = 0;

  int lx = 30;
  int ty = STATUS_H + 116 + 3*20 + 16;
  gfx->fillRect(lx, ty, 120, 20, C_BG);

  int m = secondsLeft / 60;
  int s = secondsLeft % 60;
  char buf[8]; snprintf(buf, 8, "%d:%02d", m, s);
  gfx->setTextColor(secondsLeft < 30 ? C_RED : C_ORANGE);
  gfx->setTextSize(2);
  gfx->setCursor(lx, ty);
  gfx->print(buf);

  int barW = (int)((float)SCR_W * secondsLeft / 120.0f);
  gfx->fillRect(0, SCR_H - 4, SCR_W, 4, C_DARKGREY);
  if (barW > 0) gfx->fillRect(0, SCR_H - 4, barW, 4,
                               secondsLeft < 30 ? C_RED : C_GOLD);
}

// ============================================================
//  DRAW VENDING SCREEN
// ============================================================
void drawVendingScreen(int slotIdx) {
  g_idleDrawn = false;
  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_DISPENSING);

  SlotConfig& s = (slotIdx >= 0 && slotIdx < NUM_SLOTS)
                  ? g_slots[slotIdx] : g_slots[0];

  gfx->setTextColor(C_GOLD); gfx->setTextSize(4);
  const char* vendTitle = "Dispensing...";
  gfx->setCursor(SCR_W/2 - strlen(vendTitle)*24/2, 130);
  gfx->print(vendTitle);

  gfx->setTextColor(C_WHITE); gfx->setTextSize(3);
  gfx->setCursor(SCR_W/2 - strlen(s.name_en)*18/2, 210);
  gfx->print(s.name_en);

  gfx->setTextColor(C_GREY); gfx->setTextSize(2);
  const char* sub = "Please wait...";
  gfx->setCursor(SCR_W/2 - strlen(sub)*12/2, 270);
  gfx->print(sub);

  int bx = SCR_W/2 - 200, by = 360;
  gfx->drawRect(bx, by, 400, 8, C_DARKGOLD);
  gfx->fillRect(bx+1, by+1, 238, 6, C_GREEN);

  Serial.printf("[UI] Vending screen: slot %d\n", slotIdx);
}

// ============================================================
//  DRAW COMPLETION SCREEN
// ============================================================
void drawCompletionScreen(int slotIdx, int luckyNumber, bool sacredWater) {
  g_idleDrawn = false;
  gfx->fillScreen(C_BG);

  uint32_t seed = luckyNumber + 42;
  for (int i = 0; i < 60; i++) {
    seed = seed * 1664525 + 1013904223;  // LCG — no srand/rand dependency
    int sx = (seed >> 8) % SCR_W;
    seed = seed * 1664525 + 1013904223;
    int sy = (seed >> 8) % (SCR_H - 80);
    seed = seed * 1664525 + 1013904223;
    uint8_t br = 80 + (seed & 0xFF) % 175;
    gfx->fillRect(sx, sy, 1 + ((seed>>16)&1), 1 + ((seed>>17)&1),
                  gfx->color565(br, br, br));
  }

  gfx->setTextColor(C_GOLD); gfx->setTextSize(2);
  const char* th1 = "Your Merit Lucky Number";
  gfx->setCursor(SCR_W/2 - strlen(th1)*12/2, STATUS_H + 28);
  gfx->print(th1);

  char lnBuf[8]; snprintf(lnBuf, 8, "%d", luckyNumber);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(8);
  gfx->setCursor(SCR_W/2 - strlen(lnBuf)*48/2, SCR_H/2 - 60);
  gfx->print(lnBuf);

  gfx->setTextColor(C_GOLD_DIM); gfx->setTextSize(1);
  const char* bless = "May blessings be upon you";
  gfx->setCursor(SCR_W/2 - strlen(bless)*6/2, SCR_H/2 + 44);
  gfx->print(bless);

  if (sacredWater) {
    int bY = SCR_H - 100;
    gfx->fillRect(40, bY, SCR_W - 80, 36, gfx->color565(10, 30, 60));
    _drawRoundRect(40, bY, SCR_W - 80, 36, 6, gfx->color565(79, 195, 247));
    gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
    const char* wBanner = "Sacred water blessing activated";
    gfx->setCursor(SCR_W/2 - strlen(wBanner)*6/2, bY + 14);
    gfx->print(wBanner);
  }

  int footY = SCR_H - 54;
  gfx->fillRect(0, footY, SCR_W, 54, C_DARKGOLD);
  gfx->drawRect(0, footY, SCR_W, 54, C_GOLD);
  gfx->setTextColor(C_BLACK); gfx->setTextSize(2);
  const char* thanks = "Thank you for your donation";
  gfx->setCursor(SCR_W/2 - strlen(thanks)*12/2, footY + 8);
  gfx->print(thanks);
  gfx->setTextSize(1);
  const char* subThanks = "Good deeds bring good returns";
  gfx->setCursor(SCR_W/2 - strlen(subThanks)*6/2, footY + 32);
  gfx->print(subThanks);

  Serial.printf("[UI] Completion: slot=%d lucky=%d water=%d\n",
                slotIdx, luckyNumber, sacredWater);
}

// ============================================================
//  DRAW ERROR SCREEN
// ============================================================
void drawErrorScreen(String message) {
  g_idleDrawn = false;
  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_IDLE);

  gfx->fillCircle(SCR_W/2, 170, 45, C_RED);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(4);
  gfx->setCursor(SCR_W/2 - 12, 152);
  gfx->print("X");

  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  int lineY = 240;
  String msg = message;
  msg.replace("\\n", "\n");
  int start = 0;
  while (start < (int)msg.length()) {
    int nl  = msg.indexOf('\n', start);
    int end = (nl == -1) ? msg.length() : nl;
    String line = msg.substring(start, min(end, start + 42));
    gfx->setCursor(SCR_W/2 - (int)(line.length()*12)/2, lineY);
    gfx->print(line.c_str());
    lineY += 28;
    start = end + 1;
    if (start >= (int)msg.length()) break;
    if (lineY > SCR_H - 40) break;
  }

  Serial.printf("[UI] Error: %s\n", message.c_str());
}

// ============================================================
//  SERVICE SCREEN  (5 tabs)
// ============================================================
#define SVC_TAB_W  110
#define SVC_BODY_X (SVC_TAB_W + 4)

static void _drawSvcTabBar(int activeTab) {
  const char* tabLabels[5] = {"Self\nTest","Free\nPlay","Devices","Settings","Firmware"};
  const char* tabSingle[5] = {"Self-Test","FreePlay","Devices","Settings","Firmware"};
  int tabH = (SCR_H - STATUS_H) / 5;
  for (int i = 0; i < 5; i++) {
    int ty = STATUS_H + i * tabH;
    uint16_t bg = (i == activeTab) ? C_DARKGOLD : gfx->color565(20,16,30);
    uint16_t bd = (i == activeTab) ? C_GOLD : C_MIDGREY;
    gfx->fillRect(0, ty, SVC_TAB_W, tabH - 1, bg);
    gfx->drawRect(0, ty, SVC_TAB_W, tabH - 1, bd);
    gfx->setTextColor(i == activeTab ? C_BLACK : C_GREY);
    gfx->setTextSize(1);
    int lw = strlen(tabSingle[i]) * 6;
    gfx->setCursor(SVC_TAB_W/2 - lw/2, ty + tabH/2 - 4);
    gfx->print(tabSingle[i]);
  }
}

static void _drawSvcBody_SelfTest(int bodyX) {
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(bodyX + 16, STATUS_H + 16);
  gfx->print("Self Test");
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 16, STATUS_H + 46);
  gfx->print("Tap a slot to test motor");
  // Show slot status grid
  int cols = g_grid_cols > 0 ? g_grid_cols : 5;
  for (int i = 0; i < NUM_SLOTS && i < 21; i++) {
    int col = i % cols;
    int row = i / cols;
    int cx = bodyX + 16 + col * 44;
    int cy = STATUS_H + 70 + row * 44;
    uint16_t bg = g_slots[i].enabled ? C_GREEN : C_DARKGREY;
    gfx->fillRoundRect(cx, cy, 38, 38, 4, bg);
    gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
    char nb[4]; snprintf(nb, 4, "%d", i+1);
    gfx->setCursor(cx + 19 - strlen(nb)*6, cy + 11);
    gfx->print(nb);
  }
}

static void _drawSvcBody_FreePlay(int bodyX) {
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(bodyX + 16, STATUS_H + 16);
  gfx->print("Free Play");
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 16, STATUS_H + 46);
  gfx->print("Tap slot to vend without payment");
  // Same mini-grid
  int cols = g_grid_cols > 0 ? g_grid_cols : 5;
  for (int i = 0; i < NUM_SLOTS && i < 21; i++) {
    int col = i % cols;
    int row = i / cols;
    int cx = bodyX + 16 + col * 44;
    int cy = STATUS_H + 70 + row * 44;
    gfx->fillRoundRect(cx, cy, 38, 38, 4, C_BGCELL);
    _drawRoundRect(cx, cy, 38, 38, 4, C_GOLD);
    gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
    char nb[4]; snprintf(nb, 4, "%d", i+1);
    gfx->setCursor(cx + 19 - strlen(nb)*6, cy + 11);
    gfx->print(nb);
  }
}

static void _drawSvcBody_Devices(int bodyX) {
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(bodyX + 16, STATUS_H + 16);
  gfx->print("Devices");
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  int y = STATUS_H + 50;
  // MCP23017
  gfx->setCursor(bodyX+16, y); gfx->print("MCP23017 0x20:"); y+=16;
  gfx->setCursor(bodyX+16, y); gfx->print("MCP23017 0x21:"); y+=16;
  gfx->setCursor(bodyX+16, y); gfx->print("MCP23017 0x22:"); y+=16;
  y+=8;
  gfx->setCursor(bodyX+16, y); gfx->print("WS2812B x40: OK"); y+=16;
  gfx->setCursor(bodyX+16, y); gfx->print("IR Sensors: ..."); y+=16;
  gfx->setCursor(bodyX+16, y); gfx->print("Touch GT911: OK"); y+=16;
}

static void _drawSvcBody_Settings(int bodyX) {
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(bodyX + 16, STATUS_H + 16);
  gfx->print("Settings");

  int y = STATUS_H + 52;
  // Action buttons
  // [401] Factory Reset
  _fillRoundRect(bodyX + 16, y, 260, 44, 8, C_RED);
  _drawRoundRect(bodyX + 16, y, 260, 44, 8, gfx->color565(200, 50, 50));
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(bodyX + 36, y + 13);
  gfx->print("Factory Reset (401)");
  y += 56;

  // [402] Toggle Boot PIN
  _fillRoundRect(bodyX + 16, y, 260, 44, 8, gfx->color565(30, 60, 120));
  _drawRoundRect(bodyX + 16, y, 260, 44, 8, C_GOLD);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(bodyX + 36, y + 13);
  gfx->print("Toggle Boot PIN (402)");
  y += 56;

  // Lang toggle
  _fillRoundRect(bodyX + 16, y, 260, 44, 8, gfx->color565(20, 40, 20));
  _drawRoundRect(bodyX + 16, y, 260, 44, 8, C_GREEN);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(bodyX + 36, y + 13);
  gfx->print(g_lang_th ? "Lang: TH" : "Lang: EN");
}

static void _drawSvcBody_Firmware(int bodyX) {
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(bodyX + 16, STATUS_H + 16);
  gfx->print("Firmware");
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  int y = STATUS_H + 50;
  gfx->setCursor(bodyX+16, y); gfx->print("Version: " FW_VERSION); y+=20;
  gfx->setCursor(bodyX+16, y); gfx->print("OTA: not implemented"); y+=20;
  gfx->setCursor(bodyX+16, y); gfx->print("Build: " __DATE__ " " __TIME__);
}

void drawServiceScreen(int tab) {
  gfx->fillScreen(C_BG);
  gfx->fillRect(0, 0, SCR_W, STATUS_H, C_DARKGREY);
  gfx->setTextColor(C_ORANGE); gfx->setTextSize(2);
  gfx->setCursor(10, 12);
  gfx->print("SERVICE");
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  gfx->setCursor(SCR_W - 90, 12);
  gfx->print("[EXIT]");

  _drawSvcTabBar(tab);

  int bodyX = SVC_BODY_X;
  gfx->fillRect(bodyX, STATUS_H, SCR_W - bodyX, SCR_H - STATUS_H, C_BG);

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
  _touch.read();
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
  _touch.read();
  if (!_touch.isTouched) return false;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  // EXIT label top-right of status bar
  if (tx > SCR_W - 100 && ty < STATUS_H) {
    static unsigned long _lastExitMs = 0;
    if (millis() - _lastExitMs < 200) return false;
    _lastExitMs = millis();
    return true;
  }
  return false;
}

// Returns action code (301=motor test, 401=factory reset, 402=toggle boot PIN, 0=none)
int getTouchedServiceContent(int tab) {
  _touch.read();
  if (!_touch.isTouched) return 0;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  if (tx <= SVC_TAB_W) return 0;

  static unsigned long _lastSvcCntMs = 0;
  if (millis() - _lastSvcCntMs < 200) return 0;

  if (tab == TAB_SETTINGS) {
    int bodyX = SVC_BODY_X;
    int y401  = STATUS_H + 52;
    int y402  = y401 + 56;
    if (ty >= y401 && ty <= y401 + 44 && tx >= bodyX + 16 && tx <= bodyX + 276) {
      _lastSvcCntMs = millis();
      return 401;
    }
    if (ty >= y402 && ty <= y402 + 44 && tx >= bodyX + 16 && tx <= bodyX + 276) {
      _lastSvcCntMs = millis();
      return 402;
    }
  }

  if (tab == TAB_SELFTEST || tab == TAB_FREEPLAY) {
    int bodyX = SVC_BODY_X;
    int cols  = g_grid_cols > 0 ? g_grid_cols : 5;
    for (int i = 0; i < NUM_SLOTS && i < 21; i++) {
      int col = i % cols;
      int row = i / cols;
      int cx = bodyX + 16 + col * 44;
      int cy = STATUS_H + 70 + row * 44;
      if (tx >= cx && tx <= cx + 38 && ty >= cy && ty <= cy + 38) {
        _lastSvcCntMs = millis();
        return 300 + i + 1;  // 301 = slot 1, 302 = slot 2...
      }
    }
  }
  return 0;
}

// ============================================================
//  GET TOUCHED SLOT — returns 0..NUM_SLOTS-1 or -1
// ============================================================
int getTouchedSlotXY(int tx, int ty) {
  int xOff = (g_grid_rows >= 3) ? SIDE_TAB_W : GRID_X;
  if (ty < GRID_Y) return -1;
  if (tx < xOff)   return -1;
  int col = (tx - xOff) / (CELL_W + GRID_PAD);
  int row = (ty - GRID_Y) / (CELL_H + GRID_PAD);
  if (col < 0 || col >= g_grid_cols) return -1;
  if (row < 0 || row >= g_grid_rows) return -1;
  int slotIdx = row * g_grid_cols + col;
  if (slotIdx < 0 || slotIdx >= NUM_SLOTS) return -1;
  if (!g_slots[slotIdx].enabled || !g_slots[slotIdx].configured) return -1;
  return slotIdx;
}

int getTouchedSlot() {
  _touch.read();
  if (!_touch.isTouched) return -1;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  static unsigned long lastTouchMs = 0;
  if (millis() - lastTouchMs < 80) return -1;
  int slotIdx = getTouchedSlotXY(tx, ty);
  if (slotIdx >= 0) {
    lastTouchMs = millis();
    Serial.printf("[UI] Touch: slot %d (%s %dTHB)\n",
                  slotIdx, g_slots[slotIdx].name_en, g_slots[slotIdx].price);
  }
  return slotIdx;
}

// ============================================================
//  GET TOUCHED GIFT OPTION — 0=item only, 1=water, -1=none
// ============================================================
int getTouchedGiftOption() {
  _touch.read();
  if (!_touch.isTouched) return -1;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  int cardW = 280, cardH = 200;
  int cardY = STATUS_H + 60;
  int cardAX = SCR_W/2 - cardW - 30;
  int cardBX = SCR_W/2 + 30;
  static unsigned long lastGiftTouchMs = 0;
  if (millis() - lastGiftTouchMs < 80) return -1;
  if (ty >= cardY && ty <= cardY + cardH) {
    if (tx >= cardAX && tx <= cardAX + cardW) {
      lastGiftTouchMs = millis();
      Serial.println("[UI] Gift touch: Item Only");
      return 0;
    }
    if (tx >= cardBX && tx <= cardBX + cardW) {
      lastGiftTouchMs = millis();
      Serial.println("[UI] Gift touch: +Sacred Water");
      return 1;
    }
  }
  return -1;
}

// ============================================================
//  SERVICE GESTURE CHECK — 3 taps top-right within 2s
// ============================================================
bool checkServiceGesture() {
  static int tapCount = 0;
  static unsigned long firstTapMs = 0;
  static unsigned long lastSvcTap = 0;

  _touch.read();
  if (!_touch.isTouched) return false;

  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;

  if (tx < 700 || ty > 80) { tapCount = 0; return false; }
  if (millis() - lastSvcTap < 100) return false;
  lastSvcTap = millis();

  if (tapCount == 0) firstTapMs = millis();
  tapCount++;

  if (millis() - firstTapMs > 2000) { tapCount = 1; firstTapMs = millis(); }
  if (tapCount >= 3) { tapCount = 0; Serial.println("[UI] Service gesture!"); return true; }
  return false;
}

// ============================================================
//  LANG TOGGLE CHECK
// ============================================================
bool checkLangToggleTap() {
  // Bottom-left corner tap (x<60, y>420)
  _touch.read();
  if (!_touch.isTouched) return false;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  if (tx < 60 && ty > 420) {
    static unsigned long _lastLangMs = 0;
    if (millis() - _lastLangMs < 500) return false;
    _lastLangMs = millis();
    g_lang_th = !g_lang_th;
    Serial.printf("[UI] Lang: %s\n", g_lang_th ? "TH" : "EN");
    return true;
  }
  return false;
}

// ============================================================
//  SERVICE LOG OVERLAY  (prints line at bottom of service screen)
// ============================================================
void svcLog(String msg) {
  gfx->fillRect(SVC_BODY_X, SCR_H - 24, SCR_W - SVC_BODY_X, 24, gfx->color565(10,6,18));
  gfx->setTextColor(C_GREEN);
  gfx->setTextSize(1);
  gfx->setCursor(SVC_BODY_X + 8, SCR_H - 16);
  gfx->print(msg.c_str());
}

// ============================================================
//  IDLE ANIMATION  — brief gold flash (screen, ui.h)
//  idleAnimation() in hardware.h does the LED breathing separately
// ============================================================
void idleAnimationUI() {
  for (int i = 0; i < 2; i++) {
    for (int t = 0; t < 3; t++) {
      gfx->drawRect(t, t, SCR_W - t*2, SCR_H - t*2, C_GOLD);
    }
    delay(120);
    gfx->fillRect(0, 0, SCR_W, 4, C_BG);
    gfx->fillRect(0, SCR_H-4, SCR_W, 4, C_BG);
    gfx->fillRect(0, 0, 4, SCR_H, C_BG);
    gfx->fillRect(SCR_W-4, 0, 4, SCR_H, C_BG);
    delay(80);
  }
}

// ============================================================
//  WIFI SETUP SCREEN  (R5 — blocking, calls saveWifiAndReboot)
//  Shown on first boot when NVS has no WiFi credentials.
//  Owner enters SSID + password via touchscreen keyboard.
//  On CONNECT tap: credentials saved to NVS, device restarts.
//  Pattern: same touch-detect/debounce structure as _drawNumpad().
// ============================================================

// Forward declare — defined in network.h (included before ui.h)
void saveWifiAndReboot(const String& ssid, const String& pass);

// ── Keyboard layout constants ────────────────────────────────
#define _WKB_KEY_W   68   // key width
#define _WKB_KEY_H   46   // key height
#define _WKB_GAP      4   // gap between keys
#define _WKB_X       42   // left edge for 10-key rows
#define _WKB_Y      228   // keyboard top (leaves ~184px for fields)

// Characters per row and centering offsets (to align shorter rows under row 0)
static const int _wkbCnt[4]   = { 10, 10, 9, 7 };
static const int _wkbOff[4]   = { 0, 0,
  (_WKB_KEY_W + _WKB_GAP) / 2,        // row 2: 9 keys, offset 1 half-key
  3 * (_WKB_KEY_W + _WKB_GAP) / 2 };  // row 3: 7 keys, offset 3 half-keys

// Upper/lower case rows (row 0 = digits, always same)
static const char* _wkbUpper[4] = { "1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
static const char* _wkbLower[4] = { "1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm" };

// Row 4 special key bounds (all at row y = _WKB_Y + 4*(key+gap))
// CAPS: x=42  w=100   → x range [42,142)
// SPACE: x=146 w=310  → x range [146,456)
// DEL: x=460 w=100   → x range [460,560)
// CONNECT: x=564 w=194 → x range [564,758)
#define _WKB4_CAPS_X    42
#define _WKB4_CAPS_W   100
#define _WKB4_SPC_X    146
#define _WKB4_SPC_W    310
#define _WKB4_DEL_X    460
#define _WKB4_DEL_W    100
#define _WKB4_CON_X    564
#define _WKB4_CON_W    194

static void _wkbDrawKeys(bool caps) {
  const char** rows = caps ? _wkbUpper : _wkbLower;
  for (int r = 0; r < 4; r++) {
    int rx = _WKB_X + _wkbOff[r];
    int ry = _WKB_Y + r * (_WKB_KEY_H + _WKB_GAP);
    for (int k = 0; k < _wkbCnt[r]; k++) {
      int kx = rx + k * (_WKB_KEY_W + _WKB_GAP);
      uint16_t bg = (r == 0) ? gfx->color565(15, 25, 50) : gfx->color565(30, 22, 55);
      _fillRoundRect(kx, ry, _WKB_KEY_W, _WKB_KEY_H, 6, bg);
      _drawRoundRect(kx, ry, _WKB_KEY_W, _WKB_KEY_H, 6, C_GOLD);
      char ch[2] = { rows[r][k], 0 };
      gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
      gfx->setCursor(kx + _WKB_KEY_W/2 - 6, ry + _WKB_KEY_H/2 - 8);
      gfx->print(ch);
    }
  }
  // Row 4 special keys
  int ry4 = _WKB_Y + 4 * (_WKB_KEY_H + _WKB_GAP);
  // CAPS — highlighted when active
  _fillRoundRect(_WKB4_CAPS_X, ry4, _WKB4_CAPS_W, _WKB_KEY_H, 6,
                 caps ? C_GOLD : gfx->color565(30, 22, 55));
  _drawRoundRect(_WKB4_CAPS_X, ry4, _WKB4_CAPS_W, _WKB_KEY_H, 6, C_GOLD);
  gfx->setTextColor(caps ? C_BLACK : C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(_WKB4_CAPS_X + 28, ry4 + _WKB_KEY_H/2 - 4);
  gfx->print("CAPS");
  // SPACE
  _fillRoundRect(_WKB4_SPC_X, ry4, _WKB4_SPC_W, _WKB_KEY_H, 6, gfx->color565(30, 22, 55));
  _drawRoundRect(_WKB4_SPC_X, ry4, _WKB4_SPC_W, _WKB_KEY_H, 6, C_GOLD);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(_WKB4_SPC_X + _WKB4_SPC_W/2 - 22, ry4 + _WKB_KEY_H/2 - 4);
  gfx->print("SPACE");
  // DEL
  _fillRoundRect(_WKB4_DEL_X, ry4, _WKB4_DEL_W, _WKB_KEY_H, 6, C_RED);
  _drawRoundRect(_WKB4_DEL_X, ry4, _WKB4_DEL_W, _WKB_KEY_H, 6, C_GOLD);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(_WKB4_DEL_X + 28, ry4 + _WKB_KEY_H/2 - 4);
  gfx->print("DEL");
  // CONNECT
  _fillRoundRect(_WKB4_CON_X, ry4, _WKB4_CON_W, _WKB_KEY_H, 6, C_GREEN);
  _drawRoundRect(_WKB4_CON_X, ry4, _WKB4_CON_W, _WKB_KEY_H, 6, C_GOLD);
  gfx->setTextColor(C_BLACK); gfx->setTextSize(1);
  gfx->setCursor(_WKB4_CON_X + 38, ry4 + _WKB_KEY_H/2 - 4);
  gfx->print("CONNECT");
}

// Returns char value for letter/digit tap, or special codes:
//   1 = CAPS, 2 = SPACE, 3 = DEL, 4 = CONNECT, 0 = no tap
static int _wkbGetKey(bool caps) {
  _touch.read();
  if (!_touch.isTouched) return 0;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  static unsigned long _wkbLastMs = 0;
  if (millis() - _wkbLastMs < 120) return 0;

  const char** rows = caps ? _wkbUpper : _wkbLower;

  // Letter / digit rows
  for (int r = 0; r < 4; r++) {
    int rx = _WKB_X + _wkbOff[r];
    int ry = _WKB_Y + r * (_WKB_KEY_H + _WKB_GAP);
    if (ty < ry || ty > ry + _WKB_KEY_H) continue;
    for (int k = 0; k < _wkbCnt[r]; k++) {
      int kx = rx + k * (_WKB_KEY_W + _WKB_GAP);
      if (tx >= kx && tx <= kx + _WKB_KEY_W) {
        _wkbLastMs = millis();
        return (int)(unsigned char)rows[r][k];
      }
    }
  }

  // Row 4 special keys
  int ry4 = _WKB_Y + 4 * (_WKB_KEY_H + _WKB_GAP);
  if (ty >= ry4 && ty <= ry4 + _WKB_KEY_H) {
    if (tx >= _WKB4_CAPS_X && tx < _WKB4_CAPS_X + _WKB4_CAPS_W) { _wkbLastMs = millis(); return 1; }
    if (tx >= _WKB4_SPC_X  && tx < _WKB4_SPC_X  + _WKB4_SPC_W)  { _wkbLastMs = millis(); return 2; }
    if (tx >= _WKB4_DEL_X  && tx < _WKB4_DEL_X  + _WKB4_DEL_W)  { _wkbLastMs = millis(); return 3; }
    if (tx >= _WKB4_CON_X  && tx < _WKB4_CON_X  + _WKB4_CON_W)  { _wkbLastMs = millis(); return 4; }
  }
  return 0;
}

static void _wkbDrawFields(int activeField, const String& ssid, const String& pass) {
  int fY1 = STATUS_H + 20;
  int fY2 = STATUS_H + 88;
  int fW  = SCR_W - 40;

  // SSID field
  uint16_t c1 = (activeField == 0) ? C_GOLD : C_MIDGREY;
  gfx->fillRect(0, STATUS_H + 8, SCR_W, 130, C_BG);  // clear field area
  gfx->setTextColor(c1); gfx->setTextSize(1);
  gfx->setCursor(20, fY1 - 12);
  gfx->print("WiFi Network Name (SSID):");
  _fillRoundRect(20, fY1, fW, 36, 6, gfx->color565(15, 12, 28));
  _drawRoundRect(20, fY1, fW, 36, 6, c1);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(28, fY1 + 10);
  gfx->print(ssid.length() > 0 ? ssid.c_str() : " ");

  // Password field
  uint16_t c2 = (activeField == 1) ? C_GOLD : C_MIDGREY;
  gfx->setTextColor(c2); gfx->setTextSize(1);
  gfx->setCursor(20, fY2 - 12);
  gfx->print("Password:");
  _fillRoundRect(20, fY2, fW, 36, 6, gfx->color565(15, 12, 28));
  _drawRoundRect(20, fY2, fW, 36, 6, c2);
  String masked = "";
  for (size_t i = 0; i < pass.length(); i++) masked += '*';
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(28, fY2 + 10);
  gfx->print(masked.length() > 0 ? masked.c_str() : " ");

  // Hint
  gfx->setTextColor(C_DARKGOLD); gfx->setTextSize(1);
  gfx->setCursor(20, STATUS_H + 145);
  gfx->print("Tap field to switch. CONNECT to save and restart.");
}

// Blocking — returns only via ESP.restart() inside saveWifiAndReboot()
void drawWifiSetupScreen() {
  gfx->fillScreen(C_BG);

  // Header bar
  gfx->fillRect(0, 0, SCR_W, STATUS_H, C_DARKGOLD);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(3);
  gfx->setCursor(10, 8);
  gfx->print("SATU");
  gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
  gfx->setCursor(SCR_W/2 - 88, 12);
  gfx->print("WiFi Setup");

  String ssid = "";
  String pass = "";
  int    activeField = 0;  // 0 = SSID, 1 = password
  bool   caps = true;

  _wkbDrawKeys(caps);
  _wkbDrawFields(activeField, ssid, pass);

  int fY1 = STATUS_H + 20;
  int fY2 = STATUS_H + 88;

  while (true) {
    // Field selection tap (above keyboard)
    _touch.read();
    if (_touch.isTouched) {
      int tx = _touch.points[0].x;
      int ty = _touch.points[0].y;
      if (ty >= fY1 && ty <= fY1 + 36 && activeField != 0) {
        activeField = 0;
        _wkbDrawFields(activeField, ssid, pass);
        delay(150);
        continue;
      }
      if (ty >= fY2 && ty <= fY2 + 36 && activeField != 1) {
        activeField = 1;
        _wkbDrawFields(activeField, ssid, pass);
        delay(150);
        continue;
      }
    }

    int key = _wkbGetKey(caps);
    if (key == 0) { delay(10); continue; }

    if (key == 1) {
      // CAPS toggle
      caps = !caps;
      _wkbDrawKeys(caps);

    } else if (key == 2) {
      // SPACE
      String& cur = (activeField == 0) ? ssid : pass;
      int maxLen  = (activeField == 0) ? 32 : 63;
      if ((int)cur.length() < maxLen) {
        cur += ' ';
        _wkbDrawFields(activeField, ssid, pass);
      }

    } else if (key == 3) {
      // DEL
      String& cur = (activeField == 0) ? ssid : pass;
      if (cur.length() > 0) {
        cur.remove(cur.length() - 1);
        _wkbDrawFields(activeField, ssid, pass);
      }

    } else if (key == 4) {
      // CONNECT
      if (ssid.length() == 0) {
        // Flash SSID border red to indicate required
        _drawRoundRect(20, fY1, SCR_W - 40, 36, 6, C_RED);
        delay(400);
        _wkbDrawFields(activeField, ssid, pass);
      } else {
        // Show saving message then reboot
        gfx->fillRect(0, STATUS_H, SCR_W, 130, C_BG);
        gfx->setTextColor(C_GOLD); gfx->setTextSize(2);
        gfx->setCursor(SCR_W/2 - 150, STATUS_H + 50);
        gfx->print("Saving & Restarting...");
        delay(500);
        saveWifiAndReboot(ssid, pass);  // defined in network.h — never returns
      }

    } else {
      // Regular character
      char ch = (char)key;
      String& cur = (activeField == 0) ? ssid : pass;
      int maxLen  = (activeField == 0) ? 32 : 63;
      if ((int)cur.length() < maxLen) {
        cur += ch;
        _wkbDrawFields(activeField, ssid, pass);
      }
    }
  }
}

#endif // UI_H
