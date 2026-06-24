#ifndef UI_SCREENS_H
#define UI_SCREENS_H

// ui_screens.h — Version R2 — 2026-06-24
// D-11: Welcome screen + bilingual sale screens. _drawStatusBar uses _sl() for TH/EN label.
//       Welcome screen: drawWelcomeScreen(), getTouchedWelcome(), _drawWelcomeLanguageButtons().
//       Bilingual: drawGiftOptionScreen, drawConfirmScreen, drawVendingScreen, drawCompletionScreen.
//       drawErrorScreen: EN only (operator-facing — no Thai needed).
// Previous: R1 — 2026-06-22 (EN only)
// Depends on: gfx, _touch, colors, SCR_W/H, STATUS_H, grid vars, SlotConfig, touchReadOnce,
//             StatusBarState/_sl()/_stateLabels (ui_strings.h), _fillRoundRect/_drawRoundRect,
//             printThai() (ui_strings.h), SarabanSubset_12/18/24pt (SarabanSubset.h).

// ============================================================
//  DRAW HELPERS
// ============================================================
static uint16_t _priceColor(int price) {
  if (price <= 50)  return C_PRICE_TEAL;
  if (price <= 100) return C_PRICE_GOLD;
  if (price <= 200) return C_PRICE_AMBER;
  if (price <= 300) return C_PRICE_ORANGE;
  return C_PRICE_DEEPORANGE;
}

// ============================================================
//  STATUS BAR
// ============================================================
static void _drawStatusBar(StatusBarState state) {
  gfx->fillRect(0, 0, SCR_W, STATUS_H, C_DARKGOLD);
  gfx->fillRect(0, STATUS_H - 3, SCR_W, 3, C_GOLD);

  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(10, 34);
  gfx->print("SATU");
  gfx->setFont(NULL); gfx->setTextSize(1);

  // D-11: _sl() returns TH or EN label based on g_lang_th
  const char* label = _stateLabels[state];  // EN fallback for pixel width estimate
  if (g_lang_th) {
    // Thai status bar: render at fixed position using printThai (bitmaps placeholder)
    printThai(SCR_W/2 - 60, 34, _stateLabels_TH[state], C_WHITE, &SarabanSubset_12pt);
  } else {
    gfx->setFont(&FreeSansBold12pt7b);
    gfx->setTextSize(1);
    int lw = strlen(label) * 9;
    gfx->setCursor(SCR_W/2 - lw/2, 34);
    gfx->setTextColor(C_WHITE);
    gfx->print(label);
    gfx->setFont(NULL); gfx->setTextSize(1);
  }

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
    gfx->setFont(&FreeSansBold12pt7b);
    gfx->setTextColor(C_MIDGREY); gfx->setTextSize(1);
    int lw = strlen(slotLabel) * 9;
    gfx->setCursor(x + CELL_W/2 - lw/2, y + CELL_H/2 + 7);
    gfx->print(slotLabel);
    gfx->setFont(NULL); gfx->setTextSize(1);
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
  gfx->setFont(&FreeSansBold24pt7b);
  gfx->setTextColor(priceColor); gfx->setTextSize(1);
  int pw  = strlen(priceBuf) * 20;
  int pY  = y + CELL_H/2 + 10;
  gfx->setCursor(x + CELL_W/2 - pw/2, pY);
  gfx->print(priceBuf);
  gfx->setFont(NULL); gfx->setTextSize(1);

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
//  SERVICE TAB BAR  (used by drawServiceScreen in ui.h)
// ============================================================
static void _drawSvcTabBar(int activeTab) {
  int tabH = (SCR_H - STATUS_H) / 5;  // 87px
  gfx->setFont(NULL); gfx->setTextSize(2);
  for (int i = 0; i < 5; i++) {
    int ty = STATUS_H + i * tabH;
    uint16_t bg = (i == activeTab) ? C_DARKGOLD : gfx->color565(20,16,30);
    uint16_t bd = (i == activeTab) ? C_GOLD : C_MIDGREY;
    gfx->fillRect(0, ty, SVC_TAB_W, tabH - 1, bg);
    gfx->drawRect(0, ty, SVC_TAB_W, tabH - 1, bd);
    gfx->setTextColor(i == activeTab ? C_BLACK : C_GREY);
    bool twoLine = (_svcTabL2[i][0] != '\0');
    if (twoLine) {
      int l1w = strlen(_svcTabL1[i]) * 12;
      int l2w = strlen(_svcTabL2[i]) * 12;
      int cy = ty + tabH/2 - 16;
      gfx->setCursor(SVC_TAB_W/2 - l1w/2, cy);
      gfx->print(_svcTabL1[i]);
      gfx->setCursor(SVC_TAB_W/2 - l2w/2, cy + 18);
      gfx->print(_svcTabL2[i]);
    } else {
      int lw = strlen(_svcTabL1[i]) * 12;
      gfx->setCursor(SVC_TAB_W/2 - lw/2, ty + tabH/2 - 8);
      gfx->print(_svcTabL1[i]);
    }
  }
}

// ============================================================
//  DRAW BOOT SCREEN
// ============================================================
void drawBootScreen(String status) {
  gfx->fillScreen(C_BG);

  gfx->setFont(&FreeSansBold24pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  gfx->setCursor(SCR_W/2 - 40, SCR_H/2 - 34);
  gfx->print("SATU");
  gfx->setFont(NULL); gfx->setTextSize(1);

  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GOLD_DIM); gfx->setTextSize(1);
  const char* tag = "Merit Donation System";
  int tw = strlen(tag) * 9;
  gfx->setCursor(SCR_W/2 - tw/2, SCR_H/2 + 4);
  gfx->print(tag);
  gfx->setFont(NULL); gfx->setTextSize(1);

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

  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  gfx->setCursor(10, 34);
  gfx->print("SATU");
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_ORANGE);
  gfx->setTextSize(1);
  gfx->setCursor(SCR_W/2 - 60, 8);
  gfx->print("SETUP MODE");

  int midY = SCR_H / 2 - 60;

  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  const char* prompt = "Enter this code in the admin panel:";
  int pw = strlen(prompt) * 9;
  gfx->setCursor(SCR_W/2 - pw/2, midY + 13);
  gfx->print(prompt);
  gfx->setFont(NULL); gfx->setTextSize(1);

  // Code hero
  gfx->setFont(&FreeSansBold24pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(2);
  int cw = code.length() * 30;
  gfx->setCursor(SCR_W/2 - cw/2, midY + 88);
  gfx->print(code.c_str());
  gfx->setFont(NULL); gfx->setTextSize(1);

  // Countdown
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  String cdLabel = "Retrying in " + countdown;
  int cdw = cdLabel.length() * 9;
  gfx->setCursor(SCR_W/2 - cdw/2, midY + 121);
  gfx->print(cdLabel.c_str());
  gfx->setFont(NULL); gfx->setTextSize(1);

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
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_ORANGE); gfx->setTextSize(1);
  gfx->setCursor(10, 25);
  gfx->print("DEBUG");
  gfx->setFont(NULL); gfx->setTextSize(1);

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
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_BLACK); gfx->setTextSize(1);
  const char* msg = "Tap again to confirm  |  Wait to cancel";
  int mw = strlen(msg) * 9;
  gfx->setCursor(SCR_W/2 - mw/2, barY + 26);
  gfx->print(msg);
  gfx->setFont(NULL); gfx->setTextSize(1);

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

  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  const char* title = STR_GIFT_TITLE_EN;
  int tw = strlen(title) * 12;
  if (g_lang_th) {
    printThai(SCR_W/2 - 90, STATUS_H + 38, STR_GIFT_TITLE_TH, C_GOLD, &SarabanSubset_18pt);
  } else {
    gfx->setCursor(SCR_W/2 - tw/2, STATUS_H + 38);
    gfx->print(title);
  }
  gfx->setFont(NULL); gfx->setTextSize(1);

  int cardW = 280, cardH = 200;
  int cardY = STATUS_H + 60;
  int cardAX = SCR_W/2 - cardW - 30;
  int cardBX = SCR_W/2 + 30;

  _fillRoundRect(cardAX, cardY, cardW, cardH, 12, gfx->color565(20, 18, 35));
  _drawRoundRect(cardAX, cardY, cardW, cardH, 12, gfx->color565(50, 40, 80));
  gfx->fillRect(cardAX + cardW/2 - 20, cardY + 30, 40, 40, gfx->color565(140, 90, 30));
  gfx->drawRect(cardAX + cardW/2 - 20, cardY + 30, 40, 40, C_GOLD);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  const char* labelA = STR_GIFT_ITEM_EN;
  if (g_lang_th) {
    printThai(cardAX + cardW/2 - 55, cardY + 90, STR_GIFT_ITEM_TH, C_GOLD, &SarabanSubset_12pt);
  } else {
    gfx->setCursor(cardAX + cardW/2 - strlen(labelA)*9/2, cardY + 90);
    gfx->print(labelA);
  }
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  const char* subA = "Receive your donation item";
  gfx->setCursor(cardAX + cardW/2 - strlen(subA)*6/2, cardY + 116);
  gfx->print(subA);
  char priceA[16]; snprintf(priceA, 16, "%d THB", s.price);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(_priceColor(s.price)); gfx->setTextSize(1);
  gfx->setCursor(cardAX + cardW/2 - strlen(priceA)*9/2, cardY + 148);
  gfx->print(priceA);
  gfx->setFont(NULL); gfx->setTextSize(1);

  if (g_cfg_water) {
    _fillRoundRect(cardBX, cardY, cardW, cardH, 12, gfx->color565(10, 18, 35));
    for (int t = 0; t < 2; t++)
      _drawRoundRect(cardBX+t, cardY+t, cardW-t*2, cardH-t*2, 12-t, C_GOLD);
    _drawRoundRect(cardBX-1, cardY-1, cardW+2, cardH+2, 13, gfx->color565(100, 80, 20));
    int wx = cardBX + cardW/2;
    gfx->fillCircle(wx - 8, cardY + 46, 10, gfx->color565(79, 195, 247));
    gfx->fillCircle(wx + 8, cardY + 52, 12, gfx->color565(33, 150, 243));
    gfx->setFont(&FreeSansBold12pt7b);
    gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
    const char* labelB = STR_GIFT_WATER_EN;
    if (g_lang_th) {
      printThai(cardBX + cardW/2 - 40, cardY + 90, STR_GIFT_WATER_TH, C_GOLD, &SarabanSubset_12pt);
    } else {
      gfx->setCursor(cardBX + cardW/2 - strlen(labelB)*9/2, cardY + 90);
      gfx->print(labelB);
    }
    gfx->setFont(NULL); gfx->setTextSize(1);
    gfx->setTextColor(C_GREY); gfx->setTextSize(1);
    const char* subB = "Add sacred water blessing";
    gfx->setCursor(cardBX + cardW/2 - strlen(subB)*6/2, cardY + 116);
    gfx->print(subB);
    char priceB[20]; snprintf(priceB, 20, "%d+20 THB", s.price);
    gfx->setFont(&FreeSansBold12pt7b);
    gfx->setTextColor(C_PRICE_ORANGE); gfx->setTextSize(1);
    gfx->setCursor(cardBX + cardW/2 - strlen(priceB)*9/2, cardY + 148);
    gfx->print(priceB);
    gfx->setFont(NULL); gfx->setTextSize(1);
  }

  // R-153: Back button — bottom centre, returns to product selection
  int backBtnX = SCR_W/2 - 100, backBtnY = STATUS_H + 278, backBtnW = 200, backBtnH = 48;
  _fillRoundRect(backBtnX, backBtnY, backBtnW, backBtnH, 8, gfx->color565(25, 20, 40));
  _drawRoundRect(backBtnX, backBtnY, backBtnW, backBtnH, 8, C_MIDGREY);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_MIDGREY); gfx->setTextSize(1);
  if (g_lang_th) {
    printThai(backBtnX + backBtnW/2 - 40, backBtnY + 30, STR_BACK_TH, C_MIDGREY, &SarabanSubset_12pt);
  } else {
    gfx->setCursor(backBtnX + backBtnW/2 - 28, backBtnY + 30);
    gfx->print(STR_BACK_EN);
  }
  gfx->setFont(NULL); gfx->setTextSize(1);

  Serial.printf("[UI] Gift option screen: slot %d\n", slotIdx);
}

// ============================================================
//  SHOW PAYMENT ACCEPTED BANNER  (R-131: 1.5s green overlay on QR screen)
// ============================================================
void showPaymentAccepted() {
  int bY = SCR_H - 120;
  gfx->fillRect(0, bY, SCR_W, 120, gfx->color565(10, 80, 20));
  gfx->drawRect(0, bY, SCR_W, 120, C_GREEN);
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_GREEN); gfx->setTextSize(1);
  gfx->setCursor(SCR_W/2 - 140, bY + 52);
  gfx->print("Payment Accepted");
  gfx->setFont(NULL); gfx->setTextSize(1);
  gfx->setTextColor(C_WHITE);
  const char* sub = "Dispensing your item...";
  gfx->setCursor(SCR_W/2 - (int)(strlen(sub)*6/2), bY + 82);
  gfx->print(sub);
  unsigned long t0 = millis();
  while (millis() - t0 < 1500) { _touch.read(); delay(16); }
  Serial.println("[UI] Payment accepted banner shown");
}

// ============================================================
//  DRAW QR SCREEN  (R4: fetches PNG · R-117: pause-decode-resume for PSRAM DMA contention)
// ============================================================
void drawQrScreen(String qrUrl, int amount, int slotIdx) {
  g_idleDrawn     = false;
  g_qrSecondsLeft = 120;

  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_PAYMENT);

  SlotConfig& s = (slotIdx >= 0 && slotIdx < NUM_SLOTS)
                  ? g_slots[slotIdx] : g_slots[0];

  int lx = 30;

  char amtBuf[12]; snprintf(amtBuf, 12, "B%d", amount);
  gfx->setFont(&FreeSansBold24pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  gfx->setCursor(lx, STATUS_H + 44);
  gfx->print(amtBuf);
  gfx->setFont(NULL); gfx->setTextSize(1);

  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_MIDGREY); gfx->setTextSize(1);
  gfx->setCursor(lx, STATUS_H + 95);
  gfx->print(s.name_en);
  gfx->setFont(NULL); gfx->setTextSize(1);

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

  // Fetch and render PNG QR — R-117 pause-decode-resume handles PSRAM DMA contention
  if (g_pngBuf && !qrUrl.isEmpty()) {
    Serial.printf("[UI] QR PNG URL: %s\n", qrUrl.c_str());

    size_t pngLen = fetchImageBytes(qrUrl, g_pngBuf, 200 * 1024);
    if (pngLen > 8) {
      Serial.printf("[UI] QR PNG loaded: %u bytes — rendering\n", pngLen);
      gfx->fillRect(qrAreaX, qrAreaY, 244, 244, C_WHITE);
      drawQrFromBytes(g_pngBuf, pngLen, qrAreaX + 2, qrAreaY + 2);
    } else {
      Serial.println("[UI] QR PNG fetch failed — showing fallback");
      gfx->fillRect(qrAreaX, qrAreaY, 244, 244, C_WHITE);
      gfx->setTextColor(C_GREY);
      gfx->setTextSize(1);
      gfx->setCursor(qrAreaX + 50, qrAreaY + 118);
      gfx->print("QR unavailable");
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
  gfx->fillRect(lx, ty, 120, 22, C_BG);

  int m = secondsLeft / 60;
  int s = secondsLeft % 60;
  char buf[8]; snprintf(buf, 8, "%d:%02d", m, s);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(secondsLeft < 30 ? C_RED : C_ORANGE); gfx->setTextSize(1);
  gfx->setCursor(lx, ty + 13);
  gfx->print(buf);
  gfx->setFont(NULL); gfx->setTextSize(1);

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

  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  if (g_lang_th) {
    printThai(SCR_W/2 - 110, 150, STR_VEND_TITLE_TH, C_GOLD, &SarabanSubset_18pt);
  } else {
    const char* vendTitle = STR_VEND_TITLE_EN;
    gfx->setCursor(SCR_W/2 - 85, 150);
    gfx->print(vendTitle);
  }
  gfx->setFont(NULL); gfx->setTextSize(1);

  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  // Slot name: use TH name if available and TH mode
  const char* slotName = (g_lang_th && s.name_th[0] != '\0') ? s.name_th : s.name_en;
  gfx->setCursor(SCR_W/2 - strlen(slotName)*12/2, 230);
  gfx->print(slotName);
  gfx->setFont(NULL); gfx->setTextSize(1);

  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  if (g_lang_th) {
    printThai(SCR_W/2 - 80, 283, STR_VEND_WAIT_TH, C_GREY, &SarabanSubset_12pt);
  } else {
    const char* sub = STR_VEND_WAIT_EN;
    gfx->setCursor(SCR_W/2 - strlen(sub)*9/2, 283);
    gfx->print(sub);
  }
  gfx->setFont(NULL); gfx->setTextSize(1);

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

  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  if (g_lang_th) {
    printThai(SCR_W/2 - 80, STATUS_H + 41, STR_DONE_LUCKY_TH, C_GOLD, &SarabanSubset_12pt);
  } else {
    const char* th1 = STR_DONE_LUCKY_EN;
    gfx->setCursor(SCR_W/2 - (int)strlen(th1)*10/2, STATUS_H + 41);
    gfx->print(th1);
  }
  gfx->setFont(NULL); gfx->setTextSize(1);

  char lnBuf[8]; snprintf(lnBuf, 8, "%d", luckyNumber);
  gfx->setFont(&FreeSansBold24pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(2);
  gfx->setCursor(SCR_W/2 - (int)strlen(lnBuf)*36/2, SCR_H/2 - 8);
  gfx->print(lnBuf);
  gfx->setFont(NULL); gfx->setTextSize(1);

  gfx->setTextColor(C_GOLD_DIM); gfx->setTextSize(1);
  if (g_lang_th) {
    printThai(SCR_W/2 - 80, SCR_H/2 + 44, STR_DONE_BLESS_TH, C_GOLD_DIM, &SarabanSubset_12pt);
  } else {
    const char* bless = STR_DONE_BLESS_EN;
    gfx->setCursor(SCR_W/2 - strlen(bless)*6/2, SCR_H/2 + 44);
    gfx->print(bless);
  }

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
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_BLACK); gfx->setTextSize(1);
  if (g_lang_th) {
    printThai(SCR_W/2 - 100, footY + 30, STR_DONE_THANKS_TH, C_BLACK, &SarabanSubset_12pt);
  } else {
    const char* thanks = STR_DONE_THANKS_EN;
    gfx->setCursor(SCR_W/2 - strlen(thanks)*9/2, footY + 21);
    gfx->print(thanks);
    gfx->setFont(NULL); gfx->setTextSize(1);
    gfx->setTextSize(1);
    const char* subThanks = "Good deeds bring good returns";
    gfx->setCursor(SCR_W/2 - strlen(subThanks)*6/2, footY + 32);
    gfx->print(subThanks);
  }
  gfx->setFont(NULL); gfx->setTextSize(1);

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
  gfx->setFont(&FreeSansBold24pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(SCR_W/2 - 14, 178);
  gfx->print("X");
  gfx->setFont(NULL); gfx->setTextSize(1);

  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  int lineY = 240;
  String msg = message;
  msg.replace("\\n", "\n");
  int start = 0;
  while (start < (int)msg.length()) {
    int nl  = msg.indexOf('\n', start);
    int end = (nl == -1) ? msg.length() : nl;
    String line = msg.substring(start, min(end, start + 42));
    gfx->setCursor(SCR_W/2 - (int)(line.length()*9)/2, lineY + 13);
    gfx->print(line.c_str());
    lineY += 28;
    start = end + 1;
    if (start >= (int)msg.length()) break;
    if (lineY > SCR_H - 40) break;
  }
  gfx->setFont(NULL); gfx->setTextSize(1);

  Serial.printf("[UI] Error: %s\n", message.c_str());
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
  touchReadOnce();  // R-150: shared read — see touchReadOnce() in ui.h
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
//  GET TOUCHED GIFT OPTION — 0=item only, 1=water, 2=Back, -1=none
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
  // R-151: use module-level _lastGiftTouchMs (reset by setState on STATE_GIFT_OPTION entry)
  if (millis() - _lastGiftTouchMs < 80) return -1;
  if (ty >= cardY && ty <= cardY + cardH) {
    if (tx >= cardAX && tx <= cardAX + cardW) {
      _lastGiftTouchMs = millis();
      Serial.println("[UI] Gift touch: Item Only");
      return 0;
    }
    if (tx >= cardBX && tx <= cardBX + cardW) {
      _lastGiftTouchMs = millis();
      Serial.println("[UI] Gift touch: +Sacred Water");
      return 1;
    }
  }
  // R-153: Back button — bottom centre
  int backBtnX = SCR_W/2 - 100, backBtnY = STATUS_H + 278, backBtnW = 200, backBtnH = 48;
  if (tx >= backBtnX && tx <= backBtnX + backBtnW &&
      ty >= backBtnY && ty <= backBtnY + backBtnH) {
    _lastGiftTouchMs = millis();
    Serial.println("[UI] Gift touch: Back");
    return 2;
  }
  return -1;
}

// ============================================================
//  DRAW CONFIRM SCREEN  (R-153: order summary before createOrder())
// ============================================================
void drawConfirmScreen(int slotIdx, bool wantWater) {
  g_idleDrawn = false;
  gfx->fillScreen(C_BG);
  _drawStatusBar(SB_CONFIRM);

  SlotConfig& s = g_slots[slotIdx];
  int total = s.price + (wantWater ? 20 : 0);

  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  const char* title = STR_CONF_TITLE_EN;
  if (g_lang_th) {
    printThai(SCR_W/2 - 100, STATUS_H + 42, STR_CONF_TITLE_TH, C_GOLD, &SarabanSubset_18pt);
  } else {
    int tw = strlen(title) * 12;
    gfx->setCursor(SCR_W/2 - tw/2, STATUS_H + 42);
    gfx->print(title);
  }
  gfx->setFont(NULL); gfx->setTextSize(1);

  int boxW = 500, boxH = 200;
  int boxX = (SCR_W - boxW) / 2;
  int boxY = STATUS_H + 60;
  _fillRoundRect(boxX, boxY, boxW, boxH, 12, gfx->color565(20, 18, 35));
  _drawRoundRect(boxX, boxY, boxW, boxH, 12, C_GOLD);

  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  gfx->setCursor(boxX + 24, boxY + 38);
  gfx->print(s.name_en);
  gfx->setFont(NULL); gfx->setTextSize(1);

  gfx->setTextColor(wantWater ? gfx->color565(79, 195, 247) : C_MIDGREY);
  gfx->setCursor(boxX + 24, boxY + 72);
  gfx->print(wantWater ? "+ Sacred Water blessing" : "Item only (no water)");

  gfx->drawFastHLine(boxX + 16, boxY + 100, boxW - 32, gfx->color565(60, 50, 90));

  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  char totalBuf[24]; snprintf(totalBuf, 24, "Total: %d THB", total);
  int totalW = strlen(totalBuf) * 12;
  gfx->setCursor(SCR_W/2 - totalW/2, boxY + 152);
  gfx->print(totalBuf);
  gfx->setFont(NULL); gfx->setTextSize(1);

  int btnY = boxY + boxH + 24;
  int btnW = 220, btnH = 52;
  int backX = SCR_W/2 - 260;
  int confX = SCR_W/2 + 40;

  _fillRoundRect(backX, btnY, btnW, btnH, 10, gfx->color565(25, 20, 40));
  _drawRoundRect(backX, btnY, btnW, btnH, 10, C_MIDGREY);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_MIDGREY); gfx->setTextSize(1);
  if (g_lang_th) {
    printThai(backX + btnW/2 - 40, btnY + 30, STR_BACK_TH, C_MIDGREY, &SarabanSubset_12pt);
  } else {
    gfx->setCursor(backX + btnW/2 - 28, btnY + 30);
    gfx->print(STR_BACK_EN);
  }
  gfx->setFont(NULL); gfx->setTextSize(1);

  _fillRoundRect(confX, btnY, btnW, btnH, 10, gfx->color565(20, 60, 20));
  for (int t = 0; t < 2; t++)
    _drawRoundRect(confX+t, btnY+t, btnW-t*2, btnH-t*2, 10-t, C_GREEN);
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GREEN); gfx->setTextSize(1);
  if (g_lang_th) {
    printThai(confX + btnW/2 - 42, btnY + 30, STR_CONF_BTN_TH, C_GREEN, &SarabanSubset_12pt);
  } else {
    gfx->setCursor(confX + btnW/2 - 42, btnY + 30);
    gfx->print(STR_CONF_BTN_EN);
  }
  gfx->setFont(NULL); gfx->setTextSize(1);

  Serial.printf("[UI] Confirm screen: slot %d water=%d total=%d\n", slotIdx, wantWater, total);
}

// ============================================================
//  GET TOUCHED CONFIRM — 1=Confirm, -1=Back, 0=none
// ============================================================
int getTouchedConfirm() {
  _touch.read();
  if (!_touch.isTouched) return 0;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;
  if (millis() - _lastConfirmTouchMs < 80) return 0;

  int boxW = 500, boxH = 200;
  int boxY = STATUS_H + 60;
  int btnY = boxY + boxH + 24;
  int btnW = 220, btnH = 52;
  int backX = SCR_W/2 - 260;
  int confX = SCR_W/2 + 40;

  if (ty >= btnY && ty <= btnY + btnH) {
    if (tx >= backX && tx <= backX + btnW) {
      _lastConfirmTouchMs = millis();
      Serial.println("[UI] Confirm touch: Back");
      return -1;
    }
    if (tx >= confX && tx <= confX + btnW) {
      _lastConfirmTouchMs = millis();
      Serial.println("[UI] Confirm touch: Confirm");
      return 1;
    }
  }
  return 0;
}

// ============================================================
//  SERVICE GESTURE CHECK — 3 taps top-right within 2s
// ============================================================
bool checkServiceGesture() {
  static int tapCount = 0;
  static unsigned long firstTapMs = 0;
  static unsigned long lastSvcTap = 0;

  touchReadOnce();  // R-150: shared read — same tick as getTouchedSlot in STATE_IDLE
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
//  WELCOME SCREEN  (D-11: shown on every boot before STATE_IDLE)
// ============================================================

// Welcome screen layout constants
#define _WEL_BTN_W  120
#define _WEL_BTN_H   60
#define _WEL_BTN_Y  280
#define _WEL_EN_X   (SCR_W/2 - _WEL_BTN_W - 20)  // = 260
#define _WEL_TH_X   (SCR_W/2 + 20)                // = 420

static void _drawWelcomeLanguageButtons(bool highlightTH) {
  uint16_t enBg = highlightTH ? gfx->color565(25, 20, 40) : gfx->color565(60, 40, 10);
  uint16_t thBg = highlightTH ? gfx->color565(60, 40, 10) : gfx->color565(25, 20, 40);
  uint16_t enBd = highlightTH ? C_MIDGREY : C_GOLD;
  uint16_t thBd = highlightTH ? C_GOLD    : C_MIDGREY;

  _fillRoundRect(_WEL_EN_X, _WEL_BTN_Y, _WEL_BTN_W, _WEL_BTN_H, 10, enBg);
  for (int t = 0; t < 2; t++)
    _drawRoundRect(_WEL_EN_X + t, _WEL_BTN_Y + t, _WEL_BTN_W - t*2, _WEL_BTN_H - t*2, 10 - t, enBd);
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(highlightTH ? C_MIDGREY : C_GOLD);
  gfx->setTextSize(1);
  gfx->setCursor(_WEL_EN_X + _WEL_BTN_W/2 - 22, _WEL_BTN_Y + 38);
  gfx->print("EN");
  gfx->setFont(NULL); gfx->setTextSize(1);

  _fillRoundRect(_WEL_TH_X, _WEL_BTN_Y, _WEL_BTN_W, _WEL_BTN_H, 10, thBg);
  for (int t = 0; t < 2; t++)
    _drawRoundRect(_WEL_TH_X + t, _WEL_BTN_Y + t, _WEL_BTN_W - t*2, _WEL_BTN_H - t*2, 10 - t, thBd);
  gfx->setFont(&FreeSansBold18pt7b);
  gfx->setTextColor(highlightTH ? C_GOLD : C_MIDGREY);
  gfx->setTextSize(1);
  gfx->setCursor(_WEL_TH_X + _WEL_BTN_W/2 - 22, _WEL_BTN_Y + 38);
  gfx->print("TH");
  gfx->setFont(NULL); gfx->setTextSize(1);
}

void drawWelcomeScreen() {
  g_idleDrawn = false;
  gfx->fillScreen(C_BG);

  // Gold border
  for (int t = 0; t < 3; t++)
    gfx->drawRect(t, t, SCR_W - t*2, SCR_H - t*2, C_DARKGOLD);

  // Brand title
  gfx->setFont(&FreeSansBold24pt7b);
  gfx->setTextColor(C_GOLD); gfx->setTextSize(1);
  gfx->setCursor(SCR_W/2 - 68, 120);
  gfx->print(STR_WEL_TITLE_EN);
  gfx->setFont(NULL); gfx->setTextSize(1);

  // EN subtitle
  gfx->setFont(&FreeSansBold12pt7b);
  gfx->setTextColor(C_GOLD_DIM); gfx->setTextSize(1);
  const char* subEN = STR_WEL_SUB_EN;
  gfx->setCursor(SCR_W/2 - (int)(strlen(subEN)*9)/2, 165);
  gfx->print(subEN);
  gfx->setFont(NULL); gfx->setTextSize(1);

  // TH subtitle (placeholder — bitmaps zero until fontconvert)
  printThai(SCR_W/2 - 80, 205, STR_WEL_SUB_TH, C_GOLD_DIM, &SarabanSubset_12pt);

  // Prompt line EN
  gfx->setTextColor(C_WHITE); gfx->setTextSize(1);
  const char* promptEN = STR_WEL_PROMPT_EN;
  gfx->setCursor(SCR_W/2 - (int)(strlen(promptEN)*6)/2, 246);
  gfx->print(promptEN);

  // Prompt line TH (placeholder)
  printThai(SCR_W/2 - 70, 266, STR_WEL_PROMPT_TH, C_WHITE, &SarabanSubset_12pt);

  // Language buttons — no highlight on initial draw
  _drawWelcomeLanguageButtons(false);

  // Divider
  gfx->drawFastHLine(SCR_W/4, _WEL_BTN_Y - 12, SCR_W/2, C_DARKGOLD);

  // Bottom hint
  gfx->setTextColor(C_GREY); gfx->setTextSize(1);
  const char* hint = "Tap anywhere to continue with EN";
  gfx->setCursor(SCR_W/2 - (int)(strlen(hint)*6)/2, SCR_H - 20);
  gfx->print(hint);

  Serial.println("[UI] Welcome screen drawn");
}

// getTouchedWelcome() — returns:
//   1 = EN button tapped   (sets g_lang_th = false, redraws buttons)
//   2 = TH button tapped   (sets g_lang_th = true,  redraws buttons)
//   0 = anywhere else tapped (caller proceeds to STATE_IDLE with current g_lang_th)
//  -1 = no touch
int getTouchedWelcome() {
  touchReadOnce();
  if (!_touch.isTouched) return -1;
  int tx = _touch.points[0].x;
  int ty = _touch.points[0].y;

  static unsigned long _lastWelMs = 0;
  if (millis() - _lastWelMs < 200) return -1;
  _lastWelMs = millis();

  // EN button
  if (tx >= _WEL_EN_X && tx <= _WEL_EN_X + _WEL_BTN_W &&
      ty >= _WEL_BTN_Y && ty <= _WEL_BTN_Y + _WEL_BTN_H) {
    g_lang_th = false;
    _drawWelcomeLanguageButtons(false);
    Serial.println("[UI] Welcome: EN selected");
    return 1;
  }

  // TH button
  if (tx >= _WEL_TH_X && tx <= _WEL_TH_X + _WEL_BTN_W &&
      ty >= _WEL_BTN_Y && ty <= _WEL_BTN_Y + _WEL_BTN_H) {
    g_lang_th = true;
    _drawWelcomeLanguageButtons(true);
    Serial.println("[UI] Welcome: TH selected");
    return 2;
  }

  // Anywhere else — proceed with current language
  Serial.printf("[UI] Welcome: proceed (lang=%s)\n", g_lang_th ? "TH" : "EN");
  return 0;
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
// ============================================================
void idleAnimationUI() {
  for (int i = 0; i < 2; i++) {
    for (int t = 0; t < 3; t++) {
      gfx->drawRect(t, t, SCR_W - t*2, SCR_H - t*2, C_GOLD);
    }
    unsigned long t0 = millis();
    while (millis() - t0 < 120) {
      _touch.read();
      if (_touch.isTouched) return;
      delay(16);
    }
    gfx->fillRect(0, 0, SCR_W, 4, C_BG);
    gfx->fillRect(0, SCR_H-4, SCR_W, 4, C_BG);
    gfx->fillRect(0, 0, 4, SCR_H, C_BG);
    gfx->fillRect(SCR_W-4, 0, 4, SCR_H, C_BG);
    t0 = millis();
    while (millis() - t0 < 80) {
      _touch.read();
      if (_touch.isTouched) return;
      delay(16);
    }
  }
}

#endif // UI_SCREENS_H
