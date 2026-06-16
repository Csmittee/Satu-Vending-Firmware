# CC_BUILD_PROMPT_wiring_tab.md
> Created by: Chat (Claude)
> Date: 2026-06-16
> Session goal: Build Tab 4 "⚡ Wiring" in satu-machine-builder.html
> Repo: Satu-vending-backend
> Mode: Build Mode — frontend only (public/satu-machine-builder.html)
> Flash cycles: 0 — browser tool only
> PR target: main
> Sequence: Prompt 1 of 1 — self-contained frontend build

---

## CC INTRO — PASTE THIS TO CC

```
New session. Ignore all previous context from other projects.

You are working on SATU 1.0 at:
https://github.com/Csmittee/Satu-vending-backend

Before doing anything else, read IN FULL:
1. CLAUDE.md
2. RULES.md
3. PROJECT_STATE.md
4. public/satu-machine-builder.html  (existing file — read completely)

State the name of every file you read before writing a single line.
Then execute: CC_BUILD_PROMPT_wiring_tab.md
```

---

## CONTEXT

Tab 4 "⚡ Wiring" is a new tab added to `public/satu-machine-builder.html`.
It is a pin-level interactive wiring diagram, signal flow simulator, and
Bill of Materials generator for the Satu 1.0 physical machine.

This is a browser-only tool — no backend API calls, no firmware changes.
All hardware data is hardcoded from the confirmed firmware source of truth
(hardware.h + config.h as of 2026-06-16).

Owner is a solo founder building physical hardware with components arriving.
This tool helps verify correct wiring BEFORE connecting anything.
Getting this wrong destroys components. Accuracy > aesthetics.

---

## HARDWARE REFERENCE — SOURCE OF TRUTH
> Embed these constants in the JS. Never guess pin numbers.

### ESP32-S3 (ESP32-8048S070C) GPIO assignments
```
GPIO 19  = I2C SDA  → MCP1 + MCP2
GPIO 20  = I2C SCL  → MCP1 + MCP2
GPIO  5  = WS2812B LED data (40 LEDs)
GPIO  2  = TFT backlight (PWM)
GPIO 44  = UART RX
GPIO 43  = UART TX
```

### MCP23017 — I2C addresses + pin mapping
```
MCP1 (0x20):
  GPA0–GPA7 = IR Sensors 1–8   (MCP pins 0–7,  INPUT_PULLUP, SENSOR_TRIGGERED=LOW)
  GPB0–GPB5 = Relays 1–6       (MCP pins 8–13, OUTPUT, RELAY_ON=HIGH)

MCP2 (0x21):
  GPA0–GPA1 = IR Sensors 9–10  (MCP pins 0–1,  INPUT_PULLUP, SENSOR_TRIGGERED=LOW)
  GPB0–GPB5 = Relays 7–12      (MCP pins 8–13, OUTPUT, RELAY_ON=HIGH)
    Relay 11 = Water Pump  (GPB3, pin 11)
    Relay 12 = Door Lock   (GPB5, pin 13) — HIGH=UNLOCKED, LOW=LOCKED

MCP3 (0x22): defined in config but NOT populated — show as "reserved / unpopulated"
```

### Relay → Lane mapping (1-indexed)
```
Relay  1  = Lane 1   motor  (MCP1 GPB0 pin  8)
Relay  2  = Lane 2   motor  (MCP1 GPB1 pin  9)
Relay  3  = Lane 3   motor  (MCP1 GPB2 pin 10)
Relay  4  = Lane 4   motor  (MCP1 GPB3 pin 11)
Relay  5  = Lane 5   motor  (MCP1 GPB4 pin 12)
Relay  6  = Lane 6   motor  (MCP1 GPB5 pin 13)
Relay  7  = Lane 7   motor  (MCP2 GPB0 pin  8)
Relay  8  = Lane 8   motor  (MCP2 GPB1 pin  9)
Relay  9  = Lane 9   motor  (MCP2 GPB2 pin 10)
Relay 10  = Lane 10  motor  (MCP2 GPB3 pin 11)
Relay 11  = Water Pump      (MCP2 GPB4 pin 12)
Relay 12  = Door Lock       (MCP2 GPB5 pin 13)
```

### Sensor → Lane mapping (1-indexed)
```
Sensor  1  = Lane 1  drop detect  (MCP1 GPA0 pin 0)
Sensor  2  = Lane 2  drop detect  (MCP1 GPA1 pin 1)
Sensor  3  = Lane 3  drop detect  (MCP1 GPA2 pin 2)
Sensor  4  = Lane 4  drop detect  (MCP1 GPA3 pin 3)
Sensor  5  = Lane 5  drop detect  (MCP1 GPA4 pin 4)
Sensor  6  = Lane 6  drop detect  (MCP1 GPA5 pin 5)
Sensor  7  = Lane 7  drop detect  (MCP1 GPA6 pin 6)
Sensor  8  = Lane 8  drop detect  (MCP1 GPA7 pin 7)
Sensor  9  = Lane 9  drop detect  (MCP2 GPA0 pin 0)
Sensor 10  = Lane 10 drop detect  (MCP2 GPA1 pin 1)
```

### LED zones
```
WS2812B 40px strip on GPIO5:
  Zone TOP    = LEDs  0– 9  (top accent lighting)
  Zone FLOOR1 = LEDs 10–19  (floor 1 — lanes 1–5)
  Zone FLOOR2 = LEDs 20–29  (floor 2 — lanes 6–10)
  Zone DOOR   = LEDs 30–39  (door / dispense area)
```

### Timing constants (from config.h — never change in this tool)
```
VEND_PULSE_MS      =   800  ms  relay on-time for motor
PAYMENT_TIMEOUT    = 120000  ms  (120s)
VEND_TIMEOUT       =  10000  ms  sensor must trigger within 10s of relay fire
DROP_TIMEOUT       =   5000  ms  item must drop within 5s of relay off
REMOVAL_TIMEOUT    =  30000  ms  door re-locks after 30s if item not removed
HEARTBEAT_INTERVAL = 300000  ms  (5 min)
```

---

## WIRE COLOR STANDARD — IEC 60757 + electronics convention
> Use these colors for ALL wires drawn in the diagram.
> Owner shops wire by color — this must be correct.

```
RED     #FF2020  = 5V power
ORANGE  #FF8C00  = 3.3V power
BLACK   #111111  = GND / common ground
WHITE   #F0F0F0  = 12V power
YELLOW  #FFD700  = I2C SDA
GREEN   #00C040  = I2C SCL
BLUE    #2060FF  = GPIO signal (relay IN line, sensor OUT line)
PURPLE  #9B30FF  = WS2812B data
GREY    #888888  = NC / unused / reserved
BROWN   #8B4513  = 12V return / chassis ground
```

---

## JST CONNECTOR SPECIFICATION
> Show on every connection endpoint in the diagram.

```
I2C bus (ESP32 → MCP1, ESP32 → MCP2):
  JST-PH 2.0mm 4-pin  [3.3V | GND | SDA | SCL]
  Colors: ORANGE | BLACK | YELLOW | GREEN

IR Sensor (MCP GPA → sensor module):
  JST-PH 2.0mm 3-pin  [VCC | GND | OUT]
  Colors: RED(5V) | BLACK | BLUE
  Note: sensor VCC can be 3.3V or 5V — verify module spec

Relay signal (MCP GPB → relay module IN):
  JST-XH 2.54mm 2-pin  [IN | GND]
  Colors: BLUE | BLACK

Motor load (relay NO → spring motor):
  JST-VH 3.96mm 2-pin  [MOTOR+ | MOTOR-]
  Colors: RED | BLACK
  Note: motor voltage = verify with supplier (typically 12V DC)

WS2812B LED strip:
  JST-SM 2.54mm 3-pin  [5V | GND | DATA]
  Colors: RED | BLACK | PURPLE

Door lock solenoid:
  JST-VH 3.96mm 2-pin  [12V+ | GND]
  Colors: WHITE | BROWN

Water pump:
  JST-VH 3.96mm 2-pin  [12V+ | GND]
  Colors: WHITE | BROWN

Power supply — 5V rail:
  Terminal block or XT60  [5V+ | GND]
  Feeds: relay modules, IR sensors, WS2812B strip

Power supply — 12V rail:
  Terminal block  [12V+ | GND]
  Feeds: door lock solenoid, water pump, spring motors
```

---

## ELECTRONICS WARNINGS — ALWAYS VISIBLE IN UI
> Embed as a persistent warning panel. Each warning has a confirm checkbox.
> Unchecked = ⚠ yellow. Checked = ✅ green. State persists in localStorage.

```
W-01: I2C pull-up resistors — 4.7kΩ required on SDA (GPIO19) and SCL (GPIO20) to 3.3V.
      Without these the I2C bus floats → MCP not found → no relays or sensors work.
      Add before first power-on.

W-02: Relay module driver — verify relay modules have built-in optocoupler + transistor.
      MCP23017 max output = 25mA per pin. Bare relay coils need ~80mA → MCP will fail or burn.
      Most 5V relay modules from China include driver. Check datasheet before connecting.

W-03: WS2812B data resistor — add 300–500Ω resistor between GPIO5 and LED strip data IN.
      Without this, voltage spikes can destroy the first LED in the strip permanently.

W-04: WS2812B power — 40 LEDs need dedicated 5V supply, min 2.5A. Do NOT power from USB.
      Add 100µF capacitor across 5V/GND at the strip power input connector.

W-05: Motor voltage — confirm spring motor operating voltage with supplier before wiring.
      Relay switches load voltage. Wrong voltage = motor too slow, too fast, or burnt.

W-06: Flyback diode — relay coils generate voltage spikes when switched off.
      Relay modules with driver boards include a flyback diode. Verify before connecting motors.
```

---

## TASK — BUILD TAB 4

### Tab placement
Add Tab 4 labelled `⚡ Wiring` to the existing tab bar in `satu-machine-builder.html`.
Tab IDs in existing file: confirm from reading the file. Do not break existing tabs.

---

### Tab 4 layout — 3-panel horizontal split

```
┌─────────────────────────────────────────────────────────────────┐
│ LEFT 240px fixed  │  CENTER flex (SVG canvas)  │  RIGHT 320px  │
│ Component tree    │  Pin-level wiring diagram   │  Inspector    │
│ + layer toggles   │  + animated signal flow     │  + Simulator  │
│ + wire legend     │                             │  + Log        │
└─────────────────────────────────────────────────────────────────┘
```

---

### LEFT PANEL — Component Tree + Layer Toggles

Component tree (collapsible):
```
▼ ESP32-S3
    GPIO19 — SDA
    GPIO20 — SCL
    GPIO5  — LED data
    GPIO2  — TFT BL
    GPIO44 — UART RX
    GPIO43 — UART TX
▼ MCP1 (0x20)
  ▼ GPA — Sensors
      GPA0 — Sensor 1 — Lane 1
      GPA1 — Sensor 2 — Lane 2
      ... (GPA0–GPA7)
  ▼ GPB — Relays
      GPB0 — Relay 1 — Lane 1
      ... (GPB0–GPB5)
▼ MCP2 (0x21)
  ▼ GPA — Sensors
      GPA0 — Sensor 9  — Lane 9
      GPA1 — Sensor 10 — Lane 10
  ▼ GPB — Relays
      GPB0 — Relay 7  — Lane 7
      ...
      GPB3 — Relay 11 — Water Pump
      GPB4 — Relay 12 — Door Lock
▼ WS2812B Strip (40 LEDs)
    Zone TOP    LEDs 0–9
    Zone FLOOR1 LEDs 10–19
    Zone FLOOR2 LEDs 20–29
    Zone DOOR   LEDs 30–39
▼ Power Rails
    5V rail
    3.3V rail
    12V rail
    GND
```

Layer toggle checkboxes (each hides/shows that wire group in SVG):
```
☑ 3.3V / power wires    (ORANGE)
☑ 5V / power wires      (RED)
☑ 12V / power wires     (WHITE)
☑ GND wires             (BLACK)
☑ I2C bus               (YELLOW + GREEN)
☑ Relay signal lines    (BLUE)
☑ Sensor signal lines   (BLUE, dashed)
☑ LED data line         (PURPLE)
☑ JST connectors        (show/hide labels)
☑ Warning flags         (show/hide W-01–W-06)
```

Wire color legend — static at bottom of left panel. Shows color swatch + label for each color.

---

### CENTER PANEL — SVG Pin-Level Wiring Diagram

#### Diagram layout — physical left-to-right flow

```
[Power Rails]  →  [ESP32-S3]  →  [I2C Bus]  →  [MCP1] → [Relays 1-6]  → [Motors 1-6]
                                                         → [Sensors 1-8]
                               [I2C Bus]  →  [MCP2] → [Relays 7-12] → [Motors 7-10]
                                                                      → [Door Lock]
                                                                      → [Water Pump]
                                                      → [Sensors 9-10]
               [ESP32-S3 GPIO5] → [300Ω]  → [WS2812B strip]
```

#### Node rendering requirements

Each component is drawn as a labeled rectangle showing:
- Component name (large)
- Each physical pin listed inside: `GPA0 | Sensor 1 | Lane 1`
- JST connector type at each connection point (shown as small labeled tab)
- Wire color shown as colored line segment leaving each pin

Example MCP1 node:
```
┌─────────────────────────────────────────┐
│  MCP23017  addr:0x20                    │
│  ┌──────────────┬──────────────────┐    │
│  │ GPA (IN)     │ GPB (OUT)        │    │
│  │ 0: Sensor 1  │ 8:  Relay 1 ●─── BLUE JST-XH → Relay Module 1
│  │ 1: Sensor 2  │ 9:  Relay 2 ●─── BLUE JST-XH → Relay Module 2
│  │ 2: Sensor 3  │ 10: Relay 3 ●─── ...
│  │ ...          │ ...              │    │
│  │ 7: Sensor 8  │ 13: Relay 6 ●───     │
│  └──────────────┴──────────────────┘    │
│  I2C ← JST-PH 4-pin ← ESP32 GPIO19/20  │
└─────────────────────────────────────────┘
```

#### Wire routing in SVG
- Wires are SVG `<path>` elements with rounded elbows (not straight diagonals)
- Each wire colored per the color standard above
- Wire stroke-width: power = 3px, signal = 2px, data = 2px dashed for sensors
- Where wires cross: draw bridge arc (small semicircle bump) on crossing wire
- Layer toggles show/hide entire `<g>` groups by layer name
- Hovering a wire highlights it and dims all others

#### Interactivity
- Click any component node → right panel shows Inspector for that component
- Click any wire → highlights full path (source pin → dest pin), shows Inspector
- Click any pin label in left tree → same as clicking that node in SVG
- Hover wire → tooltip shows: FROM pin, TO pin, JST type, wire color, wire gauge suggestion

---

### RIGHT PANEL — Inspector + Simulator + Log

#### Inspector (updates on click)
Shows for selected node:
```
Component:   MCP23017
Address:     0x20
I2C bus:     GPIO19 (SDA) + GPIO20 (SCL)
Pin:         GPB2 (pin 10)
Type:        OUTPUT — Relay drive
Maps to:     Relay 3 → Lane 3 motor
Connector:   JST-XH 2.54mm 2-pin [IN | GND]
Wire color:  BLUE (signal) + BLACK (GND)
Active HIGH: RELAY_ON = HIGH
Firmware:    mcp1.digitalWrite(10, HIGH)
```

#### Timing Simulator

Dropdown — select scenario:
```
1. Normal vend — Lane 3 (full path)
2. Normal vend — Door Lock cycle
3. Sensor never triggers (VEND_TIMEOUT)
4. Relay stuck ON (no relay OFF detected)
5. Missing I2C pull-up (bus fault)
6. Item not removed (REMOVAL_TIMEOUT)
7. Water pump cycle
```

[▶ Run Simulation] button — plays scenario as animated signal flow on SVG canvas.

Animation:
- Moving colored dots travel along the wire paths in direction of signal flow
- Speed proportional to real timing (compressed: 1 real second = 100ms animation)
- Each event fires in sequence with correct delays between them

#### Simulation Log
Below the simulator — scrollable log output formatted to match real serial monitor:

```
[0ms]       Order received — Lane 3
[2ms]       I2C write → MCP1 addr:0x20
[5ms]       GPB2 (pin 10) → HIGH — Relay 3 ON
[5ms]       Lane 3 motor RUNNING — VEND_PULSE_MS=800ms
[805ms]     GPB2 (pin 10) → LOW  — Relay 3 OFF
[805ms]     Motor stopped — waiting for sensor
[810ms]     GPA2 (pin 2) read → LOW — Sensor 3 TRIGGERED ✅ item dropped
[815ms]     Door UNLOCK → Relay 12 ON (MCP2 GPB5 pin 13)
[815ms]     LEDs DOOR zone → GREEN
[30815ms]   REMOVAL_TIMEOUT — item removed assumed
[30820ms]   Door LOCK → Relay 12 OFF
[30820ms]   LEDs DOOR zone → GOLD
[30825ms]   State → IDLE
```

Fault scenarios show RED lines:
```
[805ms]     GPB2 → LOW — Relay 3 OFF
[810ms]     GPA2 read → HIGH — Sensor 3 CLEAR ⚠ no drop detected
[1000ms]    GPA2 read → HIGH — still no drop
...
[10805ms]   VEND_TIMEOUT — 10000ms exceeded ❌
[10810ms]   State → IDLE — vend failed logged
```

---

### BOM TAB — Nested inside Tab 4

Add a sub-tab or top section within Tab 4 for the Bill of Materials.

#### BOM top section — Harness Length Input table
Editable table — owner fills in measured cable lengths:

```
┌──────────────────────────────────────────────────────────────────┐
│  HARNESS LENGTH REFERENCE  (measured actual — edit to update BOM) │
├─────────────────────────┬──────────┬──────────────────────────────┤
│  Run                    │  Length  │  Notes                        │
├─────────────────────────┼──────────┼──────────────────────────────┤
│  ESP32 → MCP1 (I2C)     │ [___]cm  │  JST-PH 4-pin                 │
│  ESP32 → MCP2 (I2C)     │ [___]cm  │  JST-PH 4-pin                 │
│  MCP1 → Relay bank 1    │ [___]cm  │  JST-XH 2-pin × 6             │
│  MCP2 → Relay bank 2    │ [___]cm  │  JST-XH 2-pin × 6             │
│  MCP1 → IR Sensors 1-8  │ [___]cm  │  JST-PH 3-pin × 8             │
│  MCP2 → IR Sensors 9-10 │ [___]cm  │  JST-PH 3-pin × 2             │
│  Relay → Motor (per lane)│ [___]cm  │  JST-VH 2-pin × 10            │
│  ESP32 GPIO5 → LED strip │ [___]cm  │  JST-SM 3-pin + 300Ω          │
│  Relay 12 → Door lock    │ [___]cm  │  JST-VH 2-pin                 │
│  Relay 11 → Water pump   │ [___]cm  │  JST-VH 2-pin                 │
│  5V rail distribution    │ [___]cm  │  trunk + branches             │
│  12V rail distribution   │ [___]cm  │  trunk + branches             │
└─────────────────────────┴──────────┴──────────────────────────────┘
```

#### BOM output — computed from harness table

```
SATU 1.0 — BILL OF MATERIALS  (generated 2026-06-16)
════════════════════════════════════════════════════════════
ACTIVE COMPONENTS
────────────────────────────────────────────────────────────
ESP32-S3 (ESP32-8048S070C)           qty: 1    Main controller + 7" display
MCP23017 I2C GPIO expander           qty: 2    Relay + sensor control (I2C)
Relay module 8-ch 5V optocoupler     qty: 2    Motor + pump + door control
IR sensor module (active LOW)        qty: 10   Drop detection per lane
Spring coil motor (vending)          qty: 10   One per lane
Door lock solenoid 12V               qty: 1    Relay 12 — normally locked
Water pump                           qty: 1    Relay 11 — sacred water
WS2812B LED strip (60LED/m)          qty: 1    40 LEDs — cut to length

PASSIVE COMPONENTS
────────────────────────────────────────────────────────────
4.7kΩ resistor (1/4W)                qty: 2    I2C pull-up SDA + SCL → 3.3V
300Ω–500Ω resistor (1/4W)            qty: 1    WS2812B data line protection
100µF electrolytic capacitor 10V+    qty: 1    LED strip power filter

POWER SUPPLY
────────────────────────────────────────────────────────────
5V regulated supply (min 2.5A)       qty: 1    Relay modules + sensors + LEDs
12V supply (size per solenoid spec)  qty: 1    Solenoid + motors + pump
3.3V — from ESP32 onboard regulator  —         MCP23017 VCC + I2C pull-up rail

CONNECTORS
────────────────────────────────────────────────────────────
JST-PH 2.0mm 4-pin pair (M+F)        qty: 4    I2C runs (ESP32→MCP1, →MCP2, spares)
JST-PH 2.0mm 3-pin pair (M+F)        qty: 12   IR sensors (10 used + 2 spare)
JST-XH 2.54mm 2-pin pair (M+F)       qty: 16   Relay signal (12 used + 4 spare)
JST-VH 3.96mm 2-pin pair (M+F)       qty: 16   Motor/solenoid load (12 used + 4 spare)
JST-SM 2.54mm 3-pin pair (M+F)       qty: 4    LED strip (1 used + 3 spare)

WIRE (lengths auto-calculated from harness table above)
────────────────────────────────────────────────────────────
RED    22AWG   (5V power)            [auto]m   from harness table
ORANGE 22AWG   (3.3V power)          [auto]m
BLACK  22AWG   (GND)                 [auto]m
WHITE  22AWG   (12V power)           [auto]m
YELLOW 26AWG   (I2C SDA)             [auto]m
GREEN  26AWG   (I2C SCL)             [auto]m
BLUE   26AWG   (signal lines)        [auto]m
PURPLE 26AWG   (LED data)            [auto]m
════════════════════════════════════════════════════════════
```

Wire lengths show `[auto]m` when harness table is empty, or computed total once filled.
Add 20% margin automatically to all wire lengths.

[📋 Copy BOM] button — copies plain text to clipboard.
[🖨 Print / PDF] button — opens browser print dialog (print-optimized CSS layout).

---

## DO NOT TOUCH

- Any existing tab (Tab 1 Orders, Tab 2 Sim, Tab 3 HW Trigger) — zero changes
- Backend API files — this is frontend only
- Firmware files — wrong repo
- hardware.h — R2 LOCKED — never modify
- config.h NUM_SLOTS — never redefine
- PAYMENT_MODE — stays fake

---

## IMPLEMENTATION NOTES FOR CC

1. **Single file** — all HTML, CSS, JS added to `satu-machine-builder.html`. No external files.

2. **SVG diagram** — draw with vanilla SVG elements (`<rect>`, `<path>`, `<text>`, `<circle>`).
   Use `<g id="layer-i2c">` etc. for layer groups. Toggle with `display:none`.
   No external diagramming library — keep it self-contained.

3. **Wire crossings** — detect when two paths cross geometrically and render a small arc
   (bridge) on the upper wire so lines never look merged.

4. **Animation** — use CSS `@keyframes` + JS `animateAlongPath()` for moving dots.
   Each scenario is a JS array of timed events — use `setTimeout` chain.

5. **Harness table** — store in `localStorage` key `satu_harness_lengths`.
   Auto-recalculate BOM wire totals on every input change.

6. **Warning checkboxes** — store confirmed state in `localStorage` key `satu_wiring_warnings`.

7. **Print CSS** — add `@media print` block: hide tabs, show only BOM section, black text on white.

8. **Zoom/pan on SVG canvas** — add mouse wheel zoom + drag pan on the center SVG.
   Implement with `viewBox` manipulation + `transform`. No library.

---

## VERIFICATION STEPS (CC confirms before closing)

1. Tab 4 appears in tab bar without breaking Tabs 1–3
2. All 12 relays appear in diagram with correct MCP + pin label
3. All 10 sensors appear with correct MCP + pin label
4. Layer toggles show/hide correct wire groups
5. Click Relay 3 node → inspector shows: MCP1, GPB2, pin 10, Lane 3, JST-XH
6. Click Sensor 3 node → inspector shows: MCP1, GPA2, pin 2, Lane 3, JST-PH
7. Run "Normal vend Lane 3" simulation → log matches timing spec above
8. Run "Sensor never triggers" → VEND_TIMEOUT fires at 10000ms in log
9. BOM harness table is editable, wire lengths update in BOM output
10. Print dialog opens from [🖨 Print / PDF] button
11. All 6 warnings visible and checkboxes persist after page reload

---

## MANDATORY END OF SESSION (R-84)

CC responsibilities:
1. Archive this prompt → `docs/prompts/` stamped:
   `✅ COMPLETE — 2026-06-16 — wiring tab Tab 4 build`
2. Append to RULES.md at TOP (next R-number after current highest):
   ```
   R-127: Wiring tab (Tab 4) is a pin-level browser tool — all hardware constants
   are hardcoded from hardware.h + config.h as of 2026-06-16. Never fetch
   hardware data from the backend API in this tool. Source of truth = this file.
   JST types, wire colors, and timing values must match CC_BUILD_PROMPT_wiring_tab.md.
   (Added 2026-06-16)
   ```
3. Update PROJECT_STATE.md — add session log entry + mark wiring tab COMPLETE in open items
4. Update KNOWN_GOOD.md — add snapshot at TOP
5. Commit: `feat: wiring tab Tab 4 — pin-level diagram + simulator + BOM`
6. Merge to main

NOTE: CHAT_HANDOFF.md is Chat's responsibility — CC must never write or overwrite it.

---

## PAYMENT MODE REMINDER
PAYMENT_MODE stays fake. This prompt makes zero backend changes.
Never suggest changing to live.
```
