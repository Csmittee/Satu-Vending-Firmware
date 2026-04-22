// ============================================================
// ui.h — Satu Vending Machine Display Layer
// Board: ESP32-8048S070C — 7" 800x480 capacitive touch, TFT_eSPI
// ============================================================
// SCREENS:
//   drawIdleScreen()         — Product grid (IS the idle screen on this machine)
//   drawProductSelection()   — Highlight selected product + confirm prompt
//   drawQrScreen()           — PromptPay QR code + countdown timer
//   drawVendingScreen()      — "Dispensing..." progress bar
//   drawCompletionScreen()   — "Thank you / ขอบคุณ" success
//   drawErrorScreen()        — Error message with icon
//   getTouchedProduct()      — Returns 0-9 product index or -1
//
// LAYOUT:
//   Screen: 800 x 480px landscape
//   Product grid: 5 cols x 2 rows = 10 products
//   Cell size: 140 x 200px | Grid origin: x=30, y=60
//   Status bar: y=0-50 (device_id, WiFi, clock)
//
// FONTS:
//   TFT_eSPI built-in fonts (no external font files needed)
//   Thai text: falls back to transliteration (TFT_eSPI no Thai font by default)
//
// QR CODE:
//   Rendered from URL string using a simple 1-bit QR pixel block drawing.
//   Requires qrcode.h (qrcodegen library) — #include below.
//   If library not available, displays URL as text fallback.
// ============================================================

#ifndef UI_H
#define UI_H

#include <TFT_eSPI.h>
#include "config.h"

// ── Optional QR code library ─────────────────────────────────────────────────
// Install: https://github.com/ricmoo/QRCode  (Arduino Library Manager: "QRCode")
// If not installed, QR shows as text — comment out the #define below.
#define SATU_HAS_QR_LIB
#ifdef SATU_HAS_QR_LIB
  #include <qrcode.h>
#endif

// ── TFT object ───────────────────────────────────────────────────────────────
TFT_eSPI tft = TFT_eSPI();

// ── Colour palette (Satu brand) ───────────────────────────────────────────────
#define C_GOLD       0xFE60   // Gold/amber
#define C_DARKGOLD   0x9460   // Dark gold
#define C_RED        0xF800
#define C_GREEN      0x07E0
#define C_WHITE      0xFFFF
#define C_BLACK      0x0000
#define C_DARKGREY   0x4208
#define C_LIGHTGREY  0xC618
#define C_ORANGE     0xFD20
#define C_BGBLUE     0x000F   // Deep navy background

// ── Screen dimensions ─────────────────────────────────────────────────────────
#define SCR_W   800
#define SCR_H   480

// ── Product grid layout ───────────────────────────────────────────────────────
#define GRID_COLS   5
#define GRID_ROWS   2
#define CELL_W      140
#define CELL_H      195
#define GRID_X      25     // left margin
#define GRID_Y      58     // below status bar
#define CELL_PAD    5      // inner padding

// ── Status bar ────────────────────────────────────────────────────────────────
#define STATUS_H    55

// Product names (Thai transliteration — replace with Thai UTF-8 if font supports it)
static const char* PRODUCT_NAMES[10] = {
  "Amulet A",    // 0
  "Amulet B",    // 1
  "Amulet C",    // 2
  "Blessing",    // 3
  "Merit Card",  // 4
  "Gold Leaf",   // 5
  "Prayer Bead", // 6
  "Incense",     // 7
  "Sacred Oil",  // 8
  "Temple Coin"  // 9
};

// Product prices in THB (edit to match backend product table)
static const int PRODUCT_PRICES[10] = {
  10, 20, 50, 100, 500,
  10, 20, 50, 100, 200
};

// Product emoji/icon (simple text icons — readable on any font)
static const char* PRODUCT_ICONS[10] = {
  "**", "**", "**", "**", "**",
  "**", "**", "**", "**", "**"
};

// ── State tracking for UI ─────────────────────────────────────────────────────
static int  g_highlightedProduct = -1;   // -1 = none highlighted
static bool g_idleDrawn          = false;

// ── External: device_id from network.h (for status bar) ──────────────────────
extern String g_deviceId;

// ════════════════════════════════════════════════════════════════════════════
//  INIT
// ════════════════════════════════════════════════════════════════════════════
void initUI() {
  tft.init();
  tft.setRotation(1);   // Landscape — adjust to 3 if image is upside-down
  tft.fillScreen(C_BLACK);
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setTextSize(2);
  Serial.println("[UI] Display init OK");
}

// ════════════════════════════════════════════════════════════════════════════
//  INTERNAL HELPERS
// ════════════════════════════════════════════════════════════════════════════

static void drawStatusBar(const char* label = nullptr) {
  tft.fillRect(0, 0, SCR_W, STATUS_H, C_DARKGOLD);
  tft.setTextColor(C_WHITE, C_DARKGOLD);
  tft.setTextSize(2);

  // Left: logo/name
  tft.setCursor(10, 14);
  tft.print("SATU");

  // Center: label (optional)
  if (label) {
    tft.setCursor(SCR_W / 2 - strlen(label) * 6, 14);
    tft.print(label);
  }

  // Right: device ID
  tft.setCursor(SCR_W - 130, 14);
  tft.setTextSize(1);
  tft.print(g_deviceId.isEmpty() ? "Connecting..." : g_deviceId.c_str());

  // WiFi indicator
  tft.setCursor(SCR_W - 130, 30);
  tft.print(WiFi.status() == WL_CONNECTED ? "WiFi OK" : "No WiFi");
}

static void drawCellBorder(int col, int row, uint16_t color, int thickness = 2) {
  int x = GRID_X + col * CELL_W;
  int y = GRID_Y + row * CELL_H;
  for (int t = 0; t < thickness; t++) {
    tft.drawRect(x + t, y + t, CELL_W - t * 2, CELL_H - t * 2, color);
  }
}

static void drawProductCell(int index, bool highlighted = false, bool disabled = false) {
  int col = index % GRID_COLS;
  int row = index / GRID_COLS;
  int x   = GRID_X + col * CELL_W;
  int y   = GRID_Y + row * CELL_H;

  uint16_t bgColor  = highlighted ? C_GOLD      : C_BGBLUE;
  uint16_t txtColor = highlighted ? C_BLACK      : C_WHITE;
  uint16_t bdrColor = disabled    ? C_DARKGREY   : (highlighted ? C_WHITE : C_GOLD);

  tft.fillRect(x + 2, y + 2, CELL_W - 4, CELL_H - 4, bgColor);
  drawCellBorder(col, row, bdrColor, highlighted ? 3 : 1);

  if (disabled) {
    tft.setTextColor(C_DARKGREY, bgColor);
    tft.setTextSize(1);
    tft.setCursor(x + 10, y + CELL_H / 2);
    tft.print("UNAVAIL");
    return;
  }

  // Product number (large)
  tft.setTextColor(highlighted ? C_BLACK : C_GOLD, bgColor);
  tft.setTextSize(3);
  tft.setCursor(x + CELL_PAD + 8, y + 15);
  tft.print(index + 1);

  // Product name
  tft.setTextColor(txtColor, bgColor);
  tft.setTextSize(1);
  tft.setCursor(x + CELL_PAD + 5, y + 65);
  // Wrap name if needed
  String name = PRODUCT_NAMES[index];
  tft.print(name.substring(0, 12).c_str());

  // Price (prominent)
  tft.setTextColor(highlighted ? C_RED : C_ORANGE, bgColor);
  tft.setTextSize(2);
  tft.setCursor(x + CELL_PAD + 5, y + 140);
  tft.print(PRODUCT_PRICES[index]);
  tft.print(" THB");
}

// ════════════════════════════════════════════════════════════════════════════
//  DRAW IDLE SCREEN
//  On this machine the idle screen IS the product grid.
//  Called once on state entry; not redrawn every loop tick to avoid flicker.
//  g_idleDrawn flag prevents redundant redraws.
// ════════════════════════════════════════════════════════════════════════════
void drawIdleScreen() {
  if (g_idleDrawn) return;

  tft.fillScreen(C_BLACK);
  drawStatusBar("Select Donation");

  // Draw all 10 product cells
  for (int i = 0; i < 10; i++) {
    drawProductCell(i, false, false);  // disabled state handled per-lane in caller
  }

  // Footer prompt
  tft.setTextColor(C_LIGHTGREY, C_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, SCR_H - 20);
  tft.print("Touch a product to begin your merit donation");

  g_idleDrawn     = true;
  g_highlightedProduct = -1;
  Serial.println("[UI] Idle screen drawn");
}

// ════════════════════════════════════════════════════════════════════════════
//  DRAW PRODUCT SELECTION
//  Highlights the selected product + shows confirmation prompt
// ════════════════════════════════════════════════════════════════════════════
void drawProductSelection() {
  // Reset idle drawn flag so next drawIdleScreen() redraws cleanly
  g_idleDrawn = false;

  tft.fillScreen(C_BLACK);
  drawStatusBar("Confirm");

  for (int i = 0; i < 10; i++) {
    drawProductCell(i, i == g_highlightedProduct, false);
  }

  // Confirmation prompt at bottom
  tft.fillRect(0, SCR_H - 60, SCR_W, 60, C_GOLD);
  tft.setTextColor(C_BLACK, C_GOLD);
  tft.setTextSize(2);
  tft.setCursor(20, SCR_H - 45);
  tft.print("Tap again to confirm | Wait 3s to cancel");
}

// ════════════════════════════════════════════════════════════════════════════
//  DRAW QR SCREEN
//  Shows PromptPay QR code + countdown timer
//  qrData: URL or payload string
//  amount: THB (displayed to user)
// ════════════════════════════════════════════════════════════════════════════
void drawQrScreen(String qrData, int amount) {
  g_idleDrawn = false;
  tft.fillScreen(C_BLACK);
  drawStatusBar("Scan to Pay");

  // Amount display (large, prominent)
  tft.setTextColor(C_GOLD, C_BLACK);
  tft.setTextSize(4);
  tft.setCursor(30, 75);
  tft.print(amount);
  tft.print(" THB");

  // Instructions
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setTextSize(2);
  tft.setCursor(30, 140);
  tft.print("Scan PromptPay QR code");
  tft.setCursor(30, 165);
  tft.print("with your banking app");

#ifdef SATU_HAS_QR_LIB
  // ── Generate + draw QR code (centre right of screen) ──────────────────
  QRCode qrcode;
  uint8_t qrData_buf[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrData_buf, 3, ECC_LOW, qrData.c_str());

  int qrSize   = qrcode.size;
  int scale    = min(220 / qrSize, 8);   // auto-scale, max 8px per module
  int qrPixels = qrSize * scale;
  int qrX      = SCR_W - qrPixels - 30;
  int qrY      = STATUS_H + 20;

  // White background for QR (quiet zone)
  tft.fillRect(qrX - 8, qrY - 8, qrPixels + 16, qrPixels + 16, C_WHITE);

  for (int y = 0; y < qrSize; y++) {
    for (int x = 0; x < qrSize; x++) {
      uint16_t color = qrcode_getModule(&qrcode, x, y) ? C_BLACK : C_WHITE;
      tft.fillRect(qrX + x * scale, qrY + y * scale, scale, scale, color);
    }
  }
  Serial.printf("[UI] QR drawn: %dx%d modules at scale %d\n", qrSize, qrSize, scale);

#else
  // ── Fallback: show URL as text ──────────────────────────────────────────
  tft.setTextColor(C_LIGHTGREY, C_BLACK);
  tft.setTextSize(1);
  int ty = 200;
  // Print URL in 30-char chunks (screen wrapping)
  for (int s = 0; s < qrData.length(); s += 38) {
    tft.setCursor(30, ty);
    tft.print(qrData.substring(s, s + 38).c_str());
    ty += 14;
    if (ty > SCR_H - 30) break;
  }
#endif

  // Countdown bar (static — updated separately if needed)
  tft.fillRect(0, SCR_H - 30, SCR_W, 30, C_DARKGREY);
  tft.setTextColor(C_WHITE, C_DARKGREY);
  tft.setTextSize(1);
  tft.setCursor(10, SCR_H - 20);
  tft.print("Payment window: 2 minutes");

  Serial.println("[UI] QR screen drawn");
}

// ════════════════════════════════════════════════════════════════════════════
//  DRAW VENDING SCREEN
//  Shows animated "Dispensing..." message while relay fires
// ════════════════════════════════════════════════════════════════════════════
void drawVendingScreen(int product) {
  g_idleDrawn = false;
  tft.fillScreen(C_BGBLUE);
  drawStatusBar("Dispensing");

  // Animated icon (static in this version — no LVGL)
  tft.setTextColor(C_GOLD, C_BGBLUE);
  tft.setTextSize(5);
  tft.setCursor(80, 130);
  tft.print(">> Vending <<");

  tft.setTextColor(C_WHITE, C_BGBLUE);
  tft.setTextSize(3);
  tft.setCursor(80, 230);
  if (product >= 0 && product < 10) {
    tft.print(PRODUCT_NAMES[product]);
  }

  tft.setTextColor(C_LIGHTGREY, C_BGBLUE);
  tft.setTextSize(2);
  tft.setCursor(80, 310);
  tft.print("Please wait...");

  // Progress bar (static — will fill in actual progress later)
  tft.drawRect(80, 380, 640, 30, C_WHITE);
  tft.fillRect(82, 382, 400, 26, C_GREEN);

  Serial.printf("[UI] Vending screen: lane %d\n", product);
}

// ════════════════════════════════════════════════════════════════════════════
//  DRAW COMPLETION SCREEN
//  Shows for 4 seconds (timed in state machine)
// ════════════════════════════════════════════════════════════════════════════
void drawCompletionScreen() {
  g_idleDrawn = false;
  tft.fillScreen(C_BLACK);
  drawStatusBar();

  // Big checkmark area
  tft.fillCircle(SCR_W / 2, 220, 90, C_GREEN);
  tft.setTextColor(C_WHITE, C_GREEN);
  tft.setTextSize(8);
  tft.setCursor(SCR_W / 2 - 28, 185);
  tft.print("v");   // Approximation of checkmark

  tft.setTextColor(C_GOLD, C_BLACK);
  tft.setTextSize(3);
  tft.setCursor(SCR_W / 2 - 150, 340);
  tft.print("Thank you / Kop Khun");

  tft.setTextColor(C_LIGHTGREY, C_BLACK);
  tft.setTextSize(2);
  tft.setCursor(SCR_W / 2 - 160, 400);
  tft.print("May your merit bring blessings");

  Serial.println("[UI] Completion screen drawn");
}

// ════════════════════════════════════════════════════════════════════════════
//  DRAW ERROR SCREEN
//  message: up to ~60 chars (will wrap)
// ════════════════════════════════════════════════════════════════════════════
void drawErrorScreen(String message) {
  g_idleDrawn = false;
  tft.fillScreen(C_BLACK);
  tft.fillRect(0, 0, SCR_W, STATUS_H, 0x8000);   // dark red status bar

  tft.setTextColor(C_WHITE, 0x8000);
  tft.setTextSize(2);
  tft.setCursor(10, 14);
  tft.print("SATU — System Notice");

  // Error icon
  tft.fillCircle(SCR_W / 2, 180, 70, C_RED);
  tft.setTextColor(C_WHITE, C_RED);
  tft.setTextSize(6);
  tft.setCursor(SCR_W / 2 - 18, 152);
  tft.print("!");

  // Message (wrap at 36 chars)
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setTextSize(2);
  int lineY = 290;
  int lineStart = 0;
  while (lineStart < (int)message.length()) {
    int lineEnd = min(lineStart + 36, (int)message.length());
    // Break at newline if present
    int nl = message.indexOf('\n', lineStart);
    if (nl >= lineStart && nl < lineEnd) lineEnd = nl;
    tft.setCursor(30, lineY);
    tft.print(message.substring(lineStart, lineEnd).c_str());
    lineStart = lineEnd + (message[lineEnd] == '\n' ? 1 : 0);
    lineY += 28;
    if (lineY > SCR_H - 30) break;
  }

  Serial.printf("[UI] Error screen: %s\n", message.c_str());
}

// ════════════════════════════════════════════════════════════════════════════
//  GET TOUCHED PRODUCT
//  Returns 0-9 if a product cell was touched, -1 otherwise.
//  Uses TFT_eSPI built-in touch (XPT2046 or GT911 capacitive).
//
//  NOTE: ESP32-8048S070C uses GT911 capacitive touch.
//  TFT_eSPI must be configured for GT911 in User_Setup.h.
//  If using a resistive panel, calibration data must be set.
// ════════════════════════════════════════════════════════════════════════════
int getTouchedProduct() {
  uint16_t tx, ty;
  bool touched = tft.getTouch(&tx, &ty);

  if (!touched) return -1;

  // Map raw touch to grid
  // Check if within product grid area
  if (ty < GRID_Y || ty > GRID_Y + GRID_ROWS * CELL_H) return -1;
  if (tx < GRID_X || tx > GRID_X + GRID_COLS * CELL_W) return -1;

  int col = (tx - GRID_X) / CELL_W;
  int row = (ty - GRID_Y) / CELL_H;

  if (col < 0 || col >= GRID_COLS) return -1;
  if (row < 0 || row >= GRID_ROWS) return -1;

  int productIndex = row * GRID_COLS + col;
  if (productIndex < 0 || productIndex > 9) return -1;

  // Debounce: require 50ms between touches
  static unsigned long lastTouchMs = 0;
  if (millis() - lastTouchMs < 50) return -1;
  lastTouchMs = millis();

  // Highlight touched cell
  if (g_highlightedProduct != productIndex) {
    // Un-highlight old
    if (g_highlightedProduct >= 0) drawProductCell(g_highlightedProduct, false);
    g_highlightedProduct = productIndex;
    drawProductCell(productIndex, true);
  }

  Serial.printf("[UI] Touch: col=%d row=%d → product %d\n", col, row, productIndex);
  return productIndex;
}

#endif // UI_H
