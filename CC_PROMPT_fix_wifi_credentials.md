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

## FIX 1 — config.h.example (repo file — safe, no credentials)

Create or overwrite config.h.example with this exact content.
WIFI_SSID and WIFI_PASSWORD are EMPTY STRINGS — this is intentional and correct.
The machine gets WiFi from NVS (typed on touchscreen). config.h is pin constants only.

```cpp
// ============================================================
// config.h.example
// ============================================================
// INSTRUCTIONS:
//   Copy this file → config.h in your local Arduino sketch folder.
//   DO NOT fill in WiFi credentials here.
//   WiFi is configured on the machine touchscreen (first boot setup screen).
//   config.h is gitignored — it holds pin constants only.
//   NEVER commit config.h to git.
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// WiFi fallback — LEAVE EMPTY
// Machine uses NVS WiFi set via touchscreen. These are only
// used if NVS is empty AND a non-empty value is set here.
// For normal deployment: leave both as empty strings below.
// ============================================================
const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = "";

// ============================================================
// Backend API
// ============================================================
const char* API_BASE_URL = "https://api.janishammer.com";

// ============================================================
// Slot count — must match max grid (3 rows × 7 cols = 21)
// Runtime grid is set by /hello config{} → NVS → ui.h globals
// ============================================================
#define NUM_SLOTS      21
#define NUM_COLS        7
#define MAX_SLOTS_HW   21

// ============================================================
// Pin Mapping (ESP32-S3 / ESP32-8048S070C)
// ============================================================
#define I2C_SDA      19
#define I2C_SCL      20
#define UART_RX      44
#define UART_TX      43
#define LED_DATA_PIN  5
#define TFT_BL        2

// ============================================================
// MCP23017 I2C Addresses
//   MCP1 (0x20): sensors 1-8,  relays 1-6
//   MCP2 (0x21): sensors 9-16, relays 7-12 (pump + lock)
//   MCP3 (0x22): sensors 17-21, relays 13-18  — future 21-lane
// ============================================================
#define MCP1_ADDR  0x20
#define MCP2_ADDR  0x21
#define MCP3_ADDR  0x22

// ============================================================
// MCP Pin Declarations (defined in hardware.h)
// ============================================================
extern const int mcp1_sensors[8];
extern const int mcp1_relays[6];
extern const int mcp2_sensors[2];
extern const int mcp2_relays[6];

// ============================================================
// Relay Logic
// ============================================================
#define RELAY_ON          HIGH
#define RELAY_OFF         LOW
#define SENSOR_TRIGGERED  LOW
#define SENSOR_CLEAR      HIGH
#define VEND_PULSE_MS     800

// ============================================================
// Timeouts (milliseconds)
// ============================================================
#define PAYMENT_TIMEOUT      120000
#define VEND_TIMEOUT          10000
#define DROP_TIMEOUT           5000
#define REMOVAL_TIMEOUT       30000
#define HEARTBEAT_INTERVAL   300000

// ============================================================
// LED Configuration (WS2812B strip, GPIO5)
// ============================================================
#define NUM_LEDS        40
#define LED_BRIGHTNESS  128

#define ZONE_TOP_START     0
#define ZONE_TOP_END       9
#define ZONE_FLOOR1_START 10
#define ZONE_FLOOR1_END   19
#define ZONE_FLOOR2_START 20
#define ZONE_FLOOR2_END   29
#define ZONE_DOOR_START   30
#define ZONE_DOOR_END     39

#endif // CONFIG_H
```

---

## FIX 2 — state_machine.h: add STATE_WIFI_SETUP

Add one new state to enum MachineState, between STATE_STARTUP and STATE_IDLE:

```cpp
STATE_WIFI_SETUP,   // first boot: no WiFi credentials in NVS — show setup screen
```

Full enum after change (write the complete file):
```cpp
enum MachineState {
  STATE_STARTUP,
  STATE_WIFI_SETUP,         // ← NEW R5
  STATE_IDLE,
  STATE_ID_SCANNING,
  STATE_AUTHENTICATING,
  STATE_PRODUCT_SELECTION,
  STATE_GIFT_OPTION,
  STATE_AWAITING_PAYMENT,
  STATE_VENDING,
  STATE_WAITING_DROP,
  STATE_DISPENSING,
  STATE_WAITING_REMOVAL,
  STATE_COMPLETING,
  STATE_ERROR,
  STATE_OFFLINE,
  STATE_SERVICE
};
```

---

## FIX 3 — network.h: initWiFi() reads NVS first

Replace the entire initWiFi() function with this logic.
Write the complete network.h file — do not patch.

New boot priority:
1. Read nvs_ssid / nvs_pass from NVS namespace "satu"
2. If nvs_ssid is non-empty → try WiFi.begin(nvs_ssid, nvs_pass)
3. Else if WIFI_SSID (from config.h) is non-empty → try WiFi.begin(WIFI_SSID, WIFI_PASSWORD)
4. Else → return without connecting (WiFi.status() != WL_CONNECTED signals setup screen needed)

New function signature (unchanged — caller compatibility preserved):
```cpp
void initWiFi(JsonDocument& helloDoc)
```

New internal logic pseudocode:
```
loadConfigFromNVS();                          // existing — loads grid/config

Preferences prefs;
prefs.begin("satu", true);
String nvsSSID = prefs.getString("nvs_ssid", "");
String nvsPass = prefs.getString("nvs_pass",  "");
prefs.end();

String useSSID = nvsSSID.length() > 0 ? nvsSSID :
                 (strlen(WIFI_SSID) > 0 ? String(WIFI_SSID) : "");
String usePass = nvsSSID.length() > 0 ? nvsPass :
                 (strlen(WIFI_SSID) > 0 ? String(WIFI_PASSWORD) : "");

if (useSSID.isEmpty()) {
  Serial.println("[NET] No WiFi credentials — setup screen required");
  return;   // helloDoc stays empty; caller checks WiFi.status()
}

Serial.printf("[NET] Connecting WiFi: %s (source: %s)\n",
              useSSID.c_str(), nvsSSID.length() > 0 ? "NVS" : "config.h");

WiFi.mode(WIFI_STA);
WiFi.begin(useSSID.c_str(), usePass.c_str());
// ... rest of existing connection loop + /hello logic unchanged
```

Add a new public function to network.h for saving WiFi from the setup screen:
```cpp
// ════════════════════════════════════════════════
//  SAVE WIFI TO NVS  (called from WiFi setup screen)
//  Saves SSID + password then reboots.
// ════════════════════════════════════════════════
void saveWifiAndReboot(const String& ssid, const String& pass) {
  Preferences prefs;
  prefs.begin("satu", false);
  prefs.putString("nvs_ssid", ssid);
  prefs.putString("nvs_pass",  pass);
  prefs.end();
  Serial.printf("[NET] WiFi saved to NVS: %s — rebooting\n", ssid.c_str());
  delay(500);
  ESP.restart();
}
```

Change log header in network.h: add R5 entry:
```
//   R5  — initWiFi() reads NVS (nvs_ssid/nvs_pass) first, config.h fallback second
//          Empty WIFI_SSID triggers setup screen (no crash)
//          Added: saveWifiAndReboot() for touchscreen provisioning
//          FW_VERSION → v1.0.0-r5
```

---

## FIX 4 — ui.h: add drawWifiSetupScreen()

Add a new function to ui.h. Uses the existing virtual keyboard pattern already
defined in the Settings Tab 4 WiFi input section (already in the codebase).

This is a BLOCKING screen — it loops until the user enters SSID + password and
taps CONNECT. It does NOT return — it calls saveWifiAndReboot() which reboots.

Screen layout (800×480):
```
┌─────────────────────────────────────────────────────────┐
│  SATU                    FIRST TIME SETUP               │
├─────────────────────────────────────────────────────────┤
│                                                          │
│   WiFi Setup                                            │
│   Enter your WiFi network name and password             │
│                                                          │
│   Network name (SSID):                                  │
│   ┌──────────────────────────────────────────┐          │
│   │ [tap to enter]                           │  [CLEAR] │
│   └──────────────────────────────────────────┘          │
│                                                          │
│   Password:                                             │
│   ┌──────────────────────────────────────────┐          │
│   │ ••••••••                                 │  [SHOW]  │
│   └──────────────────────────────────────────┘          │
│                                                          │
│   ┌──────────────────────────────────────────────────┐  │
│   │              CONNECT TO WIFI                     │  │
│   └──────────────────────────────────────────────────┘  │
│                                                          │
│   [virtual keyboard — same as Settings Tab 4]            │
│                                                          │
│   This screen appears only when no WiFi is configured.  │
│   After setup, this screen will never appear again      │
│   unless you do a factory reset.                        │
└─────────────────────────────────────────────────────────┘
```

Implementation notes:
- Two input fields: ssidInput (String) and passInput (String)
- Active field tracked by a boolean (editingSSID)
- Tap SSID field box → set editingSSID = true, redraw active border
- Tap Password field box → set editingSSID = false, redraw
- Virtual keyboard feeds characters to active field
- [SHOW] button toggles password masking (same pattern as Settings Tab 4)
- [CLEAR] clears the active field
- [CONNECT] button: if ssidInput is empty → flash field red + show "Enter network name"
  if ssidInput is non-empty → call saveWifiAndReboot(ssidInput, passInput)
  (password CAN be empty — open networks exist)
- Status bar shows: "SATU | SETUP" — no WiFi indicator (we have no WiFi yet)
- Use existing C_GOLD, C_BG, C_WHITE, C_RED, C_GREEN colour constants
- Use existing _fillRoundRect(), _drawRoundRect() helpers
- Use existing virtual keyboard layout from Settings Tab 4 in ui.h
  DO NOT rewrite the keyboard — reuse the exact same implementation
- Function signature:
  ```cpp
  void drawWifiSetupScreen();   // blocking — calls saveWifiAndReboot() on success
  ```

---

## FIX 5 — satu_vending.ino: boot flow checks for WiFi setup

In setup(), after initWiFi(helloDoc) returns, add this check before
processing machineStatus:

```cpp
// If no WiFi credentials were available, enter setup screen
if (WiFi.status() != WL_CONNECTED) {
  drawBootScreen("WiFi setup required...");
  delay(1000);
  drawWifiSetupScreen();   // blocking — reboots on success
  // Execution never reaches here — drawWifiSetupScreen() always reboots
  return;
}
```

This replaces the existing offline-state handling for the no-credentials case.
If WiFi is present but /hello fails: existing offline path is unchanged.

Also: update FW_VERSION reference in any boot Serial.println to say R5.

---

## DO NOT TOUCH

- hardware.h — R2 LOCKED — never open, modify, or redeclare anything it owns
- hardware.h idleAnimation() — never redeclare in any other file
- NUM_SLOTS — defined in config.h only, never in ui.h
- NVS keys — only use keys already in the approved NVS table (nvs_ssid and nvs_pass
  are already approved — no new keys needed for this feature)

---

## SELF-CHECK BEFORE COMMITTING (CC must verify each item)

- [ ] config.h.example has WIFI_SSID = "" and WIFI_PASSWORD = "" (empty strings)
- [ ] initWiFi() reads NVS before config.h, and handles empty SSID gracefully (no crash)
- [ ] saveWifiAndReboot() exists in network.h and calls ESP.restart()
- [ ] STATE_WIFI_SETUP added to enum in state_machine.h
- [ ] drawWifiSetupScreen() exists in ui.h, is blocking, calls saveWifiAndReboot()
- [ ] satu_vending.ino checks WiFi.status() after initWiFi() and routes to setup screen
- [ ] hardware.h untouched — confirm by stating its last-modified commit hash
- [ ] FW_VERSION updated to v1.0.0-r5 in network.h
- [ ] No new NVS keys introduced beyond the approved list
- [ ] Virtual keyboard in drawWifiSetupScreen() reuses existing implementation — not duplicated

---

## MANDATORY AT END OF SESSION

### 1. Append TWO rules to RULES.md at TOP with next sequential R-numbers

Rule A:
```
R-[N]: NO HARDCODED CREDENTIALS — PERMANENT RULE
       Chat and CC must NEVER write WiFi SSID, WiFi password, API keys, or any
       credential into any file tracked by git.
       config.h is gitignored — it holds pin constants only.
       WiFi credentials live in NVS only, entered via the machine touchscreen.
       If a credential is needed: use NVS (firmware) or Cloudflare secrets (backend).
       Chat must never ask owner to "put your WiFi password in a file."
       CC must never write a credential value into any committed file.
       This rule cannot be overridden by any future prompt or instruction.
```

Rule B:
```
R-[N+1]: config.h WORKFLOW — PERMANENT RULE
          config.h is gitignored. Repo holds only config.h.example with empty credentials.
          config.h.example has WIFI_SSID = "" — this is correct and intentional.
          CC reads pin constants from config.h.example (safe). Never asks for WiFi values.
          Owner maintains local config.h with empty credentials (same as example).
          WiFi is provisioned on the machine touchscreen — never in any file.
          No CC session should ever require touching WiFi credentials.
```

### 2. Update PROJECT_STATE.md
- Mark WiFi credential risk RESOLVED — 2026-06-12
- Add new entry under firmware features: "R5: First-boot WiFi setup screen — built, not yet flashed"
- Update FW_VERSION to v1.0.0-r5

### 3. Open a Pull Request
Branch: `feature/wifi-touchscreen-provisioning`
PR title: `[R5] WiFi touchscreen provisioning — eliminate credential files permanently`
PR body:
```
## What
- config.h.example: WIFI_SSID = "" — credential-free permanently
- network.h: initWiFi() reads NVS first, config.h second, handles empty gracefully
- network.h: saveWifiAndReboot() added for touchscreen provisioning
- ui.h: drawWifiSetupScreen() added — blocking first-boot screen
- state_machine.h: STATE_WIFI_SETUP added
- satu_vending.ino: boot flow routes to setup screen if no credentials

## Why
Eliminates all dependency on credential files. Owner types WiFi on the machine
touchscreen once. Every subsequent reflash skips setup (NVS survives flash).
Same provisioning pattern as Sonoff, Shelly, and all commercial IoT devices.

## Flash behaviour
First flash on this build: setup screen will appear (NVS empty)
Owner types SSID + password → machine saves to NVS → reboots → connects
All future reflashes: NVS has credentials → connects immediately, no setup screen

## Files changed
- config.h.example — empty credentials
- firmware/network.h — NVS-first boot + saveWifiAndReboot()
- firmware/ui.h — drawWifiSetupScreen()
- firmware/state_machine.h — STATE_WIFI_SETUP
- firmware/satu_vending.ino — boot routing
- RULES.md — R-[N] and R-[N+1] permanent no-credential rules

## Do not touch
- hardware.h (R2 LOCKED — confirmed untouched)

## Owner action after flash
1. First boot: setup screen appears → type WiFi SSID + password → tap CONNECT
2. Machine reboots, connects to WiFi, proceeds to /hello
3. All future reflashes: no setup screen — NVS credentials persist
```
Merge PR to main after opening.

### 4. Archive this prompt
Copy to: `docs/prompts/CC_PROMPT_fix_wifi_credentials.md`
Stamp at top of archived copy:
```
✅ COMPLETE — 2026-06-12 — R5 WiFi provisioning built. Credential files
   eliminated permanently. R-[N]/R-[N+1] locked in RULES.md. PR merged.
   Owner must flash and complete first-boot WiFi setup on touchscreen.
```

---

## PAYMENT MODE REMINDER
PAYMENT_MODE must remain = fake for this session.
No payment files are touched. No backend files are touched.
