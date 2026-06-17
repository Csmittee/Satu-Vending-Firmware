# HARDWARE_TRUTH.md — Satu 1.0
> Single source of truth for all Satu 1.0 hardware decisions.
> Last updated: 2026-06-17
> Supersedes: README.md, satu_wiring_diagram.html, Satu 1.0 R2.xml
> R-128, R-129 applied — see FIRMWARE RULES section

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

### MCP2 — Address 0x21 (A0=VCC, A1=GND, A2=GND)

| MCP Pin | Direction | Maps to | Lane |
|---------|-----------|---------|------|
| GPA0 (pin 0) | INPUT_PULLUP | IR Sensor 9  | Lane 9 drop detect |
| GPA1 (pin 1) | INPUT_PULLUP | IR Sensor 10 | Lane 10 drop detect |
| GPA2–GPA7    | — | Spare / expansion | — |
| GPB0 (pin 8)  | OUTPUT | Relay 7  | Lane 7 motor |
| GPB1 (pin 9)  | OUTPUT | Relay 8  | Lane 8 motor |
| GPB2 (pin 10) | OUTPUT | Relay 9  | Lane 9 motor |
| GPB3 (pin 11) | OUTPUT | Relay 10 | Lane 10 motor |
| GPB4 (pin 12) | OUTPUT | Relay 11 | Water Pump |
| GPB5 (pin 13) | OUTPUT | Relay 12 | Spring Flap ⚠️ see R-129 |

### MCP3 — Address 0x22 — RESERVED / NOT POPULATED
Used only in 21-lane build. Do not populate for 10-lane Satu 1.0.

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
| Mount position | 5–8cm below each spring shelf |
| Connector | JST-PH 2.0mm 3-pin [VCC\|GND\|OUT] |
| Wire colors | RED(5V) \| BLACK \| BLUE |

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

## SPRING FLAP — RELAY 12 (R-129)

| Field | Value |
|-------|-------|
| Function | Dispense gate — product drops into pickup bay |
| Relay | Relay 12 (MCP2 GPB5 pin 13) |
| Type | Normally-closed spring flap |
| Operation | Energize 300ms → opens → spring closes automatically |
| FLAP_PULSE_MS | 300ms |
| When triggered | After IR sensor confirms product drop |
| Pickup bay door | Passive — no lock, no sensor needed |
| Connector | JST-VH 3.96mm 2-pin [VCC+\|GND] |
| Wire colors | WHITE \| BROWN |

> ⚠️ R-129 (2026-06-17): This replaces door lock solenoid design.
> No REMOVAL_TIMEOUT. No waiting for item pickup.
> STATE_WAITING_REMOVAL is deleted from firmware.

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
| 12V | 12V | 10A | Spring motors, water pump, spring flap solenoid |
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
| Spring flap / pump load | JST-VH 3.96mm 2-pin | [VCC+\|GND] | WHITE\|BROWN |

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

## BILL OF MATERIALS — SATU 1.0 (10-lane)

| Component | Model | Qty | Est. THB |
|-----------|-------|-----|---------|
| ESP32-S3 display board | ESP32-8048S070C | 1 | 1600 |
| MCP23017 I2C expander | 16-bit I2C | 2 | 240 |
| IR obstacle sensor | E18-D80NK | 10 | 450 |
| 8-ch relay module 5V | Opto-isolated | 1 | 220 |
| 2-ch relay module 5V | Opto-isolated | 1 | 80 |
| Spring coil motor | Vending coil | 10 | TBD |
| Spring flap solenoid | Normally-closed | 1 | TBD |
| Water pump | 12V DC | 1 | TBD |
| WS2812B LED strip | 60 LED/m | 1m | 150 |
| 5V 5A PSU | Regulated | 1 | 250 |
| 12V 10A PSU | Regulated | 1 | 450 |
| 4.7kΩ resistor 1/4W | — | 2 | 5 |
| 300Ω resistor 1/4W | — | 1 | 3 |
| 100µF capacitor 10V+ | Electrolytic | 1 | 10 |
| JST-PH 2.0mm 4-pin (M+F) | Pre-crimped | 4 pairs | 80 |
| JST-PH 2.0mm 3-pin (M+F) | Pre-crimped | 12 pairs | 120 |
| JST-XH 2.54mm 2-pin (M+F) | Pre-crimped | 16 pairs | 80 |
| JST-VH 3.96mm 2-pin (M+F) | Pre-crimped | 16 pairs | 160 |
| Metal frame + acrylic door | 1.5mm steel | 1 | 3000 |
| **TOTAL (estimated)** | | | **~7000+** |

> Note: Spring motors and spring flap solenoid costs TBD pending supplier confirmation.
> Previous BOM (bom_2025_04.csv) total ~8070 THB — treat as reference only.

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
| W-07 | Spring flap polarity — Relay 12 | Reversed polarity locks flap open permanently |

---

## FIRMWARE RULES — HARDWARE-LINKED

These firmware rules directly affect hardware behaviour.
Any wiring or component change must be checked against these rules.

```
R-128 (2026-06-17): Motor stop is sensor-driven. VEND_PULSE_MS deleted.
      vendProduct() returns bool. Motor runs until IR sensor triggers
      OR VEND_MAX_SPIN_MS=30000ms safety cutoff. Lane empty → disabled → backend alert.
      NON-NEGOTIABLE. Timer-based motor stop permanently prohibited.

R-129 (2026-06-17): Relay 12 = spring flap. NOT door lock solenoid.
      openFlap() pulses relay 300ms. Spring closes automatically.
      No STATE_WAITING_REMOVAL. No REMOVAL_TIMEOUT. Pickup bay door is passive.

R-127 (2026-06-17): Wiring tab (Tab 4 in satu-machine-builder.html) is the
      interactive pin-level diagram. All hardware constants in that tool are
      sourced from this HARDWARE_TRUTH.md and config.h. Keep in sync.

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
| HARDWARE_TRUTH.md | ✅ CURRENT | This file — single source of truth |
