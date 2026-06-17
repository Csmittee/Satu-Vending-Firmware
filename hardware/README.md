# Satu Hardware — Satu 1.0

> ⚠️ PRIMARY REFERENCE: Read `HARDWARE_TRUTH.md` first.
> This repo will be merged into Satu-Vending-Firmware under `/hardware`.

## Quick Facts
- Board: ESP32-8048S070C (ESP32-S3, 16MB, 8MB PSRAM)
- 10 spring lanes, 2× MCP23017, 10 IR sensors, 12 relays
- Relay 12 = Spring Flap (R-129) — NOT door lock
- Motor stop = sensor-triggered (R-128) — NOT timer-based
- hardware.h is R2 LOCKED in all firmware sessions

## Files
- `HARDWARE_TRUTH.md` — complete hardware reference (AI-readable)
- `Satu design view.jpeg` — physical frame reference
- `Satu 1.0 -2.PNG` — physical frame reference
- `archive/` — old diagrams and BOM (historical reference only)
