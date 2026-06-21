# CC_BUILD_PROMPT_hardware_spec_v1_2_FINAL.md
> Session: Hardware wiring review + multi-model expansion — 2026-06-21
> Replaces: CC_BUILD_PROMPT_hardware_spec_v1_2.md (last session draft)
> Touches: hardware/HARDWARE_SPEC.md + firmware/config.h ONLY
> hardware.h — R2 LOCKED — do not touch
> Flash cycles: 1 (config.h changes require flash)

---

## SECTION 1 — READ FIRST

Before writing anything, read in full:
1. hardware/HARDWARE_SPEC.md (current v1.1 — rewrite target)
2. firmware/hardware.h (R2 LOCKED — source of truth for pin map)
3. firmware/config.h (2 constants need update)
4. RULES.md

State: "Files read: [list]. Proceeding."
If any file is missing — stop and write in CC_CHAT_LOG before proceeding.

---

## SECTION 2 — MODEL NAMING CONVENTION (locked 2026-06-21)

| Model | Grid format | Lane count | Status |
|-------|-------------|------------|--------|
| 5×2 | 5 col × 2 row | 10 lanes | CURRENT BUILD — all spec details locked |
| 5×3 | 5 col × 3 row | 15 lanes | PLANNED — placeholder spec |
| 7×3 | 7 col × 3 row | 21 lanes | PLANNED — placeholder spec |

Grid format = COLUMNS × ROWS. Never reverse.

---

## SECTION 3 — CHANGES TO HARDWARE/HARDWARE_SPEC.md

### 3-HEADER — Version bump

Change version header from:
```
> Version 1.1 — 2026-06-20
```
To:
```
> Version 1.2 — 2026-06-21
> Changes: Relay 12 corrected to magnetic pin-lock solenoid. Proximity switch added MCP2 GPA2.
>          Speaker GPIO1 assigned. IR sensor mount updated. Wire harness table added.
>          Multi-model section added: 5×2 (current), 5×3 and 7×3 (placeholder).
>          BOM updated. W-07 corrected. MCP RESET pin warning added.
> Previous: v1.1 — 2026-06-20
```

### 3-CHANGELOG — Add entry at top of CHANGE LOG table

```
| 2026-06-21 | v1.2 — Relay 12 corrected to magnetic pin-lock solenoid (two locks parallel). Proximity switch MCP2 GPA2 added. Speaker GPIO1 assigned. IR mount updated. Wire harness table added. Multi-model placeholder (5×3, 7×3) added. BOM updated. W-07 corrected. | Owner+Chat |
```

### 3A — Delete section "SPRING FLAP — RELAY 12 (R-129)"

Remove the entire section with that title.
Replace with:

```markdown
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
```

### 3B — Add new section after MAGNETIC PIN-LOCK: PROXIMITY SWITCH

```markdown
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
```

### 3C — Update IR SENSORS section

Change mount position row from:
```
| Mount position | 5–8cm below each spring shelf |
```
To:
```
| Mount position | Under tray floor bracket, one per lane, facing product drop path |
```

Add note under the table:
```
> ⚠️ PLACEHOLDER — E18-D80NK retained for 5×2 prototype.
> Rail-type drop sensor system under evaluation to reduce wiring.
> Do not finalize mount bracket or cable run until sensor type confirmed.
> Current assumption: 1× E18-D80NK per lane.
```

### 3D — Add SPEAKER section after WATER PUMP section

```markdown
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
```

### 3E — Add MCP RESET PIN warning to MCP23017 section

Add this warning block directly below the MCP1 table AND below the MCP2 table:

```
> ⚠️ MCP RESET PIN (DIP pin 18) must be tied to 3.3V.
> If floating → MCP resets randomly under electrical noise → all relays/sensors lost.
> Verify breakout board ties RESET to VCC internally.
> If not — add 10kΩ pull-up resistor from RESET pin to 3.3V before first power-on.
```

### 3F — Update MCP2 GPA2 table row

In MCP2 table, change:
```
| GPA2–GPA7    | — | Spare / expansion | — |
```
To:
```
| GPA2 (pin 2) | INPUT_PULLUP | Proximity switch | Door closed detect — FLAP_PROXIMITY_MCP_PIN=2 |
| GPA3–GPA7    | — | Spare / expansion | — |
```

### 3G — Update W-07 in ELECTRONICS WARNINGS table

Change:
```
| W-07 | Spring flap polarity — Relay 12 | Reversed polarity locks flap open permanently |
```
To:
```
| W-07 | Magnetic lock polarity — Relay 12 | HIGH=UNLOCKED, LOW=LOCKED. Reversed polarity = door unlocked at rest = fail-open = security risk. Verify before power-on. |
```

### 3H — Add WIRE HARNESS section before BOM

```markdown
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
```

### 3I — Update BOM

Remove this row:
```
| Spring flap solenoid | Normally-closed | 1 | TBD |
```

Replace with:
```
| Magnetic pin-lock solenoid 12V | Fail-secure | 2 | TBD |
```

Add these new rows:
```
| Roller microswitch SPDT | — | 1 | 20 |
| JST-PH 2.0mm 2-pin pre-crimped | — | 3 pairs | 30 |
| JST-SM 2.54mm 3-pin pre-crimped | — | 2 pairs | 40 |
| 10kΩ resistor 1/4W | MCP RESET pull-up | 2 | 5 |
```

### 3J — Add MULTI-MODEL EXPANSION section (new — add after CONFIRMED PHYSICAL DEVICE)

```markdown
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
```

---

## SECTION 4 — CHANGES TO FIRMWARE/CONFIG.H

Two constants only. Verify each is present as -1 stub before changing.

```cpp
// Change from:
#define FLAP_PROXIMITY_MCP_PIN   -1

// Change to:
#define FLAP_PROXIMITY_MCP_PIN    2   // MCP2 GPA2 — roller microswitch, CLOSED=LOW (INPUT_PULLUP)

// Change from:
#define SPEAKER_PIN              -1

// Change to:
#define SPEAKER_PIN               1   // GPIO1 — passive speaker PWM
```

Bump config.h version comment to R15.

---

## SECTION 5 — DO NOT TOUCH

- firmware/hardware.h — R2 LOCKED — zero changes
- Any src/ backend files
- Any public/ HTML files (satu-wiring.html Spring Flap language — separate session)
- satu_vending.ino
- wrangler.toml
- PAYMENT_MODE — stays fake

---

## SECTION 6 — VERIFICATION CHECKLIST

CC confirms before closing:

- [ ] HARDWARE_SPEC.md version header = v1.2
- [ ] "SPRING FLAP" section title is gone — replaced by "MAGNETIC PIN-LOCK"
- [ ] PROXIMITY SWITCH section exists — FLAP_PROXIMITY_MCP_PIN = 2
- [ ] MCP2 table: GPA2 row updated to proximity switch
- [ ] SPEAKER section exists — GPIO1
- [ ] MCP RESET pin warning under MCP1 and MCP2 tables
- [ ] W-07 updated — magnetic lock language, fail-open risk stated
- [ ] IR mount updated — under tray bracket + placeholder note
- [ ] Wire harness summary table present
- [ ] BOM: spring flap solenoid removed, 2× mag lock added, roller switch added
- [ ] MULTI-MODEL EXPANSION section present — 5×2/5×3/7×3 table
- [ ] MCP3 address marked as TBD in 5×3 section
- [ ] 7×3 marked as vision placeholder only
- [ ] IR rail-type sensor note present in multi-model section
- [ ] config.h FLAP_PROXIMITY_MCP_PIN = 2
- [ ] config.h SPEAKER_PIN = 1
- [ ] config.h version bumped to R15
- [ ] HARDWARE_SPEC.md CHANGE LOG entry added at top

---

## SECTION 7 — FIRMWARE IMPACT NOTE

These are NOT part of this prompt — flag for next firmware session:
- hardware.h unlockFlap() / lockFlap() already implement HIGH=UNLOCKED / LOW=LOCKED — correct, no change needed
- FLAP_PROXIMITY_MCP_PIN stub compiles safely at -1 — activates automatically after config.h change to 2
- After flash: verify Serial shows `[HW] Proximity polling ACTIVE on MCP2 pin 2`
- satu-wiring.html still shows "Spring Flap" language — needs update in separate session (D-9 open item)

---

## SECTION 8 — MANDATORY CLOSING TASKS

1. Write CC_CHAT_LOG entry at TOP
2. Append to RULES.md at TOP:
   ```
   R-165: HARDWARE_SPEC.md v1.2 is hardware wiring source of truth from 2026-06-21.
          Relay 12 = magnetic pin-lock solenoid (NOT spring flap).
          Two physical locks parallel-wired on relay 12.
          FLAP_PROXIMITY_MCP_PIN = 2 (MCP2 GPA2). SPEAKER_PIN = 1 (GPIO1).
          E18-D80NK drop sensor is placeholder — confirm type before bracket fabrication.
          Model naming: 5×2=10 lanes, 5×3=15 lanes, 7×3=21 lanes (col×row).
          MCP3 address for 5×3/7×3 = TBD — owner confirms when parts arrive.
   ```
3. Update PROJECT_STATE.md — hardware wiring locked 2026-06-21
4. Archive this prompt → docs/prompts/ stamped ✅ COMPLETE — 2026-06-21 — HARDWARE_SPEC v1.2
5. Bump version header on every file changed
6. Commit and merge

---

## SECTION 9 — FLASH REQUIRED?

YES — config.h changes (FLAP_PROXIMITY_MCP_PIN + SPEAKER_PIN). 1 flash cycle.
CI must pass first.
Owner verifies Serial output shows:
`[HW] Proximity polling ACTIVE on MCP2 pin 2`

---

## OPEN ITEMS NOT IN THIS PROMPT (for next session)

| Item | Notes |
|------|-------|
| satu-wiring.html Spring Flap → Magnetic Lock | D-9 — separate session |
| MCP3 address for 5×3 | Owner confirms A0/A1/A2 when parts arrive |
| Rail-type IR sensor selection | Owner sourcing — update spec when confirmed |
| 5×3 full wiring spec | Separate session when ready to build |
| 7×3 full wiring spec | Separate session — vision only for now |
