# CHAT HANDOFF — 2026-06-16
> Overwrite this file at end of every session — never append

## ⚠️ DO FIRST
1. Project → Files → GitHub sync checkbox → **CONFIRM CHECKED** (resets every new chat)

---

## WHAT WAS DONE THIS SESSION

### Backend fixes (previous session — already on main)
- CORS header added to 401 from `/v1/admin-data/`
- Machine Builder: `hw-device-select` values fixed MAC→device name, `hwDispense()` corrected fields
- Full end-to-end hardware test PASSED on SATU-4R473R 2026-06-16

### Firmware UX fixes (this session — CC_PROMPT_firmware_ux_fixes)
- **Fix 1 (R-126):** `idleAnimationUI()` in `ui.h` — `delay(120)` and `delay(80)` replaced
  with millis-based loops that poll touch every 16ms and return immediately on touch.
  Max touch latency: 400ms → 16ms. Double-tap requirement eliminated.
- **Fix 2 (R-126):** Adafruit FreeFonts replace scaled bitmap fonts on large text screens:
  - Boot "SATU": FreeSansBold24pt7b size 1
  - QR screen amount "B999": FreeSansBold24pt7b size 1
  - Vending "Dispensing...": FreeSansBold18pt7b size 1
  - Completion "Your Merit Lucky Number" title: FreeSansBold12pt7b size 1
  - Lucky number hero: FreeSansBold24pt7b size 2 (large + quality)
- R-126 added to RULES.md, PROJECT_STATE.md and KNOWN_GOOD.md updated, prompt archived

---

## WHAT OWNER MUST DO NEXT

1. Wait for CI green on PR `claude/determined-planck-4smsma`
2. Merge PR to main
3. Flash SATU-4R473R from Arduino IDE
4. Test: single-tap on idle screen → must respond immediately (no double-tap needed)
5. Test: lucky number screen, amount, "Dispensing..." — should look much cleaner
6. If font positions look slightly off → direct edit cursor y-values in ui.h (minor tuning ok)
7. Update KNOWN_GOOD.md after flash

---

## OPEN ITEMS

- [ ] `PAYMENT_TIMEOUT_MS` — change to 120000 before temple deployment (currently 60000)
      Owner direct edit in `config.h`
- [ ] Service mode 5 tabs — full build (stubs only currently) — next firmware CC session

---

## KEY FILE STATE

| File | Branch | State |
|---|---|---|
| firmware/ui.h | claude/determined-planck-4smsma | R-126 — FreeFonts + touch-aware anim |
| RULES.md | claude/determined-planck-4smsma | R-126 added |
| PROJECT_STATE.md | claude/determined-planck-4smsma | Session log + open items |
| KNOWN_GOOD.md | claude/determined-planck-4smsma | 2026-06-16 snapshot (flash pending) |
| CC_PROMPT_firmware_ux_fixes.md | Archived | docs/prompts/ ✅ COMPLETE |
