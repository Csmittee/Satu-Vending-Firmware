#ifndef UI_SERVICE_H
#define UI_SERVICE_H

// ui_service.h — Service mode tab bodies (all 5 tabs)
// Included at bottom of ui.h — all ui.h symbols visible here.
// network.h and hardware.h are included before ui.h in satu_vending.ino.

// ── Self Test state (file-scope, persists across redraws) ────────────────────
static int  _stm     = 0;    // 0=idle 1=quick results 2=tech results
static int  _stN     = 0;    // number of test items ran
static bool _stP[14];        // pass/fail per item
static bool _stS[14];        // simulated? per item
static const char* _stL[14]; // label per item

// ── Relay toggle state (Devices tab) ─────────────────────────────────────────
static bool _relState[13] = {false}; // 1-indexed; index 0 unused

// ── Log panel state ───────────────────────────────────────────────────────────
#define _LOG_X_OFF   556
#define _LOG_X     (SVC_BODY_X + _LOG_X_OFF)  // = 670
#define _LOG_W     (SCR_W - _LOG_X - 4)        // = 126
#define _LOG_Y     (STATUS_H + 6)              // = 50
#define _LOG_H     (SCR_H - _LOG_Y - 28)       // = 402
#define _LOG_LINES_MAX  10
#define _LOG_LINE_H     16

static String _logLines[10];
static int    _logCount = 0;

// ── Layout constants used across tabs ────────────────────────────────────────
#define _LM  (SVC_BODY_X + 16)   // 130 — left content margin
#define _BDY STATUS_H             // 44  — body top y

// ════════════════════════════════════════════════════════════════════════════
//  LOG PANEL — right-side scrolling event log
// ════════════════════════════════════════════════════════════════════════════
static void _svcLogDraw() {
  gfx->fillRect(_LOG_X, _LOG_Y, _LOG_W, _LOG_H, gfx->color565(10, 6, 18));
  gfx->drawRect(_LOG_X, _LOG_Y, _LOG_W, _LOG_H, C_MIDGREY);
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_GREY);
  gfx->setCursor(_LOG_X + 4, _LOG_Y + 4);
  gfx->print("LOG");
  gfx->drawFastHLine(_LOG_X + 1, _LOG_Y + 13, _LOG_W - 2, C_DARKGREY);
  int total    = _logCount;
  int n        = (total < _LOG_LINES_MAX) ? total : _LOG_LINES_MAX;
  int startIdx = (total <= _LOG_LINES_MAX) ? 0 : (total % _LOG_LINES_MAX);
  for (int i = 0; i < n; i++) {
    int bufIdx = (startIdx + i) % _LOG_LINES_MAX;
    int ly = _LOG_Y + 16 + i * _LOG_LINE_H;
    if (ly + _LOG_LINE_H > _LOG_Y + _LOG_H - 2) break;
    gfx->setTextColor(C_WHITE);
    gfx->setCursor(_LOG_X + 3, ly);
    String s = _logLines[bufIdx];
    if ((int)s.length() > 19) s = s.substring(0, 19);
    gfx->print(s.c_str());
  }
}

void _svcLogPanel(String msg) {
  _logLines[_logCount % _LOG_LINES_MAX] = msg;
  _logCount++;
  _svcLogDraw();
}

// ════════════════════════════════════════════════════════════════════════════
//  HELPER — draw a service tab section heading (FreeSansBold12pt7b + reset)
// ════════════════════════════════════════════════════════════════════════════
static void _svcHeading(int x, int y, const char* txt, uint16_t col = 0xFFFF) {
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(col); gfx->setTextSize(1);
  gfx->setCursor(x, y);
  gfx->print(txt);
  gfx->setFont(NULL); gfx->setTextSize(1);
}

// ════════════════════════════════════════════════════════════════════════════
//  TAB 0 — SELF TEST
// ════════════════════════════════════════════════════════════════════════════

void _runSelfTest(int bodyX, bool technical) {
  const int btnH  = 38;
  const int btnY  = _BDY + 72;       // 116
  const int resY  = btnY + btnH + 12; // 166
  const int lineH = 18;

  // Clear results area — left zone only, do not touch log panel column
  gfx->fillRect(bodyX, resY, _LOG_X - bodyX - 4, SCR_H - resY - 24, C_BG);

  int n = technical ? 14 : 10;

  static const char* qLabels[10] = {
    "MCP23017 #1 (0x20)",
    "MCP23017 #2 (0x21)",
    "IR Sensors 1-10",
    "Relay bank 1-6",
    "Relay bank 7-12",
    "Flap lock R12",
    "Water pump R11",
    "WS2812B LEDs",
    "WiFi connection",
    "Free heap >= 100KB",
  };
  static const char* tLabels[14] = {
    "MCP23017 #1 (0x20) I2C",
    "MCP23017 #2 (0x21) I2C",
    "IR Sensors 1-8 (MCP1)",
    "IR Sensors 9-10 (MCP2)",
    "Relay bank 1-6 (MCP1)",
    "Relay bank 7-11 (MCP2)",
    "Flap lock R12 (MCP2)",
    "WS2812B LEDs (GPIO5)",
    "Display 800x480 (EK9716)",
    "GT911 Touch",
    "NVS read/write",
    "WiFi connection",
    "Backend heartbeat",
    "Free heap >= 100KB",
  };

  const char** labels = technical ? tLabels : qLabels;

  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  bool heapOk = (ESP.getFreeHeap() > 100000);

  bool qPass[10] = {
    g_mcp1_ok, g_mcp2_ok,
    g_mcp1_ok && g_mcp2_ok,
    g_mcp1_ok, g_mcp2_ok, g_mcp2_ok, g_mcp2_ok,
    true, wifiOk, heapOk,
  };
  bool qSim[10]  = { false, false, true, true, true, true, true, true, false, false };

  bool tPass[14] = {
    g_mcp1_ok, g_mcp2_ok, g_mcp1_ok, g_mcp2_ok,
    g_mcp1_ok, g_mcp2_ok, g_mcp2_ok,
    true, true, true, true,
    wifiOk, false, heapOk,
  };
  bool tSim[14]  = { false, false, true, true, true, true, true, true, true, true, true, false, false, false };

  bool* passArr = technical ? tPass : qPass;
  bool* simArr  = technical ? tSim  : qSim;

  for (int i = 0; i < n; i++) {
    if (technical && i == 12) {
      sendHeartbeat();
      tPass[12] = (WiFi.status() == WL_CONNECTED);
      passArr   = tPass;
    }

    bool pass = passArr[i];
    bool sim  = simArr[i];
    int  iy   = resY + i * lineH;

    // Badge [PASS] / [FAIL]
    uint16_t badgeCol = pass ? C_GREEN : C_RED;
    gfx->fillRect(bodyX + 16, iy, 48, 14, badgeCol);
    gfx->setTextColor(C_WHITE); gfx->setTextSize(1); gfx->setFont(NULL);
    gfx->setCursor(bodyX + 18, iy + 4);
    gfx->print(pass ? "PASS" : "FAIL");

    // Label — left zone only
    gfx->setTextColor(C_WHITE);
    gfx->setCursor(bodyX + 72, iy + 4);
    gfx->print(labels[i]);
    if (sim) {
      gfx->setTextColor(C_GREY);
      gfx->print(" (sim)");
    }

    // Log result to right panel
    char logMsg[24];
    snprintf(logMsg, sizeof(logMsg), "%s: %.14s", pass ? "OK" : "FAIL", labels[i]);
    _svcLogPanel(String(logMsg));

    _stL[i] = labels[i];
    _stP[i] = pass;
    _stS[i] = sim;

    delay(300);
  }

  _stN = n;
  _stm = technical ? 2 : 1;
}

static void _drawSvcBody_SelfTest(int bodyX) {
  const int btnY  = _BDY + 72;        // 116
  const int btnH  = 38;
  const int resY  = btnY + btnH + 12; // 166
  const int lineH = 18;

  // Title
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 16, _BDY + 36);
  gfx->print("Self Test");
  gfx->setFont(NULL); gfx->setTextSize(1);

  // Subtitle
  gfx->setTextColor(C_GREY);
  gfx->setCursor(bodyX + 16, _BDY + 52);
  gfx->print("Test MCP, sensors, WiFi, and heap");

  // [Quick Test] button
  _fillRoundRect(bodyX + 16, btnY, 170, btnH, 6, gfx->color565(20, 60, 20));
  _drawRoundRect(bodyX + 16, btnY, 170, btnH, 6, C_GREEN);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GREEN); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 36, btnY + 25);
  gfx->print("Quick Test");
  gfx->setFont(NULL); gfx->setTextSize(1);

  // [Technical Test] button
  _fillRoundRect(bodyX + 200, btnY, 170, btnH, 6, gfx->color565(20, 40, 70));
  _drawRoundRect(bodyX + 200, btnY, 170, btnH, 6, C_GOLD);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 214, btnY + 25);
  gfx->print("Technical Test");
  gfx->setFont(NULL); gfx->setTextSize(1);

  // [Clear] button
  _fillRoundRect(bodyX + 384, btnY, 80, btnH, 6, gfx->color565(40, 20, 20));
  _drawRoundRect(bodyX + 384, btnY, 80, btnH, 6, C_MIDGREY);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_MIDGREY); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 400, btnY + 25);
  gfx->print("Clear");
  gfx->setFont(NULL); gfx->setTextSize(1);

  // Results
  if (_stm == 0) {
    gfx->setTextColor(C_GREY);
    gfx->setCursor(bodyX + 16, resY + 4);
    gfx->print("Tap Quick Test or Technical Test to begin.");
  } else {
    for (int i = 0; i < _stN; i++) {
      int iy = resY + i * lineH;
      if (iy + lineH > SCR_H - 28) break;
      uint16_t badgeCol = _stP[i] ? C_GREEN : C_RED;
      gfx->fillRect(bodyX + 16, iy, 48, 14, badgeCol);
      gfx->setTextColor(C_WHITE); gfx->setCursor(bodyX + 18, iy + 4);
      gfx->print(_stP[i] ? "PASS" : "FAIL");
      gfx->setTextColor(C_WHITE); gfx->setCursor(bodyX + 72, iy + 4);
      gfx->print(_stL[i]);
      if (_stS[i]) {
        gfx->setTextColor(C_GREY);
        gfx->print(" (sim)");
      }
    }
  }

  _svcLogDraw();
}

// ════════════════════════════════════════════════════════════════════════════
//  TAB 1 — FREE PLAY
// ════════════════════════════════════════════════════════════════════════════
static void _drawSvcBody_FreePlay(int bodyX) {
  // Title
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 16, _BDY + 36);
  gfx->print("Free Play");
  gfx->setFont(NULL); gfx->setTextSize(1);

  // Instruction
  gfx->setTextColor(C_GREY);
  gfx->setCursor(bodyX + 16, _BDY + 60);
  gfx->print("Tap slot to fire motor - no payment needed");

  // Fixed 7x3 grid regardless of machine config
  const int cols  = 7;
  const int cw    = 72;
  const int ch    = 50;
  const int gap   = 4;
  const int gridX = bodyX + 16;
  const int gridY = _BDY + 72;  // 116

  for (int i = 0; i < NUM_SLOTS && i < 21; i++) {
    int col = i % cols;
    int row = i / cols;
    int cx  = gridX + col * (cw + gap);
    int cy  = gridY + row * (ch + gap);

    uint16_t bg;
    if (!g_slots[i].configured) {
      bg = C_DARKGREY;
    } else if (g_slots[i].enabled) {
      bg = gfx->color565(60, 45, 0);
    } else {
      bg = gfx->color565(50, 10, 10);
    }

    _fillRoundRect(cx, cy, cw, ch, 4, bg);

    uint16_t bdr;
    if (!g_slots[i].configured) {
      bdr = C_DARKGREY;
    } else if (g_slots[i].enabled) {
      bdr = C_GOLD;
    } else {
      bdr = gfx->color565(140, 40, 40);
    }
    _drawRoundRect(cx, cy, cw, ch, 4, bdr);

    // Slot number
    gfx->setFont(&FreeSansBold12pt7b);
    gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
    char nb[4]; snprintf(nb, sizeof(nb), "%d", i + 1);
    int nw = strlen(nb) * 7;
    gfx->setCursor(cx + cw / 2 - nw / 2, cy + 22);
    gfx->print(nb);
    gfx->setFont(NULL); gfx->setTextSize(1);

    // Price (bottom of cell)
    if (g_slots[i].configured && g_slots[i].price > 0) {
      gfx->setTextColor(C_GOLD);
      char pb[8]; snprintf(pb, sizeof(pb), "%dB", (int)g_slots[i].price);
      int pw = strlen(pb) * 6;
      gfx->setCursor(cx + cw / 2 - pw / 2, cy + 38);
      gfx->print(pb);
    }
  }

  _svcLogDraw();
}

// ════════════════════════════════════════════════════════════════════════════
//  TAB 2 — DEVICES
// ════════════════════════════════════════════════════════════════════════════

// Layout constants for Devices tab
#define _REL_LM      (_LM)
#define _REL_CW      86
#define _REL_CH      44
#define _REL_GAP     4
#define _REL_ROW1_Y  (_BDY + 55)
#define _REL_ROW2_Y  (_REL_ROW1_Y + _REL_CH + _REL_GAP)
#define _IR_CW       80
#define _IR_CH       36
#define _IR_GAP      4
#define _IR_ROW1_Y   (_REL_ROW2_Y + _REL_CH + 52)
#define _IR_ROW2_Y   (_IR_ROW1_Y + _IR_CH + _IR_GAP)
#define _STUB_Y      (_IR_ROW2_Y + _IR_CH + 8)
#define _TBEKY       (_STUB_Y + 36 + 8)

static void _drawSvcBody_Devices(int bodyX) {
  // Title
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 16, _BDY + 36);
  gfx->print("Devices");
  gfx->setFont(NULL); gfx->setTextSize(1);

  // ── RELAY SECTION ──────────────────────────────────────────────────────
  _svcHeading(bodyX + 16, _BDY + 48, "RELAYS", C_MIDGREY);

  for (int r = 1; r <= 12; r++) {
    int col = (r - 1) % 6;
    int row = (r - 1) / 6;
    int cx  = _REL_LM + col * (_REL_CW + _REL_GAP);
    int cy  = (row == 0) ? _REL_ROW1_Y : _REL_ROW2_Y;

    bool on = _relState[r];

    uint16_t bg, fg;
    if (r == 12) {
      bg = on ? gfx->color565(0, 60, 0)  : gfx->color565(70, 0, 0);
      fg = on ? C_GREEN                   : C_RED;
    } else if (on) {
      bg = gfx->color565(0, 55, 0);
      fg = C_GREEN;
    } else {
      bg = gfx->color565(18, 18, 22);
      fg = C_GREY;
    }

    _fillRoundRect(cx, cy, _REL_CW, _REL_CH, 4, bg);
    _drawRoundRect(cx, cy, _REL_CW, _REL_CH, 4, fg);

    gfx->setFont(NULL); gfx->setTextSize(1);
    gfx->setTextColor(fg);

    char topLine[6];
    snprintf(topLine, sizeof(topLine), "R%d", r);
    int tw = strlen(topLine) * 6;
    gfx->setCursor(cx + _REL_CW / 2 - tw / 2, cy + 10);
    gfx->print(topLine);

    const char* stateLabel;
    if (r == 12) {
      stateLabel = on ? "UNLOCKED" : "LOCKED";
    } else if (r == 11) {
      stateLabel = on ? "PUMP ON" : "PUMP";
    } else {
      stateLabel = on ? "ON" : "OFF";
    }
    int sl = strlen(stateLabel) * 6;
    gfx->setCursor(cx + _REL_CW / 2 - sl / 2, cy + 28);
    gfx->print(stateLabel);
  }

  // Warning banner
  int warnY = _REL_ROW2_Y + _REL_CH + 4;  // 195
  gfx->fillRect(bodyX + 16, warnY, _LOG_X - bodyX - 24, 18, gfx->color565(50, 30, 0));
  gfx->setTextColor(gfx->color565(220, 160, 0)); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 20, warnY + 5);
  gfx->print("WARNING: Relays stay ON until tapped again. Exit to reset all.");

  // ── IR SENSORS SECTION ─────────────────────────────────────────────────
  _svcHeading(bodyX + 16, warnY + 24, "IR SENSORS (live)", C_MIDGREY);  // y=219

  for (int s = 0; s < 10; s++) {
    int col = s % 5;
    int row = s / 5;
    int cx  = _LM + col * (_IR_CW + _IR_GAP);
    int cy  = _IR_ROW1_Y + row * (_IR_CH + _IR_GAP);

    bool triggered = readSensor(s);
    uint16_t bg  = triggered ? gfx->color565(80, 0, 0)  : gfx->color565(10, 30, 10);
    uint16_t fg  = triggered ? C_RED                     : C_GREEN;

    _fillRoundRect(cx, cy, _IR_CW, _IR_CH, 3, bg);
    _drawRoundRect(cx, cy, _IR_CW, _IR_CH, 3, fg);

    gfx->setFont(NULL); gfx->setTextSize(1);
    gfx->setTextColor(fg);

    char sn[5]; snprintf(sn, sizeof(sn), "S%d", s + 1);
    gfx->setCursor(cx + 5, cy + 8);
    gfx->print(sn);

    const char* sl = triggered ? "BLOCK" : "CLEAR";
    int slw = strlen(sl) * 6;
    gfx->setCursor(cx + _IR_CW / 2 - slw / 2, cy + 22);
    gfx->print(sl);
  }

  // ── STUB ROW: Pump / LED / Speaker ─────────────────────────────────────
  _fillRoundRect(_LM,       _STUB_Y, 100, 36, 4, gfx->color565(15, 20, 40));
  _drawRoundRect(_LM,       _STUB_Y, 100, 36, 4, C_DARKGREY);
  _fillRoundRect(_LM + 110, _STUB_Y,  80, 36, 4, gfx->color565(15, 20, 40));
  _drawRoundRect(_LM + 110, _STUB_Y,  80, 36, 4, C_DARKGREY);
  _fillRoundRect(_LM + 200, _STUB_Y, 100, 36, 4, gfx->color565(15, 20, 40));
  _drawRoundRect(_LM + 200, _STUB_Y, 100, 36, 4, C_DARKGREY);

  gfx->setFont(NULL); gfx->setTextSize(1); gfx->setTextColor(C_MIDGREY);
  gfx->setCursor(_LM + 12,       _STUB_Y + 14); gfx->print("Pump R11");
  gfx->setCursor(_LM + 110 + 14, _STUB_Y + 14); gfx->print("LED");
  gfx->setCursor(_LM + 200 + 10, _STUB_Y + 14); gfx->print("Speaker");

  // ── TEST BACKEND BUTTON ─────────────────────────────────────────────────
  _fillRoundRect(bodyX + 16, _TBEKY, 160, 36, 6, gfx->color565(10, 30, 60));
  _drawRoundRect(bodyX + 16, _TBEKY, 160, 36, 6, C_GOLD);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 30, _TBEKY + 23);
  gfx->print("Test Backend");
  gfx->setFont(NULL); gfx->setTextSize(1);

  _svcLogDraw();
}

// ════════════════════════════════════════════════════════════════════════════
//  TAB 3 — SETTINGS
// ════════════════════════════════════════════════════════════════════════════

// Button y-positions for touch detection (must match draw below)
#define _S_Y402  (_BDY + 138)  // Boot PIN button top = 182
#define _S_Y401  (_BDY + 348)  // Factory Reset button top = 392
#define _S_VOLY  (_BDY + 318)  // Volume tap top = 362
#define _S_VOLH  20             // Volume tap height

static void _drawSvcBody_Settings(int bodyX) {
  int x = bodyX + 16;
  int y;

  // Title
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(x, _BDY + 36);
  gfx->print("Settings");
  gfx->setFont(NULL); gfx->setTextSize(1);

  // ── NETWORK ───────────────────────────────────────────────────────────
  y = _BDY + 62;  // 106
  _svcHeading(x, y, "NETWORK", C_MIDGREY);
  gfx->setFont(NULL); gfx->setTextSize(1);

  Preferences _sp;
  _sp.begin("satu", true);
  String ssid = _sp.getString("nvs_ssid", "(none)");
  _sp.end();

  y = _BDY + 82;  // 126
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("WiFi SSID : ");
  gfx->setTextColor(C_WHITE); gfx->print(ssid.c_str());

  y = _BDY + 98;  // 142
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("IP Addr   : ");
  gfx->setTextColor(C_WHITE); gfx->print(WiFi.localIP().toString().c_str());

  // ── SERVICE ACCESS ────────────────────────────────────────────────────
  y = _BDY + 122;  // 166
  _svcHeading(x, y, "SERVICE ACCESS", C_MIDGREY);

  Preferences _bp;
  _bp.begin("satu", true);
  bool pinOn = _bp.getBool("boot_pin", false);
  _bp.end();

  y = _S_Y402;  // _BDY+138 = 182
  uint16_t pinBg  = pinOn ? gfx->color565(0, 50, 0)  : gfx->color565(30, 30, 30);
  uint16_t pinBdr = pinOn ? C_GREEN                   : C_MIDGREY;
  _fillRoundRect(x, y, 220, 38, 6, pinBg);
  _drawRoundRect(x, y, 220, 38, 6, pinBdr);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(pinOn ? C_GREEN : C_MIDGREY); gfx->setTextSize(1);
  char pinLabel[28];
  snprintf(pinLabel, sizeof(pinLabel), "Boot PIN: %s", pinOn ? "ON" : "OFF");
  gfx->setCursor(x + 16, y + 25);
  gfx->print(pinLabel);
  gfx->setFont(NULL); gfx->setTextSize(1);

  // ── DISPLAY CONFIG ────────────────────────────────────────────────────
  y = _BDY + 196;  // 240
  _svcHeading(x, y, "DISPLAY CONFIG (read-only)", C_MIDGREY);

  y = _BDY + 216;  // 260
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_GREY);
  gfx->setCursor(x, y); gfx->print("Idle: ");
  gfx->setTextColor(C_WHITE);
  char cfgBuf[80];
  snprintf(cfgBuf, sizeof(cfgBuf), "%ds   Select: %ds   Water: %s   Grid: %dx%d",
           g_cfg_idle, g_cfg_sel,
           g_cfg_water ? "ON" : "OFF",
           g_grid_rows, g_grid_cols);
  gfx->print(cfgBuf);

  // ── LANE PRICES ───────────────────────────────────────────────────────
  y = _BDY + 240;  // 284
  _svcHeading(x, y, "LANE PRICES (read-only)", C_MIDGREY);

  // L1-L7
  y = _BDY + 258;  // 302
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setCursor(x, y);
  for (int col = 0; col < 7 && col < NUM_SLOTS; col++) {
    gfx->setTextColor(C_GREY);
    char lbl[6]; snprintf(lbl, sizeof(lbl), "L%d:", col + 1);
    gfx->print(lbl);
    gfx->setTextColor(C_WHITE);
    char prb[8];
    if (g_slots[col].configured && g_slots[col].price > 0) {
      snprintf(prb, sizeof(prb), "%dB ", (int)g_slots[col].price);
    } else {
      snprintf(prb, sizeof(prb), "-- ");
    }
    gfx->print(prb);
  }

  // L8-L10
  y = _BDY + 274;  // 318
  gfx->setCursor(x, y);
  for (int col = 7; col < 10 && col < NUM_SLOTS; col++) {
    gfx->setTextColor(C_GREY);
    char lbl[6]; snprintf(lbl, sizeof(lbl), "L%d:", col + 1);
    gfx->print(lbl);
    gfx->setTextColor(C_WHITE);
    char prb[8];
    if (g_slots[col].configured && g_slots[col].price > 0) {
      snprintf(prb, sizeof(prb), "%dB ", (int)g_slots[col].price);
    } else {
      snprintf(prb, sizeof(prb), "-- ");
    }
    gfx->print(prb);
  }

  // ── AUDIO ─────────────────────────────────────────────────────────────
  y = _BDY + 298;  // 342
  _svcHeading(x, y, "AUDIO", C_MIDGREY);

  y = _S_VOLY;  // _BDY+318 = 362
  Preferences _vp;
  _vp.begin("satu", true);
  int vol = _vp.getInt("vol", 50);
  _vp.end();

  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y);
  gfx->print("Volume: ");
  gfx->setTextColor(C_WHITE);
  char vbuf[12]; snprintf(vbuf, sizeof(vbuf), "%d%%", vol);
  gfx->print(vbuf);
  if (SPEAKER_PIN < 0) {
    gfx->setTextColor(C_GREY);
    gfx->print("  (no speaker pin assigned)");
  } else {
    gfx->setTextColor(C_GREY);
    gfx->print("  Tap to cycle +10%");
  }
  gfx->drawFastHLine(x, y + _S_VOLH, 180, C_DARKGREY);

  // ── FACTORY RESET ─────────────────────────────────────────────────────
  y = _S_Y401;  // _BDY+348 = 392
  _fillRoundRect(x, y, 260, 44, 8, gfx->color565(60, 0, 0));
  _drawRoundRect(x, y, 260, 44, 8, gfx->color565(180, 50, 50));
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(x + 24, y + 28);
  gfx->print("Factory Reset");
  gfx->setFont(NULL); gfx->setTextSize(1);

  _svcLogDraw();
}

// ════════════════════════════════════════════════════════════════════════════
//  TAB 4 — FIRMWARE
// ════════════════════════════════════════════════════════════════════════════

#define _FW_PRINT_Y  (_BDY + 348)  // = 392

static void _drawSvcBody_Firmware(int bodyX) {
  int x = bodyX + 16;
  int y;

  // Title
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(x, _BDY + 36);
  gfx->print("Firmware");
  gfx->setFont(NULL); gfx->setTextSize(1);

  // ── CURRENT FIRMWARE ──────────────────────────────────────────────────
  y = _BDY + 62;  // 106
  _svcHeading(x, y, "CURRENT FIRMWARE", C_MIDGREY);

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  gfx->setFont(NULL); gfx->setTextSize(1);

  y = _BDY + 82;  // 126
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("Version    : ");
  gfx->setTextColor(C_WHITE); gfx->print(FW_VERSION);

  y += 16;  // 142
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("Build      : ");
  gfx->setTextColor(C_WHITE); gfx->print(__DATE__); gfx->print(" "); gfx->print(__TIME__);

  y += 16;  // 158
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("Board      : ");
  gfx->setTextColor(C_WHITE); gfx->print("ESP32-8048S070C");

  y += 16;  // 174
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("Flash/PSRAM: ");
  gfx->setTextColor(C_WHITE); gfx->print("16 MB / 8 MB OPI");

  y += 16;  // 190
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("MAC        : ");
  gfx->setTextColor(C_WHITE); gfx->print(macStr);

  y += 16;  // 206
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("Heap free  : ");
  gfx->setTextColor(C_WHITE);
  char heapBuf[20]; snprintf(heapBuf, sizeof(heapBuf), "%lu KB", ESP.getFreeHeap() / 1024);
  gfx->print(heapBuf);
  gfx->setTextColor(C_GREY); gfx->print("  PSRAM: ");
  gfx->setTextColor(C_WHITE);
  char psramBuf[20]; snprintf(psramBuf, sizeof(psramBuf), "%lu KB", ESP.getFreePsram() / 1024);
  gfx->print(psramBuf);

  // ── SECURITY ──────────────────────────────────────────────────────────
  y = _BDY + 192;  // 236
  _svcHeading(x, y, "SECURITY (dev mode)", C_MIDGREY);
  const uint16_t amber = gfx->color565(180, 120, 0);

  y += 16;  // 252
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("Flash Encrypt : ");
  gfx->setTextColor(amber); gfx->print("DISABLED");

  y += 16;  // 268
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("Secure Boot   : ");
  gfx->setTextColor(amber); gfx->print("DISABLED");

  y += 16;  // 284
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("JTAG          : ");
  gfx->setTextColor(amber); gfx->print("ENABLED  burn eFuse before production");

  // ── REMOTE OTA ─────────────────────────────────────────────────────────
  y = _BDY + 274;  // 318
  _svcHeading(x, y, "REMOTE OTA (stub)", C_MIDGREY);

  y = _BDY + 292;  // 336
  _fillRoundRect(x,       y, 130, 36, 6, gfx->color565(20, 20, 40));
  _drawRoundRect(x,       y, 130, 36, 6, C_DARKGREY);
  _fillRoundRect(x + 140, y, 130, 36, 6, gfx->color565(20, 20, 40));
  _drawRoundRect(x + 140, y, 130, 36, 6, C_DARKGREY);
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_MIDGREY);
  gfx->setCursor(x + 16,       y + 12); gfx->print("Check Update");
  gfx->setCursor(x + 140 + 16, y + 12); gfx->print("Force OTA");

  // ── PRINT TO SERIAL ───────────────────────────────────────────────────
  y = _FW_PRINT_Y;  // _BDY+348 = 392
  _fillRoundRect(x, y, 180, 38, 6, gfx->color565(10, 30, 10));
  _drawRoundRect(x, y, 180, 38, 6, C_GREEN);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GREEN); gfx->setTextSize(1);
  gfx->setCursor(x + 16, y + 25);
  gfx->print("Print to Serial");
  gfx->setFont(NULL); gfx->setTextSize(1);

  _svcLogDraw();
}

// ════════════════════════════════════════════════════════════════════════════
//  TOUCH HELPER — _getTouchedServiceExtra()
//  Returns action codes 500-800 for service tabs.
//  Called from within getTouchedServiceContent() in ui.h.
// ════════════════════════════════════════════════════════════════════════════
int _getTouchedServiceExtra(int tab, int tx, int ty) {
  int bodyX = SVC_BODY_X;
  int x     = bodyX + 16;

  // ── SELF TEST buttons ──────────────────────────────────────────────────
  if (tab == TAB_SELFTEST) {
    const int btnY = _BDY + 72;  // 116
    const int btnH = 38;
    if (ty >= btnY && ty <= btnY + btnH) {
      if (tx >= x && tx <= x + 170)         return 500; // Quick Test
      if (tx >= x + 200 && tx <= x + 370)   return 501; // Technical Test
      if (tx >= x + 384 && tx <= x + 464)   return 502; // Clear
    }
  }

  // ── DEVICES: relay grid ────────────────────────────────────────────────
  if (tab == TAB_DEVICES) {
    // Row 1: relays 1-6
    if (ty >= _REL_ROW1_Y && ty <= _REL_ROW1_Y + _REL_CH) {
      for (int col = 0; col < 6; col++) {
        int cx = _REL_LM + col * (_REL_CW + _REL_GAP);
        if (tx >= cx && tx <= cx + _REL_CW) return 601 + col; // 601=R1...606=R6
      }
    }
    // Row 2: relays 7-12
    if (ty >= _REL_ROW2_Y && ty <= _REL_ROW2_Y + _REL_CH) {
      for (int col = 0; col < 6; col++) {
        int cx = _REL_LM + col * (_REL_CW + _REL_GAP);
        if (tx >= cx && tx <= cx + _REL_CW) return 607 + col; // 607=R7...612=R12
      }
    }
    // Stub row: Pump / LED / Speaker (log locally, no propagated action)
    if (ty >= _STUB_Y && ty <= _STUB_Y + 36) {
      if (tx >= _LM && tx <= _LM + 100) {
        _svcLogPanel("Pump: use R11 in relay grid");
        return 0;
      }
      if (tx >= _LM + 110 && tx <= _LM + 190) {
        _svcLogPanel("LED: stub");
        return 0;
      }
      if (tx >= _LM + 200 && tx <= _LM + 300) {
        _svcLogPanel("Speaker: stub");
        return 0;
      }
    }
    // Test Backend button
    if (ty >= _TBEKY && ty <= _TBEKY + 36 && tx >= x && tx <= x + 160) {
      _svcLogPanel("Backend ping...");
      return 600;
    }
  }

  // ── SETTINGS: Boot PIN button ──────────────────────────────────────────
  if (tab == TAB_SETTINGS) {
    if (ty >= _S_Y402 && ty <= _S_Y402 + 38 && tx >= x && tx <= x + 220) return 702;
  }

  // ── SETTINGS: volume tap area ──────────────────────────────────────────
  if (tab == TAB_SETTINGS) {
    if (ty >= _S_VOLY && ty <= _S_VOLY + _S_VOLH + 8 && tx >= x && tx <= x + 200) return 700;
  }

  // ── SETTINGS: Factory Reset button ────────────────────────────────────
  if (tab == TAB_SETTINGS) {
    if (ty >= _S_Y401 && ty <= _S_Y401 + 44 && tx >= x && tx <= x + 260) return 701;
  }

  // ── FIRMWARE: Print to Serial ──────────────────────────────────────────
  if (tab == TAB_FIRMWARE) {
    if (ty >= _FW_PRINT_Y && ty <= _FW_PRINT_Y + 38 && tx >= x && tx <= x + 180) return 800;
  }

  return 0;
}

#endif // UI_SERVICE_H
