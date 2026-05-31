// ============================================================
// config.h — Satu Vending Machine Configuration
// Board: ESP32-S3 (ESP32-8048S070C)
// ============================================================
// CHANGE LOG:
//   R3   — Added: MCP3_ADDR, WATER_PUMP_RELAY, DOOR_LOCK_RELAY, VEND_PULSE_MS
//          Fixed: API_BASE_URL to api.janishammer.com
//   R3.1 — Restored: NUM_SLOTS=10, NUM_COLS=5
//   R4   — NUM_SLOTS 10→21 (max 3×7 grid support)
//          NUM_COLS 5→7 (max columns)
//          Added NVS key reference block
// ============================================================
// NVS KEYS (“satu” namespace, max 15 chars each, max 17 keys):
//   device_id     — assigned device ID (string)
//   dev_secret    — device auth secret (string)
//   svc_pin       — 4-digit service PIN (string)
//   svc_pin_en    — service PIN enabled (bool)
//   boot_pin      — require PIN at boot (bool)
//   lang          — UI language: 0=EN, 1=TH (int)
//   cfg_idle      — idle timeout seconds (int)
//   cfg_sel       — selection timeout seconds (int)
//   cfg_water     — sacred water feature enabled (bool)
//   cfg_lucky     — lucky number feature enabled (bool)
//   nvs_grow      — grid rows from /hello (int)
//   nvs_gcol      — grid cols from /hello (int)
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// WiFi — edit these before flashing (file is gitignored)
// ============================================================
const char* WIFI_SSID     = "Jaydahome2.4G";
const char* WIFI_PASSWORD = "Jeda2322";

// ============================================================
// Backend API
// ============================================================
const char* API_BASE_URL = "https://api.janishammer.com";

// ============================================================
// Slot count — must match max grid (3 rows × 7 cols = 21)
// Runtime grid is set by /hello config{} → NVS → ui.h globals
// ============================================================
#define NUM_SLOTS      21   // R4: max supported slots (3×7)
#define NUM_COLS        7   // R4: max columns
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
#define MCP3_ADDR  0x22

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
