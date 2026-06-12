✅ COMPLETE — 2026-06-12 — extern declarations restored in network.h.
   R-88 corrected in RULES.md to document the proper extern/define split.
   Pushed to claude/loving-bohr-t3n3yf (PR #4).

---

# CC_PROMPT_hotfix_extern.md
# Satu Firmware — Restore extern declarations removed in last compile fix
# Created: 2026-06-12

## PROBLEM
network.h is included before ui.h in satu_vending.ino.
Removing extern declarations from network.h caused "not declared in this scope"
errors for g_grid_rows, g_grid_cols, g_cfg_idle, g_cfg_sel, g_cfg_water, g_cfg_lucky
inside loadConfigFromNVS() and _sendHello().

## FIX APPLIED
- network.h: restored 6 extern declarations (declaration only, no default values)
- ui.h: definitions remain as plain globals with default values (no static) — correct

Correct final pattern:
  network.h → extern int g_grid_rows;   (declaration — network.h included first)
  ui.h      → int g_grid_rows = 2;      (definition — no static)

## FILES CHANGED
- firmware/network.h — 6 extern declarations restored
- RULES.md — R-88 corrected to document extern/define split
- docs/prompts/CC_PROMPT_hotfix_extern.md — this archive

## DO NOT TOUCH
- hardware.h — R2 LOCKED — confirmed untouched
