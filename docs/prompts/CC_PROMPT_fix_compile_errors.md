✅ COMPLETE — 2026-06-12 — Both compile errors fixed. R5 ready to flash.
   R-88 (shared globals) + R-89 (PNGdec callback) locked in RULES.md.
   Pushed to claude/loving-bohr-t3n3yf (PR #4).

---

# CC_PROMPT_fix_compile_errors.md
# Satu Firmware — Fix compile errors from first R5 build attempt
# Created: 2026-06-12
# Target repo: https://github.com/Csmittee/Satu-Vending-Firmware

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

### ERROR 2 — PNGdec callback return type mismatch
```
ui.h:167: error: invalid conversion from 'void (*)(PNGDRAW*)' to 'int (*)(PNGDRAW*)'
```

## FIXES APPLIED

### FIX 1 — extern/static conflict
- network.h: removed 6 extern re-declarations (g_grid_rows, g_grid_cols, g_cfg_idle,
  g_cfg_sel, g_cfg_water, g_cfg_lucky) — Arduino sketch compilation resolves globals
  at link time, no extern needed
- ui.h: removed `static` from those 6 variables — now plain file-scope globals

### FIX 2 — PNGdec callback return type
- ui.h: `static void _pngDrawRow` → `static int _pngDrawRow` + `return 0;`

## FILES CHANGED
- firmware/network.h — 6 extern declarations removed
- firmware/ui.h — 6 static → plain global, _pngDrawRow void → int
- RULES.md — R-88 + R-89 appended
- PROJECT_STATE.md — compile fix session logged
- docs/prompts/CC_PROMPT_fix_compile_errors.md — this archive

## DO NOT TOUCH
- hardware.h — R2 LOCKED — confirmed untouched
