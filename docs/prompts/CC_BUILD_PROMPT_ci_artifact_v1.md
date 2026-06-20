# CC_BUILD_PROMPT_ci_artifact_v1.md
✅ COMPLETE — 2026-06-20 — CI artifact upload (--output-dir + actions/upload-artifact@v4)

> Created by: Chat — 2026-06-20
> Session goal: Save compiled .bin as downloadable GitHub Actions artifact
> Repo: Satu-Vending-Firmware
> Mode: CI/workflow only — zero firmware source changes
> Flash cycles: 0
> PR target: main
> Prerequisite: R10 (PR #38) merged ✅

---

## CC INTRO — PASTE THIS TO CC

New session. Ignore all previous context from other projects.

You are working on SATU 1.0 firmware at:
https://github.com/Csmittee/Satu-Vending-Firmware

Before doing anything else, read IN FULL:
1. CLAUDE.md
2. RULES.md
3. CC_SKILL.md
4. .github/workflows/ — read ALL existing workflow files
5. README.md — if exists

State every file read before writing a single line.
Then execute: CC_BUILD_PROMPT_ci_artifact_v1.md

---

## 2. CONTEXT

Owner has a 2013 MacBook Air 4GB RAM. Local Arduino IDE compile =
3-5 minutes, loud fan, slow. GitHub Actions CI already compiles on
every PR — fast, silent, no fan — but .bin artifact is never saved.

Goal: add artifact upload to existing CI so owner downloads .bin
directly from GitHub Actions tab and flashes with esptool.py.
No Arduino IDE needed ever again for flashing.

Owner flash command (already working):
esptool.py --chip esp32s3 --port /dev/cu.XXXX \
  --baud 921600 write_flash 0x0 satu_vending.ino.bin

esptool.py v5.3.0 confirmed installed on owner Mac.
Board: ESP32S3 Dev Module — 16MB flash — OPI PSRAM — Core 2.0.17

---

## 3. NEW FILES

NONE — modifying existing .github/workflows/ only.

---

## 4. TASKS

**Task 1 — Add artifact upload to existing CI workflow:**
- Read existing workflow file first — find exact .bin output path
  from the compile step (do not guess the path)
- After compile step succeeds, add:
  uses: actions/upload-artifact@v4
  name: satu-firmware-${{ github.run_number }}
  include: .bin file only
  retention-days: 7
- Do NOT change compile step, board config, or anything else

**Task 2 — Add flash instructions to CLAUDE.md:**
- Add new section: "## Flashing Without Arduino IDE"
- Content:
  1. Wait for CI green on PR
  2. GitHub → Actions tab → click the run → Download artifact
  3. Unzip → find satu_vending.ino.bin
  4. Find port: ls /dev/cu.*
  5. Flash: esptool.py --chip esp32s3 --port /dev/cu.XXXX
     --baud 921600 write_flash 0x0 satu_vending.ino.bin
- Keep it short — 10 lines max

---

## 5. DO NOT TOUCH

- hardware.h — R2 LOCKED
- Any .ino or .h firmware source files
- satu-system-tester.html — R-94 never modify
- src/ backend files
- wrangler.toml
- schema.sql
- PAYMENT_MODE — stays fake

---

## 6. VERIFICATION

- Push change to test branch
- Confirm CI runs and goes green
- Confirm artifact appears in Actions tab after compile
- Confirm .bin file is inside downloaded zip
- Confirm no firmware source files were touched

---

## 7. MANDATORY CLOSING

1. Append CC_CHAT_LOG.md at TOP — format per CC_SKILL.md
2. Archive this prompt → docs/prompts/ stamped ✅ COMPLETE — 2026-06-20 — CI artifact upload
3. Append new R-number to RULES.md (newest at TOP) + version bump
4. Update PROJECT_STATE.md — newest session at top + version bump
5. Update KNOWLEDGE_MAP.md — add note: CI produces downloadable .bin artifact
6. Bump version header on every file changed
7. Commit all in order → merge to main
8. Delete this file from repo root after archiving to docs/prompts/

---

## 8. PAYMENT MODE REMINDER

PAYMENT_MODE must remain = fake for this entire session.
Never suggest changing to live.
