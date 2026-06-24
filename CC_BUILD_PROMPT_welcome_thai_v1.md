# CC_BUILD_PROMPT_welcome_thai_v1.md
> Version 1.0 — 2026-06-24
> Task: D-11 — Welcome screen + Thai language (sale sequence only)
> Repo: Satu-Vending-Firmware
> Flash cycles: 1
> PAYMENT_MODE: fake — never change

---

## 1. CC INTRO

Read and execute: CC_BUILD_PROMPT_welcome_thai_v1.md

New session. Ignore all previous context from other projects.
Repo: https://github.com/Csmittee/Satu-Vending-Firmware

Read IN FULL before touching anything — state each file read before writing a single line:
1. CLAUDE.md
2. RULES.md
3. CC_SKILL.md
4. .claude/rules/RULES-firmware.md
5. UI_SPEC.md (full NVS table + language section)
6. firmware/state_machine.h
7. firmware/ui_strings.h
8. firmware/ui_screens.h
9. firmware/ui_service.h (read in full — risk assessment required before touching)
10. firmware/satu_vending.ino (startup flow + STATE_IDLE case)
11. firmware/network.h (loadConfigFromNVS — confirm lang key load location)
12. KNOWLEDGE_MAP.md
13. hardware/HARDWARE_SPEC.md (confirm SPEAKER_PIN status — stub or real)

**Do not write a single line of code until all 13 files are read and stated.**

---

## 2. CONTEXT

D-11 delivers three things in one PR:

1. **Welcome / splash screen** — first screen the customer sees after boot.
   Bilingual EN/TH. Shows temple name or "SATU", language selector [EN] [TH],
   and "Touch to Begin / แตะเพื่อเริ่ม". Touch anywhere (after language pick) transitions
   to the existing idle/product selection flow. This replaces the direct boot-to-idle path.

2. **Thai language in sale sequence** — once customer picks TH on welcome screen,
   all sale-facing text (status bar states, gift option, confirm screen, payment screen,
   completion screen, error screen) renders in Thai. Service mode stays English always.
   Product names stay English (name_en from backend) — Thai product names are a backend
   task deferred to the admin UI phase.

3. **Settings tab — language default toggle** — operator sets the default language
   (EN or TH) in the Settings tab of service mode. This is the power-on default.
   The customer can override on the welcome screen for their session only.
   The operator setting persists to NVS key `lang` (already approved in UI_SPEC.md).
   Toggle applies on reboot — no live hot-swap of language in service mode.

**Sequence position:** D-11 of firmware build series. Follows D-10 (ui.h split — PR #56 merged).

---

## 3. NEW FILES

**firmware/SarabanSubset.h** — Thai GFX bitmap font, subset only.

Before creating this file, CC must:
- Determine the minimum Thai glyph set needed for all sale-sequence strings
  (welcome screen + status bar + gift option + confirm + payment + completion + error)
- Estimate the resulting `.h` file size in bytes
- If the estimated size exceeds 80KB: stop, write the size estimate to CC_CHAT_LOG,
  flag to owner, and do NOT embed the font. Use EN fallback for TH renders instead,
  add a comment in ui_strings.h: `// THAI_FONT_DEFERRED — see CC_CHAT_LOG [date]`.
- If the estimated size is under 80KB: embed the font using GFXfont format
  (same format as FreeSansBold12pt7b.h / FreeSansBold18pt7b.h in the repo).
  Use Sarabun or Noto Sans Thai as source. Point size: 18pt equivalent for body text,
  24pt for hero text (welcome heading). CC decides exact sizes based on screen coordinates.
- All Thai strings in ui_strings.h must use only glyphs present in SarabanSubset.h.
  No Thai character may appear that is not in the embedded font.

KNOWLEDGE_MAP.md must be updated — add SarabanSubset.h to firmware file list.
CLAUDE.md key files list must be updated to include SarabanSubset.h.
compile-check.yml must be assessed — if the font file causes CI memory issues, flag it.

**Write NONE for any other new files.** All other changes go into existing files.

---

## 4. TASKS

### TASK 1 — state_machine.h: add STATE_WELCOME

Add `STATE_WELCOME` to the `MachineState` enum.
Position: immediately before `STATE_IDLE` (welcome is the gate to idle).
Bump version header on state_machine.h.

CC verifies from live repo — do not assume current enum order from this prompt.

### TASK 2 — ui_strings.h: add all bilingual strings

Add the following to ui_strings.h under `#ifdef LANG_TH` / `#else` / `#endif` blocks:

**Status bar state labels (TH)** — replace `_stateLabels[]` with a bilingual accessor.
Do not break the existing `static const char* _stateLabels[]` EN array.
Add a parallel `static const char* _stateLabels_TH[]` array.
Add a global: `bool g_lang_th = false;` — session language flag (not NVS — resets on reboot).
Add a global: `bool g_lang_th_default = false;` — NVS-persisted operator default.
Add an inline accessor: `inline const char* _sl(int i) { return g_lang_th ? _stateLabels_TH[i] : _stateLabels[i]; }`

**Strings required for sale sequence (TH):**
- Welcome heading: temple name or "ยินดีต้อนรับ" (Welcome)
- Welcome sub: "เครื่องบริจาควัด SATU" (Satu Merit Donation Machine)
- Touch prompt EN: "Touch to Begin" / TH: "แตะเพื่อเริ่ม"
- Status bar states: "เลือกสินค้า", "ยืนยัน", "ชำระเงิน", "กำลังจ่าย", "กำลังเชื่อมต่อ"
- Gift option heading: "ต้องการน้ำมนต์หรือไม่?"
- Gift option buttons: "ไม่ต้องการ" (Item Only), "ต้องการ" (Add Sacred Water), "ย้อนกลับ" (Back)
- Confirm screen heading: "ยืนยันการสั่งซื้อ" (Confirm Order)
- Confirm / Back buttons: "ยืนยัน" / "ย้อนกลับ"
- Payment screen: "กรุณาสแกน QR เพื่อชำระเงิน" (Please scan QR to pay)
- Completion screen: "ขอบคุณ / ทำบุญสำเร็จ" (Thank you / Merit made)
- Error screen prefix: "เกิดข้อผิดพลาด" (Error)

CC may adjust or extend this list based on what it finds in ui_screens.h draw functions.
The rule is: every EN string visible to a customer in the sale sequence must have a TH equivalent.
Product names (name_en from backend) are excluded — they stay EN always in this PR.

No Thai characters may appear in `gfx->print()` calls using FreeSans fonts.
All Thai output must use `gfx->setFont(&SarabanSubset_18pt)` or equivalent,
and `gfx->setFont(NULL)` reset immediately after.
If SarabanSubset.h was deferred (>80KB case), all TH strings fall back to EN silently.

Bump version: ui_strings.h R1 → R2.

### TASK 3 — ui_screens.h: welcome screen draw + touch functions

Add two functions:

**`void drawWelcomeScreen()`**
- Full screen fill, C_BG background
- SATU logo / heading in C_GOLD (FreeSansBold24pt7b)
- Subtitle (EN + TH if font available)
- Language selector: two touch targets [EN] [TH], side by side, centred
  - Currently selected language highlighted in C_GOLD, other in C_DARKGREY
  - On first draw: use g_lang_th_default to set initial highlight
- Touch prompt: "Touch to Begin / แตะเพื่อเริ่ม" below language selector
  - TH portion uses SarabanSubset font if available, else EN only
- Do not draw a status bar on this screen — it is pre-sale

**`int getTouchedWelcome()`**
Returns:
- `1` = EN button tapped → set g_lang_th = false, redraw highlight
- `2` = TH button tapped → set g_lang_th = true, redraw highlight
- `0` = anywhere else tapped (proceed to idle/sale)
- `-1` = no touch

CC determines coordinates from screen constants. No hardcoded pixel values in return logic.

Bump version: ui_screens.h R1 → R2.

### TASK 4 — ui_screens.h: wire g_lang_th into existing sale screens

In each customer-facing draw function that contains EN string literals for customer-visible text,
replace direct string literals with the `_sl(i)` accessor or equivalent inline conditional.

CC reads every draw function in ui_screens.h and decides which strings need bilingual treatment.
The rule: if a customer reads it on screen, it must switch with g_lang_th.
The rule: if only a technician or operator reads it (service mode, boot, WiFi setup, debug), it stays EN.

Do not touch: _drawSvcTabBar, drawBootScreen, drawWifiSetupScreen, drawDebugScreen,
drawSetupCodeScreen, drawPinOverlay, drawBootPinScreen — these stay EN always.

### TASK 5 — ui_service.h: Settings tab — default language toggle

**Risk assessment required before touching this file.**
CC reads ui_service.h in full, identifies the Settings tab body function (`_drawSvcBody_Settings`),
and assesses:
- Can a new row be added without shifting existing row Y coordinates?
- Does the tab body have spare vertical space?
- Is there a risk of row overflow below the visible area?

If the risk is LOW: add one new row to the Settings body — "Default Language" label +
[EN] [TH] toggle buttons. Touch action code: 901 (new code, must not conflict with existing).
On toggle: write NVS key `lang` ("EN" or "TH"), set g_lang_th_default accordingly.
No reboot triggered — toggle takes effect on next machine power cycle.
Add action code 901 to getTouchedServiceContent() (or equivalent touch handler in ui_service.h).

If the risk is MEDIUM or HIGH: do not add the row. Write assessment to CC_CHAT_LOG.
Flag: "D-11 Settings lang toggle deferred — reason: [risk detail]. Separate PR needed."
Proceed with Tasks 1–4 and 6 only.

Bump version on ui_service.h only if modified.

### TASK 6 — satu_vending.ino: STATE_WELCOME in state machine

In `setup()`, after `loadConfigFromNVS()` and `recalcGrid()` and boot PIN gate:
- Load `g_lang_th_default` from NVS key `lang` ("TH" → true, else false)
- Set `g_lang_th = g_lang_th_default`
- Replace the final `setState(STATE_IDLE)` + `drawIdleScreen()` call with:
  `setState(STATE_WELCOME); drawWelcomeScreen();`

In `runStateMachine()`, add `case STATE_WELCOME:` before `case STATE_IDLE:`:
```
case STATE_WELCOME: {
  int action = getTouchedWelcome();
  if (action == 1 || action == 2) {
    // language was selected and highlight updated — stay in STATE_WELCOME
    // g_lang_th already updated by getTouchedWelcome()
    break;
  }
  if (action == 0) {
    // anywhere else tapped — proceed to sale
    setState(STATE_IDLE);
    g_idleDrawn = false;
    drawIdleScreen();
  }
  // timeout: if no touch for cfg_idle seconds, reset language to default and redraw
  if (elapsed > (unsigned long)(g_cfg_idle * 1000)) {
    g_lang_th = g_lang_th_default;
    stateStartTime = millis();
    drawWelcomeScreen();
  }
  break;
}
```

CC verifies variable names from live repo — `g_idleDrawn`, `g_cfg_idle`, `g_lang_th` must
match actual declarations. Adjust if repo uses different names.

Do not change any other state transitions in the state machine.
Bump version: satu_vending.ino (version comment at top of file).

### TASK 7 — network.h: load lang default from NVS

In `loadConfigFromNVS()` (or wherever NVS keys are loaded at startup):
Add: read `lang` key → if "TH" set `g_lang_th_default = true` else `g_lang_th_default = false`.

CC confirms the correct NVS namespace (`satu`) and the exact load function from live repo.
Do not add a new NVS load function — add to the existing one.

### TASK 8 — UI_SPEC.md: update language section and NVS table

In the Language System section, update to reflect actual implementation:
- `g_lang_th` = session flag (set on welcome screen, resets to default on return to welcome)
- `g_lang_th_default` = NVS-persisted operator default
- Thai font: state whether SarabanSubset.h was embedded or deferred, and why

NVS table: `lang` key already present — update Description column:
"Operator default language EN/TH. Customer can override per session on welcome screen."

Bump version: UI_SPEC.md.

### TASK 9 — compile-check.yml: Thai font CI assessment

CC reads compile-check.yml and assesses whether adding SarabanSubset.h to the compile
will cause CI memory warnings or flash overflow.

If SarabanSubset.h is embedded: run CI and check binary size output.
If binary size exceeds 85% of partition (16M Flash, 3MB APP partition):
flag in CC_CHAT_LOG with exact sizes — do not block the PR, but warn owner.

If SarabanSubset.h was deferred: no CI change needed. Note in CC_CHAT_LOG.

---

## 5. DO NOT TOUCH

- `firmware/hardware.h` — R2 LOCKED. Do not open.
- `firmware/config.h` — no changes needed. Do not touch.
- `firmware/state_machine.h` — only add STATE_WELCOME to enum, nothing else.
- `firmware/ui_keyboard.h` — do not touch.
- `firmware/ui_service.h` — touch ONLY if risk assessment in Task 5 is LOW.
  If MEDIUM or HIGH: leave completely untouched.
- `firmware/network.h` — only Task 7 NVS load addition, nothing else.
- `satu-system-tester.html` — never modify.
- `src/` — backend source, do not touch.
- `public/` — do not touch.
- `schema.sql` — do not touch.
- `wrangler.toml` — do not touch.
- Product names — always render from `name_en` field. No `name_th` logic. Deferred.
- Service mode string literals — all stay EN. No g_lang_th checks in service code.
- drawBootScreen, drawWifiSetupScreen, drawDebugScreen, drawSetupCodeScreen,
  drawPinOverlay, drawBootPinScreen — all stay EN always.
- `idleAnimation()` — hardware.h only, do not touch.
- NVS keys — only `lang` (already in UI_SPEC.md). Do not invent new keys.
- PAYMENT_MODE — stays fake always.

---

## 6. VERIFICATION

Before closing PR, CC confirms:

**Compile:**
- [ ] GitHub Actions CI green — compile passes with no errors
- [ ] If SarabanSubset.h embedded: binary size logged in CC_CHAT_LOG (exact bytes)
- [ ] If SarabanSubset.h deferred: reason logged, EN fallback confirmed in code

**Architecture:**
- [ ] `g_lang_th` is declared in ui_strings.h — accessible to ui_screens.h via include chain
- [ ] `g_lang_th_default` is declared in ui_strings.h — accessible to network.h via include
- [ ] No Thai glyph appears in a `gfx->print()` call using FreeSans fonts
- [ ] STATE_WELCOME present in enum, case present in runStateMachine()
- [ ] Welcome screen timeout resets language to default and redraws (no drift)
- [ ] Service mode: zero Thai strings anywhere in ui_service.h or drawServiceScreen()

**NVS:**
- [ ] `lang` key loaded at startup in loadConfigFromNVS()
- [ ] `lang` key written on Settings toggle (if Task 5 not deferred)
- [ ] All NVS keys match UI_SPEC.md approved table — no new keys invented

**Rules:**
- [ ] R-137 font rule applied — setFont(NULL) after every FreeSans or Thai font block
- [ ] R-20 — braces on all switch/case variable declarations
- [ ] R-171 include chain unchanged: ui_strings.h → ui_keyboard.h → ui_screens.h → ui_service.h

---

## 7. MANDATORY CLOSING

Complete ALL steps — no partial closes:

1. **CC_CHAT_LOG.md** — append at TOP with:
   - SarabanSubset.h: embedded (size in bytes) OR deferred (reason + size estimate)
   - Task 5 Settings lang toggle: completed OR deferred (risk reason)
   - Binary size before and after (if font embedded)
   - Owner flash instruction: 1 flash cycle, confirm welcome screen appears on boot

2. **Archive this prompt** → `docs/prompts/CC_BUILD_PROMPT_welcome_thai_v1.md`
   Stamp: `✅ COMPLETE — 2026-06-24 — D-11 welcome screen + Thai language`

3. **RULES.md** — append at TOP (next R-number after R-171):
   ```
   R-172: WELCOME SCREEN — STATE_WELCOME is the customer entry gate (2026-06-24).
   Machine always boots to STATE_WELCOME after setup completes.
   STATE_IDLE is only entered from STATE_WELCOME (customer tap) or timeout reset.
   Welcome screen shows language selector — sets g_lang_th for session only.
   g_lang_th_default (NVS key: lang) is the operator-set power-on default.

   R-173: THAI LANGUAGE SCOPE — SALE SEQUENCE ONLY (2026-06-24).
   g_lang_th controls language in: welcome, status bar, gift option, confirm,
   payment, completion, error screens only.
   Service mode, boot, WiFi setup, debug, PIN screens stay EN always — no exceptions.
   Product names always render from name_en — Thai product names are a backend/admin task.
   Thai glyphs may only be rendered via SarabanSubset.h font — never via FreeSans fonts.

   R-174: SARABUN FONT RULE (2026-06-24).
   SarabanSubset.h is a GFX bitmap font containing only glyphs used in D-11 Thai strings.
   If embedded: maximum 80KB. If font exceeds 80KB, defer and use EN fallback.
   setFont(SarabanSubset) must always be followed by setFont(NULL) immediately after use.
   No Thai character may appear in gfx->print() with FreeSans or NULL font active.

   R-175: ui_service.h LANG TOGGLE EXCEPTION (2026-06-24).
   R-171 LOCKED status for ui_service.h is partially lifted for D-11 only.
   Permitted change: Settings tab — add Default Language toggle row + action code 901.
   Condition: CC must assess vertical space risk before touching. If risk is MEDIUM/HIGH,
   defer the Settings toggle and flag in CC_CHAT_LOG. All other ui_service.h content remains LOCKED.
   ```
   Version bump RULES.md.

4. **PROJECT_STATE.md** — add to SESSION LOG at TOP:
   ```
   ### 2026-06-24 — D-11: Welcome screen + Thai language (CC_BUILD_PROMPT_welcome_thai_v1.md)
   - STATE_WELCOME added — machine now boots to welcome screen with EN/TH selector
   - SarabanSubset.h: [embedded (Xkb) / deferred (reason)]
   - ui_strings.h R2: bilingual string arrays, g_lang_th + g_lang_th_default globals
   - ui_screens.h R2: drawWelcomeScreen() + getTouchedWelcome() + bilingual sale screens
   - ui_service.h: [lang toggle added / deferred — see CC_CHAT_LOG]
   - satu_vending.ino: STATE_WELCOME case + NVS lang load at startup
   - CI: [green / flag]
   - PENDING: owner flash + confirm welcome screen on boot + language switch on sale screens
   ```
   Version bump PROJECT_STATE.md.

5. **KNOWLEDGE_MAP.md** — add to firmware file list:
   ```
   SarabanSubset.h    — Thai GFX bitmap font, sale-sequence glyphs only (D-11)
                        [embedded: Xkb / deferred: see CC_CHAT_LOG]
   ```
   Version bump KNOWLEDGE_MAP.md.

6. **CLAUDE.md** — add SarabanSubset.h to key files list (firmware section).
   Version bump CLAUDE.md.

7. **UI_SPEC.md** — update language section per Task 8.
   Version bump UI_SPEC.md.

8. **SATU_ROADMAP.md** — update Open Strategic Questions table:
   Change row: "Thai language in UI | Deferred — bitmap font required, ASCII only now"
   → "Thai language in UI | ✅ D-11 delivered — sale sequence only. Product name TH = admin UI phase."
   Version bump SATU_ROADMAP.md.

9. **Bump version header** on every file changed — no exceptions.

10. **Commit all docs → merge to main.**

11. **Delete CC_BUILD_PROMPT_welcome_thai_v1.md from repo root** after archiving.

---

## 8. PAYMENT MODE REMINDER

PAYMENT_MODE = fake. This prompt touches zero payment code. Never change to live.
