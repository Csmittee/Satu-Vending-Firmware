// ============================================================
// satu_observer.ino — Satu Machine Observer Firmware
// ============================================================
// PURPOSE:
//   Passive I2C bus monitor. Sits on the same I2C bus as the
//   main SATU-4R473R ESP32 and reads MCP23017 register states
//   every 200ms. Prints human-readable changes to Serial only.
//   ZERO interference with main machine — read-only observer.
//
// HARDWARE:
//   Any ESP32 or ESP32-S3 dev board (cheap bare module fine)
//   3 wires to main machine:
//     Observer GPIO19 → Main ESP32 GPIO19 (SDA)
//     Observer GPIO20 → Main ESP32 GPIO20 (SCL)
//     Observer GND    → Main ESP32 GND     ← CRITICAL
//
// WIRING REFERENCE (from Satu hardware.h / satu_wiring_diagram.html):
//   MCP1 addr 0x20: GPA0-7 = IR sensors 1-8  | GPB0-5 = Relays 1-6
//   MCP2 addr 0x21: GPA0-1 = IR sensors 9-10 | GPB0-5 = Relays 7-12
//   Relay 11 = Water pump | Relay 12 = Door lock solenoid
//   SENSOR_TRIGGERED = LOW (item breaks IR beam)
//   RELAY_ON = HIGH (active HIGH relay board)
//
// SERIAL OUTPUT: 115200 baud
//   Opens two Serial Monitor windows side by side:
//   Left  = main machine (existing SATU-4R473R logs)
//   Right = this observer (plain English signal changes)
//
// SETUP:
//   Board: Any ESP32 or ESP32-S3 Dev Module in Arduino IDE
//   No extra libraries needed — uses Wire directly (no MCP lib)
//   Upload speed: 115200 or higher
//
// ARDUINO IDE SETTINGS:
//   Same core as main machine (ESP32 2.0.17) but any board works
//   No PSRAM needed | No special partition
// ============================================================

#include <Wire.h>

// ── I2C Pins (match main machine GPIO19/20) ──────────────────
#define OBS_SDA  19
#define OBS_SCL  20
#define OBS_FREQ 100000   // 100kHz — conservative, safe for shared bus

// ── MCP23017 Register Addresses ──────────────────────────────
#define MCP_IODIRA  0x00  // Port A direction
#define MCP_IODIRB  0x01  // Port B direction
#define MCP_GPIOA   0x12  // Port A pin states (read sensors here)
#define MCP_GPIOB   0x13  // Port B pin states (read relays here)

#define MCP1_ADDR   0x20
#define MCP2_ADDR   0x21

// ── Poll interval ─────────────────────────────────────────────
#define POLL_MS     200   // Read every 200ms — fast enough to catch events

// ── Previous state (to detect changes only) ──────────────────
uint8_t prev_mcp1_a = 0xFF;   // GPA = sensors 1-8   (HIGH = CLEAR)
uint8_t prev_mcp1_b = 0x00;   // GPB = relays 1-6    (HIGH = ON)
uint8_t prev_mcp2_a = 0xFF;   // GPA = sensors 9-10  (HIGH = CLEAR)
uint8_t prev_mcp2_b = 0x00;   // GPB = relays 7-12   (HIGH = ON)

bool mcp1_ok = false;
bool mcp2_ok = false;

// ── Relay names (1-indexed, matches RELAY_MAP in hardware.h) ─
const char* RELAY_NAMES[12] = {
  "Lane 1 Motor",
  "Lane 2 Motor",
  "Lane 3 Motor",
  "Lane 4 Motor",
  "Lane 5 Motor",
  "Lane 6 Motor",
  "Lane 7 Motor",
  "Lane 8 Motor",
  "Lane 9 Motor",
  "Lane 10 Motor",
  "Water Pump",
  "Door Lock"
};

// ────────────────────────────────────────────────────────────
//  I2C READ HELPER
// ────────────────────────────────────────────────────────────
bool readMCPReg(uint8_t addr, uint8_t reg, uint8_t &val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(addr, (uint8_t)1) != 1) return false;
  val = Wire.read();
  return true;
}

// ────────────────────────────────────────────────────────────
//  PRINT BIT CHANGE — called when a bit flips
// ────────────────────────────────────────────────────────────
void printSensorChange(int sensorNum, bool triggered) {
  // sensorNum: 1-10
  if (triggered) {
    Serial.printf("[OBS] IR%d → BLOCKED  (item in beam)\n", sensorNum);
  } else {
    Serial.printf("[OBS] IR%d → CLEAR    (beam free)\n", sensorNum);
  }
}

void printRelayChange(int relayNum, bool on) {
  // relayNum: 1-12
  const char* name = (relayNum >= 1 && relayNum <= 12)
                     ? RELAY_NAMES[relayNum - 1]
                     : "Unknown";
  if (on) {
    Serial.printf("[OBS] Relay %-2d ON   → %s\n", relayNum, name);
  } else {
    Serial.printf("[OBS] Relay %-2d OFF  → %s\n", relayNum, name);
  }
}

// ────────────────────────────────────────────────────────────
//  DIFF TWO BYTES — print what changed
// ────────────────────────────────────────────────────────────
void diffSensors(uint8_t prev, uint8_t curr,
                 int baseNum, int bitCount) {
  // baseNum: 1-indexed sensor number of bit 0
  uint8_t changed = prev ^ curr;
  for (int i = 0; i < bitCount; i++) {
    if (changed & (1 << i)) {
      bool triggered = !((curr >> i) & 1);  // LOW = triggered
      printSensorChange(baseNum + i, triggered);
    }
  }
}

void diffRelays(uint8_t prev, uint8_t curr,
                int baseNum, int bitCount) {
  // baseNum: 1-indexed relay number of bit 0
  uint8_t changed = prev ^ curr;
  for (int i = 0; i < bitCount; i++) {
    if (changed & (1 << i)) {
      bool on = (curr >> i) & 1;  // HIGH = ON
      printRelayChange(baseNum + i, on);
    }
  }
}

// ────────────────────────────────────────────────────────────
//  PRINT FULL SNAPSHOT — called once at boot and on request
// ────────────────────────────────────────────────────────────
void printSnapshot(uint8_t m1a, uint8_t m1b,
                   uint8_t m2a, uint8_t m2b) {
  Serial.println("[OBS] ─────────── SNAPSHOT ───────────");

  // IR Sensors 1-8 from MCP1 GPA
  Serial.print("[OBS] IR Sensors:  ");
  for (int i = 0; i < 8; i++) {
    bool triggered = !((m1a >> i) & 1);
    Serial.printf("IR%d:%s ", i + 1, triggered ? "BLK" : "CLR");
  }
  // IR Sensors 9-10 from MCP2 GPA bits 0-1
  for (int i = 0; i < 2; i++) {
    bool triggered = !((m2a >> i) & 1);
    Serial.printf("IR%d:%s ", i + 9, triggered ? "BLK" : "CLR");
  }
  Serial.println();

  // Relays 1-6 from MCP1 GPB bits 0-5
  Serial.print("[OBS] Relays 1-6:  ");
  for (int i = 0; i < 6; i++) {
    bool on = (m1b >> i) & 1;
    Serial.printf("R%d:%s ", i + 1, on ? "ON " : "OFF");
  }
  Serial.println();

  // Relays 7-12 from MCP2 GPB bits 0-5
  Serial.print("[OBS] Relays 7-12: ");
  for (int i = 0; i < 6; i++) {
    bool on = (m2b >> i) & 1;
    Serial.printf("R%d:%s ", i + 7, on ? "ON " : "OFF");
  }
  Serial.println();

  // Special devices
  bool doorUnlocked = (m2b >> 5) & 1;  // Relay 12 = bit 5 of MCP2 GPB
  bool pumpOn       = (m2b >> 4) & 1;  // Relay 11 = bit 4 of MCP2 GPB
  Serial.printf("[OBS] Door Lock:   %s\n", doorUnlocked ? "UNLOCKED" : "LOCKED");
  Serial.printf("[OBS] Water Pump:  %s\n", pumpOn ? "ON" : "OFF");
  Serial.println("[OBS] ──────────────────────────────────");
}

// ────────────────────────────────────────────────────────────
//  SETUP
// ────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("[OBS] ════════════════════════════════");
  Serial.println("[OBS] SATU Observer — passive I2C monitor");
  Serial.println("[OBS] Watching MCP1 (0x20) + MCP2 (0x21)");
  Serial.println("[OBS] SDA=GPIO19  SCL=GPIO20  GND=GND");
  Serial.println("[OBS] Poll interval: 200ms");
  Serial.println("[OBS] Only changes are printed after boot snapshot");
  Serial.println("[OBS] ════════════════════════════════");

  Wire.begin(OBS_SDA, OBS_SCL);
  Wire.setClock(OBS_FREQ);
  delay(100);

  // Check MCP presence
  Wire.beginTransmission(MCP1_ADDR);
  mcp1_ok = (Wire.endTransmission() == 0);
  Wire.beginTransmission(MCP2_ADDR);
  mcp2_ok = (Wire.endTransmission() == 0);

  Serial.printf("[OBS] MCP1 (0x20): %s\n", mcp1_ok ? "FOUND ✓" : "NOT FOUND ✗ — check wiring");
  Serial.printf("[OBS] MCP2 (0x21): %s\n", mcp2_ok ? "FOUND ✓" : "NOT FOUND ✗ — check wiring");

  if (!mcp1_ok && !mcp2_ok) {
    Serial.println("[OBS] WARNING: No MCPs found. Retrying every 2s...");
  }

  // Read initial state
  uint8_t m1a = 0xFF, m1b = 0x00, m2a = 0xFF, m2b = 0x00;
  if (mcp1_ok) {
    readMCPReg(MCP1_ADDR, MCP_GPIOA, m1a);
    readMCPReg(MCP1_ADDR, MCP_GPIOB, m1b);
  }
  if (mcp2_ok) {
    readMCPReg(MCP2_ADDR, MCP_GPIOA, m2a);
    readMCPReg(MCP2_ADDR, MCP_GPIOB, m2b);
  }

  prev_mcp1_a = m1a;
  prev_mcp1_b = m1b;
  prev_mcp2_a = m2a;
  prev_mcp2_b = m2b;

  if (mcp1_ok || mcp2_ok) {
    printSnapshot(m1a, m1b, m2a, m2b);
  }

  Serial.println("[OBS] Watching for changes...");
}

// ────────────────────────────────────────────────────────────
//  LOOP
// ────────────────────────────────────────────────────────────
void loop() {
  delay(POLL_MS);

  uint8_t m1a = prev_mcp1_a;
  uint8_t m1b = prev_mcp1_b;
  uint8_t m2a = prev_mcp2_a;
  uint8_t m2b = prev_mcp2_b;

  bool read_ok = false;

  // Re-check MCP presence if previously missing
  if (!mcp1_ok) {
    Wire.beginTransmission(MCP1_ADDR);
    mcp1_ok = (Wire.endTransmission() == 0);
    if (mcp1_ok) Serial.println("[OBS] MCP1 (0x20) now found ✓");
  }
  if (!mcp2_ok) {
    Wire.beginTransmission(MCP2_ADDR);
    mcp2_ok = (Wire.endTransmission() == 0);
    if (mcp2_ok) Serial.println("[OBS] MCP2 (0x21) now found ✓");
  }

  // Read current state
  if (mcp1_ok) {
    uint8_t a, b;
    if (readMCPReg(MCP1_ADDR, MCP_GPIOA, a) &&
        readMCPReg(MCP1_ADDR, MCP_GPIOB, b)) {
      m1a = a; m1b = b; read_ok = true;
    }
  }
  if (mcp2_ok) {
    uint8_t a, b;
    if (readMCPReg(MCP2_ADDR, MCP_GPIOA, a) &&
        readMCPReg(MCP2_ADDR, MCP_GPIOB, b)) {
      m2a = a; m2b = b; read_ok = true;
    }
  }

  if (!read_ok) return;

  // Diff and print only what changed
  if (m1a != prev_mcp1_a) diffSensors(prev_mcp1_a, m1a, 1, 8);
  if (m1b != prev_mcp1_b) diffRelays (prev_mcp1_b, m1b, 1, 6);
  if (m2a != prev_mcp2_a) diffSensors(prev_mcp2_a, m2a, 9, 2);
  if (m2b != prev_mcp2_b) diffRelays (prev_mcp2_b, m2b, 7, 6);

  prev_mcp1_a = m1a;
  prev_mcp1_b = m1b;
  prev_mcp2_a = m2a;
  prev_mcp2_b = m2b;
}
