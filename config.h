// ============================================================
// config.h — Satu Vending Machine Configuration
// Board: ESP32-S3 (ESP32-8048S070C)
// ============================================================
// CHANGE LOG:
//   R3   — Added: MCP3_ADDR, WATER_PUMP_RELAY, DOOR_LOCK_RELAY, VEND_PULSE_MS
//          Fixed: API_BASE_URL to api.janishammer.com
//   R3.1 — Restored: NUM_SLOTS=10, NUM_COLS=5 (network.h needs NUM_SLOTS)
//           Grid config belongs to ui.h only (GRID_COLS × GRID_ROWS).
//           config.h was redefining NUM_SLOTS=10 while ui.h set it to 21
//           causing array-size mismatch between files. ui.h is the
//           single source of truth for slot count. hardware.h uses
//           MAX_SLOTS_HW for its own array sizing.
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// WiFi — edit these before flashing
// ============================================================
const char* WIFI_SSID     = "Jaydahome2.4G";
const char* WIFI_PASSWORD = "Jeda2322";

// ============================================================
// Backend API
// ============================================================
const char* API_BASE_URL = "https://api.janishammer.com";

// ============================================================
// Hardware slot ceiling (for array sizing in hardware.h only)
// This is NOT the displayed grid count — that comes from ui.h
// and is set remotely by /hello response from backend.
// ============================================================
#define NUM_SLOTS   10
#define NUM_COLS     5
#define MAX_SLOTS_HW   21   // physical max lanes this board supports

// ============================================================
// Pin Mapping (ESP32-S3)
// ============================================================
#define I2C_SDA      19
#define I2C_SCL      20
#define UART_RX      44
#define UART_TX      43
#define LED_DATA_PIN  5
#define TFT_BL        2   // display backlight (Arduino_GFX)

// ============================================================
// MCP23017 I2C Addresses
//   MCP1 (0x20): sensors 1-8,  relays 1-6
//   MCP2 (0x21): sensors 9-16, relays 7-12 (pump + lock)
//   MCP3 (0x22): sensors 17-21, relays 13-18  — future 21-lane
// ============================================================
#define MCP1_ADDR  0x20
#define MCP2_ADDR  0x21
#define MCP3_ADDR  0x22   // stub — only wired when physical lanes > 12

// ============================================================
// MCP Pin Declarations (defined in hardware.h)
// ============================================================
extern const int mcp1_sensors[8];
extern const int mcp1_relays[6];
extern const int mcp2_sensors[2];
extern const int mcp2_relays[6];

// ============================================================
// Relay Logic
// ============================================================
#define RELAY_ON          HIGH
#define RELAY_OFF         LOW
#define SENSOR_TRIGGERED  LOW    // IR beam broken = item present
#define SENSOR_CLEAR      HIGH   // IR beam open   = nothing

// Water pump and door lock are always the last two relay channels
// regardless of how many vend lanes are active
// RELAY_PUMP and RELAY_LOCK defined in hardware.h


// Motor pulse duration
#define VEND_PULSE_MS  800    // ms to hold relay ON for one vend

// ============================================================
// Timeouts (milliseconds)
// ============================================================
#define PAYMENT_TIMEOUT      120000   // 2 min QR window
#define VEND_TIMEOUT          10000   // 10s relay watchdog
#define DROP_TIMEOUT           5000   // 5s for item to fall through IR
#define REMOVAL_TIMEOUT       30000   // 30s for user to take item
#define HEARTBEAT_INTERVAL   300000   // 5 min

// ============================================================
// LED Configuration (WS2812B strip)
// ============================================================
#define NUM_LEDS        40
#define LED_BRIGHTNESS  128

#define ZONE_TOP_START     0
#define ZONE_TOP_END       9
#define ZONE_FLOOR1_START 10
#define ZONE_FLOOR1_END   19
#define ZONE_FLOOR2_START 20
#define ZONE_FLOOR2_END   29
#define ZONE_DOOR_START   30
#define ZONE_DOOR_END     39

#endif // CONFIG_H
