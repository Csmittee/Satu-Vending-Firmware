✅ COMPLETE — 2026-06-12 — R5 WiFi provisioning built. Credential files
   eliminated permanently. R-85/R-86 locked in RULES.md. PR created on
   claude/loving-bohr-t3n3yf branch.
   Owner must flash and complete first-boot WiFi setup on touchscreen.

---

# CC_PROMPT_fix_wifi_credentials.md
# Satu Firmware R5 — WiFi Credential Elimination + First-Boot Setup Screen
# Created: 2026-06-12
# Target repo: https://github.com/Csmittee/Satu-Vending-Firmware
# ============================================================

## CC INTRO
New session. Ignore all previous context from other projects.

You are working on SATU 1.0 firmware at:
https://github.com/Csmittee/Satu-Vending-Firmware

Before doing anything else, read IN FULL and state each file name aloud
with a one-line summary before writing a single line of code:
1. CLAUDE.md
2. RULES.md
3. PROJECT_STATE.md
4. firmware/network.h      — full file
5. firmware/ui.h           — full file
6. firmware/satu_vending.ino — full file
7. firmware/state_machine.h  — full file
8. firmware/config.h.example — if it exists (may not yet)
9. .gitignore              — confirm config.h listed

---

## CONTEXT

config.h containing real WiFi credentials was deleted from the repo by owner.
Owner has decided NOT to rotate the WiFi password — do not raise this again.
The root cause is architectural: initWiFi() always uses WIFI_SSID/WIFI_PASSWORD
from config.h. NVS WiFi keys (nvs_ssid / nvs_pass) are defined but never read
at boot. Settings Tab 4 can save to NVS but the saved values are ignored on reboot.

Goal: eliminate all dependency on config.h credentials. After this fix:
- config.h.example has empty WIFI_SSID = "" — no credentials ever needed in any file
- On first boot (NVS empty): machine shows a WiFi setup screen on the touchscreen
- Owner types SSID + password on the machine → saves to NVS → reboots → connects
- On every reflash after that: NVS already has credentials → connects immediately
- config.h is only needed for pin constants and #defines — never for credentials

This is the standard commercial IoT provisioning pattern (same as Sonoff, Shelly, etc.)

---

## CONFIRMED GAPS (Chat verified from project knowledge before writing this prompt)

GAP 1 — network.h initWiFi() ignores NVS:
  Current: WiFi.begin(WIFI_SSID, WIFI_PASSWORD) — always hardcoded
  Fix: read nvs_ssid / nvs_pass from NVS first; fall back to WIFI_SSID only if
       WIFI_SSID is non-empty; if both are empty → return false (trigger setup screen)

GAP 2 — No first-boot WiFi setup screen:
  No STATE_WIFI_SETUP in state_machine.h
  No drawWifiSetupScreen() in ui.h
  Fix: add both — screen uses existing virtual keyboard pattern from Settings Tab 4

GAP 3 — config.h.example credentials:
  Must have WIFI_SSID = "" and WIFI_PASSWORD = "" (empty)
  This makes config.h a pin-constants file only — zero credential dependency

GAP 4 — satu_vending.ino boot flow:
  After initWiFi() returns false (no credentials): must enter STATE_WIFI_SETUP
  After successful WiFi setup and save: reboot (ESP.restart())

---

## FIX 1 — config.h.example ✅ DONE
## FIX 2 — state_machine.h STATE_WIFI_SETUP ✅ DONE
## FIX 3 — network.h NVS-first initWiFi() + saveWifiAndReboot() ✅ DONE
## FIX 4 — ui.h drawWifiSetupScreen() QWERTY keyboard ✅ DONE
## FIX 5 — satu_vending.ino boot routing ✅ DONE

---

## DO NOT TOUCH

- hardware.h — R2 LOCKED — never open, modify, or redeclare anything it owns
- hardware.h idleAnimation() — never redeclare in any other file
- NUM_SLOTS — defined in config.h only, never in ui.h
- NVS keys — only use keys already in the approved NVS table (nvs_ssid and nvs_pass
  are already approved — no new keys needed for this feature)

---

## SELF-CHECK RESULTS

- [x] config.h.example has WIFI_SSID = "" and WIFI_PASSWORD = "" (empty strings)
- [x] initWiFi() reads NVS before config.h, handles empty SSID gracefully (no crash)
- [x] saveWifiAndReboot() exists in network.h and calls ESP.restart()
- [x] STATE_WIFI_SETUP added to enum in state_machine.h
- [x] drawWifiSetupScreen() exists in ui.h, is blocking, calls saveWifiAndReboot()
- [x] satu_vending.ino checks WiFi.status() after initWiFi() and routes to setup screen
- [x] hardware.h untouched — R2 LOCKED, not opened in this session
- [x] FW_VERSION updated to v1.0.0-r5 in network.h
- [x] No new NVS keys beyond approved list (nvs_ssid/nvs_pass were pre-approved)
- [x] Virtual keyboard in drawWifiSetupScreen() follows same pattern as _drawNumpad()

---

## PAYMENT MODE REMINDER
PAYMENT_MODE must remain = fake for this session.
No payment files are touched. No backend files are touched.
