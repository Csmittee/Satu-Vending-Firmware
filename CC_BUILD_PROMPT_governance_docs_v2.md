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

```bash
git mv hardware/HARDWARE_TRUTH.md hardware/HARDWARE_SPEC.md
```

Add change log section immediately after the existing version header:

```markdown
## CHANGE LOG
| Date | Change | Who |
|---|---|---|
| 2026-06-20 | Renamed from HARDWARE_TRUTH.md. Added 21-lane MCP3 expansion note. | Owner+Chat |
| 2026-06-17 | Initial creation — R-128, R-129 applied | Owner |
```

Add this note to the MCP3 section (find the MCP3 / 0x22 entry and append):

```markdown
### MCP3 — Address 0x22 — RESERVED / NOT POPULATED
Used only in 21-lane build (Large model — 7×3 grid).
Do not populate for 10-lane Satu 1.0 prototype.
When populated: GPA0-GPA6 = IR Sensors 11-17, GPB0-GPB5 = Relays 13-18.
Lanes 19-21 require a 4th MCP or direct GPIO — TBD at hardware build phase.
```

Update any internal self-reference from HARDWARE_TRUTH.md → HARDWARE_SPEC.md within the file.
Bump version header → v1.1.

---

### TASK 2 — Update UI_SPEC.md (firmware repo only)

Add change log section immediately after the existing version header:

```markdown
## CHANGE LOG
| Date | Change | Who |
|---|---|---|
| 2026-06-20 | R12 type scale confirmed: MenuTitle=FreeSansBold18pt7b, SectionHeading=FreeSansBold12pt7b, Content=NULL size 2. Log panel moved to bottom (y=SCR_H-80, h=72, 4 lines). Serial mirror added to _svcLogPanel(). | Owner+Chat |
| 2026-05-31 | Initial R4 spec | Owner |
```

Add this section after the Screen Inventory table:

```markdown
## Type Scale — Service Mode (R12 confirmed, 2026-06-20)

| Element | Font | Notes |
|---|---|---|
| Menu Title | FreeSansBold18pt7b | One per tab, top of body area |
| Section Heading | FreeSansBold12pt7b | Smaller than title — not 18pt |
| Content / data rows | NULL size 2 | Up from size 1 — more readable |
| Button labels | NULL size 2 | Consistent with content rows |
| Tab sidebar labels | NULL size 2 | Up from size 1 |
| Log panel text | NULL size 1 | Keep small — 4 lines max |

Rules:
- setFont(NULL) MUST be called after every FreeSans block — no exception
- Min gap: Menu Title baseline → first Section Heading top = 28px
- Min gap: Section Heading baseline → first content row = 12px
- Min gap: last content row → next Section Heading top = 20px

## Log Panel — Bottom Position (R12 confirmed, 2026-06-20)

All service mode tabs share a bottom log panel:
- Position: x=SVC_BODY_X+8, y=SCR_H-80, w=SCR_W-SVC_BODY_X-16, h=72
- 4 lines maximum, NULL size 1, newest line at bottom
- _svcLogPanel(msg) also calls Serial.println("[SVC] " + msg)
- Content body must stop at y=SCR_H-88 to avoid overlap with log panel
```

Bump version header → v2.0 (new section = major bump).

---

### TASK 3 — Place SATU_ROADMAP.md (BOTH repos)

The file `SATU_ROADMAP.md` is attached to this prompt by the owner.
Do NOT regenerate, rewrite, or modify the content in any way.
Copy the attached file exactly as-is to both repos:

- `Satu-Vending-Firmware/SATU_ROADMAP.md`
- `Satu-vending-backend/SATU_ROADMAP.md`

Content is identical in both repos.
If the file already exists in either repo, overwrite it with the attached version.

---

### TASK 4 — Update CLAUDE.md (BOTH repos)

In the Key Files section, add these three entries:

```
- `hardware/HARDWARE_SPEC.md` — hardware single source of truth · read before touching pins, relays, sensors, or BOM · (firmware repo only)
- `UI_SPEC.md` — UI specification bible · read before touching any screen, font, layout, or service tab
- `SATU_ROADMAP.md` — product direction guide · read bullet summaries on every session open · read full section when architecture, commercial, or hardware model decision arises
```

Remove any reference to `HARDWARE_TRUTH.md` — it has been renamed.
Bump CLAUDE.md version header.

---

### TASK 5 — Update KNOWLEDGE_MAP.md (BOTH repos)

In the Document Map table, add rows:

```
| Hardware change (pins/relays/sensors/BOM) | hardware/HARDWARE_SPEC.md | hardware.h, config.h |
| UI change (screen/font/layout/service tab) | UI_SPEC.md | firmware/ui.h, ui_service.h |
| Architecture or product direction question | SATU_ROADMAP.md | PROJECT_STATE.md |
```

In the File Locations section, add:

```
hardware/HARDWARE_SPEC.md — hardware single source of truth (renamed from HARDWARE_TRUTH.md) · firmware repo only
UI_SPEC.md               — UI specification bible
SATU_ROADMAP.md          — product vision and commercial direction · both repos
```

Bump KNOWLEDGE_MAP.md version header.

---

### TASK 6 — Append rules to RULES.md (BOTH repos)

Append at TOP with next R-numbers after R-159 (firmware) / check backend current highest:

```
R-162: SATU_ROADMAP.md IS THE PRODUCT DIRECTION SOURCE OF TRUTH (2026-06-20).
  This file answers "where are we heading" — PROJECT_STATE.md answers "where are we now".
  They must mirror each other: roadmap sets direction, project state tracks position.
  Chat reads SATU_ROADMAP.md bullet summaries at every session open (mandatory).
  Full read required when: new firmware architecture, new screen design, commercial
  decision, SaaS direction, hardware model choice, or new repo created.
  CC updates SATU_ROADMAP.md when owner confirms a strategic decision.
  Never add status columns, progress tracking, or completion icons to SATU_ROADMAP.md.
  That belongs in PROJECT_STATE.md.

R-161: UI_SPEC.md IS THE UI SOURCE OF TRUTH (2026-06-20).
  All font decisions, layout rules, screen inventory, service tab specs, and NVS keys
  live here. Any UI decision made in a Chat session must trigger a UI_SPEC.md update
  in the same CC PR as the UI code change. CC reads UI_SPEC.md before any ui.h or
  ui_service.h change. Chat flags owner when a UI decision is made in conversation
  without updating UI_SPEC.md — never silently skip the update.

R-160: HARDWARE_SPEC.md IS THE HARDWARE SOURCE OF TRUTH (2026-06-20).
  Renamed from HARDWARE_TRUTH.md. Lives at hardware/HARDWARE_SPEC.md in firmware repo.
  All pin assignments, relay logic, sensor logic, BOM, and wiring decisions live here.
  Any hardware change must update this file in the same PR.
  CC reads hardware/HARDWARE_SPEC.md before any hardware.h or config.h read.
  Chat flags owner if a hardware decision is made in conversation without updating
  HARDWARE_SPEC.md.
```

Bump RULES.md version header.

---

### TASK 7 — Update WORKFLOW_SKILL reference copy (.claude/claude_project/WORKFLOW_SKILL.md — BOTH repos)

Note: The master WORKFLOW_SKILL.md lives in the Chat project folder (not repo).
The `.claude/claude_project/` copy is a reference — update it to match.
CC must not treat this copy as the master.

**Change 1 — Add to CHAT SESSION OPENING sequence (after step 2, before current step 3):**

```
3. Chat reads SATU_ROADMAP.md bullet summaries — section headers only, not full content
   → Confirms understanding of current generation, phase, and commercial context
   → Reads full section only if today's task touches that domain
```

Renumber existing step 3 → step 4, step 4 → step 5, step 5 → step 6.

**Change 2 — Add to TRIGGER → ACTION → VALIDATOR table:**

```
| UI decision made in Chat (font/layout/screen/tab) | Chat | Flag to owner → CC updates UI_SPEC.md in same PR as UI code | Chat reads CC_CHAT_LOG — confirms update present |
| Hardware decision made in Chat (pins/relays/BOM) | Chat | Flag to owner → CC updates hardware/HARDWARE_SPEC.md in same PR | Chat reads CC_CHAT_LOG — confirms update present |
| Architecture or product direction discussed in Chat | Chat | Update SATU_ROADMAP.md → owner confirms → CC commits | Next session Chat reads roadmap bullet summary |
| New Satu generation repo created | Owner + CC | Update SATU_ROADMAP.md repo table in both existing repos | Chat verifies CC_CHAT_LOG confirms both repos updated |
```

Bump version header → v2.2.

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

Before closing PR, CC confirms each item:

- [ ] `hardware/HARDWARE_SPEC.md` exists in firmware repo
- [ ] `hardware/HARDWARE_TRUTH.md` removed from filesystem (git mv, not delete)
- [ ] `hardware/HARDWARE_SPEC.md` has CHANGE LOG section after version header
- [ ] `hardware/HARDWARE_SPEC.md` has MCP3 expansion note
- [ ] `UI_SPEC.md` has CHANGE LOG section after version header
- [ ] `UI_SPEC.md` has Type Scale section + Log Panel section
- [ ] `UI_SPEC.md` version bumped to v2.0
- [ ] `SATU_ROADMAP.md` exists at root of BOTH repos — content identical
- [ ] `SATU_ROADMAP.md` has no status columns, progress tracking, or completion icons
- [ ] `CLAUDE.md` in both repos references all 3 docs with read triggers
- [ ] `CLAUDE.md` has no reference to `HARDWARE_TRUTH.md`
- [ ] `KNOWLEDGE_MAP.md` in both repos has all 3 in Document Map + File Locations
- [ ] `RULES.md` in both repos has R-160, R-161, R-162 at TOP
- [ ] `.claude/claude_project/WORKFLOW_SKILL.md` in both repos has new session opening step + 4 new trigger rows
- [ ] `.claude/claude_project/WORKFLOW_SKILL.md` version bumped to v2.2
- [ ] Zero source files touched
- [ ] CI not triggered by firmware/ source changes (docs only)

---

## 7. MANDATORY CLOSING

1. Append CC_CHAT_LOG.md at TOP — format per CC_SKILL.md
2. Archive this prompt → docs/prompts/ stamped:
   `✅ COMPLETE — 2026-06-20 — governance: HARDWARE_SPEC + UI_SPEC + SATU_ROADMAP wired into governance`
3. R-160, R-161, R-162 added in Task 6 above
4. Update PROJECT_STATE.md — add session log entry at top
5. Bump version header on every file changed this session
6. Commit all in order → merge to main in both repos

---

## 8. PAYMENT MODE REMINDER

PAYMENT_MODE stays fake for this entire session.
Zero backend logic changes. Zero firmware source changes.
