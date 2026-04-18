#ifndef UI_H
#define UI_H

#include <TFT_eSPI.h>

extern TFT_eSPI tft;

void initUI();
void drawIdleScreen();
void drawProductSelection();
void drawQrScreen(String qrData, int amount);
void drawVendingScreen(int product);
void drawCompletionScreen();
void drawErrorScreen(String message);
int getTouchedProduct();  // Returns 0-9 or -1

#endif
