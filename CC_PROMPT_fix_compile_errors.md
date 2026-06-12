# CC_PROMPT_fix_compile_errors.md
# Satu Firmware — Fix compile errors from first R5 build attempt
# Created: 2026-06-12
# Target repo: https://github.com/Csmittee/Satu-Vending-Firmware

## CC INTRO
New session. Ignore all previous context.
Repo: https://github.com/Csmittee/Satu-Vending-Firmware
Read CLAUDE.md, RULES.md, then the exact files listed below before touching anything.

---

## TWO COMPILE ERRORS — EXACT TEXT FROM OWNER

### ERROR 1 — extern vs static conflict (6 instances, same root cause)
```
ui.h:72: error: 'g_grid_rows' was declared 'extern' and later 'static'
ui.h:73: error: 'g_grid_cols' was declared 'extern' and later 'static'
ui.h:79: error: 'g_cfg_idle'  was declared 'extern' and later 'static'
ui.h:80: error: 'g_cfg_sel'   was declared 'extern' and later 'static'
ui.h:81: error: 'g_cfg_water' was declared 'extern' and later 'static'
ui.h:82: error: 'g_cfg_lucky' was declared 'extern' and later 'static'
```
network.h declares these as `extern`.
ui.h redeclares them as `static` with default values.
One file must own the definition, the other must declare extern only.

### ERROR 2 — PNGdec callback return type mismatch
```
ui.h:167: error: invalid conversion from 'void (*)(PNGDRAW*)' to 'int (*)(PNGDRAW*)'
PNG::openRAM() expects: int (*)(PNGDRAW*)
_pngDrawRow is declared: void
```
The PNGdec v1.1.6 callback signature requires `int` return type, not `void`.

---

## READ BEFORE FIXING
1. firmware/network.h — find where g_grid_rows etc are declared extern
2. firmware/ui.h — find where g_grid_rows etc are declared static, and _pngDrawRow signature
3. Do not read any other files unless needed to resolve these two errors

---

## FIXES

### FIX 1 — resolve extern vs static conflict
Variables must be defined (with value) in exactly ONE place.
network.h already declares them extern — so network.h is NOT the owner.
ui.h must be the owner: keep the definition with default value in ui.h,
remove the `static` keyword (plain global), and remove the extern declarations
from network.h (since ui.h is included after network.h in satu_vending.ino,
network.h cannot forward-declare what ui.h defines).

Correct pattern:
- ui.h: `int  g_grid_rows = 2;`  (no static — file-scope global)
- network.h: remove the extern declarations for these 6 variables entirely,
  since network.h functions that need them will get them via the global scope
  when the full sketch compiles

Verify: check that network.h functions actually USE these variables.
If they do, the linker will resolve them from ui.h's definitions at compile time.
No extern needed in network.h — Arduino sketch compilation links all files together.

### FIX 2 — PNGdec callback return type
Change _pngDrawRow signature from `void` to `void` is wrong.
PNGdec v1.1.6 PNG_DRAW_CALLBACK is defined as `int (*)(PNGDRAW*)`.
Fix: change the callback to return `int` and return 0 at the end.

```cpp
// BEFORE (wrong):
static void _pngDrawRow(PNGDRAW* pDraw) { ... }

// AFTER (correct):
static int _pngDrawRow(PNGDRAW* pDraw) {
  ...
  return 0;
}
```

---

## DO NOT TOUCH
- hardware.h — R2 LOCKED
- config.h — credentials file, gitignored, not in repo
- state_machine.h — no changes needed
- satu_vending.ino — no changes needed unless fix requires it

---

## MANDATORY AT END
1. Open PR on new branch: `fix/compile-errors-r5`
   Title: `[Fix] Resolve R5 compile errors — extern/static conflict + PNGdec callback`
2. Add to RULES.md at top (next R-number):
   `R-[N]: Global variables shared between network.h and ui.h must be defined
           (no static) in ui.h only. network.h must not redeclare them extern.
           Arduino sketch compilation resolves all globals across included files.`
   `R-[N+1]: PNGdec v1.1.6 PNG_DRAW_CALLBACK requires int return type, not void.
             Always: static int _pngDrawRow(PNGDRAW* pDraw) { ... return 0; }`
3. Update PROJECT_STATE.md — note compile errors fixed, R5 ready to flash
4. Archive this prompt to docs/prompts/ stamped ✅ COMPLETE
5. Do NOT remind owner to run 14-test suite — no backend changes this session

## PAYMENT MODE REMINDER
PAYMENT_MODE = fake. No backend files touched.
