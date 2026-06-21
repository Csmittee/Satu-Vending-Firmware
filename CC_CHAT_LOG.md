# CC_CHAT_LOG.md — Satu 1.0 (Firmware)
> Version 2.13 — 2026-06-21
> Changes: Session close — flash verified, slow response open item logged
> Previous: v2.12 — 2026-06-21
> CC writes one entry per session at TOP · Chat reads last 3 entries at session open
> Format defined in CC_SKILL.md · Max 10 lines per entry · Never delete old entries

---
## 2026-06-21 — Session close: flash verified; slow response open item
**Did:** Confirmed CI green (both runs on PR #52). PR #52 merged. Flash verified by owner: Serial Monitor active at /dev/cu.usbserial-1420 115200, [BOOT] output present, no /dev/cu.usbmodem. CDCOnBoot=default fix confirmed working on hardware.
**Open item logged:** Slow response on hardware observed post-flash. Chat hypothesis: self-test auto-starts at boot, blocks loop(). Next CC session to read satu_vending.ino setup() and self-test trigger conditions before any code change.
**Updated:** PROJECT_STATE.md v1.14, CC_CHAT_LOG.md v2.13
**New files:** NONE
**Flags:** Zero source files touched this session close. hardware.h LOCKED. PAYMENT_MODE stays fake.

---
## 2026-06-21 — Doc cleanup: R-167 rename + CI paths fix (rebase dedup)
**Did:** (1) RULES.md v2.4: cleaned stacked v2.2/v2.3 header, renamed CI FQBN rule R-165→R-167 (R-165 was taken by HARDWARE_SPEC rule from parallel rebase session). (2) PROJECT_STATE.md v1.13: removed stacked v1.11 header block + duplicate SESSION LOG heading (rebase artifact). Fixed CI FQBN session entry — corrected "Added UploadProtocol=uart0" to reflect it was added then removed as invalid arduino-cli compile option. (3) compile-check.yml: added `.github/workflows/**` to paths filter so future workflow-file changes self-trigger CI (prevents recurrence of "FQBN fix didn't trigger CI" problem). Updated comment R-165→R-167.
**Updated:** RULES.md v2.4, PROJECT_STATE.md v1.13, CC_CHAT_LOG.md v2.12, .github/workflows/compile-check.yml
**New files:** NONE
**Pending Chat verify:** CI green (workflow paths change self-triggers CI run with corrected FQBN). Flash: ⬜ pending — Serial Monitor at /dev/cu.usbserial-1420 115200 shows [BOOT]. No /dev/cu.usbmodem. Service mode: no black flash on tab switch.
**Flags:** Zero .ino or .h files touched. hardware.h LOCKED. PAYMENT_MODE stays fake.

---
## 2026-06-21 — One PR open at a time rule (R-166)
**Did:** Docs only. Added R-166 to RULES.md v2.3: never open a new CC session while a previous PR is unmerged. Sequence: CI green → owner merges → then next session. CC_CHAT_LOG, PROJECT_STATE, RULES written every session without exception. Rule captures session discipline pattern to prevent stacked PRs and doc merge conflicts.
**Updated:** RULES.md v2.3, CC_CHAT_LOG.md v2.10, PROJECT_STATE.md v1.12
**New files:** NONE
**Flags:** Zero source files touched. Docs only. hardware.h LOCKED. PAYMENT_MODE stays fake.

---
## 2026-06-21 — CI FQBN corrected to match owner Arduino IDE (R-165)
**Did:** .github/workflows/compile-check.yml — updated FQBN from `CDCOnBoot=cdc` → `CDCOnBoot=default` and added `UploadProtocol=uart0` to match owner's confirmed Arduino IDE Tools menu (ESP32S3 Dev Module, USB CDC On Boot=Disabled, Upload Mode=UART0/Hardware CDC). Root cause: CDCOnBoot=cdc routes Serial to USB CDC virtual port → artifact flash produced /dev/cu.usbmodem instead of /dev/cu.usbserial-1420 → Serial Monitor silent. Added shell comments above FQBN line referencing R-165. Added R-165 to RULES.md v2.2. Zero firmware source files touched. CI only.
**Updated:** .github/workflows/compile-check.yml, RULES.md v2.2, CC_CHAT_LOG.md v2.9, PROJECT_STATE.md v1.11
**New files:** NONE
**Pending Chat verify:** CI green → download artifact → flash with esptool → confirm Serial Monitor at /dev/cu.usbserial-1420 115200 shows [BOOT] output (not silent). Confirm no /dev/cu.usbmodem appears.
**Flags:** Zero .ino or .h files touched. hardware.h LOCKED. PAYMENT_MODE stays fake.
## 2026-06-21 — Docs-only: CLAUDE.md v1.8 + RULES.md v2.3 (flash command correction)
**Did:** CLAUDE.md v1.7→v1.8 (firmware): "Flashing Without Arduino IDE" section replaced — esptool.py→esptool, baud 921600→460800, write_flash→write-flash, port /dev/cu.XXXX→/dev/cu.usbserial-1420, relative paths→~/satu-firmware/ absolute paths, steps condensed 5→4. RULES.md v2.2→v2.3: duplicate R-157 entries consolidated into one clean entry with corrected command. Same flash command added to backend repo simultaneously (CLAUDE.md v1.5, RULES.md v1.6).
**Updated:** CLAUDE.md v1.8, RULES.md v2.3, CC_CHAT_LOG.md v2.11, PROJECT_STATE.md v1.12 (firmware)
**New files:** NONE
**Pending Chat verify:** NONE (docs-only session)
**Flags:** Zero source files touched. hardware.h R2 LOCKED. PAYMENT_MODE stays fake. CI not triggered.

---
## 2026-06-21 — Docs-only: WORKFLOW_SKILL v2.3 + CHAT_RULE v1.1 (governance)
**Did:** Updated .claude/claude_project/WORKFLOW_SKILL.md v2.2→v2.3 (both repos): replaced CHAT SESSION OPENING with self-executing 3-line protocol; replaced CHAT HANDOFF TEMPLATE opening block with 5-step embedded loading sequence (Step 1–4). Updated .claude/claude_project/CHAT_RULE.md v1.0→v1.1 (both repos): added Session Flow section — rules 20-24 (prompt discipline / scope lock / context decay / complaint detection / component detail suppression). CC_CHAT_LOG.md updated both repos.
**Updated:** .claude/claude_project/WORKFLOW_SKILL.md v2.3, .claude/claude_project/CHAT_RULE.md v1.1, CC_CHAT_LOG.md v2.10 (firmware) / v1.3 (backend)
**New files:** NONE
**Pending Chat verify:** NONE (docs-only session)
**Flags:** Zero source files touched. hardware.h R2 LOCKED. PAYMENT_MODE stays fake. CI not triggered.

---
## 2026-06-21 — HARDWARE_SPEC v1.2 + config.h R15 (CC_BUILD_PROMPT_hardware_spec_v1_2_FINAL)
**Did:** (1) hardware/HARDWARE_SPEC.md v1.1→v1.2: Replaced SPRING FLAP section with MAGNETIC PIN-LOCK (R-129 UPDATED) — 2 locks parallel on relay 12, fail-secure, HIGH=UNLOCKED. Added PROXIMITY SWITCH section (MCP2 GPA2, roller microswitch, CLOSED=LOW). Updated MCP2 table GPA2 row. Added MCP RESET PIN warning under both MCP1 and MCP2. Added SPEAKER section (GPIO1). Updated IR mount + placeholder note. W-07 corrected to magnetic lock language + fail-open risk. Added WIRE HARNESS SUMMARY (AWG/colour/JST count/run-length tables). BOM: spring flap removed → 2× mag lock + roller switch + new JST rows + 10kΩ resistors. Added MULTI-MODEL EXPANSION section (5×2 locked, 5×3/7×3 placeholder, MCP3 TBD, IR rail note). Updated BOARD table (Speaker GPIO1). Updated JST CONNECTOR STANDARD (proximity + speaker rows). Updated POWER RAILS (mag lock solenoids). (2) firmware/config.h R14→R15: FLAP_PROXIMITY_MCP_PIN -1→2, SPEAKER_PIN -1→1. (3) RULES.md v2.1→v2.2: R-165 prepended. (4) Prompt archived to docs/prompts/.
**Updated:** hardware/HARDWARE_SPEC.md v1.2, firmware/config.h R15, RULES.md v2.2, CC_CHAT_LOG.md v2.9, PROJECT_STATE.md v1.11, docs/prompts/CC_BUILD_PROMPT_hardware_spec_v1_2_FINAL.md (archived ✅ COMPLETE)
**New files:** NONE
**Pending Chat verify:** CI green → flash → verify Serial shows `[HW] Proximity polling ACTIVE on MCP2 pin 2` (needs firmware update in next session). hardware.h already has #if FLAP_PROXIMITY_MCP_PIN >= 0 guard — activates automatically with config.h R15.
**Flags:** hardware.h R2 LOCKED — NOT touched. satu_vending.ino NOT touched. public/ HTML files NOT touched (satu-wiring.html Spring Flap language = D-9 open item, separate session). PAYMENT_MODE stays fake. CI triggered by firmware/config.h change.

---
## 2026-06-21 — Remove fillScreen from drawServiceScreen (FIX 3 / R-164)
**Did:** firmware/ui.h `drawServiceScreen(int tab)` — removed `gfx->fillScreen(C_BG)` (PSRAM bus contention, R-164). Changed body clear from `fillRect(bodyX, ...)` to `fillRect(SVC_BODY_X, STATUS_H, SCR_W - SVC_BODY_X, SCR_H - STATUS_H, C_BG)` — body-only clear, explicit constant. Header bar fillRect(0,0,SCR_W,STATUS_H,C_DARKGREY) retained (small area, acceptable). Tab bar via _drawSvcTabBar() retained. This eliminates the black flash + ~80-120ms stall on every service tab tap. Root cause: same PSRAM bus contention class as R-117 (PNG decode). R-164 added to RULES.md v2.1.
**Updated:** firmware/ui.h, RULES.md v2.1, CC_CHAT_LOG.md v2.8, PROJECT_STATE.md v1.10
**New files:** NONE
**Pending Chat verify:** CI green → flash → enter service mode → tap through all 5 tabs: confirm NO black flash, smooth tab switch, all tab content renders correctly.
**Flags:** ui_service.h UNTOUCHED. hardware.h LOCKED. satu_vending.ino NOT touched. PAYMENT_MODE stays fake.

---
## 2026-06-20 — Devices tab MACHINE_LANES grid (FIX 2 / R-163)
**Did:** (1) FIX 1 — tab-change guard already present at satu_vending.ino:489 (`if (newTab >= 0 && newTab != g_service_tab)`) — no code change needed. (2) FIX 2 — firmware/config.h: added `#define MACHINE_LANES 10` (R14) after MAX_SLOTS_HW. (3) .github/workflows/compile-check.yml: added `#define MACHINE_LANES 10` to CI inline config.h to stay in sync (R-86). (4) firmware/ui_service.h TAB 2 Devices: replaced hardcoded 12-relay / 10-sensor / 3-stub layout with MACHINE_LANES-driven grid. New defines: _DEV_COLS (5 if ≤10, else 7), _DEV_CW (86 or 68), _DEV_ROWS (ceil division), _DEV_SP_Y (special row below lane grid), _DEV_WARN_Y/IH_Y/IR1_Y/TBES_Y all recalculated. Lane relays R1–MACHINE_LANES drawn in grid; R11+R12 in fixed special row below grid; WARNING banner; IR sensors S1–MACHINE_LANES in same column count. Stub row (Pump R11/LED Test/Speaker) removed. Touch handler replaced with _DEV_ROWS loop returning 600+r per relay, 611/612 for special row. Action codes 601–612 remain compatible with satu_vending.ino:548 handler. (5) RULES.md v2.0: prepended R-163.
**Updated:** firmware/config.h, .github/workflows/compile-check.yml, firmware/ui_service.h, RULES.md v2.0, CC_CHAT_LOG.md v2.7, PROJECT_STATE.md v1.9
**New files:** NONE
**Pending Chat verify:** CI green → flash → enter service mode → Devices tab: confirm 5-col × 2-row relay grid R1-R10, special row R11 pump + R12 flap below, WARNING banner, IR S1-S10 5×2 grid, Test Backend button. Tap relay to toggle, confirm action 601-610 fires correctly. Confirm no stub row.
**Flags:** FIX 1 guard already existed — satu_vending.ino NOT touched. hardware.h LOCKED. PAYMENT_MODE stays fake.

---
## 2026-06-20 — CC_BUILD_PROMPT_governance_docs_v2 (Governance Docs v2)
**Did:** Renamed hardware/HARDWARE_TRUTH.md → hardware/HARDWARE_SPEC.md (v1.1) with CHANGE LOG + MCP3 expansion note. Updated UI_SPEC.md v2.0 — added CHANGE LOG, Type Scale section, Log Panel section. Placed SATU_ROADMAP.md at root of BOTH repos. Updated CLAUDE.md (both repos) — added hardware/HARDWARE_SPEC.md, UI_SPEC.md, SATU_ROADMAP.md to Key Files. Updated KNOWLEDGE_MAP.md (both repos) — 3 Document Map rows + File Locations entries. Added R-160/R-161/R-162 to RULES.md (both repos). Updated WORKFLOW_SKILL.md (both repos) v2.2 — new step 3 + 4 trigger rows.
**Updated:** hardware/HARDWARE_SPEC.md v1.1, UI_SPEC.md v2.0, SATU_ROADMAP.md (new), CLAUDE.md v1.7/v1.4, KNOWLEDGE_MAP.md v1.4/v1.3, RULES.md v1.10/v1.5, WORKFLOW_SKILL.md v2.2 (both repos), CC_CHAT_LOG.md v2.6/v1.2, PROJECT_STATE.md v1.8
**New files:** hardware/HARDWARE_SPEC.md, SATU_ROADMAP.md (both repos)
**Deleted:** hardware/HARDWARE_TRUTH.md (firmware only), CC_BUILD_PROMPT_governance_docs_v2.md (root)
**Flags:** Docs-only. Zero source files touched. hardware.h LOCKED. PAYMENT_MODE stays fake. CI not triggered.

---
## 2026-06-20 — Service menu R13 QA fixes (6 targeted corrections after R12 flash)
**Did:** Targeted rewrite of firmware/ui_service.h only (R13). (1) DEVICES: added "RELAYS" heading using FreeSansBold12pt7b + C_MIDGREY at _DEV_RH_Y=_BDY+52=96 above relay grid. (2) DEVICES: WARNING→IR gap increased +8px (_DEV_IH_Y = _DEV_WARN_Y+38 was +30). (3) DEVICES: cells reduced cw=96→80 ch=44→36; all Devices Y positions recalculated with chained #defines (R1_Y=104, R2_Y=144, WARN_Y=184, IH_Y=222, IR1_Y=230, IR2_Y=270, STUB_Y=312, TBES_Y=354). (4+5) SETTINGS + FIRMWARE: added _svcHeadingSm() helper — NULL size 1, C_GREEN, underline (6px darker) — replaces all FreeSansBold12pt7b section headings in both tabs. (6) All Settings + Firmware sections have ≥6px gap between heading and first content row. Settings Y recalculated: Boot PIN y=156 (was 164), Factory Reset y=340 (was 350), Volume y=314 (was 330). Firmware Y recalculated: Print to Serial y=338 (was 356). All bottom edges verified ≤392 (log panel boundary). firmware/ui.h: getTouchedServiceContent y401=350→340, y402=164→156 (matches new _S_Y401/_S_Y402).
**Updated:** firmware/ui_service.h, firmware/ui.h, PROJECT_STATE.md v1.7, CC_CHAT_LOG.md v2.5
**New files:** NONE
**Pending Chat verify:** Flash; enter service mode; DEVICES: confirm RELAYS heading visible, WARNING/IR gap wider, cells 80×36; SETTINGS: confirm small green underlined headings, Boot PIN tappable at y=156, Factory Reset tappable at y=340; FIRMWARE: confirm small green underlined headings, Print to Serial at y=338.
**Flags:** Free Play + Self Test tabs UNTOUCHED. hardware.h LOCKED. PAYMENT_MODE stays fake. satu_vending.ino NOT touched.

---
## 2026-06-20 — Service menu R12 remaining visual fixes — CC_BUILD_PROMPT_service_menu_fix_v2
**Did:** Complete rewrite of firmware/ui_service.h (R12). Log panel moved from right-side (x=670, w=126) to bottom (_BLOG_Y=SCR_H-80=400, h=72, 4 entries). Content text scale NULL size 2 throughout. Self Test: Quick=140px w, Technical=180px w, Clear=90px w, all h=36; subtitle size 2. Devices: relay/IR cells both cw=96 ch=44; WARNING banner 18px gap; Test Backend centered. Settings: label/value columns (130px label); Boot PIN at _BDY+120=164; Factory Reset h=34 at _BDY+306=350; Volume at _BDY+286=330. Firmware: label/value columns (140px label); Print to Serial at _BDY+312=356; OTA stubs at _BDY+290. Sidebar: _drawSvcTabBar updated to NULL size 2, two-line labels for Self/Test and Free/Play. getTouchedServiceContent: y401=370→350 (h44→h34), y402=168→164; TAB_SELFTEST removed from slot grid check (prevents button collision with new button y=126).
**Updated:** firmware/ui_service.h, firmware/ui.h, PROJECT_STATE.md v1.6, CC_CHAT_LOG.md v2.4
**New files:** docs/prompts/CC_BUILD_PROMPT_service_menu_fix_v2.md (archived ✅ COMPLETE — 2026-06-20 — service menu R12 visual fix)
**Pending Chat verify:** Flash; enter service mode; confirm all 5 tabs; confirm log panel at bottom; confirm sidebar text size 2 two-line; confirm Self Test buttons 140/180/90px; confirm Devices cells 96px wide; confirm Settings Factory Reset at y=350 h=34; confirm Firmware Print to Serial visible.
**Flags:** Free Play tab untouched (passed R11 QA). hardware.h LOCKED — not touched. PAYMENT_MODE stays fake. satu_vending.ino NOT touched.

---
## 2026-06-20 — Service menu 13 visual fixes (R-158) — CC_BUILD_PROMPT_service_menu_fix_v1
**Did:** Complete rewrite of firmware/ui_service.h fixing all 13 visual issues from owner photo QA. GLOBAL: added _svcLogPanel() right-side log panel (x=670, w=126) with 10-entry circular buffer; called _svcLogDraw() at end of every _drawSvcBody_*. TAB 0 Self Test: btnY moved _BDY+60→_BDY+72; subtitle at _BDY+52; resY=166; lineH=18; results drawn in left zone only (not past x=670); _svcLogPanel() called per result. TAB 1 Free Play: fixed 7×3 grid (not g_grid_cols); cw=72 ch=50 gap=4; gridY=_BDY+72; instruction text at _BDY+60. TAB 2 Devices: _REL_CW 107→86 (6 cells now fit left zone); _IR_CW 62→80, _IR_CH 28→36; _IR_ROW1_Y gap +38→+52; new stub row Pump/LED/Speaker at y=327; _TBEKY recalculated=371; stub taps log locally (return 0). TAB 3 Settings: all Y positions now _BDY-relative (_S_Y402=_BDY+138=182, _S_VOLY=_BDY+318=362, _S_Y401=_BDY+348=392); lane prices show L1-L7 row then L8-L10 row. TAB 4 Firmware: all Y positions _BDY-relative (info rows at _BDY+82+16×n, SECURITY at _BDY+192, OTA at _BDY+274/_BDY+292, _FW_PRINT_Y=_BDY+348=392). _getTouchedServiceExtra(): btnY corrected, stub row added, 702 (Boot PIN) and 701 (Factory Reset) hit-tests added.
**Updated:** firmware/ui_service.h, RULES.md v1.9, PROJECT_STATE.md v1.5, CC_CHAT_LOG.md v2.3
**New files:** docs/prompts/CC_BUILD_PROMPT_service_menu_fix_v1.md (archived ✅ COMPLETE)
**Pending Chat verify:** Flash; enter service mode; check all 5 tabs against photo QA list; confirm log panel visible on right; confirm Free Play is 7×3; confirm relay cells narrower (86px); confirm Settings/Firmware Y positions match spec.
**Flags:** Log panel right edge at 796px (4px margin from SCR_W=800). satu_vending.ino NOT touched — all action codes 500-800 unchanged.
