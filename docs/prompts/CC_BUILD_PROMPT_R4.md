# ✅ COMPLETE — Archived 2026-06-13 — R4 firmware + backend completion (executed prior session)
# SATU — CC BUILD PROMPT R4
# Session: Firmware R4 + Backend Completion
# Created: 2026-05-31
# Read ALL repo files before writing a single line of code
# ============================================================

You are a senior embedded systems and full-stack engineer for the Satu project —
a Thai temple donation vending machine. Solo founder. Real money. Religious institutions.
Security and correctness are non-negotiable.

## MISSION — ONE PASS, NO STUBS EXCEPT OTA

Build firmware R4 and complete backend in one pass.
All files complete and flashable. Hardware.h is UNTOUCHABLE.

## BEFORE YOU WRITE A SINGLE LINE

1. Read hardware.h — you will NOT modify it. It owns: idleAnimation(), mcp2_sensors[2], RELAY_PUMP=11, RELAY_DOOR_LOCK=12
2. Ask user to upload current local files: satu_vending.ino, ui.h, config.h, network.h, state_machine.h
3. DO NOT use project knowledge versions of firmware — local files only
4. Read UI_SPEC.md and SECURITY.md from project knowledge (they are authoritative for R4)
5. Run mental pre-flight after every file: check all 6 rules below

## SIX RULES — NEVER VIOLATE

1. hardware.h: never replace, never modify, never redeclare anything it owns
2. NUM_SLOTS: defined in config.h only — ui.h reads it, never redefines it
3. idleAnimation(): hardware.h owns it (LED). ui.h has idleAnimationUI() (screen). Different functions. satu_vending.ino calls idleAnimation() at boot (LED) and idleAnimationUI() for screen flashes — both exist, neither conflicts.
4. NVS keys: use ONLY keys in the approved NVS table in UI_SPEC.md — no others
5. config.h WiFi credentials: note they are gitignored fallback only. Never hardcode WiFi in network.h.
6. Factory reset: must call /v1/machine/factory-reset backend FIRST, only wipe NVS on 200 OK

## WHAT WORKS NOW — DO NOT BREAK

- WiFi → /hello → slots[] → grid with 3 active products ✅
- Heartbeat HTTP 200 ✅
- Gift option screen (Item Only / +Sacred Water) ✅
- QR placeholder + 120s countdown ✅
- 14/14 backend tests passing ✅
- machine_slots D1 table with 3 test slots for SATU-4R473R ✅

## KNOWN BUG TO FIX IN CURRENT FILES

satu_vending.ino calls idleAnimation() for LED animation (hardware.h — correct)
ui.h defines idleAnimationUI() for screen flash — alias comment exists but no actual alias
Fix: satu_vending.ino should call idleAnimationUI() where screen flash is intended, keep idleAnimation() calls where LED animation is intended. Do not add alias — be explicit.

## LIBRARY — NEW FOR R4

PNGdec by bitbank2 (user installs while CC runs)
Usage: decode PNG bytes from HTTP fetch into RGB565 for gfx->draw16bitRGBBitmap()
Include: #include <PNGdec.h> in network.h
Buffer: allocate in PSRAM with ps_malloc(200*1024) — never in SRAM

---

## FIRMWARE R4 — COMPLETE BUILD LIST

### GRID SYSTEM (replaces hardcoded 5×2)

Backend sends grid_rows (1-3) and grid_cols (1-7) in /hello config{} block.
Firmware reads and caches to NVS: nvs_grow / nvs_gcol.

```cpp
// Global grid config (runtime, not compile-time)
static int g_grid_rows = 2;  // default
static int g_grid_cols = 5;  // default

// Cell size calculation — called after grid config loaded
void recalcGrid() {
  int sidew   = (g_grid_rows >= 3) ? 66 : 0;
  int avail_w = SCR_W - sidew - GRID_PAD*2;
  int avail_h = SCR_H - STATUS_H - GRID_PAD*2;
  CELL_W = (avail_w - GRID_PAD*(g_grid_cols-1)) / g_grid_cols;
  CELL_H = (g_grid_rows >= 3)
           ? (avail_h - GRID_PAD*2)
           : (avail_h - GRID_PAD*(g_grid_rows-1)) / g_grid_rows;
}
```

When rows >= 3: side tab strip 60px wide on left, shows A/B/C tabs.
Active tab shows that row's buttons (full content height, large buttons).
Slot labels: A1-A7, B1-B7, C1-C7. Maps to physical machine shelves.

### A. SETUP CODE SCREEN

Trigger: /hello returns status="pending"
```
drawSetupCodeScreen(String code):
  - Black background
  - SATU logo gold, size 4, centred top third
  - "Activate Your Machine" subtitle, size 2, gold dim
  - Setup code: large, size 6, gold, centred, letter-spaced
  - "Enter this code at dashboard.satu-th.com" — size 1, grey
  - Gold border around screen (3px inset)
  - Auto-retry /hello every 30s — show countdown "Checking in Xs..."
```

State machine: in STATE_STARTUP, if status==pending → call drawSetupCodeScreen()
Poll /hello every 30s, stay in STATE_STARTUP until status==active.

### B. BOOT PIN (BOOT_PIN feature)

On boot, after WiFi connects and /hello returns active:
Read NVS `boot_pin` (bool, default false).
If boot_pin=true: show PIN numpad overlay before drawIdleScreen().
Correct PIN → proceed to idle. Wrong PIN → shake, max 3 attempts → 30s lockout.

### C. SERVICE MODE PIN NUMPAD OVERLAY

Triggered by: 3× tap top-right corner gesture (existing checkServiceGesture())
If SVC_PIN_EN=false: enter service mode directly (no overlay)
If SVC_PIN_EN=true:

```
drawPinOverlay():
  - Semi-transparent dark overlay on top of current screen (fillRect with alpha sim)
  - White box centred: 400×320px
  - Title: "Service Access" — gold
  - 4 digit display: [*][*][ ][ ] — fills as typed
  - 3×4 numpad grid: 1-9, 0, DEL, ENTER
  - Wrong PIN: shake effect (redraw box offset ±5px 3 times), clear digits
  - 3 wrong attempts: "Locked 30s" countdown, no input accepted
  - Correct PIN: overlay dismissed, drawServiceScreen(TAB_SELFTEST)
```

Numpad must use getTouchedXY() not getTouchedSlot() — different touch zone.
Add: int getTouchedNumpad() → returns 0-9, -1=none, 10=DEL, 11=ENTER

### D. SERVICE MODE — FULL 5-TAB SCREEN

Service header (36px):
```
[🔧 SERVICE]  [device_id]  [fw_version]              [✕ EXIT]
```

Tab bar (40px): 5 tabs, 160px each
Active tab: C_GOLD bottom border, bright text
Tabs: SELF TEST | FREE PLAY | DEVICES | SETTINGS | FIRMWARE

Service mode has its own touch handler that reads raw touch coords, not grid-mapped.

#### TAB 1: SELF TEST

Top section — System Info rows (label/value pairs):
Device ID, API URL, WiFi SSID, IP, Free Heap, Uptime, Last Heartbeat, Temp (fake 40C), FW Version

Middle section — Self Test results (auto-runs on tab entry):
14 items: [PASS]/[FAIL] label in green/red, description
Items: MCP1 I2C, MCP2 I2C, IR 1-8, IR 9-10, Relay 1-10 (50ms pulse), R12 door, R11 pump,
       LEDs 40px, Display 800×480, GT911 touch, NVS, WiFi, Backend /health, Temperature

[▶ RUN ALL] button re-runs. [✕ Clear] clears results.

PING button → GET /health → display payment_mode in result area.

Log area: monospace size 1, 8 lines, dark background, auto-scroll.

#### TAB 2: FREE PLAY

Lane motor grid: same R×Out layout as idle screen (respects g_grid_rows/g_grid_cols)
If g_grid_rows >= 3: show A/B/C side tabs (same as idle screen)
Button label: name_en from g_slots[] or "Lane N"
Tap button: fires selected motor mode on that lane

Motor mode selector row:
[● PULSE (800ms)] [HOLD (press+hold)] [SLOW (3× pulse)]
Highlight selected mode button.

Special devices row:
[💧 Water Pump (hold)] — onmousedown/ontouchstart fires pump, release stops
[🚪 Door Lock Toggle] — tap to toggle LOCKED/UNLOCKED state display
[▶ Run All Lanes] — sequential PULSE all active lanes, 1s gap

IR sensor row (live, update every 500ms inside service mode loop):
IR1-IR10: CLEAR (green) / BLOCK (red) indicators, size 1

Free play log: last 5 entries with millis() timestamp, monospace

#### TAB 3: DEVICES

Relay grid (2 rows × 6 cols = 12 relays):
R1-R10 labeled with lane name. R11=PUMP, R12=DOOR
Green fill=ON, Red fill=OFF. Tap to toggle.
⚠ WARNING: "Relays stay ON until tapped again" — orange text banner

IR matrix (same as FREE PLAY IR section):
Live 500ms update.

LED zone control:
Zone row: [TOP] [FLOOR1] [FLOOR2] [DOOR] [ALL]
Colour row: [GOLD] [GREEN] [BLUE] [RED] [WHITE] [OFF]
Brightness: slider 0-255 (implement as touch-drag horizontal bar)
Pattern buttons: [Idle Gold] [Celebrate] [Error Flash]

I2C scanner:
[SCAN BUS] button → Wire scan 0x00-0x7F → list found: 0x20, 0x21, 0x5D expected

#### TAB 4: SETTINGS

All saves go to NVS using exact keys from UI_SPEC.md NVS table.
Use only pre-approved NVS keys — no new ones.

Network section:
WiFi SSID: text input (virtual keyboard — 4 rows A-Z + 0-9 + special chars + DONE)
WiFi Password: same, masked
[Save WiFi + Reboot] button

Security section:
Service PIN: 4-digit numpad input
[●─] toggles: PIN on service entry (svc_pin_en), PIN on startup (boot_pin)

Display section:
Theme picker: [Dark Gold] [Temple Blue] [Classic White]
Language: [EN] [TH] — TH is display-only placeholder in R4, EN always used

Donation features:
[●─] Sacred water (cfg_water), [●─] Lucky number (cfg_lucky)
Idle timeout select: [30s] [60s] [120s] [300s]
Selection timeout select: [5s] [10s] [15s] [30s]

Action buttons:
[💾 Save All to NVS] — green
[⚠ Factory Reset] — red, requires PIN confirmation, calls backend first

Factory reset flow:
1. Show confirmation overlay: "Type PIN to confirm factory reset"
2. Numpad input
3. Correct PIN → POST /v1/machine/factory-reset
4. On HTTP 200: clear NVS, ESP.restart()
5. On failure/offline: show "Connect to internet to reset" — do NOT wipe NVS

Virtual keyboard for WiFi text input:
Draw simple on-screen keyboard (alphanumeric rows)
Backspace + DONE buttons
Result stored in temp string, applied on Save

#### TAB 5: FIRMWARE

Static info display:
FW version, build date, board name, flash size, PSRAM size
MAC: WiFi.macAddress()
Security status: DISABLED for flash enc + secure boot (dev mode text)

OTA stubs:
[🔄 Check Update] → Serial.println("[OTA] Not yet implemented")
[⚡ Force OTA] → same

Debug/recovery:
[Show Debug Info] → draws debug screen: MAC, device_id, setup_code, IP, FW
[Print to Serial] → Serial.printf with all identity info

### E. LANGUAGE SELECTOR

In status bar: small [EN|TH] label, top-right area (tap to toggle)
Stores to NVS `lang`
Global: `static bool g_lang_th = false;`
All label strings: `const char* lbl = g_lang_th ? "ไทย" : "English";`
R4: TH shows EN text (no Thai font). Toggle still works for NVS persistence.

### F. IDLE SCREEN ADDITIONS

After drawing the grid, add at bottom (below last row, above bottom edge):
- "Tap any item to begin" — size 1, C_GOLD, centred
- Pulsing: vary text colour between C_GOLD and C_GOLD_DIM every 1s
- Track idle time: if >30s no touch → call idleAnimationUI()

### G. QR IMAGE LOADING (PNGdec)

```cpp
// In network.h: fetch PNG bytes from URL
bool fetchImageBytes(const String& url, uint8_t* buf, size_t& outLen, size_t maxLen) {
  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }
  WiFiClient* stream = http.getStreamPtr();
  outLen = 0;
  while (http.connected() && outLen < maxLen) {
    int avail = stream->available();
    if (avail > 0) {
      outLen += stream->readBytes(buf + outLen, min(avail, (int)(maxLen - outLen)));
    }
  }
  http.end();
  return outLen > 0;
}
```

```cpp
// In ui.h: PNG decode callback + draw
static Arduino_RGB_Display* _pngGfx;
static int _pngX, _pngY;
void _pngDraw(PNGDRAW* pDraw) { /* draw row to gfx */ }

void drawQrFromBytes(uint8_t* buf, size_t len, int x, int y, int maxW, int maxH) {
  PNG png;
  if (png.openRAM(buf, len, _pngDraw) == PNG_SUCCESS) {
    _pngGfx = gfx; _pngX = x; _pngY = y;
    png.decode(NULL, 0);
    png.close();
  }
}
```

drawQrScreen(): after drawing left panel, call fetchImageBytes(qrUrl, psram_buf, len, 200*1024)
If success: drawQrFromBytes(buf, len, qrAreaX, qrAreaY, 240, 220)
If fail: white box + "Scan with banking app"

PNG buffer: allocate from PSRAM at startup with ps_malloc(200*1024). Never on stack.
Declare: static uint8_t* g_pngBuf = nullptr; — init in initUI() with ps_malloc.

### H. COMPLETION CALL

reportCompletion() already exists in network.h.
Wire in satu_vending.ino _onItemRemoved(): call reportCompletion(currentOrderId, true)
Wire in WAITING_DROP timeout path: call reportCompletion(currentOrderId, false)
Add slot to payload: add `doc["slot"] = selectedSlot + 1;` in reportCompletion()

### I. NVS CONFIG CACHE

On /hello success for active device, read config{} block from response:
```cpp
if (doc.containsKey("config")) {
  JsonObject cfg = doc["config"];
  prefs.begin("satu", false);
  prefs.putInt("cfg_idle",  cfg["idle_timeout"]    | 60);
  prefs.putInt("cfg_sel",   cfg["selection_timeout"]| 15);
  prefs.putBool("cfg_water",cfg["sacred_water"]     | true);
  prefs.putBool("cfg_lucky",cfg["lucky_number"]     | true);
  if (cfg.containsKey("grid_rows")) {
    prefs.putInt("nvs_grow", cfg["grid_rows"] | 2);
    prefs.putInt("nvs_gcol", cfg["grid_cols"] | 5);
  }
  prefs.end();
}
```

On boot before /hello (in initWiFi, after loadCredentialsFromNVS):
Load NVS config into globals: g_grid_rows, g_grid_cols, idle_timeout, etc.
These are used even if /hello fails (cached values).

### J. DEBUG MODE

Gesture: 5-second hold at bottom-left corner (x<80, y>400) — silent during hold
After 5s: drawDebugScreen()
```
drawDebugScreen():
  - Black background
  - "DEBUG MODE" header in red
  - MAC: 3C:DC:75:5D:DD:2C
  - Device ID: SATU-4R473R
  - Setup code: (from NVS if available)
  - FW: v1.0.0-r4
  - IP: 192.168.1.45
  - [✕ CLOSE] button
```
No PIN required — recovery mode.

### K. NUKE COMMAND HANDLER

In handleCommands() in satu_vending.ino, add:
```cpp
else if (cmd == "nuke") {
  Serial.println("[CMD] NUKE received — wiping NVS and rebooting");
  drawErrorScreen("Remote reset initiated");
  delay(2000);
  Preferences prefs;
  prefs.begin("satu", false);
  prefs.clear();
  prefs.end();
  ESP.restart();
}
```

---

## BACKEND R4 — COMPLETE BUILD LIST

### H. /v1/machine/completion endpoint (machine.js + index.js)

```javascript
// machine.js
export async function handleMachineCompletion(request, env) {
  const auth = await authenticateDevice(request, env, body.device_id);
  if (!auth.valid) return Response.json({ error: auth.reason }, { status: 401 });
  const { device_id, order_id, success, slot } = await request.json();
  await env.DB.prepare(
    `UPDATE orders SET status=? WHERE order_id=? AND device_id=?`
  ).bind(success ? 'dispensed' : 'vend_failed', order_id, device_id).run();
  // Log to connection_logs
  await env.DB.prepare(
    `INSERT INTO connection_logs (device_id, event_type, details, timestamp)
     VALUES (?,?,?,?)`
  ).bind(device_id, 'completion',
    JSON.stringify({ success, slot, order_id }), Date.now()).run();
  return Response.json({ status: 'ok', message: 'Completion recorded' });
}
```

Route in index.js: `path === '/v1/machine/completion' && method === 'POST'`

### I. /hello config fields (machine.js)

In handleMachineHello(), for active devices, add config{} to response:
```javascript
config: {
  idle_timeout:       60,
  selection_timeout:  15,
  sacred_water:       true,
  lucky_number:       true,
  grid_rows:          2,
  grid_cols:          5
}
```
Hardcode defaults for now. Future: read from machine_config table per device.

### J. /v1/machine/factory-reset endpoint (machine.js + index.js)

```javascript
export async function handleFactoryReset(request, env) {
  const { device_id } = await request.json();
  const auth = await authenticateDevice(request, env, device_id);
  if (!auth.valid) return Response.json({ error: auth.reason }, { status: 401 });
  // Generate new setup code
  const newCode = Math.floor(100000 + Math.random() * 900000).toString();
  const device = await env.DB.prepare('SELECT mac FROM devices WHERE device_id=?').bind(device_id).first();
  // Clear ownership, generate new setup code, set pending
  await env.DB.prepare(`UPDATE devices SET owner_id=NULL, status='pending' WHERE device_id=?`).bind(device_id).run();
  await env.DB.prepare(`DELETE FROM setup_codes WHERE device_id=?`).bind(device_id).run();
  await env.DB.prepare(`INSERT INTO setup_codes (code, assigned_mac, device_id, used, created_at) VALUES (?,?,?,0,?)`).bind(newCode, device.mac, device_id, Date.now()).run();
  // Log event
  await env.DB.prepare(`INSERT INTO connection_logs (device_id, event_type, details, timestamp) VALUES (?,?,?,?)`).bind(device_id, 'factory_reset', JSON.stringify({ initiated_by: 'machine' }), Date.now()).run();
  return Response.json({ status: 'ok', message: 'Device reset. New setup code generated.' });
}
```

Route in index.js: `path === '/v1/machine/factory-reset' && method === 'POST'`

### K. /v1/admin-data/:table proxy route (index.js)

Add BEFORE JWT auth block:
```javascript
if (path.startsWith('/v1/admin-data/') && method === 'GET') {
  // Accept EITHER X-Admin-Token OR Bearer JWT with role=admin
  const adminToken = request.headers.get('X-Admin-Token');
  const authHeader = request.headers.get('Authorization');
  let authorized = false;
  if (adminToken && adminToken === env.ADMIN_SECRET) authorized = true;
  if (!authorized && authHeader?.startsWith('Bearer ')) {
    const jwt = await authenticateJWT(request, env);
    if (jwt.valid && jwt.role === 'admin') authorized = true;
  }
  if (!authorized) return Response.json({ error: 'Unauthorized' }, { status: 401 });
  const table = path.split('/v1/admin-data/')[1];
  const whitelist = ['users','devices','orders','setup_codes','connection_logs','machine_slots'];
  if (!whitelist.includes(table)) return Response.json({ error: 'Invalid table' }, { status: 400 });
  const result = await env.DB.prepare(`SELECT * FROM ${table} LIMIT 200`).all();
  return Response.json(result.results);
}
```

### L. Fix satu-admin.html apiGet() URL bug

In token mode, apiGet() currently builds wrong URL path.
Fix: in token mode, route through /v1/admin-data/:table
```javascript
async function apiGet(url) {
  const headers = {};
  if (AUTH.mode === 'jwt') {
    headers['Authorization'] = `Bearer ${AUTH.jwt}`;
    const res = await fetch(API + url, { headers });
    if (res.status === 401) { doLogout(); throw new Error('Session expired'); }
    return res;
  }
  if (AUTH.mode === 'token') {
    headers['X-Admin-Token'] = AUTH.adminSecret;
    // Extract table name from /api/TABLE → /v1/admin-data/TABLE
    const table = url.replace('/api/', '');
    const res = await fetch(`${API}/v1/admin-data/${table}`, { headers });
    if (res.status === 403 || res.status === 401) { doLogout(); throw new Error('Invalid token'); }
    return res;
  }
  throw new Error('Not authenticated');
}
```

---

## FILE OUTPUT ORDER (strict — ui.h is largest, output first)

1. config.h — add NVS key comment block (no new #defines needed — all in UI_SPEC)
2. network.h — add fetchImageBytes(), add config parsing in _sendHello(), add reportCompletion slot param
3. ui.h — FULL REWRITE: grid system, all screens, service mode 5 tabs, PIN overlay, debug screen, language toggle, idle instruction text
4. satu_vending.ino — add: setup code screen branch, boot PIN flow, nuke command, debug gesture, factory reset call, wire reportCompletion, fix idleAnimation vs idleAnimationUI calls
5. machine.js — add completion, factory-reset, config in /hello
6. index.js — add 3 new routes + fix admin token route

---

## PRE-FLIGHT CHECKLIST (run before declaring done)

- [ ] hardware.h not opened or modified
- [ ] NUM_SLOTS defined only in config.h, not in ui.h or anywhere else
- [ ] idleAnimation() (hardware.h) and idleAnimationUI() (ui.h) both exist, neither conflicts
- [ ] g_pngBuf allocated with ps_malloc() in initUI() — not on stack
- [ ] PNGdec included in ui.h or network.h: #include <PNGdec.h>
- [ ] All NVS keys match UI_SPEC.md table exactly — no invented keys
- [ ] Factory reset calls backend before wiping NVS
- [ ] /v1/machine/completion added to index.js routes
- [ ] /v1/machine/factory-reset added to index.js routes
- [ ] /v1/admin-data/:table accepts BOTH X-Admin-Token and Bearer JWT
- [ ] satu-admin.html apiGet() token mode routes to /v1/admin-data/:table
- [ ] No Thai text in any Arduino_GFX print() calls — ASCII only
- [ ] g_grid_rows and g_grid_cols are runtime variables, not compile-time constants
- [ ] Service mode exit returns to STATE_IDLE and calls drawIdleScreen()
- [ ] PIN lockout implemented: 3 wrong attempts → 30s countdown

---

## HARDWARE REFERENCE

Board: ESP32-8048S070C (ESP32-S3, 16MB flash, 8MB OPI PSRAM)
Display: Arduino_GFX RGB panel 800×480, backlight GPIO2
Touch: TAMC_GT911, SDA=19, SCL=20, INT=-1, RST=-1, ROTATION_INVERTED
MCP1 (0x20): sensors 1-8 (GPA0-7), relays 1-6 (GPB0-5)
MCP2 (0x21): sensors 9-10 (GPA0-1), relays 7-12 (GPB0-5)
LEDs: 40× WS2812B on GPIO5, zones: TOP(0-9), FL1(10-19), FL2(20-29), DOOR(30-39)
OPI PSRAM: NEVER change PSRAM mode setting or display breaks

## LIBRARIES (already installed except PNGdec)

Arduino_GFX_Library v1.4.9 (moononournation)
TAMC_GT911
ArduinoJson v7.x
Adafruit MCP23X17 v2.x
FastLED v3.x
PNGdec (bitbank2) ← USER INSTALLS WHILE CC RUNS: Tools → Manage Libraries → PNGdec
Preferences, WiFi, HTTPClient, Wire — all built-in ESP32


---

## SIMULATOR DELTA — MATCH THESE EXACTLY (from screenshots reviewed 2026-05-31)

The simulator.html in public/ is the visual spec. R4 must match it and add items below.

### What simulator already has (match exactly):

SETTINGS tab — sections in order:
1. NETWORK: WiFi SSID input, WiFi Password input
2. SERVICE ACCESS: Service Password (4-digit), Require password toggle
3. DISPLAY: Screen Theme dropdown, Idle Timeout input
4. Volume slider 0-100% → NVS key: `vol` (3 chars ✓)
5. FEATURES: Sacred Water toggle, Lucky Number toggle, ID Card Reader toggle → NVS key: `nvs_idc`
6. LANE PRICES (THB): 10 read-only price fields showing current g_slots[].price values
   Label: "Edit prices at dashboard.satu-th.com"
7. [Save to NVS] button, [Factory Reset] button

FREE PLAY tab:
- Lane buttons show name_en (EN labels in firmware — Thai only in simulator HTML)
- Motor modes: PULSE / HOLD / SLOW with description text
- Special Devices: Water Pump (hold), Door Lock Toggle, Run All Lanes (seq)
- FREE PLAY LOG section at bottom

DEVICES tab (single scrollable tab, not two):
Section 1: RELAY BANK (10 LANES + PUMP + DOOR)
- 12 relay buttons in 2 rows of 6
- Labels: R1 Lane 1 through R10 Lane 10, R11 Pump, R12 Door
- State: OFF (grey) / ON (green)
- Warning text: "Toggle any device independently. Green = ON. For diagnosis only."

Section 2: IR SENSORS (READ-ONLY LIVE)
- 10 sensors IR1-IR10, CLEAR (dark) / BLOCK (red circle)

Section 3: LED ZONES
- 4 zone bars: TOP(0-9), FLOOR 1(10-19), FLOOR 2(20-29), DOOR(30-39)
- Each shows current colour as coloured bar

Section 4: LED BRIGHTNESS
- Slider 0-255, current value displayed
- Pattern buttons: Gold Idle, Celebrate, Error Red, All Off

FIRMWARE tab (scrollable):
Section 1: CURRENT FIRMWARE
- Version, Build Date, Board, Flash Size, PSRAM

Section 2: REMOTE OTA UPDATE
- [CHECK UPDATE] button (stub)
- [Force update from URL] / [FORCE OTA] button (stub)

Section 3: SECURITY (PRODUCTION)
- Flash Encryption: DISABLED (dev mode) — orange text
- Secure Boot V2: DISABLED (dev mode) — orange text
- JTAG: ENABLED — green text
- UART Download: ENABLED — green text
- Note: "Production boards: burn eFuses after final flash. See Security_Protocol.txt"

### R4 additions on top of simulator (NOT in simulator yet):

SETTINGS tab additions:
- Boot PIN toggle → NVS key: `boot_pin`
- Language selector EN/TH → NVS key: `lang`

DEVICES tab addition:
- I2C bus scanner section (after LED section): [SCAN BUS] button → lists found addresses

SERVICE header addition:
- [Show Debug Info] button (shows MAC + device_id for recovery)

### NVS keys added vs simulator:
simulator had: svc_pin, cfg_idle, cfg_water, cfg_lucky, scr_theme
R4 adds: boot_pin, lang, vol, nvs_idc, nvs_ssid, nvs_pass, nvs_grow, nvs_gcol
All confirmed ≤15 chars.
