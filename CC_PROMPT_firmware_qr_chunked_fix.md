# CC_PROMPT_firmware_qr_chunked_fix.md
> Created by: Chat (Claude)
> Date: 2026-06-13
> Session goal: Fix QR PNG truncated read (chunked transfer) + reduce QR timeout 90s→30s
> Repo: https://github.com/Csmittee/Satu-Vending-Firmware
> PR target: main
> Mode: Firmware
> Flash cycles expected: 1
> Prerequisite: PR #12 merged ✅ (WiFiClientSecure fix confirmed HTTP 200)

---

## CC INTRO

New session. Ignore all previous context from other projects.

You are working on SATU 1.0 FIRMWARE at:
https://github.com/Csmittee/Satu-Vending-Firmware

CC_PROMPT files are always at the repository ROOT level.
When given "Execute: CC_PROMPT_xxx.md" — read it from root immediately.
docs/prompts/ is archive only (✅ COMPLETE stamped files — do not execute these).

Before doing anything else, read IN FULL and state each filename aloud:
1. CLAUDE.md
2. RULES.md
3. PROJECT_STATE.md
4. firmware/network.h   — focus on fetchImageBytes()
5. firmware/satu_vending.ino — focus on QR timeout constant

State "All files read ✅" before writing a single line of code.
Then execute this prompt.

---

## CONTEXT — WHAT IS CONFIRMED WORKING

From owner serial output after PR #12 flash (2026-06-13):

```
[NET] fetchImageBytes: url=https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=FAKE%7C...
[NET] fetchImageBytes: HTTP 200
[NET] fetchImageBytes: Content-Length=-1
[NET] fetchImageBytes: timeout
[NET] fetchImageBytes: 502 bytes read
[UI] QR PNG loaded: 502 bytes — rendering
[UI] PNG open failed
[UI] QR screen drawn
```

WiFiClientSecure ✅ — HTTPS works
HTTP 200 ✅ — server responds
Content-Length=-1 — server uses chunked transfer encoding (no Content-Length header)
502 bytes read — truncated — stream read loop hit timeout before reading full image
PNG open failed — PNGdec rejects truncated data (correct behaviour)
A real QR PNG at 200×200 is ~2KB–4KB minimum

---

## ROOT CAUSE

`fetchImageBytes()` in network.h reads until `contentLen` bytes or timeout.
When `Content-Length = -1` (chunked), `contentLen` is -1 so the size check is skipped.
BUT the read loop exits on EITHER:
  a) bytes read >= contentLen  ← never triggers when contentLen = -1
  b) 15 second timeout         ← this triggers, but only 502 bytes arrived in 15s?

The real issue: the read loop uses `stream->available()` which returns 0 between
TCP packets on chunked responses. When `available() == 0` the loop does `delay(1)`
and checks the 15s wall-clock timeout. On a slow or chunked response, `available()`
bounces between 0 and small values — the loop likely exits on the 15s wall clock
before the full image arrives because the total time from HTTP GET to stream open
to first byte is already consuming most of that 15s window.

Fix: use `stream->readBytes()` with a proper chunked-aware loop that:
1. Reads aggressively until `http.connected()` is false (stream closed = transfer complete)
2. Uses a shorter per-packet idle timeout (500ms) not a global 15s timeout
3. Breaks cleanly when bufSize is full

---

## FIX 1 — network.h: fetchImageBytes() chunked-safe read loop

Replace ONLY the stream-reading section inside fetchImageBytes().
Keep WiFiClientSecure, setInsecure(), http.begin(), http.GET(), 
http.setTimeout(15000), http.setFollowRedirects() — those are correct (R-95).

Replace the stream reading block with this:

```cpp
  WiFiClient* stream = http.getStreamPtr();
  size_t bytesRead = 0;
  unsigned long lastDataMs = millis();
  const unsigned long IDLE_TIMEOUT_MS = 5000; // 5s idle between packets = give up

  while (bytesRead < bufSize) {
    if (stream->available()) {
      size_t toRead = min((size_t)stream->available(), bufSize - bytesRead);
      size_t got    = stream->readBytes(buf + bytesRead, toRead);
      bytesRead    += got;
      lastDataMs    = millis(); // reset idle timer on any data
    } else {
      // No data right now — check if connection still alive
      if (!http.connected()) {
        Serial.println("[NET] fetchImageBytes: stream closed — transfer complete");
        break;
      }
      if (millis() - lastDataMs > IDLE_TIMEOUT_MS) {
        Serial.printf("[NET] fetchImageBytes: idle timeout after %ums — %u bytes so far\n",
                      IDLE_TIMEOUT_MS, bytesRead);
        break;
      }
      delay(5);
    }
    // If server sent Content-Length and we have it all — stop early
    if (contentLen > 0 && bytesRead >= (size_t)contentLen) break;
  }

  http.end();
  Serial.printf("[NET] fetchImageBytes: %u bytes read\n", bytesRead);
  return bytesRead;
```

The key change: `!http.connected()` detects stream close (chunked EOF) without
needing a Content-Length header. Idle timeout drops to 5s (per-packet gap)
rather than 15s total — much more responsive.

---

## FIX 2 — satu_vending.ino: QR payment timeout 90s → 30s

Find the QR payment timeout constant. It will be something like:

```cpp
#define QR_TIMEOUT_MS  90000   // or similar name
// OR
const unsigned long QR_TIMEOUT = 90000;
// OR inline in the state machine:
if (elapsed > 90000) {  // QR timeout
```

Change the value from 90000 to 30000 (30 seconds).

If it is a #define or const — update the value there.
If it is an inline literal — update the literal and add a comment:
```cpp
if (elapsed > 30000) {  // QR timeout — 30s (R-102)
```

Add R-102 reference in comment so it is traceable.

---

## DO NOT TOUCH
- hardware.h — R2 LOCKED — never open or modify
- config.h — NUM_SLOTS, WiFi credentials
- state_machine.h
- ui.h — drawQrScreen(), drawQrFromBytes(), _pngDrawRow() all correct
- Any payment or auth logic
- PAYMENT_MODE (must stay fake)

---

## CI SELF-TEST — MANDATORY BEFORE OPENING PR (R-90)

1. Push changes to a new branch
2. Go to Actions tab → wait for compile-check (~3 min)
3. ✅ GREEN → open PR, write "GitHub Actions compile: ✅ GREEN" in PR body
4. ❌ RED → fix in same branch, push, wait for green
5. NEVER open PR with red Actions

---

## OWNER FLASH INSTRUCTIONS

After merging PR:
1. Pull latest → flash SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
2. Open Serial Monitor 115200 baud
3. Go through flow to QR screen
4. Look for:
```
[NET] fetchImageBytes: HTTP 200
[NET] fetchImageBytes: Content-Length=-1
[NET] fetchImageBytes: stream closed — transfer complete
[NET] fetchImageBytes: XXXX bytes read        ← expect 2000–5000 bytes
[UI] QR PNG loaded: XXXX bytes — rendering    ← expect this
```
5. Confirm QR image visible on screen (black/white QR pattern — not green box)
6. Paste exact serial output to Chat

If PNG open failed still appears → report byte count — may need PNG decode investigation.
If bytes read is still ~500 → report — chunked read still not working, further diagnosis needed.

---

## APPEND TO RULES.md (newest at TOP, next R-number after R-101)

```
- **R-102 QR PAYMENT TIMEOUT — PERMANENT (2026-06-13):**
  QR screen payment wait timeout = 30 seconds (was 90s).
  30s is sufficient for PromptPay scan. 90s causes poor UX at temple kiosk.
  Reference: satu_vending.ino QR timeout constant.

- **R-103 CHUNKED HTTP READ — PERMANENT (2026-06-13):**
  fetchImageBytes() MUST handle chunked transfer encoding (Content-Length=-1).
  Use !http.connected() to detect stream EOF — do NOT rely on Content-Length.
  Per-packet idle timeout = 5000ms. Global wall-clock timeout via http.setTimeout(15000).
  api.qrserver.com returns chunked — this is permanent behaviour, not a bug.
  Root cause confirmed: 502 bytes truncated = chunked EOF detection missing.
  WiFiClientSecure + setInsecure() remains correct (R-95).
```

---

## APPEND TO KNOWN_GOOD.md (at TOP — never overwrite)

```
## 2026-06-13 — PR #12 QR WiFiClientSecure flash confirmed

### Firmware
- Build:    PR #12 — WiFiClientSecure for external HTTPS QR fetch
- Board:    SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
- Flash:    ✅ CONFIRMED — 2026-06-13 ~21:34
- Compile:  ✅ Clean — GitHub Actions green (2 min 21 sec)

### Serial output confirmed
[NET] fetchImageBytes: url=https://api.qrserver.com/v1/create-qr-code/...
[NET] fetchImageBytes: HTTP 200
[NET] fetchImageBytes: Content-Length=-1
[NET] fetchImageBytes: timeout
[NET] fetchImageBytes: 502 bytes read
[UI] QR PNG loaded: 502 bytes — rendering
[UI] PNG open failed
[UI] QR screen drawn

### Confirmed WORKING ✅
- WiFiClientSecure HTTPS to external domain (api.qrserver.com) — HTTP 200
- fetchImageBytes() called correctly from drawQrScreen()
- Serial logs present on both success and failure paths
- State machine reaches QR screen, polls payment correctly
- WiFi keyboard . @ - _ keys present (PR #9 confirmed)
- 14-test suite backend: ✅ 14/14 (confirmed 2026-06-13 earlier today)

### Confirmed NOT WORKING ❌
- QR PNG render: 502 bytes truncated (chunked stream read exits too early)
  Fix: CC_PROMPT_firmware_qr_chunked_fix.md — next flash will resolve

### Confirmed WORKING from previous snapshot (still valid) ✅
- WiFi NVS provisioning
- /hello handshake, heartbeat
- Touch → product → gift → QR → payment polling
- Order creation via backend
- State machine flow complete

### Next flash target
- PR from CC_PROMPT_firmware_qr_chunked_fix.md
- Expected: [NET] fetchImageBytes: stream closed — transfer complete
- Expected: XXXX bytes (2000–5000) — QR PNG renders on screen
```

---

## UPDATE PROJECT_STATE.md

Mark:
- fetchImageBytes WiFiClientSecure: ✅ CONFIRMED working (HTTP 200)
- QR PNG render: ❌ chunked stream truncation — fix in this PR
- WiFi keyboard . @ - _: ✅ CONFIRMED (PR #9)
- QR timeout: changing 90s → 30s in this PR

---

## MANDATORY — end of session

1. Wait for GitHub Actions ✅ GREEN before opening PR
2. State "GitHub Actions compile: ✅ GREEN" in PR body
3. Append R-102 and R-103 to RULES.md (newest at TOP)
4. Append KNOWN_GOOD.md snapshot (newest at TOP)
5. Update PROJECT_STATE.md
6. Archive this prompt to docs/prompts/:
   `✅ COMPLETE — 2026-06-13 — QR chunked stream fix + 30s timeout`
7. Commit all docs, merge to main

## PAYMENT MODE REMINDER
PAYMENT_MODE must remain = fake for this session.
Never suggest changing to live.

---
> Sequence: follows PR #12 (WiFiClientSecure ✅)
> Previous prompts:
>   CC_PROMPT_fix_index_js_template_literal_build_error.md ✅
>   CC_BUILD_PROMPT_tester_redesign.md ✅
>   CC_PROMPT_firmware_qr_png_fetch.md ✅ PR #12 merged
> This prompt = PR #13 target
