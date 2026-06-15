# CC_PROMPT_png_victory_cleanup.md
> Created: 2026-06-15
> Repo: Satu-Vending-Firmware
> Type: Build Mode — multi-file documentation + one firmware cleanup
> Loop: B (firmware) — no flash needed for docs, 1 flash for spy removal

---

## CC INTRO

New session. You are working on SATU 1.0 FIRMWARE at:
https://github.com/Csmittee/Satu-Vending-Firmware

You are joining at a victory moment. Read this context carefully.

Read IN FULL and state each filename before writing anything:
1. CLAUDE.md
2. RULES.md
3. firmware/ui.h
4. CC_PROMPT_png_victory_cleanup.md ← this file

Owner will also attach 4 new files as uploads — read all of them:
5. SKILL_library_onboarding.md    ← new skill
6. LIBRARY_pngdec.md              ← new library reference card
7. SKILL_problem_solving_kt.md    ← append to existing version in repo
8. SKILL_esp32s3_rgb_panel_constraints.md ← append to existing version in repo

State "All files read ✅" then execute this prompt.

---

## WHAT JUST HAPPENED — READ THIS FIRST

After 48 hours, 4 chat sessions, and 20+ PRs, PNG QR decode is now FIXED.

```
CONFIRMED ON HARDWARE 2026-06-15 16:41:32:
[UI] PNG decode: rc=0 rows=165 w=165 h=165
[UI] QR screen drawn
```

Root cause was ONE character in _pngDrawRow():
  `return 0;`  ← PNGdec v1.1.4 release note: "return 0 = stop decode early"
  `return 1;`  ← correct: continue decoding next row

This was documented in the library designer's own release notes.
We never read them. 48 hours lost. New rules R-121/122/123 prevent recurrence.

The fix is already merged (PR #21). This session is cleanup + documentation only.

---

## TASK 1 — Remove spy diagnostic lines from firmware/ui.h

In `_pngDrawRow()`, remove these two diagnostic lines that were added for investigation:

```cpp
Serial.printf("[PNG] cb row=%d y=%d w=%d drawX=%d drawY=%d\n",
              _pngRowCount, pDraw->y, pDraw->iWidth, _pngDrawX, _pngDrawY);
if (_pngRowCount == 0) Serial.printf("[PNG] free heap=%u free psram=%u\n",
              ESP.getFreeHeap(), ESP.getFreePsram());
```

Nothing else changes in ui.h. The rest of the fix (return 1, static lineBuf,
frame buffer collect-then-draw) is already correct and stays.

---

## TASK 2 — Add 4 new skill files to .claude/rules/

### Files 5 + 6 — NEW files (do not exist in repo yet)
Create these two files exactly as provided by owner's attachments:

`.claude/rules/SKILL_library_onboarding.md` ← create new
`.claude/rules/LIBRARY_pngdec.md`           ← create new

Copy content exactly from owner's uploaded files. No edits.

### Files 7 + 8 — APPEND to existing files (already in repo)
These files already exist in `.claude/rules/`. Do NOT replace them.
APPEND the new content from owner's uploads at the TOP of each file,
below the title/header line, above existing content.

`.claude/rules/SKILL_problem_solving_kt.md`              ← append new sections
`.claude/rules/SKILL_esp32s3_rgb_panel_constraints.md`   ← append new sections

Add this header above the appended content:
```
---
## UPDATE — 2026-06-15 (appended from PNG investigation session)
```

---

## TASK 3 — Update RULES.md with new rules

Append at TOP of RULES.md (newest first), using next available R-numbers
after R-120. Check current highest R-number in RULES.md first.

```
R-123: CALLBACK RETURN VALUES — for any library using callbacks, document what
       each return value means in LIBRARY_xxx.md BEFORE writing project code.
       Wrong return value = silent failure that mimics hardware bugs.
       Example: PNGdec return 0 = stop decode. return 1 = continue. (48hr lesson)

R-122: LIBRARY EXAMPLE FIRST — before writing project-specific library code,
       run the designer's own simplest example on target hardware.
       Confirm it works. "Library broken" is never the first hypothesis.

R-121: LIBRARY ONBOARDING — when any new library is added to firmware or backend:
       Chat or CC visits designer's GitHub, reads README + releases + /examples/,
       creates .claude/rules/LIBRARY_[name].md BEFORE writing any code.
       Commit the LIBRARY file first. Code second. No exceptions.
       See: .claude/rules/SKILL_library_onboarding.md for full process.

R-120: NVS writes must not occur during image decode or QR display — idle state only.

R-119: lineBuf in _pngDrawRow must be static (not stack-allocated).

R-118: Product images = JPEG ≤320×320px from backend.
       Only Omise QR = PNG (EMVCo branding requirement — cannot regenerate).

R-117: PNG decode confirmed working with return 1 in callback.
       pause-decode-resume (TFT_BL gate) was tested but not the root cause.
       Root cause was return 0 = stop. Documented in PNGdec v1.1.4 release notes.
```

---

## TASK 4 — Update WORKFLOW_SKILL.md

Add new section after "## SESSION MODES" block:

```markdown
## INTERVENTION LEVELS — CHOOSE THE RIGHT TOOL

4 levels — use the lightest one that fits:

| Level | Who | When | Example |
|---|---|---|---|
| Full CC prompt | CC reads all context | Multi-file, new feature | New screen, API endpoint |
| Hot fix prompt | CC stays in context, no re-read | Single function change | Fix one function |
| Rapid fire phrase | Chat tells owner exactly what to type to CC | One line change | Change one parameter |
| Owner direct edit | Owner edits GitHub UI directly | One character | return 0 → return 1 |

**Rule: never use a heavier intervention than needed. Saves tokens and time.**

## LIBRARY ONBOARDING — MANDATORY (R-121)

When any new library is added:
1. CC visits designer's GitHub — reads README + releases + examples
2. Creates .claude/rules/LIBRARY_[name].md before writing any code
3. Documents callback return values, memory requirements, known failure modes
4. Runs designer's example on hardware first (R-122)

See: .claude/rules/SKILL_library_onboarding.md

## PROBLEM SOLVING — KT FRAMEWORK (R-111 trigger)

When any fix fails twice on the same symptom → STOP → invoke KT IS/IS-NOT:
See: .claude/rules/SKILL_problem_solving_kt.md

Quick checklist:
- IS: where/when/what the problem EXISTS
- IS NOT: where/when/what it does NOT exist
- Hypothesis must explain BOTH IS and IS-NOT or it is eliminated
- Send a spy (Serial.printf) before sending a fix
- Read library designer's docs before any hardware hypothesis
```

---

## TASK 5 — Update KNOWN_GOOD.md at TOP

```
## 2026-06-15 — PNG QR DECODE FIXED ✅ CONFIRMED ON HARDWARE
- Root cause: _pngDrawRow() returned 0 (PNGdec stop-early signal) instead of 1
- Fix: return 1 in callback — one character
- Evidence: rc=0 rows=165 w=165 h=165 on SATU-4R473R hardware
- PRs: #21 (frame buffer + return 1), spy lines removed this session
- Flash: ✅ CONFIRMED WORKING — QR renders correctly
- Skills added: SKILL_library_onboarding.md, LIBRARY_pngdec.md
- Rules added: R-117 through R-123
```

---

## TASK 6 — Update PROJECT_STATE.md

Mark these as COMPLETE:
```
PNG QR decode: ✅ FIXED 2026-06-15
  - rc=0 rows=165 confirmed on hardware
  - Root cause: return 0 in callback = PNGdec stop-early
  - All future PNG/JPEG decode: use return 1 in callbacks
  - Reference: .claude/rules/LIBRARY_pngdec.md
```

---

## DO NOT TOUCH
- hardware.h — R2 LOCKED — never open
- config.h — never touch
- network.h — working correctly
- state_machine.h — read only
- satu_vending.ino — no change needed
- Any PNG decode logic that was fixed — return 1 stays, static lineBuf stays
- PAYMENT_MODE stays fake

---

## VERIFICATION — CC confirms before committing

1. _pngDrawRow() has NO Serial.printf spy lines
2. _pngDrawRow() still has `return 1` ← must not accidentally revert to 0
3. _pngDrawRow() still has `static uint16_t lineBuf[800]`
4. .claude/rules/ contains all 4 skill files
5. RULES.md has R-117 through R-123 at TOP
6. WORKFLOW_SKILL.md has intervention levels + library onboarding sections
7. KNOWN_GOOD.md has PNG fix entry at TOP
8. PROJECT_STATE.md marks PNG as complete
9. No other files modified

---

## CI — MANDATORY BEFORE PR

1. Push to branch: `docs/png-victory-cleanup`
2. GitHub Actions → wait for green (~3 min)
3. ✅ GREEN → PR titled:
   `docs: PNG victory cleanup — spy removal + skills + rules R-117-R-123`
4. PR body:
   ```
   GitHub Actions: ✅ GREEN
   PNG QR decode confirmed fixed: rc=0 rows=165 on hardware 2026-06-15.
   Root cause: return 0 in _pngDrawRow() = PNGdec stop-early (v1.1.4 feature).
   This PR: removes spy diagnostics, adds 4 skill files, updates rules R-117-R-123,
   updates WORKFLOW_SKILL with intervention levels and library onboarding process.
   ```

---

## MANDATORY — END OF SESSION

1. Archive this prompt → docs/prompts/ stamped:
   `✅ COMPLETE — 2026-06-15 — PNG victory cleanup — rc=0 rows=165 confirmed`
2. RULES.md updated ✅
3. PROJECT_STATE.md updated ✅
4. KNOWN_GOOD.md updated ✅
5. WORKFLOW_SKILL.md updated ✅
6. Merge to main

## PAYMENT MODE REMINDER
PAYMENT_MODE stays fake. Not relevant to this session.
