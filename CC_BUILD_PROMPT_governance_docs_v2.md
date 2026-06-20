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
3. Create `SATU_ROADMAP.md` at root of BOTH repos — content provided in Section 5
4. Wire all three into CLAUDE.md, KNOWLEDGE_MAP.md, RULES.md, WORKFLOW_SKILL reference copy
5. Add SATU_ROADMAP.md to Chat session opening sequence in WORKFLOW_SKILL

This is a docs-only session. Zero source files are touched.

---

## 3. NEW FILES

**SATU_ROADMAP.md** — create at root of BOTH repos.
Full content provided in Section 5 below.
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

### TASK 3 — Create SATU_ROADMAP.md (BOTH repos — identical content)

Create file at repo root in both repos.
This is the complete content — do not summarise, abbreviate, or reformat:

```markdown
# SATU_ROADMAP.md — Product Vision & Direction
> Version 2.0 — 2026-06-20
> Changes: Full rewrite from satu-roadmap.html. Removed all status tracking (tracker = PROJECT_STATE.md).
>          Added repo architecture, Foundation Warning, Phase 2/3/4, business context.
>          Roadmap is a direction guide — never a progress tracker.
> Previous: v1.0 — 2026-06-20 (initial extract — incomplete)

---

## HOW TO READ THIS FILE

**Chat:** Read section headers + bullet points at every session open.
Full section read only when the task touches that domain.
This file answers "where are we heading" — for "where are we now" read PROJECT_STATE.md.

**CC:** Read when owner or Chat flags an architecture decision, new screen design,
commercial direction, hardware model change, or SaaS question.

**Rule:** This file is a direction guide. Never add progress columns, status icons,
or completion tracking here. That belongs in PROJECT_STATE.md.

---

## WHAT SATU IS

**SATU — Sacred Automated Temple Unit**

A Thai temple donation vending machine built to modernise merit-making.
Donors select a sacred item (amulet, blessing card, sacred water), pay via
PromptPay QR, and the machine dispenses the item immediately.

Vision: From a single prototype machine → national digital donation infrastructure.

---

## REPO ARCHITECTURE

Satu is built across repos. Every Chat and CC session must know which repo
they are working in and what that repo owns.

| Repo | Owns | Notes |
|---|---|---|
| `Satu-vending-backend` | Cloudflare Workers API + D1 database | Serves ALL Satu generations |
| `Satu-Vending-Firmware` | ESP32 firmware for Satu 1.0 machine | Hardware + display + network |
| `Satu-2-Firmware` *(future)* | ESP32/device firmware for Satu 2.0 | Has its own sub-hardware |
| `Satu-3-Firmware` *(future)* | AI interface device firmware for Satu 3.0 | Has its own sub-hardware |

**Critical backend rule:** The backend repo is not coupled to any one generation.
It is the shared infrastructure layer — Satu 1, 2, and 3 all call the same backend,
extended over time. Never couple backend logic to physical machine assumptions.
Device type is an attribute, not a schema constraint.

---

## THREE GENERATIONS

### Satu 1.0 — Temple Donation Vending Machine
Physical machine placed inside a temple. Fully autonomous.

What it does:
- Donor selects product (amulet, blessing card, sacred water, etc.)
- Scans PromptPay QR code shown on touch display
- Machine dispenses item via spring coil motor
- Temple owner manages inventory and views revenue via web dashboard
- Remote machine management from backend (enable/disable, commands, OTA stub)

Hardware: ESP32-S3 (ESP32-8048S070C) · 7-inch touch display · 2× MCP23017
· Spring coil motors · IR sensors · WS2812B LEDs · PromptPay via Omise

Three physical models offered to temples:

| Model | Grid | Lanes | Approximate size |
|---|---|---|---|
| Small | 5×2 | 10 | Width <50cm · Height ~100cm |
| Medium | 5×3 | 15 | Width <50cm · Height ~130cm |
| Large | 7×3 | 21 | Width ~70cm · Height ~130cm |

Prototype is being built for the Small model (10 lanes, 2× MCP23017).
Firmware NUM_SLOTS in config.h controls lane count — one constant scales the whole system.
Architecture must preserve expansion path to 21 lanes without redesign.
MCP3 (address 0x22) defined in HARDWARE_SPEC.md but not populated in prototype.

### Satu 2.0 — National Digital Donation Platform
No physical machine required. QR-based giving at temple entrance, merit boards,
festival kiosks, and mobile. Every temple in Thailand accessible.

What it adds:
- 3D visual storytelling per temple ("This roof tile costs 200 THB")
- Tax-exempt donation certificates — blockchain-anchored, Revenue Department API
- Government MOU with Department of Religious Affairs, Fine Arts Dept, BOI approval
- Multi-temple reporting dashboard for temple chains
- Anti-fraud infrastructure — immutable audit trail, multi-sig approvals, public transparency
- National rollout infrastructure — same backend, extended not replaced
- BOI application (Category 4.1) — 6-12 month lead time, start before Satu 1.0 launch

Satu 2.0 has its own firmware repo when device hardware is defined.
Backend repo extends naturally — multi-tenancy already designed in.

### Satu 3.0 — AI Dharma
AI Buddhist guidance system rooted in 2,000 years of Dhamma wisdom.
Not a gimmick — deeply respectful, academically grounded.

What it adds:
- Natural conversation with Dhamma guidance (Pali Canon + Thai Buddhist commentaries)
- Merit tracking and spiritual journey record across temples
- Pilgrimage digital record and community of practice
- Multi-language: Thai · English · Chinese · Japanese · Korean
- Buddhist tourism integration — every temple becomes accessible to international visitors
- AI monk consultation interface integrated with Satu 2.0 network data

Satu 3.0 has its own firmware repo when device form factor is defined.

---

## FOUR PHASES — COMMERCIAL MAP

This is the direction map. Where we are in each phase lives in PROJECT_STATE.md.

### Phase 1 — Prototyping: Product & Software
Build the first working machine. Prove the concept. Get the first temple.

Building right now (firmware + backend + hardware):
- ESP32 firmware: state machine, touch display, WiFi, API integration, service mode
- Backend: Cloudflare Workers + D1, Omise PromptPay, temple owner dashboard
- Hardware: ESP32-S3, MCP23017, spring motors, IR sensors, wiring and enclosure
- Payment: Omise KYC completion needed before live mode

Still to build in Phase 1:
- Physical prototype machine (components arriving)
- Full end-to-end test: real scan → webhook → relay → item dispensed
- Temple owner claim flow (setup code → dashboard)
- Rate limiting fix (in-memory Map broken across Workers instances)
- Order expiry / QR timeout (Cron Trigger)
- PDPA consent flow (legal review required before launch)
- Auth login endpoint wired to JWT system

### Phase 2 — Operations, Legal & Business
Build the operational backbone before scale becomes impossible.

Production & Operations:
- Production process flow: cycle time, assembly steps, quality checkpoints
- Firmware mass production flashing: batch script, eFuse burn, unit test per device
- Supply chain: component suppliers, lead times, safety stock, MOQ negotiation

Legal & Government:
- Company registration: Thai Ltd. (not sole proprietor) before government contracts
- BOI application Category 4.1 — start immediately, 6-12 month lead time
- PDPA full compliance audit: consent management, right to erasure, DPO appointment
- Religious institution agreements: MOU template, revenue sharing model
- IP protection: utility model filing before any public demo with partners

Financial & Sales:
- P&L model per machine: hardware cost (฿10,470 COGS), Cloudflare (~$5/mo),
  Omise (1.65% PromptPay), installation, maintenance. Break-even analysis.
- Pricing model: machine sale vs revenue share vs SaaS subscription, temple size tiers
- Order and invoice system: quote → PO → invoice → delivery tracking
- Marketing and sales: temple association outreach, Buddhist holiday timing,
  influencer monks, press, forecast model

### Phase 3 — Launch & Production: First Temples, Real Revenue
Controlled launch. Collect real data. Build support infrastructure.

Launch milestones:
- Pilot: 3 temples, 3 machines — collect real data on donation amounts,
  popular items, failure modes, support load
- Backend monitoring and alerting: device heartbeat monitor, payment failure alerts,
  on-call procedure
- Field support process: remote disable/enable in backend, physical support SLA,
  spare parts inventory
- Scale to 50 machines: multi-temple reporting, batch firmware OTA,
  revenue dashboard per temple, central accounting

### Phase 4 — Growth: Satu 2.0 National Platform + Satu 3.0 AI Dharma
National infrastructure. Government partnership. AI wisdom layer.
See Satu 2.0 and Satu 3.0 sections above for full detail.

---

## FOUNDATION — BUILD BEFORE SCALE

These items must be in place before Phase 3. Missing any one blocks national scale.

**Company structure**
Register proper Thai Ltd. (not sole proprietor) before government contracts.
BOI application takes 6-12 months — start during Phase 1, not Phase 2.

**PDPA compliance**
Donor consent flow is partially coded but incomplete.
At national scale, PDPA violations carry criminal liability.
Legal review required before any live installation.

**Security hardening**
rateLimit.js is broken in production (in-memory Map, multi-instance).
At national scale Satu becomes a target.
Needs: Cloudflare WAF rules, D1 backup strategy, incident response plan.

**IP protection**
File utility model for the machine concept before any public demo with partners.
Trade secret for firmware: flash encryption designed in Security_Protocol.txt.
Once demoed publicly without filing, protection window closes.

**Financial audit trail**
Temple donations require transparent accounting for trust.
Admin dashboard needs: export, reconciliation reports, Revenue Department integration.

**Team and resource plan**
Currently solo. Before Satu 2.0 needs:
1 backend developer · 1 hardware technician · 1 business developer · legal counsel · accounting.
AI agents can extend the solo runway significantly in early stage.

---

## REVENUE MODEL

- Temple pays for the machine (one-time hardware cost)
- Satu takes a percentage of each transaction (revenue share — % TBD post-KYC)
- Alternative: SaaS subscription per machine per month
- Backend is designed for multi-tenancy — one backend serves all temples
- Long-term: backend can be licensed as SaaS to other religious or charity organisations

Omise fee: 1.65% per PromptPay transaction (deducted before Satu's share).
Cloudflare cost: approximately $5/month regardless of machine count (Workers + D1).

---

## DECISIONS THAT ARE LOCKED

These are architectural choices that must not be reopened without owner decision.
Challenging these requires bringing new information, not re-litigating old discussions.

| Decision | Rule | Detail |
|---|---|---|
| Motor stop = sensor-triggered | R-128 LOCKED | NOT timer-based — IR sensor stops motor |
| Relay 12 = spring flap | R-129 LOCKED | NOT door lock solenoid — spring closes automatically |
| 3-area layout | R-141 LOCKED | Product grid · Payment area · Status bar |
| hardware.h | R2 LOCKED | Never modify without explicit owner approval |
| PAYMENT_MODE | Always fake in dev | Live only with physical hardware + Omise KYC complete |

---

## OPEN STRATEGIC QUESTIONS

These are not locked. Owner decides when ready. Chat may challenge with new data.

| Question | Context |
|---|---|
| Transaction fee % | Post-KYC discussion with Omise sales |
| Machine pricing to temples | Pending first customer meeting |
| Machine sale vs revenue share vs SaaS | P&L model needed per Phase 2 |
| MCP3 / 21-lane expansion | Deferred — post-prototype hardware confirmed |
| Thai language in UI | Deferred — bitmap font required, ASCII only now |
| OTA firmware update | Stub exists — full implementation Phase 2+ |
| Single vs dual ESP32 | Does not block current work |
| Login screen for service mode | Deferred — post hardware QA |
```

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
