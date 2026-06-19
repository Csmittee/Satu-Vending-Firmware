# CC_CHAT_LOG.md — Satu 1.0 (Firmware)
> Version 1.5 — 2026-06-19
> Changes: Added session entry for R-86 docs fix (config.h.example removed from repo)
> Previous: v1.4 — 2026-06-19
> CC writes one entry per session at TOP · Chat reads last 3 entries at session open
> Format defined in CC_SKILL.md · Max 10 lines per entry · Never delete old entries

---
## 2026-06-19 — Docs: R-86 updated — config.h.example removed from repo model
**Did:** Updated RULES-firmware.md R-86: config.h.example is NOT a tracked template in repo. CI generates config.h inline via compile-check.yml. Removed "copy config.h.example → config.h" instruction which was incorrect.
**Updated:** .claude/rules/RULES-firmware.md v1.2, CC_CHAT_LOG.md v1.5
**New files:** NONE
**Pending Chat verify:** NONE (docs only)
**Flags:** NONE
---
## 2026-06-19 — Bug A/B/C UX fixes: idle touch, gift carry-over, selection timeout
**Did:** Bug A: touchReadOnce() in ui.h caches GT911 read for 4ms — 3 reads/tick collapsed to 1. Bug B: resetGiftTouchDebounce() called from setState(STATE_GIFT_OPTION); entry guard 250→500ms; _lastGiftTouchMs moved to module scope. Bug C: PRODUCT_SELECTION_TIMEOUT in config.h.example replaces magic number 15 in g_cfg_sel.
**Updated:** firmware/ui.h, firmware/satu_vending.ino (R8), firmware/config.h.example (CREATE), RULES.md v1.6 (R-150, R-151, R-152), CC_CHAT_LOG.md v1.4
**New files:** firmware/config.h.example
**Pending Chat verify:** Flash, serial monitor: (A) single tap on idle grid should log `[UI] Touch: slot X` immediately. (B) Tap product confirm then immediately tap gift area — must NOT fire within 500ms. (C) Leave product selection idle for 15s — should return to idle screen.
**Flags:** NONE. Flash cycles: 1.
---
## 2026-06-19 — CI paths filter: compile-check.yml skips non-firmware changes
**Did:** Added `paths: ['firmware/**']` filter to both push and pull_request triggers in .github/workflows/compile-check.yml. CI no longer runs on docs-only commits (CLAUDE.md, RULES.md, CC_CHAT_LOG.md etc.).
**Updated:** .github/workflows/compile-check.yml, CC_CHAT_LOG.md v1.3
**New files:** NONE
**Pending Chat verify:** Push a docs-only commit — confirm CI does NOT trigger. Push a firmware/ change — confirm CI DOES trigger.
**Flags:** NONE
---
## 2026-06-19 — Docs-only: CLAUDE.md Key Files update (firmware source files)
**Did:** Added firmware/satu_vending.ino, firmware/hardware.h, firmware/network.h, firmware/ui.h to CLAUDE.md Key Files section. Bumped CLAUDE.md to v1.3. No firmware source files touched.
**Updated:** CLAUDE.md v1.3, CC_CHAT_LOG.md v1.2
**New files:** NONE
**Pending Chat verify:** No firmware source files changed. No compile required (docs-only).
**Flags:** NONE
---
## 2026-06-19 — CC_PROMPT_firmware_fix_vend_loop_and_gift_touch_v1
**Did:** Fixed Bug 1 (carry-over touch) + Bug 2 (vend loop command poll). Files: firmware/satu_vending.ino (entry guard + include reorder), firmware/hardware.h (spin loop pollCommands every 500ms).
**Updated:** RULES.md v1.5 (R-148, R-149), KNOWN_GOOD.md, PROJECT_STATE.md v1.2, CC_CHAT_LOG.md v1.1
**New files:** NONE
**Pending Chat verify:** Serial output confirms motor stops within ~1s of IR Trigger button — expected `[HW] sensor_triggered command received — stopping motor after ~XXXms`. Gift screen must show ≥250ms gap after product selection confirm.
**Flags:** OVERRIDE — include reorder in satu_vending.ino (network.h moved before hardware.h). Required so CommandList/pollCommands() are visible at hardware.h compile time. Logic unchanged — pure dependency resolution. CI must be ✅ GREEN before flash.
---
## 2026-06-18 — CC_BUILD_PROMPT_governance_v1 (Governance Wiring — Firmware Repo)
**Did:** Updated CLAUDE.md — added CC_SKILL.md + CC_CHAT_LOG.md to Key Files, removed deleted hardware repo, added version header v1.1. Updated RULES.md — R-138 to R-141 added (CC_CHAT_LOG, CC_SKILL, HTML size limit, doc versioning), version bumped to 1.3. Updated KNOWLEDGE_MAP.md — added CC_SKILL/CC_CHAT_LOG entries, removed hardware repo refs, added .claude/claude_project/ section, version bumped to 1.1. Created CC_CHAT_LOG.md (this file).
**Updated:** RULES.md v1.3 (R-138→R-141), KNOWLEDGE_MAP.md v1.1, CLAUDE.md v1.1
**New files:** CC_CHAT_LOG.md (this file)
**Pending Chat verify:** Firmware CI compile check — no .ino or .h files were touched so compile should be unaffected. Confirm zero firmware source files changed ✓.
**Flags:** Docs-only session. Hardware repo references removed (repo deleted). CHAT_HANDOFF.md removed from project knowledge docs section (not in repo — Chat project folder only).
---
