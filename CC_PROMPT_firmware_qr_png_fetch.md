# CC_PROMPT_firmware_qr_png_fetch.md
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

Replace the existing `fetchImageBytes()` implementation with this version
that uses `WiFiClientSecure` with `setInsecure()` for external HTTPS.
`setInsecure()` is acceptable here — QR image is not sensitive auth data.

```cpp
#include <WiFiClientSecure.h>

size_t fetchImageBytes(const String& url, uint8_t* buf, size_t bufSize) {
  Serial.printf("[NET] fetchImageBytes: url=%s\n", url.c_str());

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NET] fetchImageBytes: WiFi not connected");
    return 0;
  }
  if (!buf || bufSize == 0) {
    Serial.println("[NET] fetchImageBytes: invalid buffer");
    return 0;
  }

  WiFiClientSecure client;
  client.setInsecure(); // QR image only — not sensitive data — R-95

  HTTPClient http;
  http.begin(client, url);
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int code = http.GET();
  Serial.printf("[NET] fetchImageBytes: HTTP %d\n", code);

  if (code != 200) {
    http.end();
    return 0;
  }

  int contentLen = http.getSize();
  Serial.printf("[NET] fetchImageBytes: Content-Length=%d\n", contentLen);

  if (contentLen > 0 && (size_t)contentLen > bufSize) {
    Serial.printf("[NET] fetchImageBytes: too large (%d > %u)\n",
                  contentLen, bufSize);
    http.end();
    return 0;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t bytesRead   = 0;
  unsigned long t0   = millis();

  while (http.connected() && bytesRead < bufSize) {
    size_t avail = stream->available();
    if (avail) {
      size_t toRead = min(avail, bufSize - bytesRead);
      bytesRead    += stream->readBytes(buf + bytesRead, toRead);
    } else {
      if (millis() - t0 > 15000) {
        Serial.println("[NET] fetchImageBytes: timeout");
        break;
      }
      delay(1);
    }
    if (contentLen > 0 && bytesRead >= (size_t)contentLen) break;
  }

  http.end();
  Serial.printf("[NET] fetchImageBytes: %u bytes read\n", bytesRead);
  return bytesRead;
}
```

### Fix 2 — ui.h: drawQrScreen() — verify call chain and add serial logs

Read the current `drawQrScreen()` implementation carefully.

Confirm this exact sequence exists:
1. `g_pngBuf` is allocated (ps_malloc) — should already be in `initUI()`
2. `fetchImageBytes(g_currentQrUrl, g_pngBuf, 200*1024)` is called
3. If bytes > 0: call `drawQrFromBytes(g_pngBuf, bytes, x, y, w, h)`
4. If bytes == 0: draw fallback white box + text "Scan with banking app"

If step 2 is missing (fetchImageBytes never called) — add it.
If step 3/4 branching is missing — add it.

Add explicit serial logs to BOTH paths:
```cpp
// Success path:
Serial.printf("[UI] QR PNG loaded: %u bytes — rendering\n", bytes);

// Failure path:
Serial.println("[UI] QR PNG failed — showing fallback");
```

### Fix 3 — ui.h: verify PNGdec callback signature (R-89)

Confirm `_pngDrawRow` returns `int` not `void`:
```cpp
static int _pngDrawRow(PNGDRAW* pDraw) {
  // ... draw row ...
  return 0;
}
```

If it returns void — fix to return int per R-89.

---

## DO NOT TOUCH
- `hardware.h` — R2 LOCKED — never open or modify
- `config.h` — NUM_SLOTS definition, WiFi credentials (empty strings)
- `state_machine.h`
- Any payment or auth code
- PAYMENT_MODE (must stay fake)

---

## CI SELF-TEST — MANDATORY BEFORE OPENING PR (R-90)

After pushing changes to a branch:
1. Go to Actions tab in Csmittee/Satu-Vending-Firmware
2. Wait for compile-check workflow to complete (~3-5 min)
3. ✅ GREEN → open PR, write "GitHub Actions compile: ✅ GREEN" in PR body
4. ❌ RED → fix in same branch, push again, wait for green
5. NEVER open a PR showing red Actions

---

## OWNER FLASH INSTRUCTIONS

After CC opens PR and Actions is green:
1. Merge PR
2. Pull latest from repo to Arduino IDE
3. Flash SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
4. Open Serial Monitor (115200 baud)
5. Go through flow until QR screen appears
6. Watch serial for these lines:
   ```
   [NET] fetchImageBytes: url=https://api.qrserver.com/...
   [NET] fetchImageBytes: HTTP 200
   [NET] fetchImageBytes: Content-Length=XXXX
   [NET] fetchImageBytes: XXXX bytes read
   [UI] QR PNG loaded: XXXX bytes — rendering
   ```
7. Report exact serial output to Chat

If serial shows `HTTP 200` but rendering still fails → PNG decode issue (next fix).
If serial shows no fetchImageBytes lines at all → call chain issue (report to Chat).

---

## UPDATE KNOWN_GOOD.md
Append at TOP (never overwrite existing entries):
```
## 2026-06-13 — QR PNG fetch fix
- Files changed: network.h (fetchImageBytes WiFiClientSecure), ui.h (drawQrScreen call chain + logs)
- Compile status: ✅ clean (GitHub Actions)
- Flash status: ⬜ not yet — owner flashes and reports serial
- Owner observation: "pending flash"
- Known issues remaining: WiFi keyboard missing . @ - _ (batched fix/wifi-keyboard-special-chars)
```

---

## MANDATORY (end of session)

1. Wait for GitHub Actions GREEN before opening PR
2. State "GitHub Actions compile: ✅ GREEN" in PR body

3. Append to RULES.md (newest at TOP):
```
- **R-95 WIFI CLIENT SECURE FOR EXTERNAL HTTPS — PERMANENT (2026-06-13):**
  fetchImageBytes() MUST use WiFiClientSecure with setInsecure() for
  external HTTPS URLs (e.g. api.qrserver.com).
  Plain HTTPClient fails silently on ESP32 for external HTTPS.
  setInsecure() is acceptable for QR image fetch — not sensitive data.
  Always add http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS).
```

4. Update PROJECT_STATE.md:
   - QR PNG fix: network.h updated with WiFiClientSecure
   - Flash verification pending — owner to report serial output
   - Note: if HTTP 200 but no render, next step is PNG decode investigation

5. Archive this prompt to `docs/prompts/` in the FIRMWARE repo:
   `✅ COMPLETE — 2026-06-13 — QR PNG fetch fix: WiFiClientSecure for external HTTPS`

6. Merge to main after CI green

---

## PAYMENT MODE REMINDER
PAYMENT_MODE must remain = fake for this session.
Never suggest changing to live.

---
> Previous prompts:
>   Prompt 1: CC_PROMPT_fix_index_js_template_literal_build_error.md ✅ DONE
>   Prompt 2: CC_BUILD_PROMPT_tester_redesign.md ✅ DONE (confirm before sending this)
> This is Prompt 3 of 3 for this session sequence.
> After owner flashes and reports serial — Chat diagnoses next step.
