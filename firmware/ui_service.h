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

// ── Layout constants used across tabs ────────────────────────────────────────
#define _LM  (SVC_BODY_X + 16)   // 130 — left content margin
#define _BDY STATUS_H             // 44  — body top y

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

// Run the self-test items one-by-one, drawing results as they complete.
// Called from satu_vending.ino action 500 (quick) or 501 (technical).
void _runSelfTest(int bodyX, bool technical) {
  const int btnH    = 38;
  const int btnY    = _BDY + 60;
  const int resY    = btnY + btnH + 12;  // 154
  const int lineH   = 16;

  // Clear results area
  gfx->fillRect(bodyX, resY, SCR_W - bodyX, SCR_H - resY - 24, C_BG);

  int n = technical ? 14 : 10;

  // ── Build item arrays ──────────────────────────────────────────────────
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

  // Pre-compute pass/fail (except item 12 in tech = live heartbeat)
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  bool heapOk = (ESP.getFreeHeap() > 100000);

  bool qPass[10] = {
    g_mcp1_ok,
    g_mcp2_ok,
    g_mcp1_ok && g_mcp2_ok,
    g_mcp1_ok,
    g_mcp2_ok,
    g_mcp2_ok,
    g_mcp2_ok,
    true,
    wifiOk,
    heapOk,
  };
  bool qSim[10] = { false, false, true, true, true, true, true, true, false, false };

  bool tPass[14] = {
    g_mcp1_ok,
    g_mcp2_ok,
    g_mcp1_ok,
    g_mcp2_ok,
    g_mcp1_ok,
    g_mcp2_ok,
    g_mcp2_ok,
    true,
    true,
    true,
    true,
    wifiOk,
    false,  // heartbeat — updated live below
    heapOk,
  };
  bool tSim[14] = { false, false, true, true, true, true, true, true, true, true, true, false, false, false };

  bool* passArr = technical ? tPass : qPass;
  bool* simArr  = technical ? tSim  : qSim;

  // ── Draw items one by one ──────────────────────────────────────────────
  for (int i = 0; i < n; i++) {
    // Live heartbeat for item 12 in tech test
    if (technical && i == 12) {
      sendHeartbeat();
      tPass[12] = (WiFi.status() == WL_CONNECTED);
      passArr = tPass;
    }

    bool pass = passArr[i];
    bool sim  = simArr[i];

    int iy = resY + i * lineH;

    // Badge [PASS] or [FAIL]
    uint16_t badgeCol = pass ? C_GREEN : C_RED;
    gfx->fillRect(bodyX + 16, iy, 48, 12, badgeCol);
    gfx->setTextColor(C_WHITE); gfx->setTextSize(1); gfx->setFont(NULL);
    gfx->setCursor(bodyX + 18, iy + 3);
    gfx->print(pass ? "PASS" : "FAIL");

    // Label
    gfx->setTextColor(C_WHITE); gfx->setCursor(bodyX + 70, iy + 3);
    gfx->print(labels[i]);

    // (sim) suffix
    if (sim) {
      gfx->setTextColor(C_GREY);
      gfx->print(" (sim)");
    }

    // Store for redraw
    _stL[i] = labels[i];
    _stP[i] = pass;
    _stS[i] = sim;

    delay(300);
  }

  _stN  = n;
  _stm  = technical ? 2 : 1;
}

static void _drawSvcBody_SelfTest(int bodyX) {
  const int btnY  = _BDY + 60;   // 104
  const int btnH  = 38;
  const int resY  = btnY + btnH + 12; // 154
  const int lineH = 16;

  // Title
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 16, _BDY + 36);
  gfx->print("Self Test");
  gfx->setFont(NULL); gfx->setTextSize(1);

  // Subtitle
  gfx->setTextColor(C_GREY);
  gfx->setCursor(bodyX + 16, _BDY + 50);
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
    return;
  }

  for (int i = 0; i < _stN; i++) {
    int iy = resY + i * lineH;
    uint16_t badgeCol = _stP[i] ? C_GREEN : C_RED;
    gfx->fillRect(bodyX + 16, iy, 48, 12, badgeCol);
    gfx->setTextColor(C_WHITE); gfx->setCursor(bodyX + 18, iy + 3);
    gfx->print(_stP[i] ? "PASS" : "FAIL");
    gfx->setTextColor(C_WHITE); gfx->setCursor(bodyX + 70, iy + 3);
    gfx->print(_stL[i]);
    if (_stS[i]) {
      gfx->setTextColor(C_GREY);
      gfx->print(" (sim)");
    }
  }
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

  gfx->setTextColor(C_GREY);
  gfx->setCursor(bodyX + 16, _BDY + 50);
  gfx->print("Tap slot to vend without payment");

  // Slot grid
  int cols = (g_grid_cols > 0) ? g_grid_cols : 5;
  const int cw = 44, ch = 44;
  for (int i = 0; i < NUM_SLOTS && i < 21; i++) {
    int col = i % cols;
    int row = i / cols;
    int cx  = bodyX + 16 + col * (cw + 2);
    int cy  = _BDY + 68 + row * (ch + 2);

    uint16_t bg;
    if (!g_slots[i].configured) {
      bg = C_DARKGREY;
    } else if (g_slots[i].enabled) {
      bg = gfx->color565(60, 45, 0);  // dark gold for enabled
    } else {
      bg = gfx->color565(50, 10, 10); // dark red for disabled
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
    gfx->setCursor(cx + cw / 2 - nw / 2, cy + 18);
    gfx->print(nb);
    gfx->setFont(NULL); gfx->setTextSize(1);

    // Price (bottom of cell)
    if (g_slots[i].configured && g_slots[i].price > 0) {
      gfx->setTextColor(C_GOLD);
      char pb[8]; snprintf(pb, sizeof(pb), "%dB", (int)g_slots[i].price);
      int pw = strlen(pb) * 6;
      gfx->setCursor(cx + cw / 2 - pw / 2, cy + 32);
      gfx->print(pb);
    }
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  TAB 2 — DEVICES
// ════════════════════════════════════════════════════════════════════════════

// Layout constants for Devices tab
#define _REL_LM      (_LM)
#define _REL_CW      107
#define _REL_CH      44
#define _REL_GAP     4
#define _REL_ROW1_Y  (_BDY + 55)
#define _REL_ROW2_Y  (_REL_ROW1_Y + _REL_CH + _REL_GAP)
#define _IR_CW       62
#define _IR_CH       28
#define _IR_GAP      4
#define _IR_ROW1_Y   (_REL_ROW2_Y + _REL_CH + 38)
#define _IR_ROW2_Y   (_IR_ROW1_Y + _IR_CH + _IR_GAP)
#define _IR_ROW3_Y   (_IR_ROW2_Y + _IR_CH + _IR_GAP)
#define _TBEKY       (_IR_ROW2_Y + _IR_CH + 12)

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
      // FLAP: OFF=LOCKED(red), ON=UNLOCKED(green)
      bg = on ? gfx->color565(0, 60, 0) : gfx->color565(70, 0, 0);
      fg = on ? C_GREEN                  : C_RED;
    } else if (on) {
      bg = gfx->color565(0, 55, 0);
      fg = C_GREEN;
    } else {
      bg = gfx->color565(18, 18, 22);
      fg = C_GREY;
    }

    _fillRoundRect(cx, cy, _REL_CW, _REL_CH, 4, bg);
    _drawRoundRect(cx, cy, _REL_CW, _REL_CH, 4, fg);

    // Relay label: R1-R10=lane label, R11=PUMP, R12=FLAP/LOCKED/UNLOCKED
    gfx->setFont(NULL); gfx->setTextSize(1);
    gfx->setTextColor(fg);

    char topLine[12];
    if (r == 11) {
      snprintf(topLine, sizeof(topLine), "R11");
    } else if (r == 12) {
      snprintf(topLine, sizeof(topLine), "R12");
    } else {
      snprintf(topLine, sizeof(topLine), "R%d", r);
    }

    // Top line: relay number
    int tw = strlen(topLine) * 6;
    gfx->setCursor(cx + _REL_CW / 2 - tw / 2, cy + 10);
    gfx->print(topLine);

    // Bottom line: state label
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
  int warnY = _REL_ROW2_Y + _REL_CH + 4;
  gfx->fillRect(bodyX + 16, warnY, SCR_W - bodyX - 24, 18, gfx->color565(50, 30, 0));
  gfx->setTextColor(gfx->color565(220, 160, 0)); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 20, warnY + 5);
  gfx->print("WARNING: Relays stay ON until tapped again. Exit to reset all.");

  // ── IR SENSORS SECTION ─────────────────────────────────────────────────
  _svcHeading(bodyX + 16, warnY + 24, "IR SENSORS (live)", C_MIDGREY);

  for (int s = 0; s < 10; s++) {
    int col = s % 5;
    int row = s / 5;
    int cx  = _LM + col * (_IR_CW + _IR_GAP);
    int cy  = _IR_ROW1_Y + row * (_IR_CH + _IR_GAP);

    bool triggered = readSensor(s);
    uint16_t bg  = triggered ? gfx->color565(80, 0, 0) : gfx->color565(10, 30, 10);
    uint16_t fg  = triggered ? C_RED : C_GREEN;

    _fillRoundRect(cx, cy, _IR_CW, _IR_CH, 3, bg);
    _drawRoundRect(cx, cy, _IR_CW, _IR_CH, 3, fg);

    gfx->setFont(NULL); gfx->setTextSize(1);
    gfx->setTextColor(fg);

    char sn[4]; snprintf(sn, sizeof(sn), "S%d", s + 1);
    gfx->setCursor(cx + 4, cy + 6);
    gfx->print(sn);

    const char* sl = triggered ? "BLOCK" : "CLEAR";
    int slw = strlen(sl) * 6;
    gfx->setCursor(cx + _IR_CW / 2 - slw / 2, cy + 19);
    gfx->print(sl);
  }

  // ── TEST BACKEND BUTTON ─────────────────────────────────────────────────
  _fillRoundRect(bodyX + 16, _TBEKY, 160, 36, 6, gfx->color565(10, 30, 60));
  _drawRoundRect(bodyX + 16, _TBEKY, 160, 36, 6, C_GOLD);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 30, _TBEKY + 23);
  gfx->print("Test Backend");
  gfx->setFont(NULL); gfx->setTextSize(1);
}

// ════════════════════════════════════════════════════════════════════════════
//  TAB 3 — SETTINGS
// ════════════════════════════════════════════════════════════════════════════

// Button y-positions for touch detection (must match draw below)
#define _S_Y402   168   // Boot PIN toggle button top
#define _S_Y401   370   // Factory Reset button top
#define _S_VOLY   326   // Volume tap area top
#define _S_VOLH   20    // Volume tap area height

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
  y = 100;
  _svcHeading(x, y, "NETWORK", C_MIDGREY);
  gfx->setFont(NULL); gfx->setTextSize(1);

  y = 118;
  Preferences _sp;
  _sp.begin("satu", true);
  String ssid = _sp.getString("nvs_ssid", "(none)");
  _sp.end();

  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("WiFi SSID : ");
  gfx->setTextColor(C_WHITE); gfx->print(ssid.c_str());

  y = 134;
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y); gfx->print("IP Addr   : ");
  gfx->setTextColor(C_WHITE); gfx->print(WiFi.localIP().toString().c_str());

  // ── SERVICE ACCESS ────────────────────────────────────────────────────
  y = 152;
  _svcHeading(x, y, "SERVICE ACCESS", C_MIDGREY);

  Preferences _bp;
  _bp.begin("satu", true);
  bool pinOn = _bp.getBool("boot_pin", false);
  _bp.end();

  y = _S_Y402;  // 168
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
  y = 222;
  _svcHeading(x, y, "DISPLAY CONFIG (read-only)", C_MIDGREY);

  y = 240;
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
  y = 260;
  _svcHeading(x, y, "LANE PRICES (read-only)", C_MIDGREY);

  y = 276;
  gfx->setFont(NULL); gfx->setTextSize(1);
  for (int row = 0; row < 2; row++) {
    gfx->setCursor(x, y + row * 16);
    for (int col = 0; col < 5; col++) {
      int idx = row * 5 + col;
      if (idx >= NUM_SLOTS) break;
      gfx->setTextColor(C_GREY);
      char lbl[8]; snprintf(lbl, sizeof(lbl), "L%d:", idx + 1);
      gfx->print(lbl);
      gfx->setTextColor(C_WHITE);
      char prb[8];
      if (g_slots[idx].configured && g_slots[idx].price > 0) {
        snprintf(prb, sizeof(prb), "%dB  ", (int)g_slots[idx].price);
      } else {
        snprintf(prb, sizeof(prb), "--   ");
      }
      gfx->print(prb);
    }
  }

  // ── AUDIO ─────────────────────────────────────────────────────────────
  y = 308;
  _svcHeading(x, y, "AUDIO", C_MIDGREY);

  y = _S_VOLY;  // 326

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
    gfx->print("  Tap to change. Assign SPEAKER_PIN to enable tone.");
  } else {
    gfx->setTextColor(C_GREY);
    gfx->print("  Tap to cycle +10%");
  }

  // Volume tap target (subtle underline)
  gfx->drawFastHLine(x, y + _S_VOLH, 180, C_DARKGREY);

  // ── FACTORY RESET ────────────────────────────────────────────────────
  y = _S_Y401;  // 370
  _fillRoundRect(x, y, 260, 44, 8, gfx->color565(60, 0, 0));
  _drawRoundRect(x, y, 260, 44, 8, gfx->color565(180, 50, 50));
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(x + 24, y + 28);
  gfx->print("Factory Reset");
  gfx->setFont(NULL); gfx->setTextSize(1);
}

// ════════════════════════════════════════════════════════════════════════════
//  TAB 4 — FIRMWARE
// ════════════════════════════════════════════════════════════════════════════

#define _FW_PRINT_Y  380   // [Print to Serial] button top

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
  y = 100;
  _svcHeading(x, y, "CURRENT FIRMWARE", C_MIDGREY);

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  gfx->setFont(NULL); gfx->setTextSize(1);
  y = 118;
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y);    gfx->print("Version   : ");
  gfx->setTextColor(C_WHITE); gfx->print(FW_VERSION);

  y += 14;
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y);    gfx->print("Build     : ");
  gfx->setTextColor(C_WHITE); gfx->print(__DATE__); gfx->print(" "); gfx->print(__TIME__);

  y += 14;
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y);    gfx->print("Board     : ");
  gfx->setTextColor(C_WHITE); gfx->print("ESP32-8048S070C");

  y += 14;
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y);    gfx->print("Flash/PSRAM: ");
  gfx->setTextColor(C_WHITE); gfx->print("16 MB / 8 MB OPI");

  y += 14;
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y);    gfx->print("MAC       : ");
  gfx->setTextColor(C_WHITE); gfx->print(macStr);

  y += 14;
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y);    gfx->print("Heap free : ");
  gfx->setTextColor(C_WHITE);
  char heapBuf[20]; snprintf(heapBuf, sizeof(heapBuf), "%lu KB", ESP.getFreeHeap() / 1024);
  gfx->print(heapBuf);
  gfx->setTextColor(C_GREY); gfx->print("  PSRAM: ");
  gfx->setTextColor(C_WHITE);
  char psramBuf[20]; snprintf(psramBuf, sizeof(psramBuf), "%lu KB", ESP.getFreePsram() / 1024);
  gfx->print(psramBuf);

  // ── SECURITY ──────────────────────────────────────────────────────────
  y += 22;
  _svcHeading(x, y, "SECURITY (dev mode)", C_MIDGREY);
  const uint16_t amber = gfx->color565(180, 120, 0);

  y += 18;
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y);    gfx->print("Flash Encrypt : ");
  gfx->setTextColor(amber); gfx->print("DISABLED");

  y += 14;
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y);    gfx->print("Secure Boot   : ");
  gfx->setTextColor(amber); gfx->print("DISABLED");

  y += 14;
  gfx->setTextColor(C_GREY); gfx->setCursor(x, y);    gfx->print("JTAG          : ");
  gfx->setTextColor(amber); gfx->print("ENABLED  burn eFuse before production");

  // ── REMOTE OTA ─────────────────────────────────────────────────────────
  y += 22;
  _svcHeading(x, y, "REMOTE OTA (stub)", C_MIDGREY);

  y += 18;
  _fillRoundRect(x,       y, 130, 34, 6, gfx->color565(20, 20, 40));
  _drawRoundRect(x,       y, 130, 34, 6, C_DARKGREY);
  _fillRoundRect(x + 140, y, 130, 34, 6, gfx->color565(20, 20, 40));
  _drawRoundRect(x + 140, y, 130, 34, 6, C_DARKGREY);
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_MIDGREY);
  gfx->setCursor(x + 16, y + 12);  gfx->print("Check Update");
  gfx->setCursor(x + 156, y + 12); gfx->print("Force OTA");

  // ── PRINT TO SERIAL ───────────────────────────────────────────────────
  y = _FW_PRINT_Y;  // 380
  _fillRoundRect(x, y, 180, 38, 6, gfx->color565(10, 30, 10));
  _drawRoundRect(x, y, 180, 38, 6, C_GREEN);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GREEN); gfx->setTextSize(1);
  gfx->setCursor(x + 16, y + 25);
  gfx->print("Print to Serial");
  gfx->setFont(NULL); gfx->setTextSize(1);
}

// ════════════════════════════════════════════════════════════════════════════
//  TOUCH HELPER — getTouchedServiceExtra()
//  Returns action codes 500-800 for new tabs.
//  Called from within getTouchedServiceContent() in ui.h.
// ════════════════════════════════════════════════════════════════════════════
int _getTouchedServiceExtra(int tab, int tx, int ty) {
  int bodyX = SVC_BODY_X;
  int x     = bodyX + 16;

  // ── SELF TEST buttons ──────────────────────────────────────────────────
  if (tab == TAB_SELFTEST) {
    const int btnY = _BDY + 60;
    const int btnH = 38;
    if (ty >= btnY && ty <= btnY + btnH) {
      if (tx >= x && tx <= x + 170)           return 500; // Quick Test
      if (tx >= x + 200 && tx <= x + 370)     return 501; // Technical Test
      if (tx >= x + 384 && tx <= x + 464)     return 502; // Clear
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
    // Test Backend button
    if (ty >= _TBEKY && ty <= _TBEKY + 36 && tx >= x && tx <= x + 160) return 600;
  }

  // ── SETTINGS: volume area ──────────────────────────────────────────────
  if (tab == TAB_SETTINGS) {
    if (ty >= _S_VOLY && ty <= _S_VOLY + _S_VOLH + 8 && tx >= x && tx <= x + 200) return 700;
  }

  // ── FIRMWARE: Print to Serial ──────────────────────────────────────────
  if (tab == TAB_FIRMWARE) {
    if (ty >= _FW_PRINT_Y && ty <= _FW_PRINT_Y + 38 && tx >= x && tx <= x + 180) return 800;
  }

  return 0;
}

#endif // UI_SERVICE_H
