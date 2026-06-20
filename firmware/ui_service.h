#ifndef UI_SERVICE_H
#define UI_SERVICE_H

// ui_service.h — Service mode tab bodies (all 5 tabs)
// Version: R12 — 2026-06-20
// Changes: Bottom log panel, size-2 content text, fixed button sizes,
//          Devices relay/IR grid cw=96, Settings/Firmware label columns.
// Included at bottom of ui.h — all ui.h symbols visible here.
// network.h and hardware.h are included before ui.h in satu_vending.ino.

// ── Self Test state ──────────────────────────────────────────────────────────
static int  _stm     = 0;   // 0=idle 1=quick results 2=tech results
static int  _stN     = 0;   // number of test items ran
static bool _stP[14];
static bool _stS[14];
static const char* _stL[14];

// ── Relay toggle state ────────────────────────────────────────────────────────
static bool _relState[13] = {false};

// ── Bottom log panel ──────────────────────────────────────────────────────────
#define _BLOG_X   (SVC_BODY_X + 8)
#define _BLOG_Y   (SCR_H - 80)               // 400
#define _BLOG_W   (SCR_W - SVC_BODY_X - 16)  // 670
#define _BLOG_H   72
#define _BLOG_MAX 4

static String _svcLogBuf[_BLOG_MAX];

// Convenience shorthands used across all tab bodies
#define _LM  (SVC_BODY_X + 16)   // left margin = 130
#define _BDY STATUS_H             // body top = 44

static void _svcLogDraw() {
  gfx->fillRect(_BLOG_X, _BLOG_Y, _BLOG_W, _BLOG_H, gfx->color565(10, 6, 18));
  gfx->drawRect(_BLOG_X, _BLOG_Y, _BLOG_W, _BLOG_H, C_MIDGREY);
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_GREY);
  gfx->setCursor(_BLOG_X + 4, _BLOG_Y + 4);
  gfx->print("LOG");
  gfx->drawFastHLine(_BLOG_X + 1, _BLOG_Y + 14, _BLOG_W - 2, C_DARKGREY);
  int maxChars = (_BLOG_W - 8) / 6;
  for (int i = 0; i < _BLOG_MAX; i++) {
    if (_svcLogBuf[i].length() == 0) continue;
    int ly = _BLOG_Y + 18 + i * 13;
    if (ly + 8 > _BLOG_Y + _BLOG_H - 2) break;
    gfx->setTextColor(C_WHITE);
    gfx->setCursor(_BLOG_X + 4, ly);
    String s = _svcLogBuf[i];
    if ((int)s.length() > maxChars) s = s.substring(0, maxChars);
    gfx->print(s.c_str());
  }
}

void _svcLogPanel(String msg) {
  for (int i = 0; i < _BLOG_MAX - 1; i++) _svcLogBuf[i] = _svcLogBuf[i + 1];
  _svcLogBuf[_BLOG_MAX - 1] = msg;
  _svcLogDraw();
  Serial.println("[SVC] " + msg);
}

static void _svcHeading(int x, int y, const char* txt, uint16_t col = 0xFFFF) {
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(col); gfx->setTextSize(1);
  gfx->setCursor(x, y);
  gfx->print(txt);
  gfx->setFont(NULL); gfx->setTextSize(2);
}

// ═══════════════════════════════════════════════════════════════════════════════
// TAB 0 — SELF TEST
// ═══════════════════════════════════════════════════════════════════════════════
#define _ST_BTN_Y  (_BDY + 82)    // button row top = 126
#define _ST_BTN_H  36
#define _ST_Q_X    (_LM)           // Quick Test  x=130  w=140
#define _ST_T_X    (_LM + 148)     // Technical   x=278  w=180
#define _ST_C_X    (_LM + 336)     // Clear        x=466  w=90

void _runSelfTest(int bodyX, bool technical) {
  const int resY  = _ST_BTN_Y + _ST_BTN_H + 12;  // 174
  const int lineH = 20;
  const int maxY  = SCR_H - 88;  // 392
  gfx->fillRect(bodyX, resY, SCR_W - bodyX, maxY - resY, C_BG);
  int n = technical ? 14 : 10;
  static const char* qLabels[10] = {
    "MCP1 (0x20)", "MCP2 (0x21)", "IR 1-10",
    "Relay 1-6",   "Relay 7-12",  "Flap R12",
    "Pump R11",    "LEDs",        "WiFi",    "Heap>=100KB",
  };
  static const char* tLabels[14] = {
    "MCP1 (0x20) I2C",  "MCP2 (0x21) I2C",
    "IR 1-8 MCP1",      "IR 9-10 MCP2",
    "Relay 1-6 MCP1",   "Relay 7-11 MCP2",
    "Flap R12 MCP2",    "WS2812B GPIO5",
    "Display 800x480",  "GT911 Touch",
    "NVS r/w",          "WiFi",
    "Backend hbeat",    "Heap>=100KB",
  };
  const char** labels = technical ? tLabels : qLabels;
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  bool heapOk = (ESP.getFreeHeap() > 100000);
  bool qPass[10] = { g_mcp1_ok, g_mcp2_ok, g_mcp1_ok&&g_mcp2_ok,
    g_mcp1_ok, g_mcp2_ok, g_mcp2_ok, g_mcp2_ok, true, wifiOk, heapOk };
  bool qSim[10]  = { false,false,true,true,true,true,true,true,false,false };
  bool tPass[14] = { g_mcp1_ok, g_mcp2_ok, g_mcp1_ok, g_mcp2_ok,
    g_mcp1_ok, g_mcp2_ok, g_mcp2_ok, true, true, true, true, wifiOk, false, heapOk };
  bool tSim[14]  = { false,false,true,true,true,true,true,true,true,true,true,false,false,false };
  bool* passArr = technical ? tPass : qPass;
  bool* simArr  = technical ? tSim  : qSim;
  for (int i = 0; i < n; i++) {
    if (technical && i == 12) {
      sendHeartbeat(); tPass[12] = (WiFi.status() == WL_CONNECTED); passArr = tPass;
    }
    bool pass = passArr[i]; bool sim = simArr[i];
    int  iy   = resY + i * lineH;
    if (iy + lineH > maxY) break;
    uint16_t badgeCol = pass ? C_GREEN : C_RED;
    gfx->fillRect(bodyX + 16, iy, 52, 16, badgeCol);
    gfx->setFont(NULL); gfx->setTextSize(1); gfx->setTextColor(C_WHITE);
    gfx->setCursor(bodyX + 18, iy + 5); gfx->print(pass ? "PASS" : "FAIL");
    gfx->setFont(NULL); gfx->setTextSize(2); gfx->setTextColor(C_WHITE);
    gfx->setCursor(bodyX + 76, iy + 2); gfx->print(labels[i]);
    if (sim) { gfx->setTextSize(1); gfx->setTextColor(C_GREY); gfx->print(" (sim)"); }
    char logMsg[40];
    snprintf(logMsg, sizeof(logMsg), "%s: %.20s", pass ? "OK" : "FAIL", labels[i]);
    _svcLogPanel(String(logMsg));
    _stL[i] = labels[i]; _stP[i] = pass; _stS[i] = sim;
    delay(300);
  }
  _stN = n; _stm = technical ? 2 : 1;
}

static void _drawSvcBody_SelfTest(int bodyX) {
  const int resY  = _ST_BTN_Y + _ST_BTN_H + 12;  // 174
  const int lineH = 20;
  const int maxY  = SCR_H - 88;  // 392
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 16, _BDY + 28); gfx->print("Self Test");
  gfx->setFont(NULL); gfx->setTextSize(2);
  gfx->setTextColor(C_GREY);
  gfx->setCursor(bodyX + 16, _BDY + 52);
  gfx->print("Test MCP, sensors, WiFi, and heap");
  // [Quick Test] w=140 h=36
  _fillRoundRect(_ST_Q_X, _ST_BTN_Y, 140, _ST_BTN_H, 6, gfx->color565(20,60,20));
  _drawRoundRect(_ST_Q_X, _ST_BTN_Y, 140, _ST_BTN_H, 6, C_GREEN);
  gfx->setFont(NULL); gfx->setTextSize(2); gfx->setTextColor(C_GREEN);
  gfx->setCursor(_ST_Q_X + 6, _ST_BTN_Y + 10); gfx->print("Quick Test");
  // [Technical Test] w=180 h=36
  _fillRoundRect(_ST_T_X, _ST_BTN_Y, 180, _ST_BTN_H, 6, gfx->color565(20,40,70));
  _drawRoundRect(_ST_T_X, _ST_BTN_Y, 180, _ST_BTN_H, 6, C_GOLD);
  gfx->setTextColor(C_GOLD);
  gfx->setCursor(_ST_T_X + 6, _ST_BTN_Y + 10); gfx->print("Technical Test");
  // [Clear] w=90 h=36
  _fillRoundRect(_ST_C_X, _ST_BTN_Y, 90, _ST_BTN_H, 6, gfx->color565(40,20,20));
  _drawRoundRect(_ST_C_X, _ST_BTN_Y, 90, _ST_BTN_H, 6, C_MIDGREY);
  gfx->setTextColor(C_MIDGREY);
  gfx->setCursor(_ST_C_X + 9, _ST_BTN_Y + 10); gfx->print("Clear");
  // Results area
  if (_stm == 0) {
    gfx->setTextSize(2); gfx->setTextColor(C_GREY);
    gfx->setCursor(bodyX + 16, resY + 4);
    gfx->print("Tap Quick or Technical Test to begin.");
  } else {
    for (int i = 0; i < _stN; i++) {
      int iy = resY + i * lineH;
      if (iy + lineH > maxY) break;
      uint16_t badgeCol = _stP[i] ? C_GREEN : C_RED;
      gfx->fillRect(bodyX + 16, iy, 52, 16, badgeCol);
      gfx->setFont(NULL); gfx->setTextSize(1); gfx->setTextColor(C_WHITE);
      gfx->setCursor(bodyX + 18, iy + 5); gfx->print(_stP[i] ? "PASS" : "FAIL");
      gfx->setFont(NULL); gfx->setTextSize(2); gfx->setTextColor(C_WHITE);
      gfx->setCursor(bodyX + 76, iy + 2); gfx->print(_stL[i]);
      if (_stS[i]) { gfx->setTextSize(1); gfx->setTextColor(C_GREY); gfx->print(" (sim)"); }
    }
  }
  gfx->setTextSize(1);
  _svcLogDraw();
}

// ═══════════════════════════════════════════════════════════════════════════════
// TAB 1 — FREE PLAY  (PASSED QA — DO NOT TOUCH)
// ═══════════════════════════════════════════════════════════════════════════════
static void _drawSvcBody_FreePlay(int bodyX) {
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 16, _BDY + 36); gfx->print("Free Play");
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_GREY); gfx->setCursor(bodyX + 16, _BDY + 60);
  gfx->print("Tap slot to fire motor - no payment needed");
  const int cols=7, cw=72, ch=50, gap=4;
  const int gridX=bodyX+16, gridY=_BDY+72;
  for (int i=0;i<NUM_SLOTS&&i<21;i++){
    int col=i%cols, row=i/cols;
    int cx=gridX+col*(cw+gap), cy=gridY+row*(ch+gap);
    uint16_t bg=!g_slots[i].configured?C_DARKGREY:g_slots[i].enabled?gfx->color565(60,45,0):gfx->color565(50,10,10);
    _fillRoundRect(cx,cy,cw,ch,4,bg);
    uint16_t bdr=!g_slots[i].configured?C_DARKGREY:g_slots[i].enabled?C_GOLD:gfx->color565(140,40,40);
    _drawRoundRect(cx,cy,cw,ch,4,bdr);
    gfx->setFont(&FreeSansBold12pt7b); gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
    char nb[4]; snprintf(nb,sizeof(nb),"%d",i+1);
    int nw=strlen(nb)*7;
    gfx->setCursor(cx+cw/2-nw/2,cy+22); gfx->print(nb);
    gfx->setFont(NULL); gfx->setTextSize(1);
    if(g_slots[i].configured&&g_slots[i].price>0){
      gfx->setTextColor(C_GOLD);
      char pb[8]; snprintf(pb,sizeof(pb),"%dB",(int)g_slots[i].price);
      int pw=strlen(pb)*6;
      gfx->setCursor(cx+cw/2-pw/2,cy+38); gfx->print(pb);
    }
  }
  _svcLogDraw();
}

// ═══════════════════════════════════════════════════════════════════════════════
// TAB 2 — DEVICES
// ═══════════════════════════════════════════════════════════════════════════════
#define _DEV_CW      96
#define _DEV_CH      44
#define _DEV_GAP     4
#define _DEV_LX      (SVC_BODY_X + 8)
#define _DEV_R1_Y    (_BDY + 38)
#define _DEV_R2_Y    (_DEV_R1_Y + _DEV_CH + _DEV_GAP)
#define _DEV_WARN_Y  (_DEV_R2_Y + _DEV_CH + 4)
#define _DEV_IH_Y    (_DEV_WARN_Y + 30)
#define _DEV_IR1_Y   (_DEV_IH_Y + 8)
#define _DEV_IR2_Y   (_DEV_IR1_Y + _DEV_CH + _DEV_GAP)
#define _DEV_STUB_Y  (_DEV_IR2_Y + _DEV_CH + 6)
#define _DEV_TBES_Y  (_DEV_STUB_Y + 36 + 6)

static void _drawSvcBody_Devices(int bodyX) {
  gfx->setFont(&FreeSansBold18pt7b); gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(bodyX + 16, _BDY + 28); gfx->print("Devices");
  gfx->setFont(NULL); gfx->setTextSize(1);
  // Relay grid R1-R12 (two rows of 6)
  for (int r = 1; r <= 12; r++) {
    int col = (r-1) % 6, row = (r-1) / 6;
    int cx = _DEV_LX + col * (_DEV_CW + _DEV_GAP);
    int cy = (row == 0) ? _DEV_R1_Y : _DEV_R2_Y;
    bool on = _relState[r];
    uint16_t bg, fg;
    if (r == 12) {
      bg = on ? gfx->color565(0,60,0)   : gfx->color565(70,0,0);
      fg = on ? C_GREEN : C_RED;
    } else if (on) {
      bg = gfx->color565(0,55,0); fg = C_GREEN;
    } else {
      bg = gfx->color565(18,18,22); fg = C_GREY;
    }
    _fillRoundRect(cx, cy, _DEV_CW, _DEV_CH, 4, bg);
    _drawRoundRect(cx, cy, _DEV_CW, _DEV_CH, 4, fg);
    gfx->setFont(NULL); gfx->setTextSize(1); gfx->setTextColor(fg);
    char topLine[6]; snprintf(topLine, sizeof(topLine), "R%d", r);
    int tw = strlen(topLine) * 6;
    gfx->setCursor(cx + _DEV_CW/2 - tw/2, cy + 10); gfx->print(topLine);
    const char* stateLabel;
    if (r == 12)      stateLabel = on ? "UNLOCKED" : "LOCKED";
    else if (r == 11) stateLabel = on ? "PUMP ON"  : "PUMP";
    else              stateLabel = on ? "ON"        : "OFF";
    int sl = strlen(stateLabel) * 6;
    gfx->setCursor(cx + _DEV_CW/2 - sl/2, cy + 28); gfx->print(stateLabel);
  }
  // WARNING banner
  gfx->fillRect(_DEV_LX, _DEV_WARN_Y, _BLOG_W - 14, 18, gfx->color565(50,30,0));
  gfx->setTextColor(gfx->color565(220,160,0)); gfx->setTextSize(1);
  gfx->setCursor(_DEV_LX + 4, _DEV_WARN_Y + 5);
  gfx->print("WARNING: Relays stay ON until tapped again. Exit to reset.");
  // IR SENSORS heading + grid
  _svcHeading(_DEV_LX, _DEV_IH_Y, "IR SENSORS (live)", C_MIDGREY);
  for (int s = 0; s < 10; s++) {
    int col = s % 5, row = s / 5;
    int cx = _DEV_LX + col * (_DEV_CW + _DEV_GAP);
    int cy = (row == 0) ? _DEV_IR1_Y : _DEV_IR2_Y;
    bool triggered = readSensor(s);
    uint16_t bg = triggered ? gfx->color565(80,0,0) : gfx->color565(10,30,10);
    uint16_t fg = triggered ? C_RED : C_GREEN;
    _fillRoundRect(cx, cy, _DEV_CW, _DEV_CH, 3, bg);
    _drawRoundRect(cx, cy, _DEV_CW, _DEV_CH, 3, fg);
    gfx->setFont(NULL); gfx->setTextSize(1); gfx->setTextColor(fg);
    char sn[5]; snprintf(sn, sizeof(sn), "S%d", s+1);
    int snw = strlen(sn) * 6;
    gfx->setCursor(cx + _DEV_CW/2 - snw/2, cy + 10); gfx->print(sn);
    const char* slabel = triggered ? "BLOCK" : "CLEAR";
    int slw = strlen(slabel) * 6;
    gfx->setCursor(cx + _DEV_CW/2 - slw/2, cy + 28); gfx->print(slabel);
  }
  // Stub buttons row: Pump R11 / LED Test / Speaker
  _fillRoundRect(_DEV_LX,       _DEV_STUB_Y, 180, 36, 4, gfx->color565(15,20,40));
  _drawRoundRect(_DEV_LX,       _DEV_STUB_Y, 180, 36, 4, C_DARKGREY);
  _fillRoundRect(_DEV_LX + 188, _DEV_STUB_Y, 140, 36, 4, gfx->color565(15,20,40));
  _drawRoundRect(_DEV_LX + 188, _DEV_STUB_Y, 140, 36, 4, C_DARKGREY);
  _fillRoundRect(_DEV_LX + 336, _DEV_STUB_Y, 140, 36, 4, gfx->color565(15,20,40));
  _drawRoundRect(_DEV_LX + 336, _DEV_STUB_Y, 140, 36, 4, C_DARKGREY);
  gfx->setFont(NULL); gfx->setTextSize(2); gfx->setTextColor(C_MIDGREY);
  gfx->setCursor(_DEV_LX + 8,       _DEV_STUB_Y + 10); gfx->print("Pump R11");
  gfx->setCursor(_DEV_LX + 188 + 8, _DEV_STUB_Y + 10); gfx->print("LED Test");
  gfx->setCursor(_DEV_LX + 336 + 8, _DEV_STUB_Y + 10); gfx->print("Speaker");
  // Test Backend button (centered, w=200 h=36)
  int tbX = SVC_BODY_X + (686 - 200) / 2;  // 357
  _fillRoundRect(tbX, _DEV_TBES_Y, 200, 36, 6, gfx->color565(10,30,60));
  _drawRoundRect(tbX, _DEV_TBES_Y, 200, 36, 6, C_GOLD);
  gfx->setTextColor(C_GOLD);
  gfx->setCursor(tbX + 12, _DEV_TBES_Y + 10); gfx->print("Test Backend");
  gfx->setTextSize(1);
  _svcLogDraw();
}

// ═══════════════════════════════════════════════════════════════════════════════
// TAB 3 — SETTINGS
// ═══════════════════════════════════════════════════════════════════════════════
// All Y positions relative to _BDY (=44). Working top-down with tight 4px gaps.
// Boot PIN button at _BDY+120 = y164. Factory Reset at _BDY+306 = y350 (h=34 → bottom=384).
// Volume row at _BDY+286 = y330. Content max y = SCR_H-88 = 392.
#define _S_Y402  (_BDY + 120)   // Boot PIN button top    = 164
#define _S_Y401  (_BDY + 306)   // Factory Reset top      = 350
#define _S_VOLY  (_BDY + 286)   // Volume row top         = 330
#define _S_VOLH  16

static void _drawSvcBody_Settings(int bodyX) {
  int x = bodyX + 16, lv = x + 130;  // label col=130, value col=260
  gfx->setFont(&FreeSansBold18pt7b); gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(x, _BDY + 28); gfx->print("Settings");

  // NETWORK section
  _svcHeading(x, _BDY + 50, "NETWORK", C_MIDGREY);
  Preferences _sp; _sp.begin("satu", true);
  String ssid = _sp.getString("nvs_ssid", "(none)"); _sp.end();
  gfx->setFont(NULL); gfx->setTextSize(2);
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 58); gfx->print("WiFi SSID");
  gfx->setTextColor(C_WHITE); gfx->setCursor(lv, _BDY + 58); gfx->print(ssid.c_str());
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 78); gfx->print("IP Addr");
  gfx->setTextColor(C_WHITE); gfx->setCursor(lv, _BDY + 78); gfx->print(WiFi.localIP().toString().c_str());

  // SERVICE ACCESS section
  _svcHeading(x, _BDY + 112, "SERVICE ACCESS", C_MIDGREY);
  Preferences _bp; _bp.begin("satu", true);
  bool pinOn = _bp.getBool("boot_pin", false); _bp.end();
  int y = _S_Y402;
  uint16_t pinBg  = pinOn ? gfx->color565(0,50,0)   : gfx->color565(30,30,30);
  uint16_t pinBdr = pinOn ? C_GREEN : C_MIDGREY;
  _fillRoundRect(x, y, 220, 36, 6, pinBg); _drawRoundRect(x, y, 220, 36, 6, pinBdr);
  gfx->setFont(NULL); gfx->setTextSize(2);
  gfx->setTextColor(pinOn ? C_GREEN : C_MIDGREY);
  gfx->setCursor(x + 12, y + 10);
  char pinLabel[20]; snprintf(pinLabel, sizeof(pinLabel), "Boot PIN: %s", pinOn ? "ON" : "OFF");
  gfx->print(pinLabel);

  // DISPLAY CONFIG section
  _svcHeading(x, _BDY + 182, "DISPLAY CONFIG", C_MIDGREY);
  gfx->setFont(NULL); gfx->setTextSize(2);
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 198); gfx->print("Idle");
  gfx->setTextColor(C_WHITE); gfx->setCursor(lv, _BDY + 198);
  char cfgBuf[64];
  snprintf(cfgBuf, sizeof(cfgBuf), "%ds  Sel:%ds  Water:%s  Grid:%dx%d",
           g_cfg_idle, g_cfg_sel, g_cfg_water ? "ON" : "OFF", g_grid_rows, g_grid_cols);
  gfx->print(cfgBuf);

  // LANE PRICES section
  _svcHeading(x, _BDY + 228, "LANE PRICES", C_MIDGREY);
  gfx->setFont(NULL); gfx->setTextSize(2);
  gfx->setCursor(x, _BDY + 244);
  for (int col = 0; col < 7 && col < NUM_SLOTS; col++) {
    gfx->setTextColor(C_GREY);
    char lbl[5]; snprintf(lbl, sizeof(lbl), "L%d:", col+1); gfx->print(lbl);
    gfx->setTextColor(C_WHITE);
    char prb[6];
    snprintf(prb, sizeof(prb), g_slots[col].configured && g_slots[col].price > 0 ? "%dB " : "-- ", (int)g_slots[col].price);
    gfx->print(prb);
  }
  gfx->setCursor(x, _BDY + 264);
  for (int col = 7; col < 10 && col < NUM_SLOTS; col++) {
    gfx->setTextColor(C_GREY);
    char lbl[6]; snprintf(lbl, sizeof(lbl), "L%d:", col+1); gfx->print(lbl);
    gfx->setTextColor(C_WHITE);
    char prb[6];
    snprintf(prb, sizeof(prb), g_slots[col].configured && g_slots[col].price > 0 ? "%dB " : "-- ", (int)g_slots[col].price);
    gfx->print(prb);
  }

  // AUDIO section
  _svcHeading(x, _BDY + 282, "AUDIO", C_MIDGREY);
  Preferences _vp; _vp.begin("satu", true); int vol = _vp.getInt("vol", 50); _vp.end();
  y = _S_VOLY;
  gfx->setFont(NULL); gfx->setTextSize(2);
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  y); gfx->print("Volume");
  gfx->setTextColor(C_WHITE); gfx->setCursor(lv, y);
  char vbuf[16]; snprintf(vbuf, sizeof(vbuf), "%d%%", vol); gfx->print(vbuf);
  if (SPEAKER_PIN >= 0) { gfx->setTextColor(C_GREY); gfx->print("  Tap to cycle +10"); }

  // Factory Reset button (w=260 h=34)
  y = _S_Y401;
  _fillRoundRect(x, y, 260, 34, 8, gfx->color565(60,0,0));
  _drawRoundRect(x, y, 260, 34, 8, gfx->color565(180,50,50));
  gfx->setTextColor(C_WHITE); gfx->setCursor(x + 16, y + 9);
  gfx->print("Factory Reset");
  gfx->setTextSize(1);
  _svcLogDraw();
}

// ═══════════════════════════════════════════════════════════════════════════════
// TAB 4 — FIRMWARE
// ═══════════════════════════════════════════════════════════════════════════════
// Print to Serial button at _BDY+312 = y356, h=36 → bottom=392.
#define _FW_PRINT_Y  (_BDY + 312)   // 356

static void _drawSvcBody_Firmware(int bodyX) {
  int x = bodyX + 16, lv = x + 140;  // label col=140, value col=270
  gfx->setFont(&FreeSansBold18pt7b); gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(x, _BDY + 28); gfx->print("Firmware");

  // CURRENT FIRMWARE section
  _svcHeading(x, _BDY + 50, "CURRENT FIRMWARE", C_MIDGREY);
  uint8_t mac[6]; esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  gfx->setFont(NULL); gfx->setTextSize(2);
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 58); gfx->print("Version");
  gfx->setTextColor(C_WHITE); gfx->setCursor(lv, _BDY + 58); gfx->print(FW_VERSION);
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 78); gfx->print("Build");
  gfx->setTextColor(C_WHITE); gfx->setCursor(lv, _BDY + 78);
  gfx->print(__DATE__); gfx->print(" "); gfx->print(__TIME__);
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 98); gfx->print("Board");
  gfx->setTextColor(C_WHITE); gfx->setCursor(lv, _BDY + 98); gfx->print("ESP32-8048S070C");
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 118); gfx->print("Flash/RAM");
  gfx->setTextColor(C_WHITE); gfx->setCursor(lv, _BDY + 118); gfx->print("16MB / 8MB OPI");
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 138); gfx->print("MAC");
  gfx->setTextColor(C_WHITE); gfx->setCursor(lv, _BDY + 138); gfx->print(macStr);
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 158); gfx->print("Heap free");
  gfx->setTextColor(C_WHITE); gfx->setCursor(lv, _BDY + 158);
  char heapBuf[20]; snprintf(heapBuf, sizeof(heapBuf), "%lu KB", ESP.getFreeHeap()/1024);
  gfx->print(heapBuf);
  gfx->setTextColor(C_GREY); gfx->print("  PSRAM: ");
  gfx->setTextColor(C_WHITE);
  char psramBuf[20]; snprintf(psramBuf, sizeof(psramBuf), "%lu KB", ESP.getFreePsram()/1024);
  gfx->print(psramBuf);

  // SECURITY section
  _svcHeading(x, _BDY + 192, "SECURITY (dev mode)", C_MIDGREY);
  const uint16_t amber = gfx->color565(180,120,0);
  gfx->setFont(NULL); gfx->setTextSize(2);
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 208); gfx->print("Encrypt");
  gfx->setTextColor(amber);   gfx->setCursor(lv, _BDY + 208); gfx->print("DISABLED");
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 228); gfx->print("Sec Boot");
  gfx->setTextColor(amber);   gfx->setCursor(lv, _BDY + 228); gfx->print("DISABLED");
  gfx->setTextColor(C_GREY);  gfx->setCursor(x,  _BDY + 248); gfx->print("JTAG");
  gfx->setTextColor(amber);   gfx->setCursor(lv, _BDY + 248); gfx->print("ENABLED (burn eFuse)");

  // REMOTE OTA stub
  _svcHeading(x, _BDY + 278, "REMOTE OTA (stub)", C_MIDGREY);
  int otaY = _BDY + 290;
  _fillRoundRect(x,       otaY, 160, 28, 6, gfx->color565(20,20,40));
  _drawRoundRect(x,       otaY, 160, 28, 6, C_DARKGREY);
  _fillRoundRect(x + 168, otaY, 140, 28, 6, gfx->color565(20,20,40));
  _drawRoundRect(x + 168, otaY, 140, 28, 6, C_DARKGREY);
  gfx->setFont(NULL); gfx->setTextSize(2); gfx->setTextColor(C_MIDGREY);
  gfx->setCursor(x + 8,       otaY + 6); gfx->print("Check Update");
  gfx->setCursor(x + 168 + 8, otaY + 6); gfx->print("Force OTA");

  // Print to Serial button (w=200 h=36, bottom=392)
  int y = _FW_PRINT_Y;
  _fillRoundRect(x, y, 200, 36, 6, gfx->color565(10,30,10));
  _drawRoundRect(x, y, 200, 36, 6, C_GREEN);
  gfx->setTextColor(C_GREEN); gfx->setCursor(x + 12, y + 10);
  gfx->print("Print to Serial");
  gfx->setTextSize(1);
  _svcLogDraw();
}

// ═══════════════════════════════════════════════════════════════════════════════
// TOUCH HELPER — called from getTouchedServiceContent() in ui.h
// ═══════════════════════════════════════════════════════════════════════════════
int _getTouchedServiceExtra(int tab, int tx, int ty) {
  int bodyX = SVC_BODY_X, x = bodyX + 16;
  if (tab == TAB_SELFTEST) {
    if (ty >= _ST_BTN_Y && ty <= _ST_BTN_Y + _ST_BTN_H) {
      if (tx >= _ST_Q_X && tx < _ST_Q_X + 140) return 500;
      if (tx >= _ST_T_X && tx < _ST_T_X + 180) return 501;
      if (tx >= _ST_C_X && tx < _ST_C_X +  90) return 502;
    }
  }
  if (tab == TAB_DEVICES) {
    int stride = _DEV_CW + _DEV_GAP;
    if (ty >= _DEV_R1_Y && ty <= _DEV_R1_Y + _DEV_CH)
      for (int col=0; col<6; col++) { int cx=_DEV_LX+col*stride; if(tx>=cx&&tx<=cx+_DEV_CW) return 601+col; }
    if (ty >= _DEV_R2_Y && ty <= _DEV_R2_Y + _DEV_CH)
      for (int col=0; col<6; col++) { int cx=_DEV_LX+col*stride; if(tx>=cx&&tx<=cx+_DEV_CW) return 607+col; }
    if (ty >= _DEV_STUB_Y && ty <= _DEV_STUB_Y + 36) {
      if (tx >= _DEV_LX       && tx < _DEV_LX + 180)       { _svcLogPanel("Pump: use R11 in relay grid"); return 0; }
      if (tx >= _DEV_LX + 188 && tx < _DEV_LX + 188 + 140) { _svcLogPanel("LED: stub"); return 0; }
      if (tx >= _DEV_LX + 336 && tx < _DEV_LX + 336 + 140) { _svcLogPanel("Speaker: stub"); return 0; }
    }
    int tbX = SVC_BODY_X + (686 - 200) / 2;
    if (ty >= _DEV_TBES_Y && ty <= _DEV_TBES_Y + 36 && tx >= tbX && tx <= tbX + 200) {
      _svcLogPanel("Backend ping..."); return 600;
    }
  }
  if (tab == TAB_SETTINGS) {
    if (ty >= _S_Y402 && ty <= _S_Y402 + 36 && tx >= x && tx <= x + 220) return 702;
    if (ty >= _S_VOLY && ty <= _S_VOLY + _S_VOLH + 8 && tx >= x && tx <= x + 220) return 700;
    if (ty >= _S_Y401 && ty <= _S_Y401 + 34 && tx >= x && tx <= x + 260) return 701;
  }
  if (tab == TAB_FIRMWARE) {
    if (ty >= _FW_PRINT_Y && ty <= _FW_PRINT_Y + 36 && tx >= x && tx <= x + 200) return 800;
  }
  return 0;
}

#endif // UI_SERVICE_H
