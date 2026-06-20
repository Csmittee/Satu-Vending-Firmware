# CC_CHAT_LOG.md — Satu 1.0 (Firmware)
> Version 2.3 — 2026-06-20
> Changes: Service menu visual fix pass (13 issues, R-158) + CI artifact upload (R-157)
> Previous: v2.2 — 2026-06-20
> CC writes one entry per session at TOP · Chat reads last 3 entries at session open
> Format defined in CC_SKILL.md · Max 10 lines per entry · Never delete old entries

---
## 2026-06-20 — Service menu 13 visual fixes (R-158) — CC_BUILD_PROMPT_service_menu_fix_v1
**Did:** Complete rewrite of firmware/ui_service.h fixing all 13 visual issues from owner photo QA. GLOBAL: added _svcLogPanel() right-side log panel (x=670, w=126) with 10-entry circular buffer; called _svcLogDraw() at end of every _drawSvcBody_*. TAB 0 Self Test: btnY moved _BDY+60→_BDY+72; subtitle at _BDY+52; resY=166; lineH=18; results drawn in left zone only (not past x=670); _svcLogPanel() called per result. TAB 1 Free Play: fixed 7×3 grid (not g_grid_cols); cw=72 ch=50 gap=4; gridY=_BDY+72; instruction text at _BDY+60. TAB 2 Devices: _REL_CW 107→86 (6 cells now fit left zone); _IR_CW 62→80, _IR_CH 28→36; _IR_ROW1_Y gap +38→+52; new stub row Pump/LED/Speaker at y=327; _TBEKY recalculated=371; stub taps log locally (return 0). TAB 3 Settings: all Y positions now _BDY-relative (_S_Y402=_BDY+138=182, _S_VOLY=_BDY+318=362, _S_Y401=_BDY+348=392); lane prices show L1-L7 row then L8-L10 row. TAB 4 Firmware: all Y positions _BDY-relative (info rows at _BDY+82+16×n, SECURITY at _BDY+192, OTA at _BDY+274/_BDY+292, _FW_PRINT_Y=_BDY+348=392). _getTouchedServiceExtra(): btnY corrected, stub row added, 702 (Boot PIN) and 701 (Factory Reset) hit-tests added.
**Updated:** firmware/ui_service.h, RULES.md v1.9, PROJECT_STATE.md v1.5, CC_CHAT_LOG.md v2.3
**New files:** docs/prompts/CC_BUILD_PROMPT_service_menu_fix_v1.md (archived ✅ COMPLETE)
**Pending Chat verify:** Flash; enter service mode; check all 5 tabs against photo QA list; confirm log panel visible on right; confirm Free Play is 7×3; confirm relay cells narrower (86px); confirm Settings/Firmware Y positions match spec.
**Flags:** Log panel right edge at 796px (4px margin from SCR_W=800). satu_vending.ino NOT touched — all action codes 500-800 unchanged.

## 2026-06-20 — CI artifact upload — CC_BUILD_PROMPT_ci_artifact_v1
**Did:** (1) Added `--output-dir ./build` to arduino-cli compile step — minimum change needed so .bin lands at a predictable path (without this flag arduino-cli writes to an unpredictable /tmp/arduino-sketch-XXXXXXXX/ location). (2) Added `actions/upload-artifact@v4` step after compile: artifact `satu-firmware-${{ github.run_number }}`, path `./build/satu_vending.ino.bin`, retention 7 days. (3) Added "Flashing Without Arduino IDE" section to CLAUDE.md — 5-step esptool.py flash workflow referencing CI artifact download. Zero firmware source files touched this session.
**Updated:** .github/workflows/compile-check.yml, CLAUDE.md v1.5, RULES.md v1.8, PROJECT_STATE.md v1.4, KNOWLEDGE_MAP.md v1.3, CC_CHAT_LOG.md v2.2
**New files:** docs/prompts/CC_BUILD_PROMPT_ci_artifact_v1.md (archived)
**Pending Chat verify:** Push any firmware/ change → CI runs → Actions tab shows artifact `satu-firmware-N` → download zip → confirm satu_vending.ino.bin inside. Test esptool.py flash from downloaded .bin on SATU-4R473R.
**Flags:** NONE. `--output-dir` is an output path flag only — zero effect on binary content, FQBN, or locked library versions. PAYMENT_MODE stays fake.
---
## 2026-06-19 — Service mode 5 tabs (R-154/R-155/R-156) — CC_BUILD_PROMPT_service_menu_v2
**Did:** (1) hardware.h: added g_mcp1_ok/g_mcp2_ok bool globals set in initMCP23017() after each begin_I2C() — only permitted change to R2-locked file. (2) NEW firmware/ui_service.h: all 5 _drawSvcBody_* + _getTouchedServiceExtra(); Self Test (Quick 10 items / Technical 14 items with live heartbeat), Free Play (slot grid gold=enabled/dimgrey=empty/darkred=disabled), Devices (relay 2×6 grid 601-612 + IR sensor live read + Test Backend 600), Settings (network info / boot PIN toggle 402 / volume cycle 700 / factory reset 401), Firmware (MAC/heap/security amber badges / print to serial 800). (3) ui.h: stubs removed; #include "ui_service.h" placed BEFORE drawServiceScreen() to satisfy forward reference; getTouchedServiceContent() extended actions 500-800; forward decl _getTouchedServiceExtra added. (4) satu_vending.ino: action handlers 500-502, 600-612, 700, 800; Free Play 301-321 uses vendProduct(); sendHeartbeat() called void (no return); g_deviceId.c_str() used directly (same translation unit). Key architectural note: include moved from end of ui.h to before drawServiceScreen() to avoid forward-reference error.
**Updated:** firmware/hardware.h, firmware/ui_service.h (new), firmware/ui.h, firmware/satu_vending.ino, RULES.md v1.7, PROJECT_STATE.md v1.3, KNOWN_GOOD.md, CLAUDE.md v1.4, KNOWLEDGE_MAP.md v1.2, CC_CHAT_LOG.md v2.1
**New files:** firmware/ui_service.h
**Pending Chat verify:** Flash; enter service mode; all 5 tabs render correctly; Quick Self Test shows 10 items pass/fail; relay toggle changes cell colour; volume increments by 10% each tap; [Print to Serial] dumps device info to Arduino serial monitor.
**Flags:** NONE. sendHeartbeat() is void — action 600 uses WiFi.status() as success proxy. SPEAKER_PIN=-1 so audio section shows "No speaker hardware". R12 label = LOCKED/UNLOCKED not ON/OFF (R-156).

---
## 2026-06-19 — R-153: STATE_CONFIRMING — confirm screen before order creation
**Did:** (1) Added STATE_CONFIRMING to state_machine.h enum (between STATE_GIFT_OPTION and STATE_AWAITING_PAYMENT, R9 changelog). (2) Updated satu_vending.ino: STATE_GIFT_OPTION choices now transition to STATE_CONFIRMING (not directly to _proceedToPayment); added Back (choice 2) → STATE_PRODUCT_SELECTION; added STATE_CONFIRMING case with 300ms entry guard, Confirm→_proceedToPayment(), Back→STATE_GIFT_OPTION, 30s timeout→idle. (3) Updated ui.h: added _lastConfirmTouchMs debounce variable; added Back button to drawGiftOptionScreen() (bottom centre 200×48); updated getTouchedGiftOption() to return 2 for Back; added drawConfirmScreen(slotIdx, wantWater) — shows product name, water option, total THB in summary box, Back and Confirm buttons; added getTouchedConfirm() returns 1=Confirm, -1=Back, 0=none. Key safety: createOrder() now called ONLY on Confirm touch — no D1 rows from abandoned flows. Customer waits for QR after confirming (owner accepted this trade-off).
**Updated:** firmware/state_machine.h, firmware/satu_vending.ino, firmware/ui.h, CC_CHAT_LOG.md v2.0
**New files:** NONE
**Pending Chat verify:** Flash, test full flow: Product select → Gift option (verify Back→product screen) → Confirm screen (verify Back→gift option, Confirm→QR). Verify D1 has no new rows after abandoning at gift or confirm screen.
**Flags:** NONE. Screen layout: confirm box 500×200 centred at y=104, Back btn x=140 y=328, Confirm btn x=440 y=328. Both 220×52px.
---
## 2026-06-19 — config.h: tracked in repo, R-86 updated to owner flash workflow model
**Did:** (1) Removed `config.h` from .gitignore so file can be tracked. (2) Updated firmware/config.h header to reflect tracked status; added `PRODUCT_SELECTION_TIMEOUT 15` (R-152) to Timeouts section. (3) Updated RULES-firmware.md R-86 v1.2→v1.3: config.h is tracked with empty WiFi strings; CC updates in same PR as new constants; owner downloads from GitHub firmware folder as part of flash workflow; CI generates its own inline copy independently. Root cause fixed: local config.h drift caused PRODUCT_SELECTION_TIMEOUT compile error — owner won't miss updates now because file timestamp will show in GitHub.
**Updated:** firmware/config.h, .gitignore, .claude/rules/RULES-firmware.md v1.3, CC_CHAT_LOG.md v1.9
**New files:** NONE
**Pending Chat verify:** Confirm firmware/config.h appears in GitHub firmware folder with today's commit. Confirm PRODUCT_SELECTION_TIMEOUT 15 is present. Next flash: download config.h from GitHub as usual.
**Flags:** NONE
---
## 2026-06-19 — Cleanup: firmware/config.h untracked via git rm --cached (R-86 full enforcement)
**Did:** git rm --cached firmware/config.h. File was committed 2 days ago despite being in .gitignore. git rm --cached removes it from tracking without deleting owner's local copy. .gitignore now takes full effect.
**Updated:** CC_CHAT_LOG.md v1.8
**New files:** NONE
**Pending Chat verify:** After merge, confirm firmware/config.h no longer appears in GitHub repo file listing.
**Flags:** NONE
---
## 2026-06-19 — Cleanup: config.h.example deleted from repo (R-86 enforcement)
**Did:** Deleted firmware/config.h.example. CI already has PRODUCT_SELECTION_TIMEOUT 15 in compile-check.yml heredoc (PR #35). R-86 says no config.h.example in repo — this removes the contradiction created in R8.
**Updated:** CC_CHAT_LOG.md v1.7
**New files:** NONE
**Pending Chat verify:** Owner must add this line to local config.h before next flash (after #define SENSOR_POLL_MS line): `#define PRODUCT_SELECTION_TIMEOUT 15`
**Flags:** NONE
---
## 2026-06-19 — Docs: SKILL 8 — Rules Hygiene Scan added to CC_SKILL.md
**Did:** Added SKILL 8 (Rules Hygiene Scan) to CC_SKILL.md after SKILL 7. Trigger: major build milestones. Output: KEEP/REMOVE/REVIEW list for owner approval. Docs-only, never auto-deletes rules. Bumped CC_SKILL.md v1.1→v1.2.
**Updated:** CC_SKILL.md v1.2, CC_CHAT_LOG.md v1.6
**New files:** NONE
**Pending Chat verify:** NONE (docs only)
**Flags:** NONE
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
