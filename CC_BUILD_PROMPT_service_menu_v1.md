# CC_BUILD_PROMPT_service_menu_v1.md
> Created by: Chat (Claude) — 2026-06-17 (REVISED — replaces earlier draft)
> Session goal: Complete all 5 service mode tabs in ui.h
> Repo: Satu-Vending-Firmware
> Mode: Firmware Mode — ui.h primary, config.h minor, satu_vending.ino action handlers
> Flash cycles: 1 expected
> PR target: main
> Sequence: Prompt 2 of 2 — run AFTER CC_BUILD_PROMPT_firmware_ux_r128_REVISED.md
>           is MERGED + FLASHED + QA PASSED (Tests A/B/C/D)
> Prerequisite: R-128, R-129, R-131, R-137 already live in firmware

---

## CC INTRO — PASTE THIS TO CC

```
New session. Ignore all previous context from other projects.

You are working on SATU 1.0 firmware at:
https://github.com/Csmittee/Satu-Vending-Firmware

Before doing anything else, read IN FULL:
1. CLAUDE.md
2. RULES.md       ← R-137 font rule is now live — apply to every screen written here
3. PROJECT_STATE.md
4. config.h
5. hardware.h     ← READ ONLY — DO NOT MODIFY (exception: g_mcp1_ok + g_mcp2_ok flags only)
6. ui.h           ← PRIMARY FILE for this prompt
7. satu_vending.ino
8. state_machine.h

State the name of every file you read before writing a single line.
Then execute: CC_BUILD_PROMPT_service_menu_v1.md
```

---

## CONTEXT

The 5-tab service mode in ui.h has stub bodies only. All 5 tab body functions
(`_drawSvcBody_SelfTest`, `_drawSvcBody_FreePlay`, `_drawSvcBody_Devices`,
`_drawSvcBody_Settings`, `_drawSvcBody_Firmware`) print placeholder text only.

This prompt replaces all 5 stubs with complete implementations.
Touch action handlers in satu_vending.ino get new action codes.

**This prompt touches ui.h, satu_vending.ino, and config.h (SPEAKER_PIN only).**
No changes to hardware.h, network.h, state_machine.h.

Prerequisite state (after firmware_ux_r128_REVISED is merged):
- vendProduct() returns bool, sensor-driven (R-128)
- unlockFlap() / lockFlap() replace unlockDoor() / lockDoor() (R-129)
- RELAY_FLAP replaces RELAY_DOOR_LOCK (R-129)
- showPaymentAccepted() exists in ui.h (R-131)
- idleAnimationUI() uses 16ms millis() loop (R-132)
- FreeSansBold24/18/12pt7b available and proven (R-137)

---

## GLOBAL RULES FOR THIS PROMPT

1. hardware.h is R2 LOCKED — exception: only g_mcp1_ok + g_mcp2_ok bool flags
   added as non-static globals set inside initMCP23017() — nothing else
2. NUM_SLOTS defined in config.h ONLY — never redefine in ui.h
3. No Thai unicode in any gfx->print() — Arduino_GFX renders ASCII only.
   Thai text corrupts silently or crashes. English labels everywhere.
4. R-137 font rule is MANDATORY for every text line written in this prompt.
   See ARCHITECTURE REFERENCE font table. No exceptions.
5. NVS keys must match UI_SPEC.md table exactly — no invented keys
6. SPEAKER_PIN = -1 stub — volume slider renders but tone feedback is skipped
7. Self Test [RUN ALL] = simulated PASS/FAIL — no live relay pulsing (Phase 2)

---

## ARCHITECTURE REFERENCE — SERVICE MODE DESIGN

### Tab layout (5 tabs — vertical left sidebar, 110px wide)
```
Tab 0: Self-Test   ← SELF TEST
Tab 1: Free Play   ← FREE PLAY
Tab 2: Devices     ← DEVICES
Tab 3: Settings    ← SETTINGS
Tab 4: Firmware    ← FIRMWARE
```

Tabs are deliberately kept separate:
- Self Test = automated functional check (Quick + Technical modes)
- Free Play = run full dispense cycle without payment (entire workflow test)
- Devices   = individual component on/off (relay, sensor, pump) for circuit diagnosis
- Settings  = config display + WiFi + factory reset
- Firmware  = device info + serial print

Free Play and Devices serve different prototype testing roles and must NOT be merged.

### Action code table (getTouchedServiceContent() returns these)
Existing codes:
- 301–321 = slot tap (Self-Test tab: motor test, Free Play tab: full vend)
- 401 = factory reset button
- 402 = toggle boot PIN

New codes added by this prompt:
- 500 = [Quick Test] button (Self-Test tab)
- 501 = [Technical Test] button (Self-Test tab)
- 502 = [Clear Results] button (Self-Test tab)
- 600 = [Test Backend] button (Devices tab)
- 601–612 = relay on/off toggle (Devices tab, relays 1–12)
- 620–629 = read sensor live (Devices tab, sensors 1–10)
- 700 = volume slider drag (Settings tab) — stub when SPEAKER_PIN < 0
- 800 = [Print to Serial] button (Firmware tab)

---

## FIX 1 — config.h: SPEAKER_PIN stub

Add to config.h pin constants section:
```cpp
// ── Speaker / Buzzer ──────────────────────────────────────────────────────
// TODO: assign GPIO when speaker is wired. Use ledcAttachPin() + ledcWriteTone().
// NVS key: "vol" (0-100). Stub as -1 until GPIO confirmed.
#define SPEAKER_PIN  -1
```

---

## FIX 2 — hardware.h: g_mcp1_ok and g_mcp2_ok flags

In initMCP23017(), after each mcp.begin_I2C() call:
```cpp
// Add as non-static globals near top of hardware.h (after #include, before functions):
bool g_mcp1_ok = false;
bool g_mcp2_ok = false;

// In initMCP23017():
if (!mcp1.begin_I2C(MCP1_ADDR)) {
  Serial.println("[HW] ERROR: MCP1 (0x20) not found!");
  g_mcp1_ok = false;
} else {
  // ... existing init code ...
  g_mcp1_ok = true;
  Serial.println("[HW] MCP1 OK");
}

if (!mcp2.begin_I2C(MCP2_ADDR)) {
  Serial.println("[HW] ERROR: MCP2 (0x21) not found!");
  g_mcp2_ok = false;
} else {
  // ... existing init code ...
  g_mcp2_ok = true;
  // Ensure flap is LOCKED on boot
  mcp2.digitalWrite(RELAY_MAP[RELAY_FLAP - 1].pin, RELAY_OFF);
  Serial.println("[HW] MCP2 OK — flap LOCKED on boot");
}
```

This is the ONLY hardware.h change permitted in this prompt.

---

## FIX 3 — TAB 0: SELF TEST — two-level test implementation

Replace `_drawSvcBody_SelfTest(int bodyX)` entirely.

### Design
Two test levels shown as two buttons side by side:

```
[▶ Quick Test]    [⚙ Technical Test]    [✕ Clear]

--- results area ---
[PASS] MCP23017 #1 ...
[PASS] MCP23017 #2 ...
...
```

**Quick Test (action 500) — target < 60 seconds:**
Designed for temple staff, morning opening. Tests that all major systems are
functional. On tap: run simulated test sequence with 300ms delay per item to
give visual feedback. Shows 10 key items. Pass/fail based on real boot state
(g_mcp1_ok, g_mcp2_ok, WiFi, heap). All items that cannot be tested without
hardware trigger show "SIM OK" label to make clear it is simulated.

```cpp
// Quick Test items (10 items, target < 60s total)
struct TestItem { const char* label; bool pass; bool simulated; };
TestItem quickTests[] = {
  { "MCP23017 #1 (0x20)",    g_mcp1_ok,                          false },
  { "MCP23017 #2 (0x21)",    g_mcp2_ok,                          false },
  { "IR Sensors 1-10",       g_mcp1_ok && g_mcp2_ok,             true  },
  { "Relay bank 1-6",        g_mcp1_ok,                          true  },
  { "Relay bank 7-12",       g_mcp2_ok,                          true  },
  { "Flap lock R12",         g_mcp2_ok,                          true  },
  { "Water pump R11",        g_mcp2_ok,                          true  },
  { "WS2812B LEDs",          true,                               true  },
  { "WiFi connection",       WiFi.status() == WL_CONNECTED,      false },
  { "Free heap >= 100KB",    ESP.getFreeHeap() > 100000,         false },
};
// Display each item with [PASS]/[FAIL] prefix + SIM label if simulated
// 300ms delay between items for visual progress effect
```

**Technical Test (action 501) — no time limit:**
Designed for technician. Runs all 14 items. Same simulated approach but with
more detail per item — includes I2C address, heap bytes, RSSI value shown inline.

```cpp
// Technical Test items (14 items)
TestItem techTests[] = {
  { "MCP23017 #1 (0x20) — I2C",    g_mcp1_ok,                         false },
  { "MCP23017 #2 (0x21) — I2C",    g_mcp2_ok,                         false },
  { "IR Sensors 1-8",               g_mcp1_ok,                         true  },
  { "IR Sensors 9-10",              g_mcp2_ok,                         true  },
  { "Relay bank 1-6",               g_mcp1_ok,                         true  },
  { "Relay bank 7-10",              g_mcp2_ok,                         true  },
  { "Flap lock R12",                g_mcp2_ok,                         true  },
  { "Water pump R11",               g_mcp2_ok,                         true  },
  { "WS2812B LEDs",                 true,                              true  },
  { "WiFi connection",              WiFi.status() == WL_CONNECTED,     false },
  { "Backend reachable",            true,                              true  },
  { "NVS device_id set",            strlen(g_device_id) > 0,           false },
  { "NVS dev_secret set",           true,                              false },
  { "Free heap >= 100KB",           ESP.getFreeHeap() > 100000,        false },
};
// 50ms delay between items — faster than Quick, more items shown
```

**Display:**
- PASS line: green `[PASS]` prefix + label. If simulated: append ` (sim)` in dim grey.
- FAIL line: red `[FAIL]` prefix + label + `← CHECK` in orange.
- Results scroll if they exceed screen height (use ry bounds check).
- Static variable `s_testMode` (0=none, 1=quick, 2=tech) persists across redraws
  so results stay visible when switching from another tab and back.

**System info rows (always visible above buttons):**
```
Device ID  : SATU-4R473R
Backend    : https://api.janishammer.com
WiFi SSID  : [ssid]
IP Address : [ip]
Free Heap  : [N] B
Uptime     : [N]s
Heartbeat  : [N]s ago
Firmware   : v1.0.0-rX
```
Use R-137 font: labels = NULL size 1, values = NULL size 1 (body text, no FreeSans needed here).

---

## FIX 4 — TAB 1: FREE PLAY — full dispense flow per slot

Replace `_drawSvcBody_FreePlay(int bodyX)` entirely.

Purpose: tap a slot → run entire dispense flow (motor + flap) without payment.
Used during prototype build to verify end-to-end dispatch on each lane.

```
[⚡ FREE PLAY — tap slot to vend without payment]
[warning: ensure lanes loaded before testing]

[ 1  ] [ 2  ] [ 3  ] [ 4  ] [ 5  ]
[name] [name] [name] [name] [name]
[price][price][price][price][price]

[ 6  ] [ 7  ] [ 8  ] [ 9  ] [10  ]
[name] [name] [name] [name] [name]
[price][price][price][price][price]

[log area at bottom via svcLog()]
```

Slot cell colors:
- Green (`color565(20,80,20)`)  = enabled + configured (has items)
- Red   (`color565(120,20,20)`) = enabled but not configured (empty/unconfigured)
- Grey  (C_DARKGREY)            = disabled

Each cell shows: slot number (FreeSansBold12pt7b size 1), name_en (NULL size 1,
truncated to 8 chars), price in B (NULL size 1, gold color).

Touch: returns 300+slot+1 from getTouchedServiceContent(). Existing handler in
satu_vending.ino already calls vendProduct(slot) for Free Play tab. Confirm this
still works with new bool vendProduct(). If vendProduct() returns false → svcLog
"Slot N EMPTY — check lane". If true → svcLog "Slot N dispensed".

---

## FIX 5 — TAB 2: DEVICES — individual component control

Replace `_drawSvcBody_Devices(int bodyX)` entirely.

Purpose: tap individual relay to toggle ON/OFF, read individual sensor state live.
Used during prototype wiring to verify each circuit in isolation.

```
[⚙ MANUAL DEVICE CONTROL]
[Toggle any device independently. GREEN = ON. Diagnosis only.]

RELAY BANK (10 LANES + PUMP + FLAP)
[ R1    ] [ R2    ] [ R3    ] [ R4    ] [ R5    ] [ R6    ]
[ Lane1 ] [ Lane2 ] [ Lane3 ] [ Lane4 ] [ Lane5 ] [ Lane6 ]
[ OFF   ] [ OFF   ] [ OFF   ] [ OFF   ] [ OFF   ] [ OFF   ]

[ R7    ] [ R8    ] [ R9    ] [ R10   ] [ R11   ] [ R12   ]
[ Lane7 ] [ Lane8 ] [ Lane9 ] [Lane10 ] [ Pump  ] [ Flap  ]
[ OFF   ] [ OFF   ] [ OFF   ] [ OFF   ] [ OFF   ] [LOCKED ]

IR SENSORS (READ-ONLY LIVE)
[ IR1   ] [ IR2   ] [ IR3   ] [ IR4   ] [ IR5   ]
[ CLEAR ] [ CLEAR ] [BLOCK  ] [ CLEAR ] [ CLEAR ]

[ IR6   ] [ IR7   ] [ IR8   ] [ IR9   ] [IR10   ]
[ CLEAR ] [ CLEAR ] [ CLEAR ] [ CLEAR ] [BLOCK  ]

[Test Backend]
```

Relay cells: dark background = OFF, green = ON.
R12 (RELAY_FLAP) special label: show "LOCKED" when OFF (pin extended), "UNLOCKED" when ON (pin retracted).
This is critical so technician understands the inverted logic.

IR sensor cells: read live via readSensor(). Dark = CLEAR, red = BLOCKED.
Refresh on each drawServiceScreen() call (not real-time polling — refresh on tab tap).

Touch: relay toggle → return 601+relay-1 (601=R1, 612=R12).
In satu_vending.ino action handler:
```cpp
} else if (action >= 601 && action <= 612) {
  int relayNum = action - 600;
  // Toggle relay state
  static bool relayState[13] = {false}; // 1-indexed
  relayState[relayNum] = !relayState[relayNum];
  setRelay(relayNum, relayState[relayNum]);
  svcLog("R" + String(relayNum) + (relayState[relayNum] ? " ON" : " OFF"));
  drawServiceScreen(TAB_DEVICES); // redraw to show new state
}
```

[Test Backend] button → action 600:
```cpp
} else if (action == 600) {
  svcLog("Testing backend...");
  bool ok = sendHeartbeat();
  svcLog(ok ? "Backend OK" : "Backend UNREACHABLE");
}
```

---

## FIX 6 — TAB 3: SETTINGS — config display + actions

Replace `_drawSvcBody_Settings(int bodyX)` entirely.

```
NETWORK
WiFi SSID  : [value editable field — future]
WiFi Pass  : ••••••••

SERVICE ACCESS
Service PIN: [4 digits — future]
Require PIN on entry: [ON/OFF toggle]  ← action 402

DISPLAY
Idle timeout   : [N]s (read-only — set by backend /hello)
Select timeout : [N]s (read-only)
Sacred water   : ON/OFF (read-only)
Language       : EN/TH (read-only — toggle via bottom-left corner tap)

LANE PRICES  (read-only — edit via dashboard — R-79)
Lane 1: [N]B   Lane 2: [N]B   Lane 3: [N]B   Lane 4: [N]B   Lane 5: [N]B
Lane 6: [N]B   Lane 7: [N]B   Lane 8: [N]B   Lane 9: [N]B  Lane 10: [N]B

AUDIO
Volume: [N]%   [slider — stub when SPEAKER_PIN=-1]

[Factory Reset ← RED]          [Boot PIN: ON/OFF ← action 402]
```

Read-only rows use NULL font size 1 (body text per R-137). Labels in grey, values in white.

Volume slider (action 700):
- When SPEAKER_PIN < 0: show "Volume: N% [TODO: assign SPEAKER_PIN in config.h]" in dim grey.
- When SPEAKER_PIN >= 0: draw track + knob, save to NVS "vol" on drag, ledcWriteTone() feedback.
- NVS read: `prefs.getInt("vol", 50)` — save even when pin not assigned.

Factory Reset (action 401) MUST gate behind PIN confirm overlay:
1. Show overlay: "Enter PIN to confirm Factory Reset"
2. Draw numpad (reuse drawPinOverlay() pattern)
3. On correct PIN: check WiFi connected first
4. If offline: svcLog "Connect to internet to reset" — abort
5. If online: call factoryResetBackend() — only wipe NVS on HTTP 200
6. This enforces R-74 and R-136. Never call factoryResetBackend() on single tap.

Boot PIN toggle (action 402): flip NVS "boot_pin" bool, svcLog result, redraw.

---

## FIX 7 — TAB 4: FIRMWARE — device info + serial print

Replace `_drawSvcBody_Firmware(int bodyX)` entirely.

```
CURRENT FIRMWARE
Version     : v1.0.0-rX
Build date  : [__DATE__ __TIME__]
Board       : ESP32-8048S070C
Flash       : 16 MB
PSRAM       : 8 MB OPI
MAC address : XX:XX:XX:XX:XX:XX
Free heap   : [N] KB
Free PSRAM  : [N] KB

SECURITY (dev mode)
Flash Encryption : DISABLED  ← amber warning
Secure Boot V2   : DISABLED  ← amber warning
JTAG             : ENABLED   [burn eFuse before production] ← amber

REMOTE OTA UPDATE
[🔄 Check Update]  → logs "OTA not implemented"
[⚡ Force OTA]     → logs "OTA not implemented"

[Print to Serial ← action 800]
```

R-137 font: section titles use FreeSansBold12pt7b. Row labels = NULL size 1.
Security warning rows: label in grey, value in `color565(180,120,0)` amber.

MAC address: use `esp_read_mac(mac, ESP_MAC_WIFI_STA)` — snprintf to XX:XX:XX format.

[Print to Serial] action 800:
```cpp
} else if (action == 800) {
  extern char g_device_id[];
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  Serial.println("[SVC] ===== DEVICE INFO =====");
  Serial.printf("[SVC] Device ID : %s\n", g_device_id);
  Serial.printf("[SVC] MAC       : %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  Serial.printf("[SVC] Firmware  : %s  Build: %s %s\n",
                FW_VERSION, __DATE__, __TIME__);
  Serial.printf("[SVC] Free heap : %lu B  PSRAM: %lu B\n",
                ESP.getFreeHeap(), ESP.getFreePsram());
  Serial.println("[SVC] ======================");
  svcLog("Printed to serial");
}
```

---

## TOUCH HANDLER ADDITIONS — getTouchedServiceContent()

Add to existing function, following same coordinate pattern as 401/402:

| Code | Tab | Region | Action |
|------|-----|--------|--------|
| 500 | TAB_SELFTEST | [Quick Test] button | Set s_testMode=1, redraw |
| 501 | TAB_SELFTEST | [Technical Test] button | Set s_testMode=2, redraw |
| 502 | TAB_SELFTEST | [Clear] button | Set s_testMode=0, redraw |
| 600 | TAB_DEVICES  | [Test Backend] button | sendHeartbeat(), svcLog |
| 601–612 | TAB_DEVICES | Relay cell grid | Toggle relay, svcLog, redraw |
| 700 | TAB_SETTINGS | Volume slider track | Map tx→vol, NVS save, tone if pin≥0 |
| 800 | TAB_FIRMWARE | [Print to Serial] button | Serial.printf device info |

Compute each button's screen coordinates from the draw functions above.
Use same debounce pattern (static _lastSvcCntMs, 200ms minimum between actions).

---

## DO NOT TOUCH

- hardware.h — R2 LOCKED. Only g_mcp1_ok + g_mcp2_ok bool flags permitted (FIX 2).
- network.h — read only
- state_machine.h — do not touch
- config.h — only SPEAKER_PIN addition (FIX 1)
- NUM_SLOTS — config.h only. Never redefine.
- idleAnimation() — hardware.h only. Never touch from ui.h.
- NVS keys — only from UI_SPEC.md table. No new keys beyond "vol".
- PAYMENT_MODE — stays fake.

---

## PRE-FLIGHT CHECKLIST (CC confirms before declaring done)

- [ ] All 5 tab bodies fully implemented — no stubs remaining
- [ ] Self Test has two buttons: Quick (10 items <60s) and Technical (14 items)
- [ ] Free Play shows slot grid with 3 colors + name + price
- [ ] Devices shows relay grid (12 relays) + IR sensors (10) + [Test Backend]
- [ ] R12 relay cell labeled LOCKED/UNLOCKED (not ON/OFF) — inverted polarity noted
- [ ] Settings shows lane prices read-only, volume stub, factory reset PIN-gated
- [ ] Firmware shows MAC, heap, security status in amber, [Print to Serial]
- [ ] All new action codes (500-502, 600-612, 700, 800) in getTouchedServiceContent()
- [ ] R-137 font rule applied to every new text line — no NULL+setTextSize>1 on Latin
- [ ] setFont(NULL) reset after every FreeSans block
- [ ] No Thai unicode in any gfx->print() string
- [ ] SPEAKER_PIN stub renders correctly when = -1
- [ ] g_mcp1_ok + g_mcp2_ok declared in hardware.h as non-static globals
- [ ] factory reset (401) gated by PIN confirm + WiFi check + HTTP 200 only
- [ ] GitHub Actions compile GREEN before merge

---

## VERIFICATION STEPS FOR OWNER (after flash)

**Check A — Self Test tab:**
1. Enter service mode (3× tap top-right corner)
2. Tab 0 shows 8 system info rows — Device ID, IP, SSID, heap, uptime visible
3. Tap [Quick Test] → 10 items appear one by one, MCP1/MCP2 show real PASS/FAIL
4. Tap [Technical Test] → 14 items, more detail per line
5. Tap [Clear] → results disappear, buttons reset

**Check B — Free Play tab:**
1. Tab 1 shows slot grid with correct 3 colors
2. Tap an enabled slot → motor spins → IR sensor stops it (R-128 confirmed)
3. svcLog shows "Slot N dispensed" or "Slot N EMPTY"
4. Expected serial during vend:
   ```
   [HW] Relay N ON — motor SPINNING + flap UNLOCKED
   [HW] Sensor N TRIGGERED — item detected after Xms
   [HW] Relay N OFF — motor stopped (sensor)
   [HW] Flap re-locked via TIMEOUT (3000ms)   ← until proximity wired
   [HW] Flap LOCKED — pin extended
   ```

**Check C — Devices tab:**
1. Tab 2 shows 12 relay cells + 10 sensor cells
2. Tap R1 → relay fires → cell turns green → tap again → OFF
3. R12 cell shows "LOCKED" when OFF, "UNLOCKED" when ON
4. Sensor cells show CLEAR/BLOCK based on live IR read
5. Tap [Test Backend] → svcLog "Backend OK"

**Check D — Settings tab:**
1. Tab 3 shows idle/select timeout values from g_cfg_idle, g_cfg_sel
2. Lane prices visible (0 B until backend populates)
3. Volume shows "Volume: 50% [TODO: assign SPEAKER_PIN in config.h]"
4. Tapping [Factory Reset] shows PIN confirm prompt, does NOT reset immediately

**Check E — Firmware tab:**
1. Tab 4 shows correct MAC (verify against Arduino IDE serial output)
2. Security rows show DISABLED in amber
3. Tap [Print to Serial] → serial monitor shows device info block

---

## RULES TO ADD (append to RULES.md at TOP)

```
R-140: Service mode Devices tab (Tab 2) shows relay R12 as LOCKED/UNLOCKED
       not ON/OFF, because R12 polarity is inverted vs motor relays.
       LOCKED = relay OFF (pin extended). UNLOCKED = relay ON (pin retracted).
       This prevents technician confusion when testing flap circuit. (2026-06-17)

R-139: Self Test has two modes: Quick (10 items, <60s, staff-facing) and
       Technical (14 items, no time limit, technician-facing).
       Both are simulated PASS/FAIL in Phase 1. Simulated items labeled (sim).
       Real hardware pulse test (relay fires + sensor reads) deferred to Phase 2.
       Free Play tab and Devices tab are the manual hardware test tools for Phase 1. (2026-06-17)

R-138: Service mode action codes reserved: 500-502 Self Test, 600-612 Devices,
       700 Volume slider, 800 Print to Serial. Never reuse for other actions. (2026-06-17)
```

---

## MANDATORY END OF SESSION (R-84)

1. Archive prompt → `docs/prompts/` stamped:
   `✅ COMPLETE — 2026-06-17 — service mode 5 tabs — R-138 R-139 R-140`

2. Append R-138, R-139, R-140 to RULES.md at TOP

3. Update PROJECT_STATE.md — session log + mark service mode COMPLETE

4. Update KNOWN_GOOD.md at TOP:
   `2026-06-17 — Service mode 5 tabs — Self Test / Free Play / Devices / Settings / Firmware`

5. Commit: `feat: service mode 5 tabs complete — R-138 R-139 R-140`

6. Merge to main after GitHub Actions compile GREEN

**CHAT_HANDOFF.md is Chat's responsibility — CC must never write it.**

---

## PAYMENT MODE REMINDER
PAYMENT_MODE stays fake. This prompt makes zero backend changes.
Never suggest changing to live.

---
*Prompt 2 of 2 — run AFTER CC_BUILD_PROMPT_firmware_ux_r128_REVISED.md merged + flashed + QA passed*
*Next after this: service mode Phase 2 — real hardware self test (live relay + sensor read)*
