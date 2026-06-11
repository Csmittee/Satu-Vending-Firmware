# CHAT HANDOFF — 2026-06-11
> Overwrite this file at end of every session — never append

## ⚠️ DO FIRST
1. Project → Files → GitHub sync checkbox → **CONFIRM CHECKED** (resets every new chat)
2. Paste this handoff into Chat

---

## 🔴 CRITICAL — config.h EXPOSED IN REPO
`config.h` (WiFi SSID + password) was committed to the firmware repo.
**Owner must:**
1. Delete config.h from repo immediately
2. Rotate the WiFi password on that network
3. Do NOT re-commit config.h — it is in .gitignore for this reason

---

## 🆕 NEW SYSTEM (added 2026-06-11)
`WORKFLOW_SKILL.md` now exists in this repo. Two loops: Loop A (cloud) and Loop B (firmware).
Session closing discipline is now mandatory.

---

## WHAT HAPPENED LAST SESSION (R4 build)
- Firmware R4 written and committed to this repo (CC_BUILD_PROMPT_R4.md)
- All 5 firmware files updated: satu_vending.ino, ui.h, network.h, state_machine.h, config.h
- hardware.h NOT modified — remains R2 LOCKED
- R4 is NOT yet compiled or flashed — owner must do this

---

## CURRENT STATE

| File | Version | Status |
|------|---------|--------|
| satu_vending.ino | R4 | ✅ In repo — NOT yet compiled |
| ui.h | R4 | ✅ In repo — service mode 5 tabs status unclear |
| state_machine.h | R4 | ✅ In repo |
| config.h | R4 | ✅ In repo — ⚠️ WiFi creds exposed — delete from repo |
| network.h | R4 | ✅ In repo |
| hardware.h | R2 | ⚠️ LOCKED — do NOT replace or modify |

---

## NEXT SESSION — EXACT ORDER

**Step 1:** Pull R4 files from this firmware repo to local Arduino sketch folder
- Do NOT compile from repo files directly — copy to Arduino sketch folder
- Do NOT touch hardware.h — R2 LOCKED

**Step 2:** Arduino IDE → Sketch → Verify (compile only — DO NOT flash yet)
- Report exact error text to Chat if compile fails
- Copy from serial monitor / error window — exact text, no paraphrasing

**Step 3:** If compile passes → Flash to device
- Open serial monitor at 115200 baud immediately after flash
- Expected boot sequence:
  ```
  [BOOT] Satu starting...
  [NVS] Loading config...
  [WiFi] Connecting to [SSID]...
  [WiFi] Connected — IP: [xxx.xxx.xxx.xxx]
  [HELLO] Sending /hello to backend...
  [HELLO] device_id: SATU-XXXXXX
  [HELLO] Slots loaded: 10
  [STATE] → IDLE
  ```
- Report any deviation to Chat

**Step 4:** Smoke test
- Tap a slot on the product grid → should advance to DONOR screen
- Tap Skip → should advance to GIFT_OPTION screen
- Tap Item Only → should show QR payment screen
- Report pass/fail to Chat

---

## OWNER ACTION REQUIRED

| Item | Priority | Notes |
|------|----------|-------|
| Delete config.h from repo | 🔴 IMMEDIATE | WiFi credentials exposed |
| Rotate WiFi password | 🔴 IMMEDIATE | After deleting config.h from repo |
| Pull R4 files | 🟡 NEXT | Before compile step |
| Compile verify | 🟡 NEXT | Arduino IDE → Sketch → Verify |
| Flash and smoke test | 🟡 AFTER COMPILE | Report serial output to Chat |

---

## ARDUINO IDE SETTINGS

```
Board:        ESP32S3 Dev Module
Flash:        16MB (128Mb)
Partition:    16M Flash (3MB APP/9.9MB FATFS)
PSRAM:        OPI PSRAM  ← NEVER CHANGE — display breaks without this
Upload Speed: 460800
Port:         /dev/cu.usbserial-1420
```

---

## LIBRARIES INSTALLED

```
Arduino_GFX (moononournation) — v1.4.9 ONLY (1.6.5 requires core 3.x — DO NOT UPGRADE)
TAMC_GT911 — touch controller
PNGdec (bitbank2/Larry Bank) — v1.1.6 — install via Library Manager before first flash
ArduinoJson — current stable
TFT_eSPI — REMOVE if installed (incompatible with RGB panel — causes compile errors)
ESP32 core — 2.0.17 ONLY (3.x breaks WiFi completely — DO NOT UPGRADE)
```

---

## NVS KEYS (namespace: satu, all ≤15 chars)

```
device_id    dev_secret   wifi_ssid    wifi_pass
svc_pin      svc_pin_en   boot_pin     cfg_idle
cfg_sel      cfg_water    cfg_lucky    scr_theme
lang         vol          nvs_idc      nvs_grow
nvs_gcol
```
