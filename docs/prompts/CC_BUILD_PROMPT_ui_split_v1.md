# CC_BUILD_PROMPT_ui_split_v1.md
> ✅ COMPLETE — 2026-06-22 — D-10 ui.h split (4 files: ui.h R6, ui_strings.h R1, ui_keyboard.h R1, ui_screens.h R1)
> Version 1.0 — 2026-06-22
> Task: D-10 — Split ui.h (1819 lines) into ui.h + ui_strings.h + ui_keyboard.h + ui_screens.h
> Repo: firmware
> Flash cycles: 1 (pure refactor — zero functional change)
> PAYMENT_MODE: fake — never change

---

## 1. CC INTRO

Read and execute: CC_BUILD_PROMPT_ui_split_v1.md

Before touching any file:
1. Read CLAUDE.md + RULES.md + CC_SKILL.md
2. Read firmware/ui.h in full — this is your source of truth, not this prompt
3. Read firmware/ui_service.h in full
4. Read firmware/satu_vending.ino — confirm it includes only `#include "ui.h"`
5. Confirm ui.h current version is R5 — if different, pause and flag

This is a PURE REFACTOR. Zero functional change. Zero symbol moves that haven't been traced.
CI green = proof the split is correct. If CI fails, the split has a dependency error — fix it.

---

## 2. OBJECTIVE

ui.h is currently 1819 lines. Split into 4 files following the architecture decided 2026-06-22.
`satu_vending.ino` must not change — it still sees only `#include "ui.h"`.

**Target file map:**

| File | Target lines | Contents |
|------|-------------|----------|
| `ui.h` | ~200 | HW objects, display init, bus, touch, touchReadOnce, color palette, screen dimensions, grid vars, debounce statics, initUI(), recalcGrid(), _initDefaultSlots() |
| `ui_strings.h` | ~100 | All string/text literals used across screen functions — EN only now, #ifdef LANG_TH stub for D-11 |
| `ui_keyboard.h` | ~200 | WiFi setup keyboard + PIN numpad overlay — self-contained, no callers elsewhere in ui.h today |
| `ui_screens.h` | ~1100 | All customer-facing screen draw functions + touch functions (drawIdleScreen, drawQRScreen, drawSlotCell, getTouchedSlot, drawGiftOptionScreen, getTouchedGiftOption, drawConfirmScreen, drawPaymentScreen, drawDispensingScreen, drawCompletionScreen, drawSetupCodeScreen, drawPinOverlay, _drawStatusBar, _priceColor, _initDefaultSlots helpers) |
| `ui_service.h` | ~500 | Already split — DO NOT TOUCH |

---

## 3. NEW FILES TO CREATE

**firmware/ui_strings.h** — new file  
**firmware/ui_keyboard.h** — new file  
**firmware/ui_screens.h** — new file  
**firmware/ui.h** — rewrite (trim to HW primitives + include chain)

**KNOWLEDGE_MAP.md** must be updated — add ui_strings.h, ui_keyboard.h, ui_screens.h to firmware file list.

---

## 4. INCLUDE CHAIN — NON-NEGOTIABLE ORDER

ui.h is the only file `satu_vending.ino` includes. ui.h includes the rest in this order:

```cpp
// At bottom of ui.h, after all HW definitions:
#include "ui_strings.h"    // no deps — text literals only
#include "ui_keyboard.h"   // depends on: gfx, _touch, colors, SCR_W/H, touchReadOnce
#include "ui_screens.h"    // depends on: gfx, _touch, colors, SCR_W/H, grid vars, strings
#include "ui_service.h"    // depends on: all of the above (unchanged)
```

**Rule: no circular includes. No file includes ui.h. Each file assumes its deps are already defined above it in the chain.**

---

## 5. SPLIT RULES — READ BEFORE MOVING ANYTHING

### 5.1 Trace before move
Before moving any symbol (function, variable, #define, struct), CC must:
1. Search all 6 firmware files for every reference to that symbol
2. Confirm the file where it lands can see all its dependencies via the include chain
3. If any dependency is unclear — leave the symbol in ui.h and add a comment explaining why

### 5.2 Static vs global
- `static` variables that are referenced only within one file: move with that file
- `static` variables referenced across files: leave in ui.h (they resolve via include chain)
- Global `int g_grid_rows`, `int g_grid_cols`, `int CELL_W`, `int CELL_H` — stay in ui.h (shared with network.h)

### 5.3 hardware.h is LOCKED
Do not open, read, or touch hardware.h. R2 LOCKED.

### 5.4 satu_vending.ino is LOCKED
Do not change a single character. It still includes only `#include "ui.h"`.

### 5.5 ui_service.h is LOCKED
Do not touch. It already includes its own dependencies by sitting last in the chain.

### 5.6 Version headers
Every new file gets a version header:
```cpp
// ui_strings.h — Version R1 — 2026-06-22
// Split from ui.h R5. Text literals only. No Thai yet — D-11 will add #ifdef LANG_TH.
```

ui.h gets bumped: R5 → R6.

### 5.7 Include guards
Every new file must have `#ifndef` / `#define` / `#endif` guards.

### 5.8 #ifdef LANG_TH stub in ui_strings.h
Add at top of ui_strings.h:
```cpp
// #define LANG_TH   // uncomment in D-11 to enable Thai language
```
All string constants defined once in EN. No Thai content yet. D-11 handles it.

---

## 6. DO NOT TOUCH

- `firmware/hardware.h` — LOCKED (R2)
- `firmware/satu_vending.ino` — LOCKED
- `firmware/ui_service.h` — LOCKED
- `firmware/config.h` — LOCKED
- `firmware/state_machine.h` — LOCKED
- `firmware/network.h` — LOCKED
- `src/` — backend, do not touch
- `public/` — do not touch
- `satu-system-tester.html` — do not touch

---

## 7. MANDATORY CLOSING

Complete all steps — no partial closes:

1. **CC_CHAT_LOG.md** — append at TOP:
   - State all 4 new/modified files
   - State ui.h line count before (1819) and after split
   - Flag: owner must flash and confirm CI green before this is called done

2. **Archive this prompt** → `docs/prompts/CC_BUILD_PROMPT_ui_split_v1.md`  
   Stamp: `✅ COMPLETE — 2026-06-22 — D-10 ui.h split (4 files)`

3. **RULES.md** — append at TOP (next R-number after R-170):
   ```
   R-171: ui.h SPLIT ARCHITECTURE LOCKED (2026-06-22).
   ui.h includes: ui_strings.h → ui_keyboard.h → ui_screens.h → ui_service.h in that order.
   satu_vending.ino includes only ui.h. No file in the chain may include ui.h.
   Thai strings go in ui_strings.h only via #ifdef LANG_TH (D-11).
   ```
   Version bump RULES.md.

4. **PROJECT_STATE.md** — add to SESSION LOG at TOP:
   ```
   ### 2026-06-22 — D-10: ui.h split (CC_BUILD_PROMPT_ui_split_v1.md)
   - ui.h R5 (1819 lines) split into ui.h R6 + ui_strings.h R1 + ui_keyboard.h R1 + ui_screens.h R1
   - ui_service.h unchanged
   - satu_vending.ino unchanged — still #include "ui.h" only
   - CI: [pending] — green = split correct
   - PENDING: owner flash + confirm compile + boot to idle screen
   ```
   Version bump PROJECT_STATE.md.

5. **KNOWLEDGE_MAP.md** — add to firmware file list:
   ```
   ui_strings.h       — text literals, EN/TH switch (#ifdef LANG_TH — D-11)
   ui_keyboard.h      — WiFi keyboard + PIN numpad overlay
   ui_screens.h       — all customer screen draw + touch functions
   ```
   Version bump KNOWLEDGE_MAP.md.

6. **Version bump every file changed** — ui.h R6, each new file R1.

7. **Commit + merge to main.**

---

## 8. PAYMENT MODE REMINDER

PAYMENT_MODE = fake. Never change. This prompt touches zero payment code.
