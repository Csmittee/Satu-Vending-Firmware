// ============================================================
// config.h — Satu Vending Machine Configuration
// Board: ESP32-S3 (ESP32-8048S070C)
// ============================================================
// CHANGE LOG:
//   R3 — Added: NUM_SLOTS (configurable 1-21, default 10)
//        Added: NUM_COLS (grid columns, default 5)
//        Added: MCP3_ADDR for future 21-lane expansion
//        Fixed: API_BASE_URL to api.janishammer.com
//        Added: WATER_PUMP_RELAY, DOOR_LOCK_RELAY constants
//        Added: VEND_PULSE_MS
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// WiFi — edit these before flashing
// ============================================================
const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// ============================================================
// Backend API
// ============================================================
const char* API_BASE_URL = "https://api.janishammer.com";

// ============================================================
// Slot / Lane Configuration
//   NUM_SLOTS : how many dispensing lanes are physically fitted
//               Default 10 (5×2 grid). Max 21 (7×3).
//               Owner can also set this remotely — firmware
//               uses whatever /hello returns, falling back here.
//   NUM_COLS  : columns in the product grid on screen
// ============================================================
#define NUM_SLOTS   10    // ← change to match physical machine
#define NUM_COLS     5    // ← change to match NUM_SLOTS layout

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
//   MCP3 (0x22): sensors 17-21, relays 13-18  ← future 21-lane
// ============================================================
#define MCP1_ADDR  0x20
#define MCP2_ADDR  0x21
#define MCP3_ADDR  0x22   // stub — only wired when NUM_SLOTS > 12

// ============================================================
// MCP Pin Declarations (defined in hardware.h)
// ============================================================
extern const int mcp1_sensors[8];   // GPA0-7
extern const int mcp1_relays[6];    // GPB0-5
extern const int mcp2_sensors[8];   // GPA0-7  (was 2, now full 8)
extern const int mcp2_relays[6];    // GPB0-5

// ============================================================
// Relay Logic
// ============================================================
#define RELAY_ON          HIGH
#define RELAY_OFF         LOW
#define SENSOR_TRIGGERED  LOW    // IR beam broken = item present
#define SENSOR_CLEAR      HIGH   // IR beam open   = nothing

// Special relay assignments (relay number, 1-indexed)
#define WATER_PUMP_RELAY  (NUM_SLOTS + 1)   // first relay after all lanes
#define DOOR_LOCK_RELAY   (NUM_SLOTS + 2)   // second relay after all lanes

// Motor pulse duration
#define VEND_PULSE_MS     800    // ms to hold relay ON for one vend

// ============================================================
// Timeouts (milliseconds)
// ============================================================
#define PAYMENT_TIMEOUT      120000   // 2 min QR window
#define VEND_TIMEOUT          10000   // 10s relay watchdog
#define DROP_TIMEOUT           5000   // 5s for item to fall through IR
#define REMOVAL_TIMEOUT       30000   // 30s for user to take item
#define HEARTBEAT_INTERVAL   300000   // 5 min heartbeat

// ============================================================
// LED Configuration (WS2812B strip)
// ============================================================
#define NUM_LEDS        40
#define LED_BRIGHTNESS  128

// Zones
#define ZONE_TOP_START     0
#define ZONE_TOP_END       9
#define ZONE_FLOOR1_START 10
#define ZONE_FLOOR1_END   19
#define ZONE_FLOOR2_START 20
#define ZONE_FLOOR2_END   29
#define ZONE_DOOR_START   30
#define ZONE_DOOR_END     39

#endif // CONFIG_H
