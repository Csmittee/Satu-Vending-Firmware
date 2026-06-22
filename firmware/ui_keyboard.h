#ifndef UI_KEYBOARD_H
#define UI_KEYBOARD_H

// ui_keyboard.h — Version R1 — 2026-06-22
// Split from ui.h R5. PIN numpad and WiFi setup keyboard.
// Depends on: gfx, _touch, colors, SCR_W/H, STATUS_H — all defined in ui.h above this include.

// ============================================================
//  NUMPAD HELPER  (used by PIN screens and satu_vending.ino)
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
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(10, 34);
  gfx->print("SATU");
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  gfx->setCursor(SCR_W/2 - 58, 25);
  gfx->print("Enter PIN");
  gfx->setFont(NULL); gfx->setTextSize(1);

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
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  const char* title = "Service PIN";
  gfx->setCursor(boxX + boxW/2 - strlen(title)*9/2, boxY + 25);
  gfx->print(title);
  gfx->setFont(NULL); gfx->setTextSize(1);

  int npX = boxX + 20;
  int npY = boxY + 50;
  // PIN display
  gfx->fillRect(npX, npY - 4, boxW - 40, 24, gfx->color565(20,16,30));
  gfx->drawRect(npX, npY - 4, boxW - 40, 24, C_GOLD);
  _drawNumpad(npX, npY + 28);
  return "";  // caller drives the loop via getTouchedNumpad
}

// ============================================================
//  WIFI SETUP KEYBOARD  (R5 — blocking, calls saveWifiAndReboot)
//  Shown on first boot when NVS has no WiFi credentials.
//  Owner enters SSID + password via touchscreen keyboard.
//  On CONNECT tap: credentials saved to NVS, device restarts.
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
// CAPS: x=42  w=80    -> [42,122)
// '.': x=126  w=48    -> [126,174)   '@': x=178 -> [178,226)
// '-': x=230  w=48    -> [230,278)   '_': x=282 -> [282,330)
// SPACE: x=334 w=186  -> [334,520)
// DEL: x=524 w=80    -> [524,604)
// CONNECT: x=608 w=150 -> [608,758)
#define _WKB4_CAPS_X    42
#define _WKB4_CAPS_W    80
#define _WKB4_SYM_X    126   // first symbol key (. @ - _), each 48px wide with 4px gap
#define _WKB4_SYM_W     48
#define _WKB4_SPC_X    334
#define _WKB4_SPC_W    186
#define _WKB4_DEL_X    524
#define _WKB4_DEL_W     80
#define _WKB4_CON_X    608
#define _WKB4_CON_W    150

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
  // Symbol keys: . @ - _
  static const char _wkbSyms[4] = { '.', '@', '-', '_' };
  for (int s = 0; s < 4; s++) {
    int sx = _WKB4_SYM_X + s * (_WKB4_SYM_W + _WKB_GAP);
    _fillRoundRect(sx, ry4, _WKB4_SYM_W, _WKB_KEY_H, 6, gfx->color565(15, 25, 50));
    _drawRoundRect(sx, ry4, _WKB4_SYM_W, _WKB_KEY_H, 6, C_GOLD);
    char sl[2] = { _wkbSyms[s], 0 };
    gfx->setTextColor(C_WHITE); gfx->setTextSize(2);
    gfx->setCursor(sx + _WKB4_SYM_W/2 - 6, ry4 + _WKB_KEY_H/2 - 8);
    gfx->print(sl);
  }
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
    { // symbol keys . @ - _
      static const char _wkbSyms2[4] = { '.', '@', '-', '_' };
      for (int s = 0; s < 4; s++) {
        int sx = _WKB4_SYM_X + s * (_WKB4_SYM_W + _WKB_GAP);
        if (tx >= sx && tx < sx + _WKB4_SYM_W) { _wkbLastMs = millis(); return (int)(unsigned char)_wkbSyms2[s]; }
      }
    }
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
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(10, 34);
  gfx->print("SATU");
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(SCR_W/2 - strlen("WiFi Setup")*9/2, 25);
  gfx->print("WiFi Setup");
  gfx->setFont(NULL); gfx->setTextSize(1);

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

#endif // UI_KEYBOARD_H
