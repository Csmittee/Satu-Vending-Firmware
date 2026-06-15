# KNOWN_GOOD.md
> Human updates only — after every confirmed flash or test
> Never overwrite — always append new snapshots at TOP

---

## 2026-06-15 — PNG decode fix R-117 (pending owner flash)
- Files: `firmware/ui.h` — drawQrFromBytes() re-enabled with pause-decode-resume · lineBuf static · drawQrScreen() on PNG path
- Compile: ⬜ GitHub Actions pending
- Flash: ⬜ pending owner flash
- Expected serial: `[UI] PNG decode: rc=0 rows=165 w=165 h=165`
- Root cause documented: PSRAM DMA bus contention — ESP32-8048S070C class boards
- Branch: `claude/practical-cray-wsim4a` → PR `fix/png-psram-contention-r117`

---

## 2026-06-15 — State after session close

### Current hardware state
```
Flashed:  Owner reflashing to R5.3 (pre-bitmap) from Mac trash backup
Reason:   Backend bitmap endpoint reverted (PR #21) — firmware main
          has bitmap code but backend has no /bitmap endpoint
Next:     PNGdec diagnostic session — esp_ptr_in_psram() in initUI()
```

### Firmware main state (repo, not what’s on hardware)
```
Branch:   main
PR #17:   MERGED 2026-06-14 — bitmap ui.h on main
PR #14:   R5.3 blocking readBytes (last confirmed flash to hardware)
Warning:  Firmware main has bitmap code — do NOT flash from main until
          backend bitmap endpoint is restored OR PNGdec fix lands
```

---

## 2026-06-14 — QR bitmap draw ✅ CONFIRMED flash

### Firmware
```
Build:    R-114 — drawQrFromBitmap() direct pixel draw, no PNGdec
Branch:   claude/cool-hopper-6owumd (merged to main via PR #17)
Files:    ui.h (drawQrFromBitmap added, drawQrFromBytes commented out, drawQrScreen updated)
Compile:  ✅ GitHub Actions CI Run #46 — GREEN
Flash:    ✅ CONFIRMED — owner flashed 2026-06-14, QR bitmap rendered on screen
```

### What changed
- drawQrFromBytes() (PNGdec) commented out — library broken on all 4 PNG variants (PRs #16-#19)
- drawQrFromBitmap() added — direct gfx->fillRect() per pixel, no decode library
- drawQrScreen() now fetches /bitmap endpoint (qrUrl + "/bitmap") via existing fetchImageBytes()
- Backend bitmap format: 4-byte header (width+height uint16 BE) + 1 byte/pixel (0x00=black 0xFF=white)

### Confirmed WORKING ✅ (fake mode only)
- QR bitmap renders on hardware screen
- drawQrFromBitmap() rc=0, draw complete confirmed via serial

### NOT working in live mode ❌
- Live Omise returns real PromptPay PNG — cannot be re-served as raw bitmap
- Bitmap approach is fake-mode-only — NOT a final solution
- Backend bitmap endpoint REVERTED 2026-06-15 (PR #21)

### Serial output confirmed
```
[UI] drawQrFromBitmap: done
[UI] QR screen drawn
```

---

## Snapshot: 2026-06-14 — PR #14 R5.3 blocking readBytes CONFIRMED ✅ QR RENDERS

### Firmware
```
Build:    PR #14 — R5.3 blocking readBytes() loop (R-105)
Board:    SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
Flash:    CONFIRMED — 2026-06-14 ~09:50
```

### Serial output confirmed (owner report 2026-06-14 ~09:50)
```
[NET] fetchImageBytes: url=https://api.janishammer.com/v1/qr/fake_chg_pu8wzvoa
[NET] fetchImageBytes: HTTP 200
[NET] fetchImageBytes: Content-Length=449
[NET] fetchImageBytes: 449 bytes read
[UI] QR PNG loaded: 449 bytes — rendering
[UI] QR screen drawn
[STATE] Order SATU-20260614-851453 — 10 THB — water=0
[NET] Order SATU-20260614-851453: pending
```

### Test suite
```
satu-system-tester.html:  14/14 ✅ ALL PASSING
Backend PRs:              #12 (QR endpoint), #13 (CORS + HEAD fix) merged/deployed
Payment mode:             fake_omise
```

### What changed (vs previous snapshot — 497 bytes / idle timeout)
- Root cause was available()=0 between TCP packets → 5s idle timer fired at 497 bytes
- R5.3 fix: stream->readBytes() blocks until data arrives or stream closes
- Result: full 449-byte PNG received in one clean transfer, QR renders on screen

---

## Snapshot: 2026-06-13 — PR #13 QR Chunked Fix flash CONFIRMED (available() root cause confirmed)


### Firmware
```
Build:    PR #13 — R5.2 chunked-safe read loop
Board:    SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
Flash:    CONFIRMED — 2026-06-13 ~22:08
```

### Serial output confirmed (owner report 2026-06-13 ~22:08)
```
[NET] fetchImageBytes: HTTP 200
[NET] fetchImageBytes: Content-Length=-1
[NET] fetchImageBytes: idle timeout after 5000ms — 497 bytes so far
[NET] fetchImageBytes: 497 bytes read
[UI] QR PNG loaded: 497 bytes - rendering
[UI] PNG open failed
```

### Root cause confirmed ✅
stream->available() = 0 between TCP packets → 5s idle timer fires at 497 bytes.
QR PNG from api.qrserver.com is ~3KB+. available()-based loop exits too early.
Fix: blocking readBytes() — R-105 — PR #14 target.

### Confirmed WORKING ✅
- WiFiClientSecure HTTPS (R-97) — HTTP 200 on external api.qrserver.com
- Payment timeout 30s (R-102)
- chunked transfer detection — Content-Length=-1 logged correctly

### Confirmed NOT WORKING ❌
- QR PNG render: available()-based idle timer exits at 497 bytes
  Root cause: available()=0 between TCP packets on ESP32 (R-105)
  Fix: CC_PROMPT_firmware_qr_blocking_read — PR #14

---

## Snapshot: 2026-06-13 — QR Chunked Fix (CC_PROMPT_firmware_qr_chunked_fix)

### Firmware
```
Build:       R5.2
Files:       network.h (chunked-safe read loop), config.h (PAYMENT_TIMEOUT 120s→30s)
Compile:     CI pending — push to claude/vibrant-cray-cqp2em
Flash:       PENDING — owner to flash and report serial output
Serial:      "[NET] fetchImageBytes: stream closed — transfer complete" = success
             "[NET] fetchImageBytes: XXXX bytes read" — expect 2000-5000 bytes
```

### What changed
- `network.h fetchImageBytes()`: chunked-safe read loop — detects EOF via !http.connected()
  Per-packet idle timeout 5s replaces broken 15s global wall-clock
  Root cause: 502 bytes truncated because stream.available()=0 between TCP packets
- `config.h PAYMENT_TIMEOUT`: 120000 → 30000 (30s QR wait, was 2 min)
- `RULES.md`: R-102 (QR timeout) + R-103 (chunked read) appended

---

## Snapshot: 2026-06-13 — PR #12 QR WiFiClientSecure flash CONFIRMED ✅

### Firmware
```
Build:    PR #12 — WiFiClientSecure for external HTTPS QR fetch
Board:    SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
Flash:    ✅ CONFIRMED — 2026-06-13 ~21:34
Compile:  ✅ Clean — GitHub Actions green (2 min 21 sec)
```

### Serial output confirmed (owner report 2026-06-13)
```
[NET] fetchImageBytes: url=https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=FAKE|...
[NET] fetchImageBytes: HTTP 200
[NET] fetchImageBytes: Content-Length=-1
[NET] fetchImageBytes: timeout
[NET] fetchImageBytes: 502 bytes read
[UI] QR PNG loaded: 502 bytes — rendering
[UI] PNG open failed
[UI] QR screen drawn
```

### Confirmed WORKING ✅
- WiFiClientSecure HTTPS to external domain (api.qrserver.com) — HTTP 200
- fetchImageBytes() called correctly from drawQrScreen()
- Serial logs present on both success and failure paths
- State machine reaches QR screen, polls payment correctly
- WiFi keyboard . @ - _ keys present (PR #9 confirmed)
- 14-test suite backend: ✅ 14/14 (confirmed 2026-06-13 earlier)

### Confirmed NOT WORKING ❌
- QR PNG render: 502 bytes truncated (chunked stream read exits too early)
  Fix: CC_PROMPT_firmware_qr_chunked_fix.md — next flash will resolve

---

## Snapshot: 2026-06-12 — R5 First Flash ✅

### Backend
```
Endpoint:    https://api.janishammer.com
Test suite:  NOT RUN post-R4 — no backend changes today
Last known:  14/14 passing (pre-R4, date unknown)
Commit:      check backend repo main branch
```

### Firmware
```
Build:       R5
Board:       SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
Flash:       ✅ CONFIRMED clean — 2026-06-12
Compile:     ✅ Clean after 3 fix loops:
             - extern/static conflict (6 globals)
             - PNGdec callback void→int
             - g_setupCode undefined reference
             - loadConfigFromNVS() extern scope
```

### Serial output confirmed (2026-06-12 ~17:00)
```
[NET] WiFi OK — IP: 192.168.1.171
[NET] NVS loaded: device_id=SATU-4R473R FW: v1.0.0-r3  ← r3 in NVS, ok
[NET] /hello OK: device_id=SATU-4R473R status=active slots=3
[UI]  Slot 1: Small Amulet 100 THB en=1
[UI]  Slot 2: Blessing Card 50 THB en=1
[UI]  Slot 3: Large Amulet 200 THB en=1
[SATU] Network OK
[STATE] 0 → 1
[UI]  Idle screen drawn (10-slot grid 5x2)
[SATU] Ready
[NET] Heartbeat: HTTP 200
[UI]  Touch: col=1 row=0 → slot 1 (Blessing Card 50THB)
[STATE] 1 → 4
[UI]  Product selection: slot 1
[UI]  Gift option screen: slot 1
[STATE] 4 → 5 → Gift: Item Only
[NET]  Order SATU-20260612-001635 — 20 THB — water=0
[STATE] 5 → 6
[UI]  QR screen drawn
[NET]  Order polling: pending (every 3s)
```

### Confirmed WORKING ✅
- WiFi connects via NVS
- /hello handshake with backend
- Heartbeat every 5 min
- Touch → product selection → gift option → QR screen
- Order creation and polling
- State machine 0→1→4→5→6

### Confirmed NOT WORKING ❌
- QR PNG: green box shown, no PNG renders
  fetchImageBytes() silent fail — qr_code_url format unknown
- Service mode: not tested (stubs only)
- WiFi setup screen keyboard: missing . @ - _ characters

### Confirmed DESIGNED but not tested ◻
- WiFi setup screen (appears on empty NVS — confirmed logic correct)
- Countdown visual timer
- Service mode 5 tabs
- Factory reset flow
- Completion/dispensing flow

### Arduino IDE settings used (confirmed working)
```
Board:     ESP32S3 Dev Module
PSRAM:     OPI PSRAM ← CRITICAL — black screen if changed
Flash:     16MB
Partition: 16M Flash (3MB APP/9.9MB FATFS)
Upload:    460800
Port:      /dev/cu.usbserial-1420
Core:      2.0.17
```

---

## Snapshot: PRE-R4 (date unknown) — Backend only
```
Test suite: 14/14 passing
Backend:    all endpoints responding
Payment:    PAYMENT_MODE=fake confirmed
```
