# CC_PROMPT_firmware_qr_blocking_read.md
> Created by: Chat (Claude)
> Date: 2026-06-13
> Session goal: Fix QR PNG truncated read — replace available()-based loop with blocking readBytes()
> Repo: https://github.com/Csmittee/Satu-Vending-Firmware
> PR target: main
> Mode: Firmware hotfix
> Flash cycles expected: 1

## ✅ COMPLETE — 2026-06-13 — QR blocking read fix (R5.3)

### What was done
- `firmware/network.h fetchImageBytes()`: R5.3 — blocking readBytes() loop replaces available()-based loop
  Key fix: stream->readBytes() blocks until data arrives, stream closes, or 10s per-read timeout fires
  available()-based idle timer (5s) removed entirely
  stream->setTimeout(10000) set before read loop
  CHUNK = 512 bytes per readBytes() call
- `RULES.md`: R-105 appended at TOP (available()=0 between TCP packets — permanent rule)
- `KNOWN_GOOD.md`: PR #13 confirmed flash snapshot appended at TOP
- `PROJECT_STATE.md`: session log entry added
- Branch: claude/vibrant-cray-cqp2em

### Root cause confirmed from PR #13 flash serial output (owner report 2026-06-13 ~22:08)
```
[NET] fetchImageBytes: HTTP 200
[NET] fetchImageBytes: Content-Length=-1
[NET] fetchImageBytes: idle timeout after 5000ms — 497 bytes so far
[NET] fetchImageBytes: 497 bytes read
[UI] QR PNG loaded: 497 bytes - rendering
[UI] PNG open failed
```
stream->available() = 0 between TCP packets → 5s idle timer fires at 497 bytes.
QR PNG from api.qrserver.com is ~3KB+. available()-based loop always exits too early.

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
   - "stream closed" + 2000+ bytes → QR should render (PNG decode next gate)
   - "stream closed" + still ~500 bytes → report byte count, further diagnosis
   - "PNG open failed" still → report, PNG decode investigation next

### PAYMENT MODE REMINDER
PAYMENT_MODE must remain = fake.
Never suggest changing to live.
