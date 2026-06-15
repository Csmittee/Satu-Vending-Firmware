# CC_PROMPT_diagnose_psram_pngdec.md
> Created: 2026-06-14
> Repo: Satu-Vending-Firmware
> Loop: B (firmware) — 1 flash cycle expected
> Goal: Add ONE diagnostic line. Measure. Do not fix anything yet.

---

## CC INTRO

New session. Ignore all previous context.

You are working on SATU 1.0 FIRMWARE at:
https://github.com/Csmittee/Satu-Vending-Firmware

Read IN FULL and state each filename before writing anything:
1. CLAUDE.md
2. RULES.md
3. firmware/ui.h
4. CC_PROMPT_diagnose_psram_pngdec.md ← this file

State "All files read ✅" then execute this prompt.

---

## CONTEXT — WHY THIS DIAGNOSTIC

PNGdec 1.1.6 openRAM() has returned rc=8 rows=1 for every PNG variant
tested across PRs #16-#20 (grayscale, RGB, stored zlib, deflate zlib).
The PNG itself is valid — renders correctly in browser and simulator.
The library is not confirmed broken — it works on thousands of ESP32 projects.

Most likely root cause: g_pngBuf is not in PSRAM despite ps_malloc().
ps_malloc() silently falls back to internal RAM when PSRAM is unavailable
at allocation time. PNGdec's zlib inflate requires a 32KB sliding window.
In fragmented internal RAM this fails, producing rc=8 exactly as observed.

We have never verified whether g_pngBuf actually lands in PSRAM.
This diagnostic confirms or eliminates that hypothesis in one flash cycle.

R-115: After 2 fix loops with no result — STOP. Measure first. Fix second.
This prompt follows R-115. No fix code. Diagnostic only.

---

## TASK — Add 3 diagnostic lines to ui.h initUI()

Find initUI() in firmware/ui.h.
Find the ps_malloc line:
  g_pngBuf = (uint8_t*)ps_malloc(200 * 1024);

Immediately AFTER that line add:

```cpp
// R-116 PSRAM DIAGNOSTIC — remove after root cause confirmed
Serial.printf("[UI] g_pngBuf addr : %p\n", g_pngBuf);
Serial.printf("[UI] g_pngBuf PSRAM: %s\n", esp_ptr_in_psram(g_pngBuf) ? "YES" : "NO");
Serial.printf("[UI] Free PSRAM    : %u bytes\n", ESP.getFreePsram());
```

Also find drawQrFromBytes() in firmware/ui.h.
Inside the openRAM success block, immediately BEFORE png.decode():

```cpp
// R-116 PNGdec buffer diagnostic
Serial.printf("[UI] PNGdec buf addr : %p\n", buf);
Serial.printf("[UI] PNGdec buf PSRAM: %s\n", esp_ptr_in_psram(buf) ? "YES" : "NO");
Serial.printf("[UI] PNGdec buf len  : %u\n", (unsigned)len);
Serial.printf("[UI] lineBuf stack   : will use %u bytes on stack\n",
              (unsigned)(png.getWidth() * 2));
```

These 7 lines are the complete change. Nothing else.

---

## DO NOT TOUCH
- hardware.h — R2 LOCKED — never open
- config.h
- state_machine.h
- network.h
- satu_vending.ino
- Any PNG format, zlib, or decode logic
- PAYMENT_MODE stays fake

---

## WHAT OWNER DOES AFTER FLASH

Trigger one order on the physical machine.
Copy the serial output from boot through to QR screen.
Report the exact values of these lines to Chat:

```
[UI] g_pngBuf addr : 0x????????
[UI] g_pngBuf PSRAM: YES or NO   ← THIS IS THE KEY LINE
[UI] Free PSRAM    : ??????? bytes
[UI] PNGdec buf addr : 0x????????
[UI] PNGdec buf PSRAM: YES or NO
[UI] PNGdec buf len  : ?????
[UI] lineBuf stack   : will use ??? bytes on stack
[UI] PNG decode: rc=? rows=? w=? h=?
```

---

## WHAT THE RESULTS MEAN (for Chat to action tomorrow)

### If PSRAM: NO
Root cause confirmed. g_pngBuf fell back to internal RAM.
Fix: verify Arduino IDE partition = 16M Flash (3MB APP/9.9MB FATFS)
and PSRAM = OPI PSRAM in board settings. These are locked settings
but worth confirming. Then force PSRAM allocation check on boot.

### If PSRAM: YES and lineBuf > 1600 bytes
Root cause: lineBuf[800] on stack overflows for wide images.
Fix: move lineBuf to PSRAM static allocation.

### If PSRAM: YES and lineBuf normal
Root cause: elsewhere. Report full serial to Chat for next diagnosis.
Do NOT change any code until Chat reviews the full output.

---

## CI SELF-TEST — MANDATORY BEFORE OPENING PR

1. Push to new branch
2. Actions tab → wait for compile-check (~3 min)
3. ✅ GREEN → open PR titled "diag: PSRAM + PNGdec buffer diagnostic (R-116)"
4. ❌ RED → fix, push, wait for green
5. NEVER open PR with red Actions

---

## MANDATORY end of session

1. GitHub Actions ✅ GREEN before opening PR
2. Write in PR body: "GitHub Actions compile: ✅ GREEN — diagnostic only, no logic change"
3. Update KNOWN_GOOD.md at TOP:
```
## 2026-06-15 — PSRAM PNGdec diagnostic (pending owner flash)
- Files: ui.h (7 diagnostic Serial.printf lines in initUI + drawQrFromBytes)
- Compile: ✅ GitHub Actions green
- Flash: ⬜ pending owner flash + serial report
- Purpose: confirm whether g_pngBuf lands in PSRAM
```
4. Do NOT update RULES.md — R-116 already added by revert session
5. Archive this prompt → docs/prompts/ after owner reports serial:
   ✅ COMPLETE — 2026-06-15 — PSRAM diagnostic — result: [YES/NO]

## PAYMENT MODE REMINDER
PAYMENT_MODE stays fake. Never change.
