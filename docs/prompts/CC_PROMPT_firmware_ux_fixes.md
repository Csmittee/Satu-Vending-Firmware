# CC_PROMPT_firmware_ux_fixes.md
> ✅ COMPLETE — 2026-06-16 — firmware UX fixes R-126
> Created by: Chat (Claude)
> Date: 2026-06-16
> Session goal: Fix 4 UX issues found during first full hardware test
> Repo: Satu-Vending-Firmware
> Mode: Fix Mode — firmware only (satu_vending.ino, ui.h)
> Flash cycles: 1 (owner flashes after merge)
> PR target: main
> Sequence: Prompt 1 of 1 — self-contained firmware UX fixes

---

## OWNER ACTION BEFORE RUNNING CC

1. Save this file as `CC_PROMPT_firmware_ux_fixes.md` in ROOT of
   `Csmittee/Satu-Vending-Firmware` repo
2. Confirm GitHub sync checked
3. Paste CC INTRO below into new CC session

---

## CC INTRO — PASTE THIS TO CC

```
Read CLAUDE.md, RULES.md, PROJECT_STATE.md from:
https://github.com/Csmittee/Satu-Vending-Firmware

Then execute: CC_PROMPT_firmware_ux_fixes.md

You are NOT bound by any solution in this prompt.
Verify root cause from live files, then fix correctly.
```

---

## CONTEXT — FIRST FULL HARDWARE TEST RESULTS

Full end-to-end hardware test completed 2026-06-16 on SATU-4R473R.
Payment → relay → door → completion → idle — all working.

4 UX issues found and reported by owner. CC (backend session) already
analyzed root causes. This prompt implements the firmware fixes.

CC backend analysis summary (read-only context, not instructions):
- #1 touch delay: idleAnimationUI() blocks main loop ~16ms/frame,
  touchCheckIdle() never gets CPU time between frames
- #2 completion HTTP 400: FIXED in backend session (hwDispense field names)
- #3 long dispense time: was caused by #2 — now fixed via backend
- #4 font quality: GFX bitmap fonts scaled up look blocky — need
  native-size Adafruit fonts instead of scaled bitmaps

---

## FIX 1 — Touch delay on idle screen (#1)

### Root cause (confirmed by CC backend session)
`idleAnimationUI()` in `ui.h` runs a blocking animation loop ~16ms per frame.
`touchCheckIdle()` in `satu_vending.ino` is called after the animation loop
returns — but the loop never yields. Touch is only checked between full
animation cycles, causing 16ms+ delay per touch event. Feels unresponsive,
requires double-tap.

### What CC must do
Read `ui.h` idleAnimationUI() and `satu_vending.ino` main loop / STATE_IDLE
handler to understand the current animation + touch check structure.

Fix: move touch check INSIDE the animation loop so it is polled every frame.
If touch detected mid-animation, break out immediately and process touch.
The animation must not block touch response for more than 1 frame (~16ms).

Do NOT remove the idle animation — just make it interruptible.
Do NOT change animation speed or visual appearance.

---

## FIX 2 — Font quality on large text (#4)

### Root cause (confirmed by CC backend session)
Arduino_GFX 1.4.9 scales small bitmap fonts up for large sizes.
Scaling causes blocky/pixelated appearance on large text (amounts, titles).
Native Adafruit GFX fonts render at their designed size — no scaling artifacts.

### What CC must do
Read `ui.h` to identify which screens use large scaled fonts (QR amount,
idle screen title, vending screen, completion screen lucky number).

Fix: replace scaled bitmap font calls with appropriately-sized Adafruit GFX
fonts from the FreeFonts included with Adafruit_GFX library. These are already
available in Arduino IDE — no new library needed.

Suggested mapping (CC must verify availability and adjust):
- Large amounts (฿ 2,000): FreeSansBold24pt7b or FreeSansBold18pt7b
- Screen titles: FreeSans12pt7b or FreeSans9pt7b  
- Body text: keep existing small font — already renders cleanly

CC must verify font names against what Arduino_GFX + Adafruit_GFX actually
provide in the installed version (1.4.9). Do not guess font names.
If a font is not available, use the closest available size.

After font change, verify text still fits within screen bounds (800x480).
Adjust x/y positioning if needed to maintain visual alignment.

---

## DO NOT TOUCH

- hardware.h — R2 LOCKED — never modify
- config.h PAYMENT_TIMEOUT_MS — currently 60000 (1 min for HW testing)
- network.h — no changes
- state_machine.h — no changes
- satu_observer.ino — no changes
- Backend files — firmware repo only

---

## ARDUINO IDE SETTINGS (never change)
```
Board:     ESP32S3 Dev Module
Flash:     16MB
Partition: 16M Flash (3MB APP/9.9MB FATFS)
PSRAM:     OPI PSRAM
Upload:    460800
Core:      2.0.17 ONLY
```

## LIBRARIES (locked)
```
GFX Library for Arduino  1.4.9
TAMC_GT911               latest
ArduinoJson              7.x
Adafruit MCP23017        2.x
FastLED                  3.x
PNGdec                   1.1.6  (return 1 = continue, return 0 = stop — R-123)
```

---

## RULE TO ADD — R-126

Append to RULES.md at TOP:
```
R-126: Idle animation must NEVER block touch polling.
Touch check must be called inside the animation frame loop, not after it.
Maximum touch latency = 1 frame (~16ms). Double-tap requirement = animation bug.
Any future animation added to idle screen must follow this same pattern.
Font rule: never scale small bitmap fonts for large display sizes.
Use native Adafruit GFX FreeFonts at their designed size instead.
(Added 2026-06-16)
```

---

## PROJECT_STATE.md UPDATES

Add to SESSION LOG (newest at TOP):
```
### 2026-06-16 — Firmware UX fixes post first hardware test (R-126)
- First full end-to-end hardware test PASSED on SATU-4R473R
  Payment → relay → door → completion → idle — confirmed working
- Fix 1: touch delay on idle — touchCheckIdle() moved inside animation loop
- Fix 2: font quality — replaced scaled bitmaps with Adafruit FreeFonts
  on large text screens (amount, title, lucky number)
- Items #2 and #3 (completion HTTP 400 + long dispense) fixed in backend
  session (PR #26) — not firmware issues
- R-126 added to RULES.md
- Flash required after merge
```

Add to OPEN ITEMS:
```
- [ ] PAYMENT_TIMEOUT_MS — return to 120000 before temple deployment
      Currently 60000 (1 min) for HW testing. config.h owner direct edit.
- [ ] Service mode firmware — ui.h 5 tabs full build (stubs only)
      Next firmware CC session after this one.
```

---

## MANDATORY END OF SESSION (R-84)

1. Archive → `docs/prompts/` stamped:
   `✅ COMPLETE — 2026-06-16 — firmware UX fixes R-126`
2. Append R-126 to RULES.md at TOP
3. Update PROJECT_STATE.md — session log + open items (above)
4. Overwrite CHAT_HANDOFF.md with current state
5. Update KNOWN_GOOD.md — add at TOP:
   `2026-06-16 — R-126 firmware UX — touch + font fixes — flash required`
6. Commit: `fix: idle touch latency + font quality R-126`
7. Merge to main — owner flashes after merge

---

## PAYMENT MODE REMINDER
PAYMENT_MODE stays fake. Never suggest changing to live.
