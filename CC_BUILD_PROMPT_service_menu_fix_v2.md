# CC_BUILD_PROMPT_service_menu_fix_v2.md
> Created by: Chat — 2026-06-20
> Session goal: R12 — Fix remaining visual issues from R11 QA photo review
> Repo: Satu-Vending-Firmware
> Mode: Firmware — ui_service.h only
> Flash cycles: 1 expected
> PR target: main
> Prerequisite: R11 (PR #38 fix) merged ✅

---

## CC INTRO — PASTE THIS TO CC

```
New session. Ignore all previous context from other projects.
You are working on SATU 1.0 firmware at:
https://github.com/Csmittee/Satu-Vending-Firmware

Before doing anything else, read IN FULL:
1. CLAUDE.md
2. RULES.md
3. CC_SKILL.md
4. firmware/ui_service.h   ← PRIMARY FILE — rewrite this session
5. firmware/ui.h           ← read SVC_BODY_X, STATUS_H, font constants only

State every file read before writing a single line.
Then execute: CC_BUILD_PROMPT_service_menu_fix_v2.md
```

---

## 2. CONTEXT — WHAT R11 PHOTO QA FOUND

Owner flashed R11 (v1.0.0-r5) and photographed all 5 tabs.
Free Play tab PASSED. All other tabs have remaining issues.

**CONFIRMED PASSING — DO NOT TOUCH:**
- Free Play tab: 7×3 grid ✅, slot cells ✅, instruction text ✅

**SCREEN DIMENSIONS (confirmed):**
- SCR_W=800 SCR_H=480
- SVC_BODY_X=114 (left sidebar width)
- STATUS_H=44 (top header height)
- Body area: x=114→800, y=44→480
- Body width available: 686px
- Body height available: 436px

---

## 3. TYPOGRAPHY RULES — APPLY TO ALL TABS

This is the single most important fix. Current problem: Menu Title is too large,
Section headings are almost same size as Menu Title, content text is too small.

**NEW TYPE SCALE — replace all existing font usage in ui_service.h:**

| Element | Font | Approx px height |
|---|---|---|
| Menu Title (e.g. "Self Test") | FreeSansBold18pt7b | ~24px — KEEP CURRENT |
| Section Heading (e.g. "NETWORK") | FreeSansBold12pt7b | ~16px — DOWN from 18pt |
| Content / data rows | NULL size 2 | ~16px — UP from size 1 |
| Button labels | NULL size 2 | ~16px |
| Tab sidebar labels | NULL size 2 | ~16px — UP from size 1 |
| Log panel text | NULL size 1 | ~8px — keep small |

**Rules:**
- Menu Title: FreeSansBold18pt7b — one per tab, top of body
- Section headings: FreeSansBold12pt7b — NOT 18pt — smaller than title
- Content rows: NULL size 2 — more readable than size 1
- After every FreeSans block: call setFont(NULL) before drawing anything else
- Minimum gap between Menu Title baseline and first Section Heading top: 28px
- Minimum gap between Section Heading baseline and first content row: 12px
- Minimum gap between last content row baseline and next Section Heading top: 20px

---

## 4. LOG PANEL — MOVE TO BOTTOM, ALL TABS

**Remove** the right-side log panel from all tabs entirely.

**Add** a bottom log panel drawn on every tab:
```
Position: x=SVC_BODY_X+8, y=SCR_H-80, w=SCR_W-SVC_BODY_X-16, h=72
Style: dark background color565(10,6,18), border C_MIDGREY 1px
Title: "LOG" in C_GREY NULL size 1 at x+4, y+10
Content area: x+4, y+18 — 4 lines, NULL size 1, newest line at bottom
```

Log buffer: static String _svcLog[4] — shift up on new entry, newest at bottom.

Function: `void _svcLogPanel(String msg)` — shifts buffer, redraws panel only.
Call `_svcLogPanel()` for every interactive event in every tab.
Keep `svcLog()` in ui.h unchanged.

**Content body height** for each tab must stop at y = SCR_H - 88 to leave room for log panel.
Max usable body: y=44 to y=392 = 348px for content.

---

## 5. SERIAL MIRROR LOG — NEW FEATURE

Every `_svcLogPanel(String msg)` call must ALSO call `Serial.println("[SVC] " + msg)`.
This mirrors all service mode events to serial monitor automatically.
No new action code needed — just add the Serial.println inside _svcLogPanel().

This replaces the need for a separate "debug mode" toggle.
Every tap, every test result, every relay toggle appears in serial monitor.

---

## 6. TAB FIXES

### TAB 0 — SELF TEST

Layout with new type scale and bottom log panel:
```
"Self Test"          FreeSansBold18pt7b    y = STATUS_H + 28
subtitle text        NULL size 2           y = STATUS_H + 58
[Quick Test] [Technical Test] [Clear]      y = STATUS_H + 82   h=36
```

Button width fix:
- [Quick Test]: min width = 140px
- [Technical Test]: min width = 180px  ← was overflowing frame
- [Clear]: min width = 90px
- All buttons: same height 36px, gap 8px between

Results area (draws below buttons when test runs):
```
x = SVC_BODY_X + 8
y = STATUS_H + 130
w = SCR_W - SVC_BODY_X - 16
max_y = SCR_H - 88   ← stops above log panel
line_height = 20px
font: NULL size 2
```
Each line: "[PASS]" in C_GREEN or "[FAIL]" in C_RED — then label in C_WHITE.
"(sim)" suffix in C_GREY when simulated=true.
_svcLogPanel() receives each result as test runs.

When no test run yet: show "Tap Quick Test or Technical Test to begin." in NULL size 2.

---

### TAB 2 — DEVICES

**Issue A — WARNING text overlapped by IR SENSORS heading:**
Add explicit gap. WARNING text draws at _REL_ROW2_Y + _REL_CH + 8.
IR SENSORS heading draws at WARNING_Y + 28 minimum.
Use FreeSansBold12pt7b for "IR SENSORS" heading (not 18pt).

**Issue B — Relay grid (2×6) and IR grid (2×5) cell size mismatch:**
- Relay cells: cw=96 ch=44 — 6 per row, 2 rows = 12 cells
- IR cells: cw=96 ch=44 — SAME SIZE as relay cells, 5 per row, 2 rows = 10 cells
- Both grids left-aligned at x=SVC_BODY_X+8
- Label format: relay top="R1" bottom="OFF"/"ON"/"LOCKED"/"UNLOCKED"
               IR top="S1" bottom="CLEAR"/"BLOCK"

**Issue C — Pump R11, LED, Speaker buttons too small:**
Draw as full-width row buttons, not small inline buttons:
```
[Pump R11]     w=180 h=36   x=SVC_BODY_X+8
[LED Test]     w=140 h=36   x=SVC_BODY_X+196
[Speaker]      w=140 h=36   x=SVC_BODY_X+344
```
All at same y, below IR grid + 12px gap.

**Issue D — Test Backend button overflow:**
Min width = 200px, h=40, centered in content zone.

All interactions → _svcLogPanel() + Serial mirror.

---

### TAB 3 — SETTINGS

Rewrite _drawSvcBody_Settings() entirely with explicit Y positions.
Use FreeSansBold12pt7b for ALL section headings (not 18pt).
Use NULL size 2 for ALL content rows.
Stop content at y = SCR_H - 88.

```
"Settings"               FreeSansBold18pt7b    y = STATUS_H + 28
"NETWORK"                FreeSansBold12pt7b    y = STATUS_H + 62
WiFi SSID                NULL size 2           y = STATUS_H + 84
IP Addr                  NULL size 2           y = STATUS_H + 104
"SERVICE ACCESS"         FreeSansBold12pt7b    y = STATUS_H + 130
[Boot PIN: ON/OFF]       NULL size 2 button    y = STATUS_H + 150  h=36
"DISPLAY CONFIG"         FreeSansBold12pt7b    y = STATUS_H + 202
Idle/Select/Water/Grid   NULL size 2           y = STATUS_H + 222  (one line, comma separated)
"LANE PRICES"            FreeSansBold12pt7b    y = STATUS_H + 250
L1-L5 row                NULL size 2           y = STATUS_H + 268
L6-L10 row               NULL size 2           y = STATUS_H + 286
"AUDIO"                  FreeSansBold12pt7b    y = STATUS_H + 312
Volume                   NULL size 2           y = STATUS_H + 330
[Factory Reset]          RED button h=40       y = STATUS_H + 352
```

Content row format: left-aligned LABEL in C_GREY, then ": ", then VALUE in C_WHITE.
Example: draw "WiFi SSID" in grey at x=SVC_BODY_X+12, then value at x=SVC_BODY_X+140.
All labels right-pad to consistent column: label column 130px wide, value starts at +130.

---

### TAB 4 — FIRMWARE

Rewrite _drawSvcBody_Firmware() entirely with explicit Y positions.
Use FreeSansBold12pt7b for ALL section headings.
Use NULL size 2 for ALL content rows.
Stop content at y = SCR_H - 88.

```
"Firmware"               FreeSansBold18pt7b    y = STATUS_H + 28
"CURRENT FIRMWARE"       FreeSansBold12pt7b    y = STATUS_H + 62
Version                  NULL size 2           y = STATUS_H + 82
Build                    NULL size 2           y = STATUS_H + 100
Board                    NULL size 2           y = STATUS_H + 118
Flash/PSRAM              NULL size 2           y = STATUS_H + 136
MAC                      NULL size 2           y = STATUS_H + 154
Heap/PSRAM free          NULL size 2           y = STATUS_H + 172
"SECURITY"               FreeSansBold12pt7b    y = STATUS_H + 198
Flash Encrypt            NULL size 2           y = STATUS_H + 218  amber color
Secure Boot              NULL size 2           y = STATUS_H + 236  amber color
JTAG                     NULL size 2           y = STATUS_H + 254  amber color
"REMOTE OTA"             FreeSansBold12pt7b    y = STATUS_H + 280
[Check Update] [Force OTA]  NULL size 2 btns   y = STATUS_H + 298  same row
[Print to Serial]           GREEN btn h=40     y = STATUS_H + 348
```

Content row format: same label/value column layout as Settings.
Label column 140px wide. Value starts at +140.
Security values: amber color565(180,120,0).

---

### TAB SIDEBAR — font fix

Left sidebar tab labels currently NULL size 1 — too small for frame.
Change to NULL size 2 for all 5 tab labels.
Active tab: C_GOLD text + gold left border 3px.
Inactive tab: C_GREY text.

---

## 7. DO NOT TOUCH

- hardware.h — R2 LOCKED
- satu_vending.ino — action handlers 500-800 correct, do not touch
- state_machine.h — do not touch
- network.h — do not touch
- config.h — do not touch
- firmware/ui.h svcLog() — do not touch, _svcLogPanel() is separate
- Free Play tab (_drawSvcBody_FreePlay) — PASSED QA, do not touch
- satu-system-tester.html — R-94, never modify
- PAYMENT_MODE — stays fake

---

## 8. VERIFICATION — R-158 UI PR CHECKLIST

CC must paste this checklist in the PR description with every item checked:

- [ ] Menu Title = FreeSansBold18pt7b, Section Heading = FreeSansBold12pt7b, Content = NULL size 2
- [ ] setFont(NULL) called after every FreeSans block
- [ ] Minimum 28px gap: Menu Title baseline → first Section Heading top
- [ ] Minimum 20px gap: last content row → next Section Heading top
- [ ] Bottom log panel on every tab: y=SCR_H-80, h=72
- [ ] Content body stops at y=SCR_H-88 — nothing draws over log panel
- [ ] _svcLogPanel() also calls Serial.println("[SVC] " + msg)
- [ ] Self Test buttons: Quick=140px, TechnicalTest=180px, Clear=90px — no overflow
- [ ] Devices: relay cells and IR cells same size cw=96 ch=44
- [ ] Devices: WARNING text minimum 28px above IR SENSORS heading
- [ ] Devices: Test Backend button min width 200px
- [ ] Settings: label column 130px, value right of label — consistent alignment
- [ ] Firmware: label column 140px, value right of label — consistent alignment
- [ ] Sidebar tab labels NULL size 2
- [ ] Free Play tab untouched — no changes
- [ ] CI GREEN before merge

---

## 9. MANDATORY CLOSING (R-84)

1. Append CC_CHAT_LOG.md at TOP — format per CC_SKILL.md
2. Archive this prompt → docs/prompts/ stamped:
   `✅ COMPLETE — 2026-06-20 — service menu R12 visual fix`
3. Append R-158 to RULES.md at TOP (if not already added):
   "R-158: UI PR CHECKLIST mandatory for every PR touching UI .h files. (2026-06-20)"
4. Update PROJECT_STATE.md — newest session at top + version bump
5. KNOWN_GOOD.md — do NOT update until owner re-QA confirms pass
6. Bump version header on every file changed
7. Commit → merge to main after CI GREEN
8. Delete this file from repo root after archiving

---

## 10. PAYMENT MODE REMINDER

PAYMENT_MODE must remain = fake.
Never suggest changing to live.
