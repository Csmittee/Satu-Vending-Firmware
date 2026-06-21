# HARDWARE_SPEC.md — Satu 1.0
> Version 1.2 — 2026-06-21
> Changes: Relay 12 corrected to magnetic pin-lock solenoid. Proximity switch added MCP2 GPA2.
>          Speaker GPIO1 assigned. IR sensor mount updated. Wire harness table added.
>          Multi-model section added: 5×2 (current), 5×3 and 7×3 (placeholder).
>          BOM updated. W-07 corrected. MCP RESET pin warning added.
> Previous: v1.1 — 2026-06-20
> Single source of truth for all Satu 1.0 hardware decisions.
> Supersedes: HARDWARE_TRUTH.md, README.md, satu_wiring_diagram.html, Satu 1.0 R2.xml
> R-128, R-129 applied — see FIRMWARE RULES section

---

## CHANGE LOG
| Date | Change | Who |
|---|---|---|
| 2026-06-21 | v1.2 — Relay 12 corrected to magnetic pin-lock solenoid (two locks parallel). Proximity switch MCP2 GPA2 added. Speaker GPIO1 assigned. IR mount updated. Wire harness table added. Multi-model placeholder (5×3, 7×3) added. BOM updated. W-07 corrected. | Owner+Chat |
| 2026-06-20 | Renamed from HARDWARE_TRUTH.md. Added CHANGE LOG section. Expanded MCP3 21-lane expansion note. | Owner+Chat |
| 2026-06-17 | Initial creation — R-128, R-129 applied | Owner |

---

## BOARD

| Field | Value |
|-------|-------|
| Model | ESP32-8048S070C |
| MCU | ESP32-S3 |
| Flash | 16MB |
| PSRAM | 8MB OPI |
| Display | 7" RGB 800×480 (Arduino_GFX driver) |
| Backlight | GPIO2 (PWM) |
| Touch | TAMC_GT911 capacitive |
| Touch SDA | GPIO19 |
| Touch SCL | GPIO20 |
| Touch INT | -1 (not used) |
| Touch RST | -1 (not used) |
| Touch rotation | ROTATION_INVERTED |
| UART RX | GPIO44 |
| UART TX | GPIO43 |
| LED data | GPIO5 |
| Speaker | GPIO1 (passive PWM) |

---

## I2C BUS

| Signal | GPIO |
|--------|------|
| SDA | GPIO19 |
| SCL | GPIO20 |
| Pull-up | 4.7kΩ to 3.3V on both lines — REQUIRED (W-01) |

Both MCP23017 chips share this I2C bus.

---

## MCP23017 — I/O EXPANDERS

### MCP1 — Address 0x20 (A0=GND, A1=GND, A2=GND)

| MCP Pin | Direction | Maps to | Lane |
|---------|-----------|---------|------|
| GPA0 (pin 0) | INPUT_PULLUP | IR Sensor 1 | Lane 1 drop detect |
| GPA1 (pin 1) | INPUT_PULLUP | IR Sensor 2 | Lane 2 drop detect |
| GPA2 (pin 2) | INPUT_PULLUP | IR Sensor 3 | Lane 3 drop detect |
| GPA3 (pin 3) | INPUT_PULLUP | IR Sensor 4 | Lane 4 drop detect |
| GPA4 (pin 4) | INPUT_PULLUP | IR Sensor 5 | Lane 5 drop detect |
| GPA5 (pin 5) | INPUT_PULLUP | IR Sensor 6 | Lane 6 drop detect |
| GPA6 (pin 6) | INPUT_PULLUP | IR Sensor 7 | Lane 7 drop detect |
| GPA7 (pin 7) | INPUT_PULLUP | IR Sensor 8 | Lane 8 drop detect |
| GPB0 (pin 8)  | OUTPUT | Relay 1 | Lane 1 motor |
| GPB1 (pin 9)  | OUTPUT | Relay 2 | Lane 2 motor |
| GPB2 (pin 10) | OUTPUT | Relay 3 | Lane 3 motor |
| GPB3 (pin 11) | OUTPUT | Relay 4 | Lane 4 motor |
| GPB4 (pin 12) | OUTPUT | Relay 5 | Lane 5 motor |
| GPB5 (pin 13) | OUTPUT | Relay 6 | Lane 6 motor |

> ⚠️ MCP RESET PIN (DIP pin 18) must be tied to 3.3V.
> If floating → MCP resets randomly under electrical noise → all relays/sensors lost.
> Verify breakout board ties RESET to VCC internally.
> If not — add 10kΩ pull-up resistor from RESET pin to 3.3V before first power-on.

### MCP2 — Address 0x21 (A0=VCC, A1=GND, A2=GND)

| MCP Pin | Direction | Maps to | Lane |
|---------|-----------|---------|------|
| GPA0 (pin 0) | INPUT_PULLUP | IR Sensor 9  | Lane 9 drop detect |
| GPA1 (pin 1) | INPUT_PULLUP | IR Sensor 10 | Lane 10 drop detect |
| GPA2 (pin 2) | INPUT_PULLUP | Proximity switch | Door closed detect — FLAP_PROXIMITY_MCP_PIN=2 |
| GPA3–GPA7    | — | Spare / expansion | — |
| GPB0 (pin 8)  | OUTPUT | Relay 7  | Lane 7 motor |
| GPB1 (pin 9)  | OUTPUT | Relay 8  | Lane 8 motor |
| GPB2 (pin 10) | OUTPUT | Relay 9  | Lane 9 motor |
| GPB3 (pin 11) | OUTPUT | Relay 10 | Lane 10 motor |
| GPB4 (pin 12) | OUTPUT | Relay 11 | Water Pump |
| GPB5 (pin 13) | OUTPUT | Relay 12 | Magnetic Pin-Lock ⚠️ see R-129 UPDATED |

> ⚠️ MCP RESET PIN (DIP pin 18) must be tied to 3.3V.
> If floating → MCP resets randomly under electrical noise → all relays/sensors lost.
> Verify breakout board ties RESET to VCC internally.
> If not — add 10kΩ pull-up resistor from RESET pin to 3.3V before first power-on.

### MCP3 — Address 0x22 — RESERVED / NOT POPULATED
Used only in 21-lane build (Large model — 7×3 grid).
Do not populate for 10-lane Satu 1.0 prototype.
When populated: GPA0-GPA6 = IR Sensors 11-17, GPB0-GPB5 = Relays 13-18.
Lanes 19-21 require a 4th MCP or direct GPIO — TBD at hardware build phase.

---

## RELAY LOGIC

```
RELAY_ON  = HIGH
RELAY_OFF = LOW
```

All relays default OFF at boot. Motor relays are momentary — never left ON.

---

## SENSOR LOGIC

```
SENSOR_TRIGGERED = LOW  (IR beam broken — item present)
SENSOR_CLEAR     = HIGH (IR beam intact — no item)
Mode: INPUT_PULLUP
```

Sensor polls every 10ms during motor spin (SENSOR_POLL_MS = 10).

---

## IR SENSORS

| Field | Value |
|-------|-------|
| Model | E18-D80NK |
| Type | NPN normally-open |
| VCC | 5V |
| Output | Active LOW when beam broken |
| Mount position | Under tray floor bracket, one per lane, facing product drop path |
| Connector | JST-PH 2.0mm 3-pin [VCC\|GND\|OUT] |
| Wire colors | RED(5V) \| BLACK \| BLUE |

> ⚠️ PLACEHOLDER — E18-D80NK retained for 5×2 prototype.
> Rail-type drop sensor system under evaluation to reduce wiring.
> Do not finalize mount bracket or cable run until sensor type confirmed.
> Current assumption: 1× E18-D80NK per lane.

---

## SPRING MOTORS (VENDING COILS)

| Field | Value |
|-------|-------|
| Count | 10 (one per lane) |
| Voltage | 12V DC (confirm with supplier) |
| Stop logic | **Sensor-triggered — R-128** |
| Safety cutoff | VEND_MAX_SPIN_MS = 30000ms (~10 turns) |
| Speed | 90–180° per second |
| Connector | JST-VH 3.96mm 2-pin [MOTOR+\|MOTOR-] |
| Wire colors | RED \| BLACK |

> ⚠️ R-128 (2026-06-17): Motor NEVER stops on a fixed timer.
> Motor runs until IR sensor triggers. VEND_PULSE_MS is deleted.
> If sensor never triggers within 30 seconds → lane disabled → backend alert.

---

## MAGNETIC PIN-LOCK — RELAY 12 (R-129 UPDATED)

| Field | Value |
|-------|-------|
| Function | Dispense gate lock — holds door closed between vends |
| Relay | Relay 12 (MCP2 GPB5 pin 13) |
| Type | Magnetic pin-lock solenoid — fail-secure |
| Polarity | HIGH = pin RETRACTED = UNLOCKED |
|  | LOW = pin EXTENDED = LOCKED (default, fail-secure) |
| Qty | 2 locks — parallel wired to same relay 12 |
| Open trigger | Relay 12 HIGH at same moment motor starts |
| Close trigger | Proximity switch CLOSED → relay LOW |
| Safety fallback | FLAP_RELOCK_TIMEOUT = 3000ms if proximity never fires |
| Halt condition | Proximity never fires within timeout → no-sell mode |
| Connector | JST-VH 3.96mm 2-pin [12V+|GND] per lock |
| Wire colors | WHITE | BROWN |
| Load run length | 50cm max |
| AWG | 20 AWG |

> ⚠️ R-129 UPDATED 2026-06-21: Design changed from spring-flap pulse to magnetic
> pin-lock solenoid. Two physical locks wired in parallel on relay 12.
> Door is mechanical spring-return. No electronic open command needed —
> product weight opens door, spring closes it. Controller only LOCKS and UNLOCKS.
> Pickup bay front is always open — no door on customer side.

---

## PROXIMITY SWITCH — DOOR CLOSED DETECT

| Field | Value |
|-------|-------|
| Function | Detects collection door returned to closed/locked position |
| Type | Mechanical roller microswitch (SPDT) |
| MCP | MCP2 |
| MCP pin | GPA2 (pin 2) — INPUT_PULLUP |
| Config constant | FLAP_PROXIMITY_MCP_PIN = 2 |
| Logic | CLOSED = LOW (door shut, switch pressed) |
|  | OPEN = HIGH (door open or not yet closed) |
| Qty | 1 — single switch covers full-width door |
| Connector | JST-PH 2.0mm 2-pin [COM|NO] |
| Wire colors | BLACK | BLUE |
| Run length | 50cm |
| AWG | 24 AWG |
| Mount | Door frame — roller pressed when door fully closes |

---

## WATER PUMP — RELAY 11

| Field | Value |
|-------|-------|
| Relay | Relay 11 (MCP2 GPB4 pin 12) |
| Voltage | 12V |
| Run time | 3000ms per cycle |
| Connector | JST-VH 3.96mm 2-pin [12V+\|GND] |
| Wire colors | WHITE \| BROWN |

---

## SPEAKER

| Field | Value |
|-------|-------|
| GPIO | GPIO1 |
| Config constant | SPEAKER_PIN = 1 |
| Type | Passive 2-pin speaker (PWM driven) |
| Connector | JST-PH 2.0mm 2-pin — match speaker stock connector |
| Wire colors | RED | BLACK |
| Run length | 40cm |
| AWG | 24 AWG |

---

## WS2812B LED STRIP

| Field | Value |
|-------|-------|
| Data pin | GPIO5 |
| Total LEDs | 40 |
| Density | 60 LEDs/m — cut to 40 |
| Zone TOP | LEDs 0–9 (top accent) |
| Zone FLOOR1 | LEDs 10–19 (lanes 1–5) |
| Zone FLOOR2 | LEDs 20–29 (lanes 6–10) |
| Zone DOOR | LEDs 30–39 (flap/dispense area) |
| Data resistor | 300–500Ω between GPIO5 and strip data IN (W-03) |
| Power | Dedicated 5V supply min 2.5A (W-04) |
| Cap | 100µF across 5V/GND at strip input |
| Connector | JST-SM 2.54mm 3-pin [5V\|GND\|DATA] |

---

## POWER RAILS

| Rail | Voltage | Amps | Feeds |
|------|---------|------|-------|
| 5V | 5V | 5A min | ESP32, MCP23017, relay coils, IR sensors, LEDs |
| 12V | 12V | 10A | Spring motors, water pump, magnetic lock solenoids |
| 3.3V | 3.3V | — | From ESP32 onboard regulator — MCP VCC + I2C pull-ups |

> ⚠️ Relay board 12V load must NOT come from ESP32 5V rail (max 500mA).
> Use dedicated 12V supply for all motor/pump/solenoid loads.

---

## JST CONNECTOR STANDARD

| Connection | JST Type | Pins | Wire colors |
|-----------|----------|------|-------------|
| I2C bus (ESP32 → MCP) | JST-PH 2.0mm 4-pin | [3.3V\|GND\|SDA\|SCL] | ORANGE\|BLACK\|YELLOW\|GREEN |
| IR Sensor | JST-PH 2.0mm 3-pin | [VCC\|GND\|OUT] | RED\|BLACK\|BLUE |
| Relay signal (MCP → relay IN) | JST-XH 2.54mm 2-pin | [IN\|GND] | BLUE\|BLACK |
| Motor load (relay NO → motor) | JST-VH 3.96mm 2-pin | [MOTOR+\|MOTOR-] | RED\|BLACK |
| LED strip | JST-SM 2.54mm 3-pin | [5V\|GND\|DATA] | RED\|BLACK\|PURPLE |
| Mag lock / pump load | JST-VH 3.96mm 2-pin | [VCC+\|GND] | WHITE\|BROWN |
| Proximity switch | JST-PH 2.0mm 2-pin | [COM\|NO] | BLACK\|BLUE |
| Speaker | JST-PH 2.0mm 2-pin | [+\|GND] | RED\|BLACK |

---

## WIRE COLOR STANDARD (IEC 60757)

| Color | Hex | Use |
|-------|-----|-----|
| RED | #FF2020 | 5V power |
| ORANGE | #FF8C00 | 3.3V power |
| BLACK | #111111 | GND |
| WHITE | #F0F0F0 | 12V power |
| YELLOW | #FFD700 | I2C SDA |
| GREEN | #00C040 | I2C SCL |
| BLUE | #2060FF | Signal lines (relay IN, sensor OUT) |
| PURPLE | #9B30FF | WS2812B data |
| GREY | #888888 | NC / unused |
| BROWN | #8B4513 | 12V return / chassis ground |

---

## ELECTRONICS WARNINGS

| ID | Warning | Risk if ignored |
|----|---------|----------------|
| W-01 | I2C pull-up 4.7kΩ on SDA+SCL to 3.3V | I2C bus floats — MCPs not found — no relays/sensors |
| W-02 | Relay module must have optocoupler driver | MCP pin max 25mA, relay coil needs 80mA → MCP burns |
| W-03 | 300–500Ω resistor on GPIO5 → LED data | Voltage spike destroys first LED permanently |
| W-04 | WS2812B needs dedicated 5V 2.5A+ supply | Brown-out or fire risk from USB/ESP32 rail |
| W-05 | Confirm motor voltage with supplier | Wrong voltage = too slow, too fast, or burnt |
| W-06 | Verify relay module has flyback diode | Relay coil spike destroys transistor on switch-off |
| W-07 | Magnetic lock polarity — Relay 12 | HIGH=UNLOCKED, LOW=LOCKED. Reversed polarity = door unlocked at rest = fail-open = security risk. Verify before power-on. |

---

## WIRE HARNESS SUMMARY — 5×2 PROTOTYPE

### AWG by use
| AWG | Use |
|-----|-----|
| 24 AWG | I2C, signal, sensors, UART, speaker |
| 22 AWG | 5V power trunk |
| 20 AWG | 12V motor/pump/lock loads |
| 18 AWG | 12V main trunk |
| 16 AWG | 220V mains |

### Wire color standard (IEC 60757)
| Color | Use |
|-------|-----|
| RED | 5V power |
| ORANGE | 3.3V power |
| BLACK | GND |
| WHITE | 12V power |
| YELLOW | I2C SDA |
| GREEN | I2C SCL |
| BLUE | Signal lines (relay IN, sensor OUT) |
| PURPLE | WS2812B data |
| BROWN | 12V return / chassis ground |
| GREY | NC / unused |

### JST connector count — 5×2 prototype
| Type | Pins | Count | Use |
|------|------|-------|-----|
| JST-PH 2.0mm | 4-pin | 6 pairs | I2C daisy chain (ESP32→MCP1→MCP2) + UART card reader |
| JST-PH 2.0mm | 3-pin | 12 pairs | IR sensors ×10 + 2 spare |
| JST-PH 2.0mm | 2-pin | 3 pairs | Door proximity switch, speaker, spare |
| JST-XH 2.54mm | 2-pin | 14 pairs | Relay signal ×12 + 2 spare |
| JST-VH 3.96mm | 2-pin | 18 pairs | Motors ×10, pump ×1, mag locks ×2, spare ×5 |
| JST-SM 2.54mm | 3-pin | 2 pairs | LED strip |

### Run lengths — 5×2 prototype
| Connection | Max length | AWG |
|-----------|-----------|-----|
| ESP32 → MCP1 (I2C) | 20cm | 24 |
| MCP1 → MCP2 (I2C daisy) | 20cm | 24 |
| MCP → IR sensors | 60cm | 24 |
| MCP → relay signal | 20cm | 24 |
| Relay → motors | 60cm | 20 |
| Relay → water pump | 100cm | 20 |
| Relay → mag locks | 50cm | 20 |
| ESP32 → card reader (UART) | 40cm | 24 |
| ESP32 → LED strip | 40cm | 24 |
| PSU → controller zone | 100cm | 22/18 |
| Wall → PSU mains | 150cm | 16 |

---

## BILL OF MATERIALS — SATU 1.0 (10-lane)

| Component | Model | Qty | Est. THB |
|-----------|-------|-----|----------|
| ESP32-S3 display board | ESP32-8048S070C | 1 | 1600 |
| MCP23017 I2C expander | 16-bit I2C | 2 | 240 |
| IR obstacle sensor | E18-D80NK | 10 | 450 |
| 8-ch relay module 5V | Opto-isolated | 1 | 220 |
| 2-ch relay module 5V | Opto-isolated | 1 | 80 |
| Spring coil motor | Vending coil | 10 | TBD |
| Magnetic pin-lock solenoid 12V | Fail-secure | 2 | TBD |
| Roller microswitch SPDT | — | 1 | 20 |
| Water pump | 12V DC | 1 | TBD |
| WS2812B LED strip | 60 LED/m | 1m | 150 |
| 5V 5A PSU | Regulated | 1 | 250 |
| 12V 10A PSU | Regulated | 1 | 450 |
| 4.7kΩ resistor 1/4W | — | 2 | 5 |
| 300Ω resistor 1/4W | — | 1 | 3 |
| 10kΩ resistor 1/4W | MCP RESET pull-up | 2 | 5 |
| 100µF capacitor 10V+ | Electrolytic | 1 | 10 |
| JST-PH 2.0mm 4-pin (M+F) | Pre-crimped | 4 pairs | 80 |
| JST-PH 2.0mm 3-pin (M+F) | Pre-crimped | 12 pairs | 120 |
| JST-PH 2.0mm 2-pin (M+F) | Pre-crimped | 3 pairs | 30 |
| JST-XH 2.54mm 2-pin (M+F) | Pre-crimped | 16 pairs | 80 |
| JST-VH 3.96mm 2-pin (M+F) | Pre-crimped | 16 pairs | 160 |
| JST-SM 2.54mm 3-pin (M+F) | Pre-crimped | 2 pairs | 40 |
| Metal frame + acrylic door | 1.5mm steel | 1 | 3000 |
| **TOTAL (estimated)** | | | **~7000+** |

> Note: Magnetic lock solenoids and spring motor costs TBD pending supplier confirmation.
> Previous BOM (bom_2025_04.csv) total ~8070 THB — treat as reference only.

---

## FIRMWARE RULES — HARDWARE-LINKED

These firmware rules directly affect hardware behaviour.
Any wiring or component change must be checked against these rules.

```
R-128 (2026-06-17): Motor stop is sensor-driven. VEND_PULSE_MS deleted.
      vendProduct() returns bool. Motor runs until IR sensor triggers
      OR VEND_MAX_SPIN_MS=30000ms safety cutoff. Lane empty → disabled → backend alert.
      NON-NEGOTIABLE. Timer-based motor stop permanently prohibited.

R-129 UPDATED (2026-06-21): Relay 12 = magnetic pin-lock solenoid. NOT spring flap.
      HIGH=UNLOCKED (pin retracted). LOW=LOCKED (fail-secure, pin extended).
      Two physical locks parallel-wired on relay 12.
      unlockFlap() called at motor start. lockFlap() called on proximity CLOSED or timeout.
      FLAP_PROXIMITY_MCP_PIN=2 (MCP2 GPA2) activates proximity polling.
      FLAP_RELOCK_TIMEOUT=3000ms = safety fallback if proximity never fires.

R-165 (2026-06-21): HARDWARE_SPEC.md v1.2 is hardware wiring source of truth.
      Relay 12 = magnetic pin-lock solenoid (NOT spring flap).
      FLAP_PROXIMITY_MCP_PIN=2 (MCP2 GPA2). SPEAKER_PIN=1 (GPIO1).
      E18-D80NK drop sensor is placeholder — confirm type before bracket fabrication.

R-127 (2026-06-17): Wiring tab (satu-wiring.html) is the
      interactive pin-level diagram. All hardware constants in that tool are
      sourced from this HARDWARE_SPEC.md and config.h. Keep in sync.

hardware.h is R2 LOCKED — never modify without explicit owner approval.
All runtime pin constants live in config.h.
hardware.h is the physical abstraction layer.
```

---

## ARDUINO IDE SETTINGS (locked — never change)

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| Flash size | 16MB |
| Partition scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |
| Upload speed | 460800 |
| Arduino core | 2.0.17 ONLY |

---

## CONFIRMED PHYSICAL DEVICE

| Field | Value |
|-------|-------|
| Device ID | SATU-4R473R |
| MAC | 3C:DC:75:5D:DD:2C |
| Status | Active — physical ESP32 on bench |

---

## MULTI-MODEL EXPANSION — PLACEHOLDER

All details in this section are PLACEHOLDER only.
Do not build or wire based on this section without owner confirmation.
5×2 (10-lane) is the only locked spec.

### Model overview
| Model | Grid | Lanes | MCP count | Relay count | IR sensors | Status |
|-------|------|-------|-----------|-------------|------------|--------|
| 5×2 | 5 col × 2 row | 10 | 2 (MCP1+MCP2) | 12 (10 motor + pump + lock) | 10 | ✅ LOCKED — build this |
| 5×3 | 5 col × 3 row | 15 | 3 (MCP1+MCP2+MCP3) | 17 (15 motor + pump + lock) | 15 | 🟡 PLACEHOLDER |
| 7×3 | 7 col × 3 row | 21 | TBD | 23 (21 motor + pump + lock) | 21 | 🟡 PLACEHOLDER |

> ⚠️ Relay count notes:
> All models: 2 physical magnetic locks wired in parallel on one relay (current design).
> If second independent lock relay is needed — add relay 13 (5×3) or relay 24 (7×3).
> Confirm with owner before wiring larger models.

### 5×3 — 15 lanes (PLACEHOLDER)

**Relay count: 17 minimum**
- Relays 1–15: Lane motors
- Relay 16: Water pump
- Relay 17: Magnetic lock (2 locks parallel — same as 5×2)
- Relay 18: Placeholder — reserved for 2nd lock circuit if needed

**MCP requirements (PLACEHOLDER):**
- MCP1 (0x20): Sensors 1–8, Relays 1–6 — same as 5×2
- MCP2 (0x21): Sensors 9–16, Relays 7–12 — sensor capacity extended
- MCP3 (address TBD): Sensors 13–15, Relays 13–18

> ⚠️ MCP3 I2C address: NOT ASSIGNED.
> Confirm A0/A1/A2 jumper setting on breakout board when parts arrive.
> CC must update HARDWARE_SPEC.md and config.h MCP3_ADDR when owner confirms.

**IR sensors: 15× E18-D80NK (placeholder — rail-type system under evaluation)**

**Relay module configuration (PLACEHOLDER):**
- 3× 8-channel relay modules likely sufficient for 17–18 relays (24 channels total)
- Confirm exact module count and wiring before ordering

### 7×3 — 21 lanes (PLACEHOLDER)

**Relay count: 23 minimum**
- Relays 1–21: Lane motors
- Relay 22: Water pump
- Relay 23: Magnetic lock (2 locks parallel)
- Relay 24: Placeholder — reserved for 2nd lock circuit if needed

**MCP requirements (PLACEHOLDER):**
- MCP1 (0x20): Sensors 1–8, Relays 1–6 — same as 5×2
- MCP2 (0x21): Sensors 9–16, Relays 7–12
- MCP3 (address TBD): Sensors 17–21, Relays 13–18
- Additional MCP or relay expander may be required — TBD

> ⚠️ 7×3 requires recalculation of full MCP + relay architecture.
> Do not begin 7×3 wiring without a dedicated spec session.
> Keep as vision placeholder only.

**IR sensors: 21× E18-D80NK (placeholder — rail-type system under evaluation)**

> ⚠️ IR SENSOR NOTE (all models):
> Rail-type drop sensor system under evaluation to reduce wiring complexity.
> A single rail sensor can detect product drop across multiple lanes —
> significantly reducing cable runs for 5×3 and 7×3 builds.
> E18-D80NK (one per lane) is current assumption for 5×2 prototype only.
> Do not finalize sensor type or bracket for larger models until owner confirms.
> When new sensor type is confirmed — update this section and config.h accordingly.

### config.h MACHINE_LANES constant
Current value in config.h:
```cpp
#define MACHINE_LANES  10   // R-163: lane count for current build (10/15/21). 10=5×2, 15=5×3, 21=7×3
```
This constant already supports all three models.
Only change MACHINE_LANES when physically building a different model.
All other code (ui_service.h relay grid, hardware.h sensor arrays) reads from this constant.

---

## FILE HISTORY (this repo)

| File | Status | Notes |
|------|--------|-------|
| satu_wiring_diagram.html | Archived — superseded | Old static diagram, door lock language |
| Satu 1.0 R2.xml | Archived — outdated | Door lock language, pre-R-129 |
| SATU 1.0 R 1 Diagram.drawio | Archived — R1 only | Early revision |
| Satu 1.0 Diagram.pdf | Archived — outdated | Pre-R-128 motor logic |
| Satu design view.jpeg | Keep — physical reference | Frame design |
| Satu 1.0 -2.PNG | Keep — physical reference | Frame design |
| supplier/bom_2025_04.csv | Archived — superseded | BOM table above is current |
| README.md | Superseded by this file | Key facts moved here |
| HARDWARE_TRUTH.md | Archived — renamed | Renamed to HARDWARE_SPEC.md 2026-06-20 |
| HARDWARE_SPEC.md | ✅ CURRENT | This file — single source of truth |
