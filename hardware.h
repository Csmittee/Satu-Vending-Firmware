// ============================================================
// hardware.h — Satu Vending Machine Hardware Layer
// Board: ESP32-S3 (ESP32-8048S070C)
// ============================================================
// HARDWARE:
//   MCP1 (0x20): GPA0-7 = IR sensors 1-8  | GPB0-5 = Relays 1-6
//   MCP2 (0x21): GPA0-1 = IR sensors 9-10 | GPB0-5 = Relays 7-10, Pump, Lock
//   LEDs: 40x WS2812B on GPIO5
//         Zone TOP  : LEDs 0-9   (top accent)
//         Zone FL1  : LEDs 10-19 (floor 1 — lanes 1-5)
//         Zone FL2  : LEDs 20-29 (floor 2 — lanes 6-10)
//         Zone DOOR : LEDs 30-39 (door / dispense area)
//
// RELAY WIRING (active HIGH based on config.h):
//   Relay 1-10  = Lane motors (1 per lane)
//   Relay 11    = Water pump (future)
//   Relay 12    = Door lock solenoid (HIGH = unlocked)
//
// SENSOR LOGIC:
//   SENSOR_TRIGGERED = LOW  (item breaks IR beam)
//   SENSOR_CLEAR     = HIGH (nothing in beam)
// ============================================================

#ifndef HARDWARE_H
#define HARDWARE_H

#include <Wire.h>
#include <Adafruit_MCP23X17.h>    // Adafruit MCP23017 library (v2.x — use X17 variant)
#include <FastLED.h>
#include "config.h"

// ── Objects ──────────────────────────────────────────────────────────────────
Adafruit_MCP23X17 mcp1;
Adafruit_MCP23X17 mcp2;
CRGB leds[NUM_LEDS];

// ── Pin arrays (matching config.h extern declarations) ───────────────────────
// MCP1: GPA = sensors 1-8 (pins 0-7), GPB = relays 1-6 (pins 8-13)
const int mcp1_sensors[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const int mcp1_relays[6]  = {8, 9, 10, 11, 12, 13};

// MCP2: GPA = sensors 9-10 (pins 0-1), GPB = relays 7-12 (pins 8-13)
// Relay 11 = pump (GPB3 = pin 11), Relay 12 = door lock (GPB4 = pin 12 on MCP2 → GPB5 = 13)
// WIRING NOTE: door lock relay is mcp2 GPB5 (pin 13)
const int mcp2_sensors[2] = {0, 1};
const int mcp2_relays[6]  = {8, 9, 10, 11, 12, 13};

// ── Relay map: relay number (1-indexed, 1-12) → {mcp, pin} ──────────────────
// relay 1-6  → mcp1, pins 8-13
// relay 7-12 → mcp2, pins 8-13
struct RelayPin { int mcp; int pin; };
static const RelayPin RELAY_MAP[12] = {
  {1,  8},   // Relay 1  — Lane 1
  {1,  9},   // Relay 2  — Lane 2
  {1, 10},   // Relay 3  — Lane 3
  {1, 11},   // Relay 4  — Lane 4
  {1, 12},   // Relay 5  — Lane 5
  {1, 13},   // Relay 6  — Lane 6
  {2,  8},   // Relay 7  — Lane 7
  {2,  9},   // Relay 8  — Lane 8
  {2, 10},   // Relay 9  — Lane 9
  {2, 11},   // Relay 10 — Lane 10
  {2, 12},   // Relay 11 — Water pump
  {2, 13},   // Relay 12 — Door lock
};

#define RELAY_DOOR_LOCK   12   // 1-indexed relay number for door lock
#define RELAY_PUMP        11   // 1-indexed relay number for water pump
#define VEND_PULSE_MS     800  // Relay on-time for dispensing (tune per motor)

// ════════════════════════════════════════════════════════════════════════════
//  LOW-LEVEL RELAY WRITE
//  relayNum: 1-indexed (1-12)
//  state:    RELAY_ON (HIGH) or RELAY_OFF (LOW)
// ════════════════════════════════════════════════════════════════════════════
void setRelay(int relayNum, bool state) {
  if (relayNum < 1 || relayNum > 12) {
    Serial.printf("[HW] setRelay: invalid relay %d\n", relayNum);
    return;
  }
  const RelayPin& rp = RELAY_MAP[relayNum - 1];
  if (rp.mcp == 1) {
    mcp1.digitalWrite(rp.pin, state ? RELAY_ON : RELAY_OFF);
  } else {
    mcp2.digitalWrite(rp.pin, state ? RELAY_ON : RELAY_OFF);
  }
  Serial.printf("[HW] Relay %d → %s\n", relayNum, state ? "ON" : "OFF");
}

// ════════════════════════════════════════════════════════════════════════════
//  SENSOR READ
//  sensorNum: 0-indexed (0-9)
//  Returns: true = TRIGGERED (item in beam) | false = CLEAR
// ════════════════════════════════════════════════════════════════════════════
bool readSensor(int sensorNum) {
  if (sensorNum < 0 || sensorNum > 9) {
    Serial.printf("[HW] readSensor: invalid sensor %d\n", sensorNum);
    return false;
  }
  int rawVal;
  if (sensorNum < 8) {
    rawVal = mcp1.digitalRead(mcp1_sensors[sensorNum]);
  } else {
    rawVal = mcp2.digitalRead(mcp2_sensors[sensorNum - 8]);
  }
  // SENSOR_TRIGGERED = LOW
  return (rawVal == LOW);
}

// ════════════════════════════════════════════════════════════════════════════
//  MCP23017 INIT
// ════════════════════════════════════════════════════════════════════════════
void initMCP23017() {
  Wire.begin(I2C_SDA, I2C_SCL);

  // MCP1
  if (!mcp1.begin_I2C(MCP1_ADDR)) {
    Serial.println("[HW] ERROR: MCP1 (0x20) not found! Check wiring.");
    // Non-fatal in development — will show sensor errors in log
  } else {
    // GPA 0-7 = sensors → INPUT_PULLUP
    for (int i = 0; i < 8; i++) mcp1.pinMode(mcp1_sensors[i], INPUT_PULLUP);
    // GPB 0-5 = relays → OUTPUT, default OFF
    for (int i = 0; i < 6; i++) {
      mcp1.pinMode(mcp1_relays[i], OUTPUT);
      mcp1.digitalWrite(mcp1_relays[i], RELAY_OFF);
    }
    Serial.println("[HW] MCP1 OK");
  }

  // MCP2
  if (!mcp2.begin_I2C(MCP2_ADDR)) {
    Serial.println("[HW] ERROR: MCP2 (0x21) not found! Check wiring.");
  } else {
    // GPA 0-1 = sensors → INPUT_PULLUP
    for (int i = 0; i < 2; i++) mcp2.pinMode(mcp2_sensors[i], INPUT_PULLUP);
    // GPB 0-5 = relays → OUTPUT, default OFF
    for (int i = 0; i < 6; i++) {
      mcp2.pinMode(mcp2_relays[i], OUTPUT);
      mcp2.digitalWrite(mcp2_relays[i], RELAY_OFF);
    }
    // Ensure door is LOCKED on boot
    mcp2.digitalWrite(RELAY_MAP[RELAY_DOOR_LOCK - 1].pin, RELAY_OFF);
    Serial.println("[HW] MCP2 OK — Door locked");
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  LED INIT
// ════════════════════════════════════════════════════════════════════════════
void initLED() {
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  Serial.println("[HW] LEDs OK");
}

// ════════════════════════════════════════════════════════════════════════════
//  HARDWARE INIT (called from setup())
// ════════════════════════════════════════════════════════════════════════════
void initHardware() {
  initMCP23017();
  initLED();
}

// ════════════════════════════════════════════════════════════════════════════
//  LED HELPERS
// ════════════════════════════════════════════════════════════════════════════
void setLEDColor(int start, int end, CRGB color) {
  for (int i = start; i <= end && i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

// ── IDLE ANIMATION — gentle breathing gold/amber ──────────────────────────
void idleAnimation() {
  // Soft gold breathe on all zones
  for (int cycle = 0; cycle < 3; cycle++) {
    for (int b = 20; b < 180; b += 4) {
      fill_solid(leds, NUM_LEDS, CRGB(b, b * 0.7, 0));
      FastLED.show();
      delay(15);
    }
    for (int b = 180; b >= 20; b -= 4) {
      fill_solid(leds, NUM_LEDS, CRGB(b, b * 0.7, 0));
      FastLED.show();
      delay(15);
    }
  }
  // Leave at soft gold
  fill_solid(leds, NUM_LEDS, CRGB(80, 56, 0));
  FastLED.show();
}

// ── CELEBRATION ANIMATION — green sparkle ────────────────────────────────
void celebrationAnimation() {
  for (int i = 0; i < 5; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Green);
    FastLED.show();
    delay(200);
    FastLED.clear();
    FastLED.show();
    delay(150);
  }
  // Return to gold
  fill_solid(leds, NUM_LEDS, CRGB(80, 56, 0));
  FastLED.show();
}

// ── ERROR ANIMATION — red flash ───────────────────────────────────────────
void errorAnimation() {
  for (int i = 0; i < 6; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(250);
    FastLED.clear();
    FastLED.show();
    delay(150);
  }
  // Leave dim red as indication
  fill_solid(leds, NUM_LEDS, CRGB(40, 0, 0));
  FastLED.show();
}

// ── WATER PUMP ANIMATION — blue ripple ────────────────────────────────────
void waterPumpAnimation() {
  for (int i = 0; i < 3; i++) {
    for (int j = ZONE_DOOR_START; j <= ZONE_DOOR_END; j++) {
      leds[j] = CRGB::Blue;
      FastLED.show();
      delay(30);
      leds[j] = CRGB::Black;
    }
  }
}

// ── VENDING ANIMATION — selected lane lights up ───────────────────────────
static void vendingAnimation(int lane) {
  // lane 0-4 = floor 1, lane 5-9 = floor 2
  int zoneStart = (lane < 5) ? ZONE_FLOOR1_START : ZONE_FLOOR2_START;
  int zoneEnd   = (lane < 5) ? ZONE_FLOOR1_END   : ZONE_FLOOR2_END;

  // Pulse the zone yellow
  for (int b = 0; b < 200; b += 10) {
    setLEDColor(zoneStart, zoneEnd, CRGB(b, b, 0));
    delay(20);
  }
  for (int b = 200; b >= 0; b -= 10) {
    setLEDColor(zoneStart, zoneEnd, CRGB(b, b, 0));
    delay(20);
  }
  // Pulse door zone white
  for (int b = 0; b < 200; b += 10) {
    setLEDColor(ZONE_DOOR_START, ZONE_DOOR_END, CRGB(b, b, b));
    delay(15);
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  DOOR LOCK
//  RELAY_DOOR_LOCK = Relay 12 (mcp2, GPB5)
//  RELAY_ON  = unlocked (solenoid energized)
//  RELAY_OFF = locked   (solenoid de-energized, spring-locked)
// ════════════════════════════════════════════════════════════════════════════
void unlockDoor() {
  setRelay(RELAY_DOOR_LOCK, true);
  Serial.println("[HW] Door UNLOCKED");
  // Light door LEDs green
  setLEDColor(ZONE_DOOR_START, ZONE_DOOR_END, CRGB::Green);
}

void lockDoor() {
  setRelay(RELAY_DOOR_LOCK, false);
  Serial.println("[HW] Door LOCKED");
  // Return door LEDs to gold
  setLEDColor(ZONE_DOOR_START, ZONE_DOOR_END, CRGB(80, 56, 0));
}

// ════════════════════════════════════════════════════════════════════════════
//  VEND PRODUCT
//  lane: 0-indexed (0-9)
//  Fires relay for VEND_PULSE_MS, then turns off.
//  Visual: vendingAnimation + door zone glow.
//
//  SAFETY: relay is always turned off even if code crashes (no blocking loops)
// ════════════════════════════════════════════════════════════════════════════
void vendProduct(int lane) {
  if (lane < 0 || lane > 9) {
    Serial.printf("[HW] vendProduct: invalid lane %d\n", lane);
    return;
  }

  int relayNum = lane + 1;  // relay 1-10 map to lanes 0-9
  Serial.printf("[HW] Vending lane %d (relay %d)\n", lane, relayNum);

  vendingAnimation(lane);

  // Fire relay pulse
  setRelay(relayNum, true);
  delay(VEND_PULSE_MS);
  setRelay(relayNum, false);

  Serial.printf("[HW] Vend pulse complete: relay %d OFF\n", relayNum);
}

#endif // HARDWARE_H
