# KNOWN_GOOD.md
> Human updates only — after every confirmed flash or test
> Never overwrite — always append new snapshots at TOP

---

## 2026-06-19 — Firmware R9 (R-153 STATE_CONFIRMING) — ✅ CONFIRMED ON HARDWARE
- **Build:** state_machine.h + satu_vending.ino + ui.h R9 — config.h now tracked in repo (R-86)
- **Board:** SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
- **Branch:** claude/optimistic-goodall-0p7gil — PR #36
- **Flash:** ✅ CONFIRMED — owner flashed 2026-06-19
- **Test:** HW Trigger used for payment confirmation (fake mode)
- **Full flow confirmed from serial:**
  ```
  [STATE] 2 → 5    (IDLE → PRODUCT_SELECTION)
  [UI] Touch: slot 0 (Small Amulet 100THB)
  [STATE] 5 → 6    (PRODUCT_SELECTION → GIFT_OPTION)
  [UI] Gift option screen: slot 0
  [UI] Gift touch: Item Only
  [STATE] 6 → 7    (GIFT_OPTION → CONFIRMING)
  [UI] Confirm screen: slot 0 water=0 total=100
  [UI] Confirm touch: Confirm
  [NET] Order SATU-20260619-996361 — 10 THB — water=0
  [STATE] 7 → 8    (CONFIRMING → AWAITING_PAYMENT)
  [UI] QR PNG URL: ...
  [NET] fetchImageBytes: HTTP 200 Content-Length=27458
  [UI] PNG decode: rc=0 rows=165 w=165 h=165
  [UI] QR screen drawn
  [NET] Order SATU-20260619-996361: paid   ← HW Trigger webhook
  [STATE] Payment confirmed via poll
  [UI] Payment accepted banner shown
  [STATE] 8 → 9    (AWAITING_PAYMENT → VENDING)
  [HW] Relay 12 → ON / Flap UNLOCKED
  [HW] Relay 1 → ON — motor SPINNING + flap UNLOCKED
  [HW] sensor_triggered — stopping motor after 2790ms
  [HW] Relay 1 → OFF — motor stopped (sensor)
  [HW] Relay 12 → OFF / Flap LOCKED
  [HW] Flap re-locked via TIMEOUT (3000ms)
  [STATE] 9 → 10   (VENDING → COMPLETING)
  [UI] Completion: slot=0 lucky=79 water=0
  [NET] Completion report: HTTP 200
  [STATE] 10 → 2   (COMPLETING → IDLE)
  [UI] Idle screen drawn (5x2 grid)
  ```
- **Also confirmed:** Back button on gift option screen (→ product selection). Payment timeout path (7→10→2).
- **QR UX:** White box during fetch eliminated — customer now sees Confirm screen during QR fetch. Backlight flash on PNG decode is imperceptible (~100ms). No re-fetch after QR appears.
- **D1 safety:** createOrder() only called on Confirm touch. Abandoned flows = zero D1 rows.

---

## 2026-06-19 — Firmware R7 (R-148/R-149) — CI ⬜ pending
- satu_vending.ino R7: STATE_GIFT_OPTION entry guard (R-148) — 250ms touch ignore on entry; network.h included before hardware.h (compile dependency fix)
- hardware.h R7: vendProduct() spin loop polls pollCommands() every 500ms (R-149)
- Files changed: satu_vending.ino, hardware.h only
- CI: ⬜ GitHub Actions pending
- Flash: ⬜ 1 cycle required — owner flash after CI green
- Expected serial (IR trigger test):
  ```
  [HW] Relay N ON — motor SPINNING + flap UNLOCKED
  [HW] sensor_triggered command received — stopping motor after ~XXXms
  [HW] Relay N OFF — motor stopped (sensor)
  [HW] Flap re-locked via TIMEOUT (3000ms) — proximity not wired or stuck
  [HW] Flap LOCKED — pin extended
  [STATE] Complete — slot=N ...
  ```

## 2026-06-17 — Firmware R6 (R-128/R-129/R-131/R-137) — CI pending ⬜
- **config.h R6:** RELAY_FLAP=12, VEND_MAX_SPIN_MS=30000, SENSOR_POLL_MS=10, FLAP_RELOCK_TIMEOUT=3000, FLAP_PROXIMITY_MCP_PIN=-1, SPEAKER_PIN=-1
- **hardware.h R6:** vendProduct() bool/sensor-driven stop, unlockFlap(), lockFlap() added; unlockDoor()/lockDoor() removed
- **state_machine.h R6:** STATE_WAITING_DROP, STATE_DISPENSING, STATE_WAITING_REMOVAL removed — synchronous vend
- **satu_vending.ino R6:** _onPaymentConfirmed(), _onItemDropped(), _onLaneEmpty() — synchronous flow
- **ui.h R6:** showPaymentAccepted() R-131 (1.5s green banner) + full R-137 font audit applied to all screens
- CI: ⬜ GitHub Actions pending
- Flash: ⬜ Pending owner flash after CI green
- Branch: `claude/magical-feynman-rrkais`
- Expected Serial (normal vend):
  ```
  [HW] Relay N ON — motor SPINNING + flap UNLOCKED
  [HW] Sensor N TRIGGERED — item detected after Xms
  [HW] Relay N OFF — motor stopped (sensor)
  [HW] Flap re-locked via TIMEOUT (3000ms)
  [HW] Flap LOCKED — pin extended
  [STATE] Complete — slot=N lucky=NN water=0
  ```

---

## 2026-06-16 — R-126 firmware UX — touch + font fixes — flash required ⬜
- ui.h: `idleAnimationUI()` now polls touch every 16ms — no longer blocks
- ui.h: Large text screens use Adafruit FreeFonts (24pt/18pt/12pt) instead of scaled bitmaps
- RULES.md: R-126 added
- CI: pending GitHub Actions green before merge
- Flash: required by owner after PR merge
- Baseline: SATU-4R473R confirmed passing full end-to-end test 2026-06-16

## 2026-06-15 — PNG QR DECODE FIXED ✅ CONFIRMED ON HARDWARE
- Root cause: `_pngDrawRow()` returned `0` (PNGdec v1.1.4 stop-early signal) instead of `1`
- Fix: `return 1` in callback — one character
- Evidence: `[UI] PNG decode: rc=0 rows=165 w=165 h=165` on SATU-4R473R 2026-06-15 16:41:32
- Branch: `claude/practical-cray-wsim4a` → PR open
- Flash: ✅ CONFIRMED WORKING — QR renders correctly on hardware
- Skills added: `SKILL_library_onboarding.md`, `LIBRARY_pngdec.md`
- Rules added/corrected: R-89 (corrected return 0→1), R-117 (corrected root cause), R-121, R-122, R-123

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

### Firmware main state (repo, not what's on hardware)
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

### Confirmed WORKING ✅ (fake mode only)
- QR bitmap renders on hardware screen
- drawQrFromBitmap() rc=0, draw complete confirmed via serial

### NOT working in live mode ❌
- Live Omise returns real PromptPay PNG — cannot be re-served as raw bitmap
- Bitmap approach is fake-mode-only — NOT a final solution
- Backend bitmap endpoint REVERTED 2026-06-15 (PR #21)

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

---

## Snapshot: 2026-06-12 — R5 First Flash ✅

### Firmware
```
Build:       R5
Board:       SATU-4R473R (MAC: 3C:DC:75:5D:DD:2C)
Flash:       ✅ CONFIRMED clean — 2026-06-12
Compile:     ✅ Clean after 3 fix loops
```

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
