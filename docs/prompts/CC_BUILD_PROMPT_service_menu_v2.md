# CC_BUILD_PROMPT_service_menu_v2.md
> ✅ COMPLETE — 2026-06-19 — service mode 5 tabs functional — R-154 R-155 R-156
> Created by: Chat (Claude) — 2026-06-19 (REVISED from v1 — 2026-06-17)
> Session goal: Complete all 5 service mode tabs in ui.h — functional only
> Repo: Satu-Vending-Firmware
> Mode: Firmware Mode — ui.h primary, satu_vending.ino action handlers, config.h minor
> Flash cycles: 1 expected
> PR target: main
> Sequence: Run AFTER R9 (PR #37) merged + flashed + QA PASSED ✅ CONFIRMED 2026-06-19
> Prerequisite: R-128 through R-153 already live in firmware

---

## CC INTRO — PASTE THIS TO CC

```
New session. Ignore all previous context from other projects.

You are working on SATU 1.0 firmware at:
https://github.com/Csmittee/Satu-Vending-Firmware

Before doing anything else, read IN FULL:
1. CLAUDE.md
2. RULES.md
3. CC_SKILL.md
4. PROJECT_STATE.md
5. config.h            ← NOW TRACKED IN REPO (R9) — download from GitHub
6. hardware.h          ← READ ONLY — R2 LOCKED (exception: g_mcp1_ok/g_mcp2_ok flags only)
7. ui.h           ← include target — add #include "ui_service.h" at bottom
8. ui_service.h   ← PRIMARY FILE — create new, include guard required
9. satu_vending.ino
10. state_machine.h

State the name of every file you read before writing a single line.
Then execute: CC_BUILD_PROMPT_service_menu_v2.md
```

---

## CONTEXT

The 5-tab service mode in ui.h has stub bodies only. All 5 tab body functions
(`_drawSvcBody_SelfTest`, `_drawSvcBody_FreePlay`, `_drawSvcBody_Devices`,
`_drawSvcBody_Settings`, `_drawSvcBody_Firmware`) print placeholder text only.

This prompt replaces all 5 stubs with complete functional implementations.
Touch action handlers in satu_vending.ino get new action codes.

**This prompt touches ui_service.h (NEW FILE), ui.h (add include only), 
satu_vending.ino, and config.h (SPEAKER_PIN only if missing).**

**Scope: Functional only. No login screen. No Thai language. No fancy graphics.
English labels, working buttons, real hardware interaction. That is all.**

Current firmware state (confirmed QA passed 2026-06-19 on SATU-4R473R):
- vendProduct() returns bool, sensor-driven (R-128) ✅
- unlockFlap() / lockFlap() exist in hardware.h (R-129) ✅
- RELAY_FLAP = 12 in config.h (R-129) ✅
- showPaymentAccepted() exists in ui.h (R-131) ✅
- idleAnimationUI() uses 16ms millis() loop (R-132) ✅
- FreeSansBold24/18/12pt7b available and proven (R-137) ✅
- STATE_CONFIRMING in state_machine.h (R-153) ✅
- config.h is tracked in repo — CC edits it directly (R-86 updated R9) ✅
- PRODUCT_SELECTION_TIMEOUT = 15 already in config.h (R-152) ✅
- SPEAKER_PIN = -1 already in config.h (R-134) ✅ — verify before adding

---

## GLOBAL RULES FOR THIS PROMPT

1. hardware.h is R2 LOCKED — only permitted exception: g_mcp1_ok + g_mcp2_ok bool
   flags added as non-static globals set inside initMCP23017() — nothing else
2. NUM_SLOTS defined in config.h ONLY — never redefine in ui.h
3. No Thai unicode in any gfx->print() — Arduino_GFX renders ASCII only.
   Thai text corrupts silently or crashes. English labels everywhere.
4. R-137 font rule is MANDATORY for every text line written in this prompt:
   - FreeSansBold24pt7b = hero numbers only
   - FreeSansBold18pt7b = screen titles
   - FreeSansBold12pt7b = section headings
   - NULL size 1 = body text, labels, values
   - setFont(NULL) MUST be called after every FreeSans block
5. NVS keys must match UI_SPEC.md table exactly — no invented keys
6. SPEAKER_PIN = -1 stub — volume slider renders but tone feedback is skipped
7. Self Test [RUN ALL] = simulated PASS/FAIL — no live relay pulsing (Phase 2)
8. Before adding any #define to config.h — verify it is not already present

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

### Action code table (getTouchedServiceContent() returns these)

Existing codes (already live — DO NOT reuse):
- 301–321 = slot tap (Self-Test tab: motor test, Free Play tab: full vend)
- 401 = factory reset button
- 402 = toggle boot PIN

New codes added by this prompt:
- 500 = [Quick Test] button (Self-Test tab)
- 501 = [Technical Test] button (Self-Test tab)
- 502 = [Clear Results] button (Self-Test tab)
- 600 = [Test Backend] button (Devices tab)
- 601–612 = relay on/off toggle (Devices tab, relays 1–12)
- 700 = volume slider drag (Settings tab) — stub when SPEAKER_PIN < 0
- 800 = [Print to Serial] button (Firmware tab)

Rule numbers for these codes: R-154, R-155, R-156 (see RULES TO ADD below).

---

## FIX 1 — hardware.h: g_mcp1_ok and g_mcp2_ok flags

This is the ONLY hardware.h change permitted in this prompt.

Add as non-static globals near top of hardware.h (after #include, before functions):
```cpp
bool g_mcp1_ok = false;
bool g_mcp2_ok = false;
```

In initMCP23017(), set these after each begin_I2C() result:
```cpp
if (!mcp1.begin_I2C(MCP1_ADDR)) {
  Serial.println("[HW] ERROR: MCP1 (0x20) not found! Check wiring.");
  g_mcp1_ok = false;
} else {
  // ... existing init code ...
  g_mcp1_ok = true;
}

if (!mcp2.begin_I2C(MCP2_ADDR)) {
  Serial.println("[HW] ERROR: MCP2 (0x21) not found! Check wiring.");
  g_mcp2_ok = false;
} else {
  // ... existing init code ...
  g_mcp2_ok = true;
  mcp2.digitalWrite(RELAY_MAP[RELAY_FLAP - 1].pin, RELAY_OFF); // fail-secure lock on boot
  Serial.println("[HW] MCP2 OK - flap LOCKED on boot");
}
```

Note: Serial output from current firmware already shows this behavior —
the bool flags just expose it to service mode UI.

---

## FIX 2 — TAB 0: SELF TEST

Replace `_drawSvcBody_SelfTest(int bodyX)` entirely.

### Design: two buttons + results area

```
[▶ Quick Test]    [⚙ Technical Test]    [✕ Clear]

──── results area ────────────────────────────────
[PASS] MCP23017 #1 (0x20)
[PASS] MCP23017 #2 (0x21)
[PASS/SIM] IR Sensors 1-10
...
```

**Quick Test (action 500) — target < 60 seconds:**
10 items. Real values where available (MCP flags, WiFi, heap).
Items that require live hardware interaction labeled "(sim)".

```cpp
struct TestItem { const char* label; bool pass; bool simulated; };
TestItem quickTests[] = {
  { "MCP23017 #1 (0x20)",   g_mcp1_ok,                        false },
  { "MCP23017 #2 (0x21)",   g_mcp2_ok,                        false },
  { "IR Sensors 1-10",      g_mcp1_ok && g_mcp2_ok,           true  },
  { "Relay bank 1-6",       g_mcp1_ok,                        true  },
  { "Relay bank 7-12",      g_mcp2_ok,                        true  },
  { "Flap lock R12",        g_mcp2_ok,                        true  },
  { "Water pump R11",       g_mcp2_ok,                        true  },
  { "WS2812B LEDs",         true,                             true  },
  { "WiFi connection",      WiFi.status() == WL_CONNECTED,    false },
  { "Free heap >= 100KB",   ESP.getFreeHeap() > 100000,       false },
};
```

300ms delay between items for visual feedback. Show [PASS] in green, [FAIL] in red.
Add "(sim)" suffix in grey when simulated=true.

**Technical Test (action 501) — no time limit:**
14 items. Same approach, more detail per item — include I2C addresses,
heap bytes, RSSI value shown inline.

```cpp
TestItem techTests[] = {
  { "MCP23017 #1 (0x20) I2C",    g_mcp1_ok,                       false },
  { "MCP23017 #2 (0x21) I2C",    g_mcp2_ok,                       false },
  { "IR Sensors 1-8 (MCP1)",     g_mcp1_ok,                       true  },
  { "IR Sensors 9-10 (MCP2)",    g_mcp2_ok,                       true  },
  { "Relay bank 1-6 (MCP1)",     g_mcp1_ok,                       true  },
  { "Relay bank 7-11 (MCP2)",    g_mcp2_ok,                       true  },
  { "Flap lock R12 (MCP2)",      g_mcp2_ok,                       true  },
  { "WS2812B LEDs (GPIO5)",      true,                            true  },
  { "Display 800x480 (EK9716)",  true,                            true  },
  { "GT911 Touch",               true,                            true  },
  { "NVS read/write",            true,                            true  },
  { "WiFi connection",           WiFi.status() == WL_CONNECTED,   false },
  { "Backend heartbeat",         false,                           false }, // live sendHeartbeat()
  { "Free heap >= 100KB",        ESP.getFreeHeap() > 100000,      false },
};
```

For item "Backend heartbeat": call sendHeartbeat() live — real pass/fail.

**Clear (action 502):** Reset s_testMode=0, clear results area, redraw.

---

## FIX 3 — TAB 1: FREE PLAY

Replace `_drawSvcBody_FreePlay(int bodyX)` entirely.

Show slot grid — same layout as idle screen (respects g_grid_cols / g_grid_rows).
Each slot cell shows: slot name + price. Color coding:
- Gold = enabled slot with item
- Dim grey = empty slot
- Dark red = disabled slot

Tap a slot cell → returns 301 + slotIdx (existing code range — already handled).

Existing satu_vending.ino handler for 301–321 fires motor for that slot directly
(no payment, no order — pure hardware test). CC must verify this handler exists
and calls vendProduct(slotIdx) directly. If it does not exist yet, add it.

Add svcLog line: "Free play: slot N - [OK/EMPTY]" based on vendProduct() return value.

Expected serial during free play vend (for owner QA):
```
[HW] Relay N ON - motor SPINNING + flap UNLOCKED
[HW] sensor_triggered command received - stopping motor after ~XXXms
[HW] Relay N OFF - motor stopped (sensor)
[HW] Flap re-locked via TIMEOUT (3000ms) - proximity not wired or stuck
[HW] Flap LOCKED - pin extended
```

---

## FIX 4 — TAB 2: DEVICES

Replace `_drawSvcBody_Devices(int bodyX)` entirely.

### Section 1: RELAY GRID (12 relays, 2 rows × 6 cols)

Each cell shows relay number + state.
- R1–R10: labeled with lane name from g_slots[] if available, else "R1"..."R10"
- R11: labeled "PUMP"
- R12: labeled "FLAP" — special: shows "LOCKED" when OFF, "UNLOCKED" when ON

Cell colors:
- OFF state: dark fill, dim text
- ON state: green fill, white text
- R12 OFF (LOCKED): red fill — makes the danger state obvious

Touch: relay toggle → return 601 + (relayNum - 1)
(601=R1, 602=R2, ... 612=R12)

In satu_vending.ino action handler:
```cpp
} else if (action >= 601 && action <= 612) {
  int relayNum = action - 600;
  static bool relayState[13] = {false}; // 1-indexed
  relayState[relayNum] = !relayState[relayNum];
  setRelay(relayNum, relayState[relayNum]);
  svcLog("R" + String(relayNum) + (relayState[relayNum] ? " ON" : " OFF"));
  drawServiceScreen(TAB_DEVICES);
}
```

WARNING banner below relay grid in amber:
"WARNING: Relays stay ON until tapped again. Exit service mode to reset all."

### Section 2: IR SENSORS (10 sensors, read-only)

10 sensor cells in a row. Read live via readSensor() on tab draw.
- CLEAR = dark fill + "CLEAR" label
- BLOCKED = red fill + "BLOCK" label

Refresh on each drawServiceScreen(TAB_DEVICES) call. Not real-time polling.

### Section 3: TEST BACKEND button (action 600)

Single button [Test Backend].
In satu_vending.ino:
```cpp
} else if (action == 600) {
  svcLog("Testing backend...");
  bool ok = sendHeartbeat();
  svcLog(ok ? "Backend OK" : "Backend UNREACHABLE");
}
```

---

## FIX 5 — TAB 3: SETTINGS

Replace `_drawSvcBody_Settings(int bodyX)` entirely.

All fields read-only except factory reset and boot PIN toggle.
No editable text fields in this version — functional display only.

### Layout:

```
NETWORK
  WiFi SSID  : [nvs_ssid value]
  IP Address : [WiFi.localIP().toString()]

DISPLAY CONFIG  (read-only — set by backend /hello)
  Idle timeout   : [g_cfg_idle]s
  Select timeout : [g_cfg_sel]s
  Sacred water   : [ON/OFF]
  Grid layout    : [g_grid_rows]x[g_grid_cols]

SERVICE ACCESS
  [Boot PIN: ON / OFF]  ← action 402, toggle NVS "boot_pin"

LANE PRICES  (read-only — edit via dashboard)
  Lane 1: [N]B  Lane 2: [N]B  ... Lane 10: [N]B

AUDIO
  Volume: [N]%  [stub text if SPEAKER_PIN < 0]

[Factory Reset]  ← RED button — gated by PIN confirm overlay
```

Read-only rows: NULL font size 1. Labels in grey (`color565(120,120,120)`), values in white.
Section headings: FreeSansBold12pt7b.

Volume (action 700):
- SPEAKER_PIN < 0: show "Volume: N% — assign SPEAKER_PIN in config.h" in dim grey.
  NVS read/write still works: `prefs.getInt("vol", 50)` / `prefs.putInt("vol", val)`.
- SPEAKER_PIN >= 0: draw slider track + knob, ledcWriteTone() feedback on drag.

Factory Reset (action 401) — MUST gate behind PIN confirm overlay:
1. Show overlay: "Enter PIN to confirm Factory Reset"
2. Reuse drawPinOverlay() pattern already in ui.h
3. On correct PIN: check WiFi connected first
4. If offline: svcLog "No network - connect first" — abort
5. If online: call factoryResetBackend() — only wipe NVS on HTTP 200
6. This enforces R-74 and R-136.

Boot PIN toggle (action 402):
Flip NVS "boot_pin" bool. svcLog "Boot PIN: ON" / "Boot PIN: OFF". Redraw button.

---

## FIX 6 — TAB 4: FIRMWARE

Replace `_drawSvcBody_Firmware(int bodyX)` entirely.

### Layout:

```
CURRENT FIRMWARE
  Version     : v1.0.0-r9        ← FW_VERSION from network.h
  Build date  : Jun 19 2026      ← __DATE__ __TIME__
  Board       : ESP32-8048S070C
  Flash       : 16 MB
  PSRAM       : 8 MB OPI
  MAC address : XX:XX:XX:XX:XX:XX
  Free heap   : N KB
  Free PSRAM  : N KB

SECURITY (dev mode)
  Flash Encrypt : DISABLED        ← amber color565(180,120,0)
  Secure Boot   : DISABLED        ← amber
  JTAG          : ENABLED         ← amber + "burn eFuse before production"

REMOTE OTA (stub)
  [Check Update]  → svcLog "OTA not implemented"
  [Force OTA]     → svcLog "OTA not implemented"

[Print to Serial]   ← action 800
```

MAC: use `esp_read_mac(mac, ESP_MAC_WIFI_STA)`.
Section headings: FreeSansBold12pt7b. Row labels: NULL size 1 grey. Values: NULL size 1 white.
Security rows: value in amber `color565(180,120,0)`.

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

Add to existing function, following same coordinate pattern as 401/402.
Use same debounce: static _lastSvcCntMs, 200ms minimum between actions.

| Code | Tab | Region | Action |
|------|-----|--------|--------|
| 500 | TAB_SELFTEST | [Quick Test] button | Set s_testMode=1, run test, redraw |
| 501 | TAB_SELFTEST | [Technical Test] button | Set s_testMode=2, run test, redraw |
| 502 | TAB_SELFTEST | [Clear] button | Set s_testMode=0, clear results, redraw |
| 600 | TAB_DEVICES | [Test Backend] button | sendHeartbeat(), svcLog result |
| 601–612 | TAB_DEVICES | Relay cell grid | Toggle relay, svcLog, redraw |
| 700 | TAB_SETTINGS | Volume slider track | Map tx→vol 0-100, NVS save |
| 800 | TAB_FIRMWARE | [Print to Serial] button | Serial.printf device info block |

Compute button coordinates from the draw functions above.
Do NOT hardcode coordinates here — derive them from the same constants used to draw.

---

## DO NOT TOUCH

- hardware.h — R2 LOCKED. Only g_mcp1_ok + g_mcp2_ok flags permitted (FIX 1).
- network.h — read only
- state_machine.h — do not touch (STATE_CONFIRMING already added by R9)
- config.h — verify SPEAKER_PIN present before adding. No other additions.
- NUM_SLOTS — config.h only. Never redefine.
- ui_service.h include guard: #ifndef UI_SERVICE_H / #define UI_SERVICE_H / #endif
- idleAnimation() — hardware.h only. Never touch from ui.h.
- NVS keys — only from UI_SPEC.md approved table. "vol" is the only new key.
- PAYMENT_MODE — stays fake.
- Login screen — NOT this session.
- Thai language — NOT this session.
- Fancy graphics / animations — NOT this session.

---

## PRE-FLIGHT CHECKLIST (CC confirms before declaring done)

- [ ] All 5 tab bodies fully implemented — no stubs remaining
- [ ] Self Test: Quick (10 items, real MCP/WiFi/heap) + Technical (14 items, live heartbeat)
- [ ] Free Play: slot grid with name+price, tap fires motor, svcLog result
- [ ] Devices: 12 relay cells + 10 IR sensor cells + [Test Backend] button
- [ ] R12 relay cell: LOCKED (red) / UNLOCKED (green) — not ON/OFF
- [ ] Settings: read-only display, factory reset PIN-gated, boot PIN toggle, volume stub
- [ ] Firmware: MAC, heap, security amber, [Print to Serial] action 800
- [ ] All new action codes (500-502, 600-612, 700, 800) in getTouchedServiceContent()
- [ ] R-137 font rule applied to every new text line — no NULL+setTextSize>1 on Latin
- [ ] setFont(NULL) reset after every FreeSans block
- [ ] No Thai unicode in any gfx->print() string
- [ ] SPEAKER_PIN stub message renders correctly when = -1
- [ ] g_mcp1_ok + g_mcp2_ok declared in hardware.h as non-static globals
- [ ] Factory reset gated: PIN confirm + WiFi check + HTTP 200 only
- [ ] Free play 301–321 handler confirmed or added in satu_vending.ino
- [ ] SPEAKER_PIN already in config.h — verified before adding
- [ ] GitHub Actions compile GREEN before merge
- [ ] - [ ] ui_service.h has correct include guard — #ifndef UI_SERVICE_H
- [ ] ui.h has #include "ui_service.h" at bottom — only change to ui.h

---

## VERIFICATION STEPS FOR OWNER (after flash)

**Check A — Self Test tab:**
1. Enter service mode (3× tap top-right corner)
2. Tap [Quick Test] → 10 items appear one by one
3. MCP1/MCP2: real PASS (components arriving — may show FAIL now, expected)
4. WiFi: real PASS if connected
5. Tap [Technical Test] → 14 items, item 13 "Backend heartbeat" calls API live
6. Tap [Clear] → results disappear

**Check B — Free Play tab:**
1. Slot grid shows with name + price from backend
2. Tap slot → motor spins → IR sensor stops it (no payment needed)
3. Serial shows motor ON → sensor TRIGGERED → motor OFF → flap LOCKED

**Check C — Devices tab:**
1. 12 relay cells + 10 IR sensor cells visible
2. Tap R1 → relay fires → cell turns green → tap again → OFF
3. R12 shows "LOCKED" (red) when OFF, "UNLOCKED" (green) when ON
4. Tap [Test Backend] → svcLog "Backend OK" or "Backend UNREACHABLE"
5. IR cells show CLEAR/BLOCK based on live read

**Check D — Settings tab:**
1. WiFi SSID + IP visible
2. Idle/select timeouts from backend visible
3. Volume shows "N% — assign SPEAKER_PIN in config.h" stub
4. Tap [Factory Reset] → PIN overlay appears — does NOT reset immediately
5. Tap [Boot PIN] → toggles ON/OFF → svcLog confirms

**Check E — Firmware tab:**
1. MAC address visible — matches Arduino IDE serial output
2. Security rows show DISABLED in amber
3. Tap [Print to Serial] → serial monitor shows [SVC] device info block

---

## RULES TO ADD (append to RULES.md at TOP — next available numbers after R-153)

```
R-156: Service mode Devices tab shows relay R12 as LOCKED/UNLOCKED not ON/OFF.
       LOCKED = relay OFF (pin extended, fail-secure). UNLOCKED = relay ON (pin retracted).
       Cell color: LOCKED=red, UNLOCKED=green. Prevents technician confusion. (2026-06-19)

R-155: Self Test has two modes: Quick (10 items, real MCP/WiFi/heap values, simulated
       hardware items labeled "(sim)") and Technical (14 items, live backend heartbeat).
       Phase 1 = simulated for hardware-dependent items. Phase 2 = live relay pulse + sensor read.
       (2026-06-19)

R-154: Service mode action codes reserved: 500-502 Self Test, 600-612 Devices,
       700 Volume slider, 800 Print to Serial. Never reuse for other actions. (2026-06-19)
```

---

## MANDATORY END OF SESSION (R-84)

1. Archive prompt → `docs/prompts/` stamped:
   `✅ COMPLETE — 2026-06-19 — service mode 5 tabs functional — R-154 R-155 R-156`

2. Append R-154, R-155, R-156 to RULES.md at TOP

3. Update PROJECT_STATE.md — session log + mark service mode COMPLETE

4. Update KNOWN_GOOD.md at TOP:
   `2026-06-19 — Service mode 5 tabs functional — Self Test / Free Play / Devices / Settings / Firmware`

5. Update CLAUDE.md — add firmware/ui_service.h to Key Files section

6. Update KNOWLEDGE_MAP.md — add ui_service.h entry under Firmware file locations

7. Update CC_CHAT_LOG.md at TOP with session entry (CC_SKILL.md format)

8. Commit: `feat: service mode 5 tabs functional — R-154 R-155 R-156`

9. Merge to main after GitHub Actions compile GREEN

**CHAT_HANDOFF.md is Chat's responsibility — CC must never write it.**

---

## PAYMENT MODE REMINDER
PAYMENT_MODE stays fake. This prompt makes zero backend changes. Never suggest changing to live.

---

## CHANGES FROM v1 (2026-06-17) — WHAT WAS UPDATED

| Item | v1 | v2 |
|------|----|----|
| Sequence | After R6 | After R9 ✅ |
| Prerequisites | R-128 to R-137 | R-128 to R-153 |
| CC INTRO file list | 8 files, no CC_SKILL | 9 files, includes CC_SKILL.md |
| config.h status | gitignored | Tracked in repo (R9) — CC edits directly |
| SPEAKER_PIN | Add to config.h | Verify first — already added R-134 |
| PRODUCT_SELECTION_TIMEOUT | Not mentioned | Already in config.h (R-152) — do not re-add |
| STATE_CONFIRMING | Not in state machine | Already live (R-153) — do not touch |
| Rules to add | R-138/R-139/R-140 ❌ TAKEN | R-154/R-155/R-156 ✅ correct |
| Session closing | R-138/R-139/R-140 | R-154/R-155/R-156 |
| Scope note | Not stated | Explicit: functional only, no login, no Thai |
| CC_CHAT_LOG step | Missing | Added as step 5 in closing |

---
*Next after this: service mode Phase 2 — real hardware self test (live relay pulse + sensor read)*
*After that: login screen + Thai language (deferred by owner decision 2026-06-19)*
