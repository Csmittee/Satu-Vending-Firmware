# WORKFLOW_SKILL.md — Satu 1.0
> Single source of truth: Satu-vending-backend/WORKFLOW_SKILL.md
> This copy: firmware repo mirror — kept in sync by Chat after each session
> Last updated: 2026-06-15

---

## INTERVENTION LEVELS — CHOOSE THE RIGHT TOOL

4 levels — use the lightest one that fits:

| Level | Who | When | Example |
|---|---|---|---|
| Full CC prompt | CC reads all context | Multi-file, new feature | New screen, API endpoint |
| Hot fix prompt | CC stays in context, no re-read | Single function change | Fix one function |
| Rapid fire phrase | Chat tells owner exactly what to type to CC | One line change | Change one parameter |
| Owner direct edit | Owner edits GitHub UI directly | One character | return 0 → return 1 |

**Rule: never use a heavier intervention than needed. Saves tokens and time.**

---

## LIBRARY ONBOARDING — MANDATORY (R-121)

When any new library is added:
1. CC visits designer's GitHub — reads README + releases + examples
2. Creates `.claude/rules/LIBRARY_[name].md` before writing any code
3. Documents callback return values, memory requirements, known failure modes
4. Runs designer's example on hardware first (R-122)

See: `.claude/rules/SKILL_library_onboarding.md`

---

## PROBLEM SOLVING — KT FRAMEWORK (R-111 trigger)

When any fix fails twice on the same symptom → STOP → invoke KT IS/IS-NOT:
See: `.claude/rules/SKILL_problem_solving_kt.md`

Quick checklist:
- IS: where/when/what the problem EXISTS
- IS NOT: where/when/what it does NOT exist
- Hypothesis must explain BOTH IS and IS-NOT or it is eliminated
- **Step 0 before Phase 3a: read library designer's docs (R-121) — eliminates API misuse hypotheses**
- Send a spy (Serial.printf) before sending a fix
- Read library designer's docs before any hardware hypothesis

---

## SESSION MODES

### Build Mode (full CC prompt)
- CC reads: CLAUDE.md + RULES.md + PROJECT_STATE.md + all affected source files
- Use when: new feature, multi-file change, new screen
- Output: PR with CI green

### Fix Mode (single-file CC prompt)
- CC reads: CLAUDE.md + RULES.md + 1-2 source files
- Use when: single bug, single function, clear fix
- Output: push to branch, CI green, owner flashes

### Rapid Fire (phrase to CC)
- CC stays in current context
- Use when: one-line change confirmed by owner
- Output: immediate push, CI check

### Owner Direct Edit
- Owner edits file directly in GitHub UI
- Use when: one character, trivial, no risk of side effects
- Output: manual commit, owner flashes

---

## CC PROMPT FILING RULES (R-50, R-104)

- CC_PROMPT files live at REPO ROOT while active
- Owner pushes to root → tells CC to execute by filename → CC reads from root
- After CC executes and merges: CC moves file to `docs/prompts/` stamped `✅ COMPLETE — [date]`
- `docs/prompts/` = archive only — CC never looks there for an active prompt
- Naming: `CC_PROMPT_[topic].md`

---

## SESSION CLOSING CHECKLIST (R-84)

Every CC session ends with:
1. Archive prompt → `docs/prompts/` stamped ✅ COMPLETE
2. Append RULES.md (newest rule at top)
3. Update PROJECT_STATE.md (newest session at top of SESSION LOG)
4. Overwrite CHAT_HANDOFF.md
5. Update KNOWN_GOOD.md (newest snapshot at top)
6. Commit all docs
7. Merge to main

No session closes without this sequence complete.
