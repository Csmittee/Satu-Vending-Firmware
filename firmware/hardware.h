// ============================================================
// hardware.h — Satu Vending Machine Hardware Layer
// Board: ESP32-S3 (ESP32-8048S070C)
// ============================================================
// HARDWARE:
//   MCP1 (0x20): GPA0-7 = IR sensors 1-8  | GPB0-5 = Relays 1-6
//   MCP2 (0x21): GPA0-1 = IR sensors 9-10 | GPB0-5 = Relays 7-10, Pump, Flap
//   LEDs: 40x WS2812B on GPIO5
//         Zone TOP  : LEDs 0-9   (top accent)
//         Zone FL1  : LEDs 10-19 (floor 1 — lanes 1-5)
//         Zone FL2  : LEDs 20-29 (floor 2 — lanes 6-10)
//         Zone DOOR : LEDs 30-39 (door / dispense area)
//
// RELAY WIRING (active HIGH based on config.h):
//   Relay 1-10  = Lane motors (1 per lane)
//   Relay 11    = Water pump (future)
//   Relay 12    = Exit flap solenoid (R-129: HIGH=UNLOCKED, LOW=LOCKED fail-secure)
//
// SENSOR LOGIC:
//   SENSOR_TRIGGERED = LOW  (item breaks IR beam)
//   SENSOR_CLEAR     = HIGH (nothing in beam)
//
// R2 LOCKED — never modify or redeclare anything owned by this file
// RELAY_FLAP defined in config.h (not here — R2 LOCKED)
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
// Relay 11 = pump (GPB3 = pin 11), Relay 12 = exit flap solenoid (GPB5 = pin 13)
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
  {2, 13},   // Relay 12 — Exit flap solenoid (R-129)
};

// RELAY_FLAP defined in config.h — not duplicated here (R2 LOCKED)
#define RELAY_PUMP        11   // 1-indexed relay number for water pump

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
    // R-129: ensure flap is LOCKED (pin extended) on boot
    mcp2.digitalWrite(RELAY_MAP[RELAY_FLAP - 1].pin, RELAY_OFF);
    Serial.println("[HW] MCP2 OK — flap LOCKED on boot");
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
//  FLAP LOCK — R-129 solenoid pin lock
//  RELAY_FLAP = Relay 12 (mcp2, GPB5 = pin 13)
//  HIGH = pin RETRACTED = UNLOCKED (solenoid energized)
//  LOW  = pin EXTENDED  = LOCKED   (solenoid de-energized, spring-held — fail-secure)
// ════════════════════════════════════════════════════════════════════════════

// unlockFlap() — retract pin, allow flap to swing open
// Call at same moment motor starts. Flap stays unlocked for entire vend.
void unlockFlap() {
  setRelay(RELAY_FLAP, true);
  Serial.println("[HW] Flap UNLOCKED — pin retracted");
  setLEDColor(ZONE_DOOR_START, ZONE_DOOR_END, CRGB::Green);
}

// lockFlap() — extend pin, prevent flap from opening
// Call only when proximity confirms flap at rest, or on FLAP_RELOCK_TIMEOUT.
void lockFlap() {
  setRelay(RELAY_FLAP, false);
  Serial.println("[HW] Flap LOCKED — pin extended");
  setLEDColor(ZONE_DOOR_START, ZONE_DOOR_END, CRGB(80, 56, 0));
}

// ════════════════════════════════════════════════════════════════════════════
//  VEND PRODUCT — R-128 sensor-driven motor stop + R-129 pin-lock flap
//  lane: 0-indexed (0-9)
//
//  Sequence:
//    1. unlockFlap() + motor ON — simultaneous
//    2. Spin loop: read IR sensor every SENSOR_POLL_MS
//    3. Sensor fires → motor OFF
//    4. Wait for proximity switch CLOSED → lockFlap()
//    5. Safety: FLAP_RELOCK_TIMEOUT → lockFlap() if proximity never fires
//
//  Returns: true  = item confirmed dropped (sensor triggered)
//           false = lane empty (VEND_MAX_SPIN_MS elapsed, no sensor)
//
//  RELAY POLARITY NOTE:
//    Motor relays 1–10: HIGH=ON  LOW=OFF
//    RELAY_FLAP (12):   HIGH=UNLOCKED  LOW=LOCKED  ← INVERTED — do not confuse
// ════════════════════════════════════════════════════════════════════════════
bool vendProduct(int lane) {
  if (lane < 0 || lane > 9) {
    Serial.printf("[HW] vendProduct: invalid lane %d\n", lane);
    return false;
  }

  int relayNum = lane + 1;

  vendingAnimation(lane);

  // Step 1: unlock flap + start motor simultaneously
  unlockFlap();
  setRelay(relayNum, true);
  Serial.printf("[HW] Relay %d ON — motor SPINNING + flap UNLOCKED\n", relayNum);

  // Step 2: spin loop — sensor poll every SENSOR_POLL_MS
  unsigned long spinStart = millis();
  bool sensorFired = false;

  while (millis() - spinStart < VEND_MAX_SPIN_MS) {
    if (readSensor(lane)) {
      sensorFired = true;
      Serial.printf("[HW] Sensor %d TRIGGERED — item detected after %lums\n",
                    lane + 1, millis() - spinStart);
      break;
    }
    delay(SENSOR_POLL_MS);
  }

  // Step 3: stop motor
  setRelay(relayNum, false);
  if (sensorFired) {
    Serial.printf("[HW] Relay %d OFF — motor stopped (sensor)\n", relayNum);
  } else {
    Serial.printf("[HW] Relay %d OFF — SAFETY CUTOFF %dms — lane %d EMPTY\n",
                  relayNum, VEND_MAX_SPIN_MS, lane + 1);
  }

  // Step 4: wait for proximity switch → lockFlap()
  // Stubs safely when FLAP_PROXIMITY_MCP_PIN = -1 (not yet wired)
  unsigned long flapStart = millis();
  bool proxFired = false;

#if FLAP_PROXIMITY_MCP_PIN >= 0
  while (millis() - flapStart < FLAP_RELOCK_TIMEOUT) {
    // CLOSED = LOW (INPUT_PULLUP, same logic as IR sensors)
    if (mcp2.digitalRead(FLAP_PROXIMITY_MCP_PIN) == LOW) {
      proxFired = true;
      Serial.printf("[HW] Proximity CLOSED — flap at rest after %lums\n",
                    millis() - flapStart);
      break;
    }
    delay(10);
  }
#endif

  // Step 5: lock flap — on proximity or timeout
  lockFlap();
  if (!proxFired) {
    Serial.printf("[HW] Flap re-locked via TIMEOUT (%dms) — proximity not wired or stuck\n",
                  FLAP_RELOCK_TIMEOUT);
  }

  return sensorFired;
}

#endif // HARDWARE_H
