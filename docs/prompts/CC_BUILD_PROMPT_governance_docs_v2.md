✅ COMPLETE — 2026-06-20 — governance: HARDWARE_SPEC + UI_SPEC + SATU_ROADMAP wired into governance

# CC_BUILD_PROMPT_governance_docs_v2.md
> Created by: Chat — 2026-06-20
> Replaces: CC_BUILD_PROMPT_governance_docs_v1.md (never sent — gaps found)
> Session goal: Wire SATU_ROADMAP.md + HARDWARE_SPEC.md + UI_SPEC.md into governance
> Repos: Satu-Vending-Firmware (primary) + Satu-vending-backend (mirror tasks)
> Mode: Docs only — zero source code changes
> Flash cycles: 0
> PR target: main (both repos)

---

## 1. CC INTRO

```
Read and execute: CC_BUILD_PROMPT_governance_docs_v2.md
New session. Ignore all previous context from other projects.

You are working on SATU 1.0 — TWO repos:
  Firmware: https://github.com/Csmittee/Satu-Vending-Firmware
  Backend:  https://github.com/Csmittee/Satu-vending-backend

Read IN FULL from firmware repo before touching anything:
  1. CLAUDE.md
  2. RULES.md
  3. CC_SKILL.md
  4. KNOWLEDGE_MAP.md
  5. hardware/HARDWARE_TRUTH.md   ← rename target in Task 1
  6. UI_SPEC.md                   ← update target in Task 2

State every file read before writing a single line.
```

---

## 2. CONTEXT

Three documents exist in the firmware repo but are NOT wired into governance:
- `hardware/HARDWARE_TRUTH.md` — hardware single source of truth
- `UI_SPEC.md` — UI specification bible for all screen, font, layout decisions
- Neither appears in CLAUDE.md key files with read triggers
- Neither appears in the TRIGGER → ACTION → VALIDATOR table
- No roadmap document exists in either repo

Owner decisions confirmed 2026-06-20:
1. Rename `hardware/HARDWARE_TRUTH.md` → `hardware/HARDWARE_SPEC.md`
2. Keep `UI_SPEC.md` name — add change log section
3. Place `SATU_ROADMAP.md` at root of BOTH repos — file is attached to this prompt, copy as-is, do not regenerate content
4. Wire all three into CLAUDE.md, KNOWLEDGE_MAP.md, RULES.md, WORKFLOW_SKILL reference copy
5. Add SATU_ROADMAP.md to Chat session opening sequence in WORKFLOW_SKILL

This is a docs-only session. Zero source files are touched.

---

## 3. NEW FILES

**SATU_ROADMAP.md** — place at root of BOTH repos.
File is owner-attached — copy as-is. Never regenerate.
This is a direction guide — never a progress tracker. No status columns ever.

---

## 4. TASKS

### TASK 1 — Rename hardware/HARDWARE_TRUTH.md → hardware/HARDWARE_SPEC.md (firmware repo only)

Add change log section, expand MCP3 note, bump version to v1.1.

### TASK 2 — Update UI_SPEC.md (firmware repo only)

Add CHANGE LOG section, Type Scale section, Log Panel section. Bump to v2.0.

### TASK 3 — Place SATU_ROADMAP.md (BOTH repos)

Copy owner-attached file exactly as-is.

### TASK 4 — Update CLAUDE.md (BOTH repos)

Add 3 Key Files entries. Remove HARDWARE_TRUTH.md references. Bump version.

### TASK 5 — Update KNOWLEDGE_MAP.md (BOTH repos)

Add 3 Document Map rows + 3 File Locations entries. Bump version.

### TASK 6 — Append rules to RULES.md (BOTH repos)

R-160, R-161, R-162 at TOP. Bump version.

### TASK 7 — Update WORKFLOW_SKILL reference copy (BOTH repos)

New step 3 in session opening + 4 trigger rows. Bump to v2.2.

---

## 5. DO NOT TOUCH

- Any .ino or .h firmware source files
- schema.sql
- wrangler.toml
- src/ backend source files
- public/ HTML files
- satu-system-tester.html — R-94
- hardware.h — R2 LOCKED
- PAYMENT_MODE — stays fake
- KNOWN_GOOD.md — no hardware QA this session

---

## 6. VERIFICATION

- [x] hardware/HARDWARE_SPEC.md exists in firmware repo
- [x] hardware/HARDWARE_TRUTH.md removed from filesystem
- [x] hardware/HARDWARE_SPEC.md has CHANGE LOG section after version header
- [x] hardware/HARDWARE_SPEC.md has MCP3 expansion note
- [x] UI_SPEC.md has CHANGE LOG section after version header
- [x] UI_SPEC.md has Type Scale section + Log Panel section
- [x] UI_SPEC.md version bumped to v2.0
- [x] SATU_ROADMAP.md exists at root of BOTH repos — content identical
- [x] SATU_ROADMAP.md has no status columns, progress tracking, or completion icons
- [x] CLAUDE.md in both repos references all 3 docs with read triggers
- [x] CLAUDE.md has no reference to HARDWARE_TRUTH.md
- [x] KNOWLEDGE_MAP.md in both repos has all 3 in Document Map + File Locations
- [x] RULES.md in both repos has R-160, R-161, R-162 at TOP
- [x] .claude/claude_project/WORKFLOW_SKILL.md in both repos has new session opening step + 4 new trigger rows
- [x] .claude/claude_project/WORKFLOW_SKILL.md version bumped to v2.2
- [x] Zero source files touched
- [x] CI not triggered by firmware/ source changes (docs only)

---

## 7. MANDATORY CLOSING

1. Append CC_CHAT_LOG.md at TOP — format per CC_SKILL.md
2. Archive this prompt → docs/prompts/ stamped ✅ COMPLETE — 2026-06-20 — governance: HARDWARE_SPEC + UI_SPEC + SATU_ROADMAP wired into governance
3. R-160, R-161, R-162 added in Task 6 above
4. Update PROJECT_STATE.md — add session log entry at top
5. Bump version header on every file changed this session
6. Commit all in order → merge to main in both repos

---

## 8. PAYMENT MODE REMINDER

PAYMENT_MODE stays fake for this entire session.
Zero backend logic changes. Zero firmware source changes.
