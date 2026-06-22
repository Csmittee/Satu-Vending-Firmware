#ifndef UI_STRINGS_H
#define UI_STRINGS_H

// ui_strings.h — Version R1 — 2026-06-22
// Split from ui.h R5. Text literals and enums used across screen functions.
// EN only. #ifdef LANG_TH stub below — D-11 will add Thai language strings.

// #define LANG_TH   // uncomment in D-11 to enable Thai language

// ============================================================
//  STATUS BAR STATE
// ============================================================
enum StatusBarState { SB_IDLE=0, SB_CONFIRM, SB_PAYMENT, SB_DISPENSING, SB_CONNECTING };

static const char* _stateLabels[] = {
  "Select Item",
  "Confirm",
  "Payment",
  "Dispensing",
  "Connecting..."
};

// ============================================================
//  SERVICE TAB LABELS
// ============================================================
static const char* _svcTabL1[5] = { "Self",  "Free", "Devices", "Settings", "Firmware" };
static const char* _svcTabL2[5] = { "Test",  "Play", "",        "",         "" };

#endif // UI_STRINGS_H
