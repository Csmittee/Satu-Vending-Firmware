// ============================================================
// ui.h — Satu Vending Machine Display Layer  R3
// Board  : ESP32-8048S070C — 7" 800×480 capacitive touch
// Driver : Arduino_GFX (RGB panel, EK9716)
// Touch  : TAMC_GT911  SDA=19 SCL=20 ROTATION_INVERTED
// ============================================================
// CHANGE LOG:
//   R3 — Full rewrite: TFT_eSPI → Arduino_GFX
//        Grid: 5×2 (10 lanes) → 7×3 (21 lanes)
//        Remote slot config: SlotConfig struct, loaded from /hello
//        Price-as-hero design: price color auto-tiers
//        Status bar: 5 states (Thai label, device_id, WiFi)
//        New screens: drawGiftOptionScreen()
//        Lucky+ThankYou: combined ceremonial screen
//        All colours: RGB888 via gfx->color565()
// ============================================================
// PUBLIC API (called by satu_vending.ino / state machine):
//   initUI()
//   loadSlotsFromJson(JsonArray)   — call after /hello response
//   drawIdleScreen()
//   drawProductSelection(int slot)
//   drawGiftOptionScreen(int slot) — Item Only vs +Sacred Water
//   drawQrScreen(String qr, int amount, int slot)
//   updateQrTimer(int secondsLeft)
//   drawVendingScreen(int slot)
//   drawCompletionScreen(int slot, int luckyNumber, bool sacredWater)
//   drawErrorScreen(String msg)
//   drawBootScreen(String status)
//   int getTouchedSlot()           — 0-20 or -1
//   int getTouchedGiftOption()     — 0=item only, 1=water, -1=none
// ============================================================

#ifndef UI_H
#define UI_H

#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <TAMC_GT911.h>
#include <ArduinoJson.h>
#include "config.h"

// ── Optional QR library ──────────────────────────────────────────────────────
// Arduino Library Manager: search "QRCode" by Richard Moore
#define SATU_HAS_QR_LIB
#ifdef SATU_HAS_QR_LIB
  #include <qrcode.h>
#endif

// ============================================================
//  DISPLAY + TOUCH OBJECTS  (matches Working_test_for_the_screen)
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
//  COLOUR PALETTE  — RGB565 via color565(r,g,b)
// ============================================================
// Precompute at runtime in initUI() — stored as uint16_t globals
static uint16_t C_BLACK, C_WHITE, C_BG, C_BGCELL, C_BGCELL_SEL;
static uint16_t C_GOLD, C_DARKGOLD, C_GOLD_DIM;
static uint16_t C_GREY, C_DARKGREY, C_MIDGREY;
static uint16_t C_GREEN, C_RED, C_ORANGE;
// Price tier colours (from design spec)
static uint16_t C_PRICE_TEAL;       // 50  THB  #4FC3F7
static uint16_t C_PRICE_GOLD;       // 100 THB  #C9A84C
static uint16_t C_PRICE_AMBER;      // 200 THB  #F0A500
static uint16_t C_PRICE_ORANGE;     // 300 THB  #FF8C00
static uint16_t C_PRICE_DEEPORANGE; // 500 THB  #FF5722

// ============================================================
//  SCREEN DIMENSIONS
// ============================================================
#define SCR_W       800
#define SCR_H       480
#define STATUS_H     44   // status bar height (px)

// ============================================================
//  GRID LAYOUT  7 cols × 3 rows = 21 slots
// ============================================================
#define GRID_COLS     7
#define GRID_ROWS     3
#define GRID_PAD      5    // gap between cells
#define GRID_X        4    // left edge
#define GRID_Y       (STATUS_H + 3)
// Cell size computed from available space
#define CELL_W       ((SCR_W - GRID_X*2 - GRID_PAD*(GRID_COLS-1)) / GRID_COLS)  // ~109px
#define CELL_H       ((SCR_H - GRID_Y   - GRID_PAD*(GRID_ROWS-1)) / GRID_ROWS)  // ~143px
#define NUM_SLOTS    (GRID_COLS * GRID_ROWS)   // 21

// ============================================================
//  SLOT CONFIG STRUCT
//  Populated from /hello JSON → machine_slots[]
//  Falls back to defaults if backend returns nothing
// ============================================================
struct SlotConfig {
  char   name_th[32];   // Thai name (UTF-8)  e.g. "พระเครื่องใหญ่"
  char   name_en[32];   // English fallback    e.g. "Large Amulet"
  int    price;         // THB — any value, owner-defined
  bool   enabled;       // false = grey/disabled
  bool   configured;    // false = never set by owner → show "Slot N"
};

static SlotConfig g_slots[NUM_SLOTS];

// ── Default slot config (used until /hello populates g_slots) ───────────────
static void _initDefaultSlots() {
  for (int i = 0; i < NUM_SLOTS; i++) {
    snprintf(g_slots[i].name_en, 32, "Slot %d", i + 1);
    g_slots[i].name_th[0] = '\0';
    g_slots[i].price      = 0;
    g_slots[i].enabled    = false;    // unconfigured = disabled
    g_slots[i].configured = false;
  }
}

// ── Load slots from ArduinoJson array (call after /hello parse) ──────────────
// Expected JSON: [{"slot":1,"name_th":"...","name_en":"...","price":100,"enabled":true}, ...]
void loadSlotsFromJson(JsonArray arr) {
  for (JsonObject obj : arr) {
    int idx = (int)obj["slot"] - 1;  // slot 1-21 → index 0-20
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
static int  g_selectedSlot    = -1;
static bool g_idleDrawn       = false;
static int  g_qrSecondsLeft   = 120;

// External globals (from network.h / satu_vending.ino)
extern String g_deviceId;

// ============================================================
//  HELPER: price → tier colour
// ============================================================
static uint16_t _priceColor(int price) {
  if (price <= 50)       return C_PRICE_TEAL;
  if (price <= 100)      return C_PRICE_GOLD;
  if (price <= 200)      return C_PRICE_AMBER;
  if (price <= 300)      return C_PRICE_ORANGE;
  return                        C_PRICE_DEEPORANGE;
}

// ============================================================
//  HELPER: draw filled rounded rect (Arduino_GFX has fillRoundRect)
// ============================================================
static inline void _fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
  gfx->fillRoundRect(x, y, w, h, r, c);
}
static inline void _drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
  gfx->drawRoundRect(x, y, w, h, r, c);
}

// ============================================================
//  STATUS BAR
//  Design: gold gradient bar, SATU left, Thai state centre,
//          device_id + WiFi right
// ============================================================
static const char* _stateLabels[] = {
  "เลือกสิ่งที่ต้องการ",   // idle / select
  "ยืนยันการเลือก",         // confirm
  "ชำระเงิน",               // payment
  "กำลังจ่ายของ",           // dispensing
  "Connecting..."            // connecting (ASCII only)
};
enum StatusBarState { SB_IDLE=0, SB_CONFIRM, SB_PAYMENT, SB_DISPENSING, SB_CONNECTING };

static void _drawStatusBar(StatusBarState state) {
  // Gold gradient background (simulate with two rects)
  gfx->fillRect(0, 0, SCR_W, STATUS_H, C_DARKGOLD);
  gfx->fillRect(0, STATUS_H-4, SCR_W, 4, C_GOLD);

  // Left: SATU logo
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(3);
  gfx->setCursor(10, 8);
  gfx->print("SATU");

  // Centre: Thai state label
  const char* label = _stateLabels[state];
  int labelW = strlen(label) * 12;  // approx for size 2
  gfx->setTextSize(2);
  gfx->setCursor(SCR_W/2 - labelW/2, 12);
  gfx->print(label);

  // Right: device_id + WiFi
  gfx->setTextSize(1);
  String devLabel = g_deviceId.isEmpty() ? "Connecting..." : g_deviceId;
  gfx->setCursor(SCR_W - devLabel.length()*6 - 4, 8);
  gfx->print(devLabel.c_str());

  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  gfx->setTextColor(wifiOk ? C_GREEN : C_RED);
  const char* wifiStr = wifiOk ? "WiFi OK" : "No WiFi";
  gfx->setCursor(SCR_W - strlen(wifiStr)*6 - 4, 22);
  gfx->print(wifiStr);
  gfx->setTextColor(C_WHITE);  // reset
}

// ============================================================
//  SINGLE PRODUCT CELL  (used by drawIdleScreen & drawProductSelection)
// ============================================================
static void _drawCell(int idx, bool selected) {
  int col = idx % GRID_COLS;
  int row = idx / GRID_COLS;
  int x   = GRID_X + col * (CELL_W + GRID_PAD);
  int y   = GRID_Y + row * (CELL_H + GRID_PAD);

  SlotConfig& s = g_slots[idx];
  bool disabled = !s.enabled || !s.configured;

  // ── Background ──────────────────────────────────────────────────────────
  uint16_t bgColor  = disabled ? C_DARKGREY
                    : selected ? C_BGCELL_SEL
                    : C_BGCELL;
  uint16_t bdrColor = disabled ? C_MIDGREY
                    : selected ? C_GOLD
                    : gfx->color565(42, 31, 74);  // #2a1f4a
  int bdrThick = selected ? 2 : 1;

  _fillRoundRect(x, y, CELL_W, CELL_H, 6, bgColor);
  for (int t = 0; t < bdrThick; t++) {
    _drawRoundRect(x+t, y+t, CELL_W-t*2, CELL_H-t*2, 6-t, bdrColor);
  }
  if (selected) {
    // Glow: extra dim outer ring
    _drawRoundRect(x-1, y-1, CELL_W+2, CELL_H+2, 7,
                   gfx->color565(100, 80, 20));
  }

  if (disabled) {
    // ── Unconfigured / disabled cell ─────────────────────────────────────
    char slotLabel[12];
    snprintf(slotLabel, 12, "%d", idx + 1);
    gfx->setTextSize(2);
    gfx->setTextColor(C_MIDGREY);
    int lw = strlen(slotLabel) * 12;
    gfx->setCursor(x + CELL_W/2 - lw/2, y + CELL_H/2 - 8);
    gfx->print(slotLabel);
    return;
  }

  // ── Configured cell ──────────────────────────────────────────────────────

  // Slot number badge (small circle, top-centre)
  int badgeX = x + CELL_W/2;
  int badgeY = y + 14;
  gfx->fillCircle(badgeX, badgeY, 10,
                  selected ? C_GOLD : gfx->color565(30, 22, 55));
  gfx->setTextColor(selected ? C_BLACK : C_MIDGREY);
  gfx->setTextSize(1);
  char numBuf[4];
  snprintf(numBuf, 4, "%d", idx + 1);
  int nw = strlen(numBuf) * 6;
  gfx->setCursor(badgeX - nw/2, badgeY - 4);
  gfx->print(numBuf);

  // Price HERO — large, colour-tiered
  uint16_t priceColor = selected ? C_WHITE : _priceColor(s.price);
  char priceBuf[10];
  snprintf(priceBuf, 10, "%d", s.price);
  gfx->setTextSize(4);
  gfx->setTextColor(priceColor);
  int pw = strlen(priceBuf) * 24;   // textsize 4 = ~24px/char
  int priceY = y + CELL_H/2 - 14;
  gfx->setCursor(x + CELL_W/2 - pw/2, priceY);
  gfx->print(priceBuf);

  // THB label (small, grey)
  gfx->setTextSize(1);
  gfx->setTextColor(selected ? C_GOLD_DIM : C_GREY);
  gfx->setCursor(x + CELL_W/2 - 9, priceY + 34);
  gfx->print("THB");

  // Reset colour
  gfx->setTextColor(C_WHITE);
}

// ============================================================
//  INIT UI
// ============================================================
void initUI() {
  // Backlight ON
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  // Init display
  gfx->begin();
  gfx->fillScreen(0x0000);

  // Init touch
  Wire.begin(19, 20);
  _touch.begin();
  _touch.setRotation(ROTATION_INVERTED);

  // Build colour palette
  C_BLACK           = gfx->color565(0,   0,   0);
  C_WHITE           = gfx->color565(255, 255, 255);
  C_BG              = gfx->color565(10,  8,   18);   // #0a0812
  C_BGCELL          = gfx->color565(15,  14,  26);   // #0f0e1a
  C_BGCELL_SEL      = gfx->color565(31,  21,  0);    // #1f1500
  C_GOLD            = gfx->color565(201, 168, 76);   // #C9A84C
  C_DARKGOLD        = gfx->color565(120, 90,  20);   // status bar bg
  C_GOLD_DIM        = gfx->color565(139, 105, 20);   // #8B6914
  C_GREY            = gfx->color565(85,  85,  85);
  C_DARKGREY        = gfx->color565(25,  20,  35);
  C_MIDGREY         = gfx->color565(85,  85,  100);
  C_GREEN           = gfx->color565(76,  175, 80);
  C_RED             = gfx->color565(244, 67,  54);
  C_ORANGE          = gfx->color565(255, 140, 0);
  C_PRICE_TEAL      = gfx->color565(79,  195, 247);  // #4FC3F7
  C_PRICE_GOLD      = gfx->color565(201, 168, 76);   // #C9A84C
  C_PRICE_AMBER     = gfx->color565(240, 165, 0);    // #F0A500
  C_PRICE_ORANGE    = gfx->color565(255, 140, 0);    // #FF8C00
  C_PRICE_DEEPORANGE= gfx->color565(255, 87,  34);   // #FF5722

  // Init default slots
  _initDefaultSlots();

  Serial.println("[UI] Display init OK — Arduino_GFX R3");
}

// ============================================================
//  DRAW BOOT SCREEN
//  Called during WiFi connect. status = "Connecting to WiFi..."
// ============================================================
void drawBootScreen(String status) {
  gfx->fillScreen(C_BG);

  // SATU logo centre
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(5);
  int lw = 4 * 30;  // "SATU" = 4 chars × ~30px
  gfx->setCursor(SCR_W/2 - lw/2, SCR_H/2 - 60);
  gfx->print("SATU");

  // Tagline
  gfx->setTextColor(C_GOLD_DIM);
  gfx->setTextSize(2);
  const char* tag = "Merit Donation System";
  int tw = strlen(tag) * 12;
  gfx->setCursor(SCR_W/2 - tw/2, SCR_H/2 - 10);
  gfx->print(tag);

  // Status line
  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  int sw = status.length() * 6;
  gfx->setCursor(SCR_W/2 - sw/2, SCR_H/2 + 40);
  gfx->print(status.c_str());

  // Progress bar frame
  int bx = SCR_W/2 - 140, by = SCR_H/2 + 60;
  gfx->drawRect(bx, by, 280, 4, C_DARKGOLD);

  Serial.printf("[UI] Boot screen: %s\n", status.c_str());
}

// ============================================================
//  DRAW IDLE SCREEN  (product grid)
// ============================================================
void drawIdleScreen() {
  if (g_idleDrawn) return;

  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_IDLE);

  for (int i = 0; i < NUM_SLOTS; i++) {
    _drawCell(i, false);
  }

  g_idleDrawn    = true;
  g_selectedSlot = -1;
  Serial.println("[UI] Idle screen drawn (21-slot grid)");
}

// ============================================================
//  DRAW PRODUCT SELECTION
//  Highlights selected slot + shows gold confirmation bar
// ============================================================
void drawProductSelection(int slotIdx) {
  g_idleDrawn    = false;
  g_selectedSlot = slotIdx;

  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_CONFIRM);

  for (int i = 0; i < NUM_SLOTS; i++) {
    _drawCell(i, i == slotIdx);
  }

  // Confirmation bar — full width gold at bottom
  // Design: "Tap again to confirm  |  Wait 3s to cancel"
  int barH = 44;
  int barY = SCR_H - barH;
  gfx->fillRect(0, barY, SCR_W, barH, C_DARKGOLD);
  gfx->drawRect(0, barY, SCR_W, barH, C_GOLD);
  gfx->setTextColor(C_BLACK);
  gfx->setTextSize(2);
  const char* msg = "Tap again to confirm  |  Wait 3s to cancel";
  int mw = strlen(msg) * 12;
  gfx->setCursor(SCR_W/2 - mw/2, barY + 13);
  gfx->print(msg);

  Serial.printf("[UI] Product selection: slot %d\n", slotIdx);
}

// ============================================================
//  DRAW GIFT OPTION SCREEN
//  Two large cards: "Item Only" vs "+ Sacred Water (+20 THB)"
//  Design from screenshots: dark cards, gold selected border
// ============================================================
void drawGiftOptionScreen(int slotIdx) {
  g_idleDrawn = false;
  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_CONFIRM);

  SlotConfig& s = g_slots[slotIdx];

  // Title
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(2);
  const char* title = "Choose your blessing option";
  int tw = strlen(title) * 12;
  gfx->setCursor(SCR_W/2 - tw/2, STATUS_H + 18);
  gfx->print(title);

  // ── Card A: Item Only ───────────────────────────────────────────────────
  int cardW = 280, cardH = 200;
  int cardY = STATUS_H + 60;
  int cardAX = SCR_W/2 - cardW - 30;
  int cardBX = SCR_W/2 + 30;

  // Card A background
  _fillRoundRect(cardAX, cardY, cardW, cardH, 12,
                 gfx->color565(20, 18, 35));
  _drawRoundRect(cardAX, cardY, cardW, cardH, 12,
                 gfx->color565(50, 40, 80));

  // Icon (box emoji substitute — just a rectangle icon)
  gfx->fillRect(cardAX + cardW/2 - 20, cardY + 30, 40, 40,
                gfx->color565(140, 90, 30));
  gfx->drawRect(cardAX + cardW/2 - 20, cardY + 30, 40, 40,
                C_GOLD);

  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(2);
  const char* labelA = "Item Only";
  int lw = strlen(labelA) * 12;
  gfx->setCursor(cardAX + cardW/2 - lw/2, cardY + 90);
  gfx->print(labelA);

  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  const char* subA = "Receive your donation item";
  int sw = strlen(subA) * 6;
  gfx->setCursor(cardAX + cardW/2 - sw/2, cardY + 116);
  gfx->print(subA);

  // Price of item
  char priceA[16];
  snprintf(priceA, 16, "%d THB", s.price);
  gfx->setTextColor(_priceColor(s.price));
  gfx->setTextSize(2);
  int pw = strlen(priceA) * 12;
  gfx->setCursor(cardAX + cardW/2 - pw/2, cardY + 148);
  gfx->print(priceA);

  // ── Card B: + Sacred Water ──────────────────────────────────────────────
  _fillRoundRect(cardBX, cardY, cardW, cardH, 12,
                 gfx->color565(10, 18, 35));
  // Gold border (selected / recommended)
  for (int t = 0; t < 2; t++) {
    _drawRoundRect(cardBX+t, cardY+t, cardW-t*2, cardH-t*2, 12-t, C_GOLD);
  }
  // Outer glow ring
  _drawRoundRect(cardBX-1, cardY-1, cardW+2, cardH+2, 13,
                 gfx->color565(100, 80, 20));

  // Water drop icon (filled circles)
  int wx = cardBX + cardW/2;
  gfx->fillCircle(wx - 8, cardY + 46, 10,
                  gfx->color565(79, 195, 247));
  gfx->fillCircle(wx + 8, cardY + 52, 12,
                  gfx->color565(33, 150, 243));

  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(2);
  const char* labelB = "+ Sacred Water";
  int lbw = strlen(labelB) * 12;
  gfx->setCursor(cardBX + cardW/2 - lbw/2, cardY + 90);
  gfx->print(labelB);

  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  const char* subB = "Add a sacred water spray blessing";
  int sbw = strlen(subB) * 6;
  gfx->setCursor(cardBX + cardW/2 - sbw/2, cardY + 113);
  gfx->print(subB);

  // Thai sub
  gfx->setTextColor(gfx->color565(120, 160, 200));
  const char* subBTH = "num man mongkol";  // transliteration
  int sbtw = strlen(subBTH) * 6;
  gfx->setCursor(cardBX + cardW/2 - sbtw/2, cardY + 127);
  gfx->print(subBTH);

  // Price (+20 THB)
  char priceB[20];
  snprintf(priceB, 20, "%d+20 THB", s.price);
  gfx->setTextColor(C_PRICE_ORANGE);
  gfx->setTextSize(2);
  int pbw = strlen(priceB) * 12;
  gfx->setCursor(cardBX + cardW/2 - pbw/2, cardY + 152);
  gfx->print(priceB);

  Serial.printf("[UI] Gift option screen: slot %d\n", slotIdx);
}

// ============================================================
//  DRAW QR SCREEN
//  Design: left=amount+product+instructions+timer, right=QR
// ============================================================
void drawQrScreen(String qrData, int amount, int slotIdx) {
  g_idleDrawn     = false;
  g_qrSecondsLeft = 120;

  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_PAYMENT);

  SlotConfig& s = (slotIdx >= 0 && slotIdx < NUM_SLOTS)
                  ? g_slots[slotIdx] : g_slots[0];

  // ── Left panel ──────────────────────────────────────────────────────────
  int lx = 30;

  // Amount — large gold
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(5);
  char amtBuf[12];
  snprintf(amtBuf, 12, "B%d", amount);   // B = baht symbol approx
  gfx->setCursor(lx, STATUS_H + 18);
  gfx->print(amtBuf);

  // Product name
  gfx->setTextColor(C_MIDGREY);
  gfx->setTextSize(2);
  gfx->setCursor(lx, STATUS_H + 82);
  gfx->print(s.name_en);

  // Instructions
  gfx->setTextSize(1);
  gfx->setTextColor(C_GREY);
  const char* ins[] = {
    "1. Open your banking app",
    "2. Select PromptPay QR / scan QR",
    "3. Scan the code & confirm"
  };
  int iy = STATUS_H + 116;
  for (int i = 0; i < 3; i++) {
    // Left accent line
    gfx->fillRect(lx, iy, 2, 12, C_DARKGOLD);
    gfx->setCursor(lx + 8, iy);
    gfx->print(ins[i]);
    iy += 20;
  }

  // Timer — will be updated by updateQrTimer()
  gfx->setTextColor(C_ORANGE);
  gfx->setTextSize(2);
  char timerBuf[8];
  snprintf(timerBuf, 8, "  2:00");
  gfx->setCursor(lx, iy + 16);
  gfx->print(timerBuf);

  // ── Right panel — QR ────────────────────────────────────────────────────
  int qrAreaX = SCR_W - 260;
  int qrAreaY = STATUS_H + 10;

#ifdef SATU_HAS_QR_LIB
  QRCode qrcode;
  uint8_t qrBuf[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrBuf, 3, ECC_LOW, qrData.c_str());

  int qrSz    = qrcode.size;
  int scale   = 220 / qrSz;          // auto-scale to fit ~220px
  int qrPx    = qrSz * scale;
  int qrX     = qrAreaX + (240 - qrPx) / 2;
  int qrY     = qrAreaY + 10;

  // White quiet-zone background
  gfx->fillRect(qrX - 8, qrY - 8, qrPx + 16, qrPx + 16, C_WHITE);

  for (int r = 0; r < qrSz; r++) {
    for (int c = 0; c < qrSz; c++) {
      uint16_t col = qrcode_getModule(&qrcode, c, r) ? C_BLACK : C_WHITE;
      gfx->fillRect(qrX + c*scale, qrY + r*scale, scale, scale, col);
    }
  }
  int qrBottom = qrY + qrPx + 16;
#else
  // Fallback: plain text
  int qrBottom = qrAreaY + 40;
  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  gfx->setCursor(qrAreaX, qrAreaY + 10);
  gfx->print(qrData.substring(0, 30).c_str());
#endif

  // PromptPay label
  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  const char* ppLabel = "PromptPay";
  gfx->setCursor(qrAreaX + (240 - strlen(ppLabel)*6)/2, qrBottom + 4);
  gfx->print(ppLabel);

  // Thai scan label
  gfx->setTextColor(gfx->color565(79, 195, 247));
  const char* scanLabel = "scan to pay";   // transliteration placeholder
  gfx->setCursor(qrAreaX + (240 - strlen(scanLabel)*6)/2, qrBottom + 18);
  gfx->print(scanLabel);

  // Payment progress bar (static at bottom — animates in updateQrTimer)
  gfx->fillRect(0, SCR_H - 4, SCR_W, 4, C_DARKGREY);
  gfx->fillRect(0, SCR_H - 4, SCR_W, 4, C_GOLD);

  Serial.println("[UI] QR screen drawn");
}

// ============================================================
//  UPDATE QR TIMER  (call every second from state machine)
//  Redraws just the timer text + progress bar — no full refresh
// ============================================================
void updateQrTimer(int secondsLeft) {
  g_qrSecondsLeft = secondsLeft;
  if (secondsLeft < 0) secondsLeft = 0;

  // Erase old timer text region
  int lx = 30;
  int ty = STATUS_H + 116 + 3*20 + 16;   // same Y as initial draw
  gfx->fillRect(lx, ty, 120, 20, C_BG);

  // Draw new time
  int m = secondsLeft / 60;
  int s = secondsLeft % 60;
  char buf[8];
  snprintf(buf, 8, "%d:%02d", m, s);
  gfx->setTextColor(secondsLeft < 30 ? C_RED : C_ORANGE);
  gfx->setTextSize(2);
  gfx->setCursor(lx, ty);
  gfx->print(buf);

  // Update bottom progress bar
  int barW = (int)((float)SCR_W * secondsLeft / 120.0f);
  gfx->fillRect(0, SCR_H - 4, SCR_W, 4, C_DARKGREY);
  if (barW > 0) gfx->fillRect(0, SCR_H - 4, barW, 4,
                               secondsLeft < 30 ? C_RED : C_GOLD);
}

// ============================================================
//  DRAW VENDING SCREEN
//  Shows bouncing animation text + progress bar
// ============================================================
void drawVendingScreen(int slotIdx) {
  g_idleDrawn = false;
  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_DISPENSING);

  SlotConfig& s = (slotIdx >= 0 && slotIdx < NUM_SLOTS)
                  ? g_slots[slotIdx] : g_slots[0];

  // Animated dots cycle (static frame — main loop can call again for animation)
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(4);
  const char* vendTitle = "Dispensing...";
  int tw = strlen(vendTitle) * 24;
  gfx->setCursor(SCR_W/2 - tw/2, 130);
  gfx->print(vendTitle);

  // Product name
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(3);
  int nw = strlen(s.name_en) * 18;
  gfx->setCursor(SCR_W/2 - nw/2, 210);
  gfx->print(s.name_en);

  gfx->setTextColor(C_GREY);
  gfx->setTextSize(2);
  const char* sub = "Please wait...";
  int sw = strlen(sub) * 12;
  gfx->setCursor(SCR_W/2 - sw/2, 270);
  gfx->print(sub);

  // Progress bar
  int bx = SCR_W/2 - 200, by = 360;
  gfx->drawRect(bx, by, 400, 8, C_DARKGOLD);
  // Partial fill (animated fill should come from repeated calls — here 60%)
  gfx->fillRect(bx+1, by+1, 238, 6, C_GREEN);

  Serial.printf("[UI] Vending screen: slot %d\n", slotIdx);
}

// ============================================================
//  DRAW COMPLETION SCREEN
//  Ceremonial: lucky number + Thai blessing + sacred water banner
//  Design from screenshot: dark starfield bg, large gold number
// ============================================================
void drawCompletionScreen(int slotIdx, int luckyNumber, bool sacredWater) {
  g_idleDrawn = false;
  gfx->fillScreen(C_BG);

  // Dark radial overlay (fake gradient with concentric rects — approx)
  for (int r = 0; r < 6; r++) {
    uint8_t lv = 10 + r * 4;
    gfx->drawRect(r*2, r*2, SCR_W - r*4, SCR_H - r*4,
                  gfx->color565(lv, lv/2, lv*2));
  }

  // Stars (random dots — seeded by lucky number for determinism)
  srand(luckyNumber + 42);
  for (int i = 0; i < 60; i++) {
    int sx = rand() % SCR_W;
    int sy = rand() % (SCR_H - 80);
    uint8_t br = 80 + rand() % 175;
    gfx->fillRect(sx, sy, 1+(rand()%2), 1+(rand()%2),
                  gfx->color565(br, br, br));
  }

  // ── Thai title ───────────────────────────────────────────────────────────
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(2);
  // Thai text — ASCII transliteration (no Thai font in Arduino_GFX by default)
  // To display real Thai: load a custom font with Thai glyphs
  const char* th1 = "Your Merit Lucky Number";
  int t1w = strlen(th1) * 12;
  gfx->setCursor(SCR_W/2 - t1w/2, STATUS_H + 28);
  gfx->print(th1);

  gfx->setTextColor(C_GOLD_DIM);
  gfx->setTextSize(1);
  const char* en1 = "YOUR MERIT LUCKY NUMBER";
  int e1w = strlen(en1) * 6;
  gfx->setCursor(SCR_W/2 - e1w/2, STATUS_H + 58);
  gfx->print(en1);

  // ── Lucky number HERO ────────────────────────────────────────────────────
  char lnBuf[8];
  snprintf(lnBuf, 8, "%d", luckyNumber);
  gfx->setTextColor(C_GOLD);
  gfx->setTextSize(8);    // ~48px per char at size 8
  int lnw = strlen(lnBuf) * 48;
  gfx->setCursor(SCR_W/2 - lnw/2, SCR_H/2 - 60);
  gfx->print(lnBuf);

  // Glow rings around number
  int cx = SCR_W/2, cy = SCR_H/2 - 25;
  for (int r = 3; r > 0; r--) {
    uint8_t alpha = 20 * r;
    gfx->drawCircle(cx, cy, 60 + r*8,
                    gfx->color565(alpha, alpha/2, 0));
  }

  // ── Blessing text ─────────────────────────────────────────────────────────
  gfx->setTextColor(C_GOLD_DIM);
  gfx->setTextSize(1);
  const char* bless1 = "Compare with the temple fortune sticks";
  int b1w = strlen(bless1) * 6;
  gfx->setCursor(SCR_W/2 - b1w/2, SCR_H/2 + 44);
  gfx->print(bless1);

  const char* bless2 = "May blessings be upon you";
  int b2w = strlen(bless2) * 6;
  gfx->setCursor(SCR_W/2 - b2w/2, SCR_H/2 + 58);
  gfx->print(bless2);

  // ── Sacred water banner (if activated) ──────────────────────────────────
  if (sacredWater) {
    int bY = SCR_H - 100;
    gfx->fillRect(40, bY, SCR_W - 80, 36,
                  gfx->color565(10, 30, 60));
    _drawRoundRect(40, bY, SCR_W - 80, 36, 6,
                   gfx->color565(79, 195, 247));
    gfx->setTextColor(C_WHITE);
    gfx->setTextSize(1);
    const char* wBanner = "Sacred water blessing activated  |  num man mongkol thuuk phrom";
    int wbw = strlen(wBanner) * 6;
    gfx->setCursor(SCR_W/2 - wbw/2, bY + 12);
    gfx->print(wBanner);
  }

  // ── Thank you footer (gold bar) ──────────────────────────────────────────
  int footY = SCR_H - (sacredWater ? 54 : 54);
  gfx->fillRect(0, footY, SCR_W, 54, C_DARKGOLD);
  gfx->drawRect(0, footY, SCR_W, 54, C_GOLD);

  gfx->setTextColor(C_BLACK);
  gfx->setTextSize(2);
  const char* thanks = "Thank you for your donation";
  int tkw = strlen(thanks) * 12;
  gfx->setCursor(SCR_W/2 - tkw/2, footY + 8);
  gfx->print(thanks);

  gfx->setTextSize(1);
  const char* subThanks = "Good deeds bring good returns";
  int stw = strlen(subThanks) * 6;
  gfx->setCursor(SCR_W/2 - stw/2, footY + 32);
  gfx->print(subThanks);

  Serial.printf("[UI] Completion screen: slot=%d lucky=%d water=%d\n",
                slotIdx, luckyNumber, sacredWater);
}

// ============================================================
//  DRAW ERROR SCREEN
// ============================================================
void drawErrorScreen(String message) {
  g_idleDrawn = false;
  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_CONNECTING);  // most errors occur during connecting

  // Error icon (red circle with X)
  gfx->fillCircle(SCR_W/2, 170, 45, C_RED);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(4);
  gfx->setCursor(SCR_W/2 - 12, 152);
  gfx->print("X");

  // Message
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);

  // Word-wrap at ~50 chars or newline
  int lineY = 240;
  int start = 0;
  String msg = message;
  msg.replace("\\n", "\n");

  while (start < (int)msg.length()) {
    int nl = msg.indexOf('\n', start);
    int end = (nl == -1) ? msg.length() : nl;
    String line = msg.substring(start, min(end, start + 42));
    int lw = line.length() * 12;
    gfx->setCursor(SCR_W/2 - lw/2, lineY);
    gfx->print(line.c_str());
    lineY += 28;
    start = end + 1;
    if (start >= (int)msg.length()) break;
    if (lineY > SCR_H - 40) break;
  }

  Serial.printf("[UI] Error screen: %s\n", message.c_str());
}

// ============================================================
//  GET TOUCHED SLOT
//  Returns 0-20 (slot index) or -1 if no valid touch
//  Reads TAMC_GT911 (not TFT_eSPI)
// ============================================================
int getTouchedSlot() {
  _touch.read();
  if (!_touch.isTouched) return -1;

  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;

  // Must be within grid area
  if (ty < GRID_Y) return -1;
  if (ty > GRID_Y + GRID_ROWS * (CELL_H + GRID_PAD)) return -1;
  if (tx < GRID_X) return -1;
  if (tx > GRID_X + GRID_COLS * (CELL_W + GRID_PAD)) return -1;

  int col = (tx - GRID_X) / (CELL_W + GRID_PAD);
  int row = (ty - GRID_Y) / (CELL_H + GRID_PAD);

  if (col < 0 || col >= GRID_COLS) return -1;
  if (row < 0 || row >= GRID_ROWS) return -1;

  int slotIdx = row * GRID_COLS + col;
  if (slotIdx < 0 || slotIdx >= NUM_SLOTS) return -1;

  // Disabled slot — ignore touch
  if (!g_slots[slotIdx].enabled || !g_slots[slotIdx].configured) return -1;

  // Debounce: 80ms
  static unsigned long lastTouchMs = 0;
  if (millis() - lastTouchMs < 80) return -1;
  lastTouchMs = millis();

  Serial.printf("[UI] Touch: col=%d row=%d → slot %d (%s %dTHB)\n",
                col, row, slotIdx,
                g_slots[slotIdx].name_en, g_slots[slotIdx].price);
  return slotIdx;
}

// ============================================================
//  GET TOUCHED GIFT OPTION
//  Call during STATE_GIFT_OPTION
//  Returns: 0 = Item Only, 1 = +Sacred Water, -1 = none
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

  // Debounce
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
//  SERVICE MODE ENTRY CHECK
//  Returns true if 3 taps in top-right corner within 2 seconds
//  Call from main loop (STATE_IDLE only)
// ============================================================
bool checkServiceGesture() {
  static int    tapCount    = 0;
  static unsigned long firstTapMs = 0;

  _touch.read();
  if (!_touch.isTouched) return false;

  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;

  // Top-right zone: x > 700, y < 80
  if (tx < 700 || ty > 80) { tapCount = 0; return false; }

  static unsigned long lastSvcTap = 0;
  if (millis() - lastSvcTap < 100) return false;  // debounce
  lastSvcTap = millis();

  if (tapCount == 0) firstTapMs = millis();
  tapCount++;

  if (millis() - firstTapMs > 2000) { tapCount = 1; firstTapMs = millis(); }
  if (tapCount >= 3) { tapCount = 0; Serial.println("[UI] Service gesture!"); return true; }
  return false;
}

// ============================================================
//  IDLE ANIMATION  (brief gold flash on boot — call once)
// ============================================================
void idleAnimation() {
  // Simple fade-in effect: flash gold border around screen 2×
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

#endif // UI_H
