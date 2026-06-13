# CC_PROMPT_hotfix_keyboard_dot.md
> Archived: 2026-06-13
> Status: DONE — committed to claude/loving-bohr-t3n3yf

## Problem
WiFi setup screen keyboard had no dot (.) character.
Owner cannot type SSID: Jaydahome2.4G — blocks all WiFi connection.

## Fix
In firmware/ui.h — `drawWifiSetupScreen()` keyboard Row 4:
- Shrunk CAPS: 100px → 80px
- Added 4 symbol keys (. @ - _) at 48px each between CAPS and SPACE
- SPACE shrunk: 310px → 186px (still usable)
- DEL moved to x=524, w=80
- CONNECT moved to x=608, w=150
- New constants: `_WKB4_SYM_X=126`, `_WKB4_SYM_W=48`
- `_wkbDrawKeys()`: loop draws 4 symbol keys with gold border
- `_wkbGetKey()`: loop detects symbol key taps, returns ASCII value directly
- Main loop already handles char values ≥32 as regular characters — no change needed

## Files Changed
- `firmware/ui.h` — Row 4 `#define` block, `_wkbDrawKeys()`, `_wkbGetKey()`
