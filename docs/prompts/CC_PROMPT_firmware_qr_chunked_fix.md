# CC_PROMPT_firmware_qr_chunked_fix.md
> Created by: Chat (Claude)
> Date: 2026-06-13
> Session goal: Fix QR PNG truncated read (chunked transfer) + reduce QR timeout 90s→30s
> Repo: https://github.com/Csmittee/Satu-Vending-Firmware
> PR target: main
> Mode: Firmware
> Flash cycles expected: 1
> Prerequisite: PR #12 merged ✅ (WiFiClientSecure fix confirmed HTTP 200)

## ✅ COMPLETE — 2026-06-13 — QR chunked stream fix + 30s timeout

### What was done
- `network.h fetchImageBytes()`: chunked-safe read loop replaces broken 15s wall-clock loop
  Key fix: `!http.connected()` detects chunked EOF, per-packet idle timeout 5000ms
  Root cause confirmed: stream.available()=0 between TCP packets → 15s timer fires at ~502 bytes
- `config.h PAYMENT_TIMEOUT`: 120000 → 30000 (30s QR wait, was 2 min)
- `RULES.md`: R-103 (chunked read) + R-102 (QR timeout) appended
- `KNOWN_GOOD.md`: PR #12 confirmed flash snapshot + R5.2 pending snapshot appended at TOP
- `PROJECT_STATE.md`: session log entry added
- Branch: claude/vibrant-cray-cqp2em
- CI: pending GitHub Actions on push
- Flash: PENDING — owner to flash SATU-4R473R and report serial

### Owner flash instructions
After PR merges:
1. Pull latest → flash SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
2. Serial Monitor 115200 baud → trigger QR screen
3. Expected serial:
   ```
   [NET] fetchImageBytes: HTTP 200
   [NET] fetchImageBytes: Content-Length=-1
   [NET] fetchImageBytes: stream closed — transfer complete
   [NET] fetchImageBytes: XXXX bytes read        ← expect 2000-5000
   [UI] QR PNG loaded: XXXX bytes - rendering    ← expect this
   ```
4. Report exact output to Chat
   - If "stream closed" + 2000+ bytes → QR should render
   - If "stream closed" + still ~500 bytes → report byte count, further diagnosis
   - If "PNG open failed" still appears → report, PNG decode investigation next

---

## ORIGINAL PROMPT

> Created by: Chat (Claude)
> Date: 2026-06-13
> Session goal: Fix QR PNG truncated read (chunked transfer) + reduce QR timeout 90s→30s
> Repo: https://github.com/Csmittee/Satu-Vending-Firmware
> PR target: main
> Mode: Firmware
> Flash cycles expected: 1
> Prerequisite: PR #12 merged ✅ (WiFiClientSecure fix confirmed HTTP 200)

### Context — confirmed from owner serial after PR #12 flash
- HTTP 200 ✅ — WiFiClientSecure works
- Content-Length=-1 — chunked transfer encoding
- 502 bytes read — truncated, stream exit too early
- PNG open failed — PNGdec rejects truncated data (correct)

### Root cause
stream.available()=0 between TCP packets on chunked response → delay(1) loop → 15s
global wall-clock fires before full image arrives. Fix: detect !http.connected() as EOF.

### Fix 1 — network.h: chunked-safe read loop (replaces stream reading section)
Key: !http.connected() detects EOF for chunked. Per-packet idle 5000ms.

### Fix 2 — config.h: PAYMENT_TIMEOUT 120000 → 30000 (30s, R-102)

### PAYMENT MODE REMINDER
PAYMENT_MODE must remain = fake. Never suggest changing to live.

---
> Previous: CC_PROMPT_firmware_qr_png_fetch.md ✅ PR #12 merged
> This prompt = PR #13 target
