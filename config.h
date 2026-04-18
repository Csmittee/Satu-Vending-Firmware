#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// WiFi Configuration
// ============================================================
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// ============================================================
// Backend API
// ============================================================
const char* API_BASE_URL = "https://satu-api.yourname.workers.dev";

// ============================================================
// Pin Mapping (ESP32-S3)
// ============================================================
#define I2C_SDA 19
#define I2C_SCL 20
#define UART_RX 44
#define UART_TX 43
#define LED_DATA_PIN 5

// ============================================================
// MCP23017 Addresses
// ============================================================
#define MCP1_ADDR 0x20
#define MCP2_ADDR 0x21

// ============================================================
// MCP1 Pin Mapping (Sensors 1-8, Relays 1-6)
// ============================================================
extern const int mcp1_sensors[8];   // {0,1,2,3,4,5,6,7}
extern const int mcp1_relays[6];    // {8,9,10,11,12,13}

// ============================================================
// MCP2 Pin Mapping (Sensors 9-10, Relays 7-10, Pump, Lock)
// ============================================================
extern const int mcp2_sensors[2];   // {0,1}
extern const int mcp2_relays[6];    // {8,9,10,11,12,13}

// ============================================================
// Relay States
// ============================================================
#define RELAY_ON  HIGH
#define RELAY_OFF LOW
#define SENSOR_TRIGGERED LOW
#define SENSOR_CLEAR HIGH

// ============================================================
// Timeouts (milliseconds)
// ============================================================
#define PAYMENT_TIMEOUT 120000
#define VEND_TIMEOUT 10000
#define DROP_TIMEOUT 5000
#define REMOVAL_TIMEOUT 30000
#define HEARTBEAT_INTERVAL 300000

// ============================================================
// LED Configuration
// ============================================================
#define NUM_LEDS 40
#define LED_BRIGHTNESS 128

// LED Zones
#define ZONE_TOP_START 0
#define ZONE_TOP_END 9
#define ZONE_FLOOR1_START 10
#define ZONE_FLOOR1_END 19
#define ZONE_FLOOR2_START 20
#define ZONE_FLOOR2_END 29
#define ZONE_DOOR_START 30
#define ZONE_DOOR_END 39

#endif
