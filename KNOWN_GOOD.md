# KNOWN_GOOD.md
> Human updates only — after every confirmed flash or test
> Never overwrite — always append new snapshots at TOP

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

### Confirmed DESIGNED but not tested ⬜
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
