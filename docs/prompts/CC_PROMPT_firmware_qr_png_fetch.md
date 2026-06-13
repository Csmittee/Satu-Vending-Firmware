# CC_PROMPT_firmware_qr_png_fetch.md
> Created by: Chat (Claude)
> Date: 2026-06-13
> Session goal: Fix QR PNG rendering on physical ESP32 — HTTPS fetch silent fail
> Sequence: Prompt 3 of 3 (backend fix ✅ → tester redesign ✅ → QR firmware fix)
> PR target: main (Satu-Vending-Firmware repo)
> Mode: Firmware Mode
> Prerequisite: Prompts 1 + 2 GREEN ✅
> Flash cycles expected: 1

## ✅ COMPLETE — 2026-06-13 — QR PNG fetch fix: WiFiClientSecure for external HTTPS

### What was done
- `network.h fetchImageBytes()`: Replaced plain HTTPClient with WiFiClientSecure + setInsecure()
  Root cause: plain HTTPClient silently fails on external HTTPS — no cert chain on ESP32
- `ui.h drawQrScreen()`: Added serial logs on success path ("[UI] QR PNG loaded: N bytes") 
  and failure path ("[UI] QR PNG failed — showing fallback")
- `RULES.md`: R-97 appended — WiFiClientSecure for external HTTPS permanent rule
- `KNOWN_GOOD.md`: Snapshot 2026-06-13 appended at top
- `PROJECT_STATE.md`: Session log entry added
- Branch: claude/vibrant-cray-cqp2em
- CI: pending GitHub Actions on push
- Flash: PENDING — owner to flash SATU-4R473R and report serial

### Owner flash instructions
After PR merges:
1. Pull latest → flash SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
2. Serial Monitor 115200 baud → trigger QR screen
3. Expected serial:
   ```
   [NET] fetchImageBytes: url=https://api.qrserver.com/...
   [NET] fetchImageBytes: HTTP 200
   [NET] fetchImageBytes: Content-Length=XXXX
   [NET] fetchImageBytes: XXXX bytes read
   [UI] QR PNG loaded: XXXX bytes - rendering
   ```
4. Report exact output to Chat

If HTTP 200 but rendering still fails → PNG decode issue (next fix session).

---

## ORIGINAL PROMPT

> Created by: Chat (Claude)
> Date: 2026-06-13
> Session goal: Fix QR PNG rendering on physical ESP32 — HTTPS fetch silent fail
> Sequence: Prompt 3 of 3 (backend fix ✅ → tester redesign ✅ → QR firmware fix)
> PR target: main (Satu-Vending-Firmware repo)
> Mode: Firmware Mode
> Prerequisite: Prompts 1 + 2 GREEN ✅
> Flash cycles expected: 1

---

## CC INTRO
New session. Ignore all previous context from other projects.

You are working on SATU 1.0 FIRMWARE at:
https://github.com/Csmittee/Satu-Vending-Firmware

Before doing anything else, read IN FULL:
1. CLAUDE.md
2. RULES.md
3. PROJECT_STATE.md
4. firmware/network.h  — focus on fetchImageBytes()
5. firmware/ui.h       — focus on drawQrScreen() and drawQrFromBytes()
6. firmware/satu_vending.ino — focus on STATE_QR handling

State the name of every file you read before writing a single line.
Then execute this prompt.

---

## CONTEXT — WHAT IS BROKEN

Physical ESP32 (SATU-4R473R, MAC: 3C:DC:75:5D:DD:2C) shows a green box
where the QR PNG should appear. Serial log shows:

```
[UI] QR screen drawn     ← screen renders OK
                          ← NO fetchImageBytes log ever appears
```

The simulator (browser) loads the same QR URL fine.
The ESP32 does not.

---

## ROOT CAUSE

The backend returns `qr_code_url` pointing to an external HTTPS URL:
```
https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=FAKE%7C...
```

Current `fetchImageBytes()` in `network.h` uses plain `HTTPClient`.
Plain `HTTPClient` on ESP32 silently fails on external HTTPS URLs because
it has no SSL/TLS certificate chain for external domains.

The function either:
A) Is not being called at all from `drawQrScreen()` in ui.h, OR
B) Is called but fails silently (returns 0) with no serial log

Either way: the fallback green/white box is shown with no error logged.

---

## FIX — Two files: network.h and ui.h

### Fix 1 — network.h: fetchImageBytes() — use WiFiClientSecure

Replace the existing `fetchImageBytes()` implementation with WiFiClientSecure + setInsecure().

### Fix 2 — ui.h: drawQrScreen() — verify call chain and add serial logs

Add explicit serial logs to BOTH paths:
- Success: `Serial.printf("[UI] QR PNG loaded: %u bytes — rendering\n", bytes);`
- Failure: `Serial.println("[UI] QR PNG failed — showing fallback");`

### Fix 3 — ui.h: verify PNGdec callback signature (R-89)

Confirm `_pngDrawRow` returns `int` not `void` — was already correct, no change needed.

---

## DO NOT TOUCH
- `hardware.h` — R2 LOCKED — never open or modify
- `config.h` — NUM_SLOTS definition, WiFi credentials (empty strings)
- `state_machine.h`
- Any payment or auth code
- PAYMENT_MODE (must stay fake)

---

## PAYMENT MODE REMINDER
PAYMENT_MODE must remain = fake for this session.
Never suggest changing to live.

---
> Previous prompts:
>   Prompt 1: CC_PROMPT_fix_index_js_template_literal_build_error.md ✅ DONE
>   Prompt 2: CC_BUILD_PROMPT_tester_redesign.md ✅ DONE
> This is Prompt 3 of 3 for this session sequence.
> After owner flashes and reports serial — Chat diagnoses next step.
