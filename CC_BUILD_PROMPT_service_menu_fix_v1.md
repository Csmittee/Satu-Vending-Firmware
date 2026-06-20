# CC_BUILD_PROMPT_service_menu_fix_v1.md
> Created by: Chat — 2026-06-20
> Session goal: Fix all 13 visual issues in service menu identified by owner photo QA
> Repo: Satu-Vending-Firmware
> Mode: Firmware — ui_service.h primary, ui.h minor, RULES.md docs
> Flash cycles: 1 expected
> PR target: main
> Prerequisite: R10 (PR #38) merged ✅ — this is the fix pass

---

## CC INTRO — PASTE THIS TO CC

```
Read and execute: CC_BUILD_PROMPT_service_menu_fix_v1.md

New session. Ignore all previous context from other projects.
You are working on SATU 1.0 firmware at:
https://github.com/Csmittee/Satu-Vending-Firmware

Before doing anything else, read IN FULL:
1. CLAUDE.md
2. RULES.md
3. CC_SKILL.md
4. firmware/ui_service.h   ← PRIMARY FILE this session
5. firmware/ui.h           ← read layout constants + svcLog + SVC_BODY_X
6. firmware/config.h       ← read NUM_SLOTS, g_grid_rows, g_grid_cols

State every file read before writing a single line.
Then execute: CC_BUILD_PROMPT_service_menu_fix_v1.md
```

---

## 2. CONTEXT

Owner flashed R10 (PR #38) and photographed all 5 service menu tabs.
Photo QA confirmed 13 visual issues across all tabs.
Two root causes affect every tab:

ROOT CAUSE A — Section headings drawn with zero top padding.
Every tab draws a large title (FreeSansBold18pt7b) then immediately draws
a section heading (_svcHeading) at y = title_y + ~12px. The heading
font baseline overlaps the title descenders. Fix: increase vertical
spacing between title and first section heading by minimum 20px.

ROOT CAUSE B — Right half of screen (x > ~550) is empty on all tabs.
Log panel was specced in the original prompt but never drawn.
Fix: add a persistent log panel on right side of every tab.

Screen dimensions confirmed: SCR_W=800 SCR_H=480 SVC_BODY_X=114
Available body width: 800-114 = 686px
Split: left content zone 0→540px from bodyX, right log panel 550→780px from bodyX.

---

## 3. NEW FILES

NONE — modifying firmware/ui_service.h only for visual fixes.
RULES.md gets R-158 appended (docs only).

---

## 4. TASKS — 13 ISSUES, ALL IN ui_service.h

### GLOBAL FIX — Right-side log panel (affects all 5 tabs)

Add a persistent log panel drawn on the right side of every tab body.
Panel position: x = SVC_BODY_X + 550, y = STATUS_H + 10, w = 220, h = SCR_H - STATUS_H - 20
Panel style: dark background color565(10,6,18), border C_MIDGREY 1px
Title: "LOG" in C_GREY size 1 at panel top
Content: 10 lines monospace, auto-scroll newest at bottom
Add function: `void _svcLogPanel(String msg)` — appends to panel, redraws panel only
Replace all existing `svcLog()` calls in ui_service.h with `_svcLogPanel()`
Keep `svcLog()` in ui.h unchanged — it is used by other screens

---

### TAB 0 — SELF TEST (Issues 1, 3)

**Issue 1 — Section heading overlap:**
Current: _svcHeading drawn at _BDY + 48 = y92, title drawn at _BDY + 36 = y80
Fix: move title to _BDY + 30, move subtitle "Test MCP..." to _BDY + 52,
move buttons (Quick Test / Technical Test / Clear) to _BDY + 72
Add 8px top padding before any heading drawn after title.

**Issue 3 — No results area:**
Current: _runSelfTest() draws results starting at btnY + btnH + 12 = y154
but results only go to left half. Right half is the new log panel.
Fix: results draw in left zone x=bodyX+16 to x=bodyX+530, line height 18px
Each result line: [PASS] in C_GREEN or [FAIL] in C_RED, then label in C_WHITE
Results area: y=154 to y=440, max 16 lines visible
Scroll if more than 16 items (technical = 14 so no scroll needed now)
Log panel on right receives each result as it completes: "PASS: WiFi" or "FAIL: MCP1"

---

### TAB 1 — FREE PLAY (Issues 4, 5)

**Issue 4 — Grid layout 5×4 wrong:**
Current: cols = g_grid_cols > 0 ? g_grid_cols : 5 — this gives 5 cols
Fix: Free Play grid must ALWAYS use 7 columns × 3 rows = 21 slots max
regardless of g_grid_cols (which reflects idle screen layout, not service grid)
Cell size: cw=76 ch=52 gap=4
Grid start: x=bodyX+16 y=_BDY+72
This fills left zone (7×76 + 6×4 = 556px — fits in 540px zone if cw=74)
Recalculate: cw=74 ch=50 gap=4 → 7×74 + 6×4 = 542px ✅

**Issue 5 — Right half empty:**
Log panel (global fix above) fills right side.
Add instruction text above grid: "Tap slot to fire motor — no payment needed"
in C_GREY size 1 at y = _BDY + 60.
Log panel receives: "Slot N fired" / "Motor ON" / "Timeout 3000ms" / "Motor OFF"

---

### TAB 2 — DEVICES (Issues 6, 7, 8, 9)

**Issue 6 — WARNING text overlapped by IR SENSORS title:**
Current: WARNING drawn at _REL_ROW2_Y + _REL_CH + 4 = too close to IR heading
Fix: add 8px gap after WARNING text before IR SENSORS heading
Recalculate _IR_ROW1_Y = _REL_ROW2_Y + _REL_CH + 52 (was 38)
Update _IR_ROW2_Y, _TBEKY accordingly

**Issue 7 — Relay and IR cell sizes inconsistent:**
Current: relay cells _REL_CW=107 _REL_CH=44, IR cells _IR_CW=62 _IR_CH=28
Fix: IR cells to _IR_CW=80 _IR_CH=36 — closer to relay cell proportion
IR label format: "S1" centered top, "CLEAR"/"BLOCK" centered bottom
Relay label format: "R1" centered top, "OFF"/"ON" or "LOCKED"/"UNLOCKED" centered bottom
Both use same font (size 1 monospace) and same padding — consistent theme

**Issue 8 — Missing water pump, LED, speaker stubs:**
After IR SENSORS section add:
- Water Pump row: single button [R11 PUMP] same style as relay cell
  tap = toggle relay 11, log panel: "Pump ON" / "Pump OFF"
- LED Test row: [LED TEST] button — stub, log panel: "LED: assign WS_PIN"
- Speaker row: [SPEAKER TEST] button — stub, log panel: "Speaker: assign SPEAKER_PIN"
All three in left zone below IR grid, before Test Backend button
Recalculate _TBEKY to be below these new rows

**Issue 9 — No tap feedback / log:**
All relay taps, IR reads, Test Backend — route result to _svcLogPanel()
Format: "R1 ON", "R12 LOCKED", "S3 BLOCK", "Backend OK / UNREACHABLE"

---

### TAB 3 — SETTINGS (Issue 10)

**Issue 10 — All section headings overlap previous content:**
Current: Settings / NETWORK / IP / SERVICE ACCESS / Boot PIN / DISPLAY CONFIG /
LANE PRICES / AUDIO all stacked with insufficient vertical spacing.

Fix — rewrite _drawSvcBody_Settings() with explicit Y positions:
```
Title "Settings"    FreeSansBold18pt7b    y = _BDY + 30
"NETWORK"           _svcHeading           y = _BDY + 62   ← +32 from title
WiFi SSID line      size 1                y = _BDY + 82
IP line             size 1                y = _BDY + 98
"SERVICE ACCESS"    _svcHeading           y = _BDY + 122  ← +24 gap
Boot PIN button     h=38                  y = _BDY + 138
"DISPLAY CONFIG"    _svcHeading           y = _BDY + 196  ← +20 gap after button
Idle/Select/Water   size 1                y = _BDY + 216
"LANE PRICES"       _svcHeading           y = _BDY + 240  ← +24 gap
L1-L7 row           size 1                y = _BDY + 258
L8-L10 row          size 1                y = _BDY + 274 (if needed)  
"AUDIO"             _svcHeading           y = _BDY + 298  ← +24 gap
Volume line         size 1                y = _BDY + 318
Factory Reset btn   h=44                  y = _BDY + 348
```
All headings: _svcHeading() with C_GREEN (matches photo style)
All data lines: C_WHITE size 1

---

### TAB 4 — FIRMWARE (Issue 12)

**Issue 12 — Section headings overlap data rows:**
Same pattern as Settings. Fix with explicit Y positions:
```
Title "Firmware"         FreeSansBold18pt7b  y = _BDY + 30
"CURRENT FIRMWARE"       _svcHeading         y = _BDY + 62
Version line             size 1              y = _BDY + 82
Build line               size 1              y = _BDY + 98
Board line               size 1              y = _BDY + 114
Flash/PSRAM line         size 1              y = _BDY + 130
MAC line                 size 1              y = _BDY + 146
Heap line                size 1              y = _BDY + 162
"SECURITY (dev mode)"    _svcHeading         y = _BDY + 192  ← +30 gap
Flash Encrypt line       size 1              y = _BDY + 212
Secure Boot line         size 1              y = _BDY + 228
JTAG line                size 1              y = _BDY + 244
"REMOTE OTA (stub)"      _svcHeading         y = _BDY + 274  ← +30 gap
Check Update button      h=36                y = _BDY + 292
Force OTA button         h=36                y = _BDY + 292  (same row, offset x)
Print to Serial button   h=38                y = _BDY + 348
```

---

## 5. DO NOT TOUCH

- hardware.h — R2 LOCKED
- satu_vending.ino — action handlers 500-800 already correct, do not touch
- state_machine.h — do not touch
- network.h — do not touch
- config.h — do not touch
- satu-system-tester.html — R-94 never modify
- src/ backend files — do not touch
- wrangler.toml — do not touch
- PAYMENT_MODE — stays fake
- NUM_SLOTS — defined in config.h only, never redefine
- ui.h svcLog() — keep as is, add _svcLogPanel() to ui_service.h separately

---

## 6. VERIFICATION — UI PR CHECKLIST (R-158)

Before closing PR, CC must verify and check every item:

- [ ] All grids use fixed 7×3 in Free Play — not g_grid_cols
- [ ] No text element wider than SCR_W - SVC_BODY_X - 32 = 654px
- [ ] Every button frame width >= text pixel length + 32px padding
- [ ] Relay cells and IR cells use same proportional theme
- [ ] Minimum font: size 1 monospace (6px/char) for all user-facing labels
- [ ] Section headings minimum 20px below previous content baseline
- [ ] Every tab has right-side log panel drawn
- [ ] Log panel receives output from every interactive element
- [ ] No UI element overlaps tab header (STATUS_H = 44px boundary)
- [ ] R11/R12 special labels: PUMP / LOCKED/UNLOCKED — not generic ON/OFF
- [ ] All label formats consistent: R1-R12 relays, S1-S10 sensors
- [ ] Firmware tab data rows left-aligned label, right-aligned value
- [ ] Settings tab all sections have minimum 24px gap between them

CI must be GREEN before merge.

---

## 7. MANDATORY CLOSING

1. Append CC_CHAT_LOG.md at TOP — format per CC_SKILL.md
2. Archive this prompt → docs/prompts/ stamped ✅ COMPLETE — 2026-06-20 — service menu fix 13 issues
3. Append R-158 to RULES.md at TOP + version bump:
   "R-158: UI PR CHECKLIST — CC must include and fill this checklist in every PR
   touching any .h UI file. PR cannot merge without all items checked.
   Chat verifies checklist present before telling owner to flash. (2026-06-20)"
4. Update PROJECT_STATE.md — newest session at top + version bump
5. Update KNOWN_GOOD.md — do NOT update until owner re-QA confirms pass
6. Bump version header on every file changed
7. Commit all in order → merge to main
8. Delete this file from repo root after archiving to docs/prompts/

---

## 8. PAYMENT MODE REMINDER

PAYMENT_MODE must remain = fake for this entire session.
Never suggest changing to live.
