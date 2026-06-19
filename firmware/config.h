// ============================================================
// config.h — Satu Vending Machine Configuration
// ============================================================
// TRACKED IN REPO (R-86). WiFi fields intentionally empty (R-85).
// CC updates this file in the same PR as any new constant addition.
// Owner downloads from GitHub firmware folder as part of flash workflow.
// WiFi credentials entered via touchscreen → saved to NVS (nvs_ssid / nvs_pass).
// CI generates its own config.h inline via compile-check.yml independently.
// ============================================================
// CHANGE LOG:
//   R3   — Added MCP3_ADDR, WATER_PUMP_RELAY, DOOR_LOCK_RELAY, VEND_PULSE_MS
//          Fixed API_BASE_URL to api.janishammer.com
//   R3.1 — Restored NUM_SLOTS=10, NUM_COLS=5
//   R4   — NUM_SLOTS 10→21 (max 3×7 grid support)
//          NUM_COLS 5→7 (max columns)
//          Added NVS key reference block
//   R5   — WIFI_SSID/WIFI_PASSWORD EMPTY — credentials now entered
//          via touchscreen provisioning, stored in NVS (nvs_ssid / nvs_pass)
//   R6   — VEND_PULSE_MS, DROP_TIMEOUT, REMOVAL_TIMEOUT removed (R-128)
//          RELAY_FLAP added (R-129 — replaces RELAY_DOOR_LOCK)
//          VEND_MAX_SPIN_MS, SENSOR_POLL_MS, FLAP_RELOCK_TIMEOUT added (R-128/R-129)
//          FLAP_PROXIMITY_MCP_PIN, SPEAKER_PIN added
//   R9   — PRODUCT_SELECTION_TIMEOUT added (R-152, 15s idle → idle screen)
//          File now tracked in repo (R-86 update)
// ============================================================
// NVS KEYS ("satu" namespace, all ≤15 chars):
//   device_id  — assigned device ID (string)
//   dev_secret — device auth secret (string)
//   nvs_ssid   — WiFi SSID saved by provisioning screen (string)
//   nvs_pass   — WiFi password saved by provisioning screen (string)
//   svc_pin    — service PIN (string)
//   svc_pin_en — service PIN enabled (bool)
//   boot_pin   — require PIN at boot (bool)
//   lang       — UI language: 0=EN, 1=TH (int)
//   cfg_idle   — idle timeout seconds (int)
//   cfg_sel    — selection timeout seconds (int)
//   cfg_water  — sacred water feature enabled (bool)
//   cfg_lucky  — lucky number feature enabled (bool)
//   nvs_grow   — grid rows from /hello (int)
//   nvs_gcol   — grid cols from /hello (int)
//   vol        — speaker volume 0-100 (int)
//   nvs_idc    — ID card reader enabled (bool)
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// WiFi — intentionally empty (R5)
// Credentials are entered via the touchscreen WiFi Setup screen
// on first boot and persisted to NVS (nvs_ssid / nvs_pass).
// You may leave these empty for NVS-provisioned machines.
// ============================================================
const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = "";

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
//   MCP2 (0x21): sensors 9-16, relays 7-12 (pump + flap)
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

#define RELAY_FLAP       12   // R-129: solenoid pin lock. HIGH=unlocked, LOW=locked (fail-secure)

// ============================================================
// Timeouts (milliseconds)
// ============================================================
#define PAYMENT_TIMEOUT       30000   // 30s QR window (R-102, was 120s)
#define VEND_TIMEOUT          10000   // 10s relay watchdog
#define HEARTBEAT_INTERVAL   300000   // 5 min
#define PRODUCT_SELECTION_TIMEOUT 15  // R-152: idle seconds on selection screen → return to idle

// ── Vend + Flap timing (R-128, R-129) ────────────────────────────────────
#define VEND_MAX_SPIN_MS      30000  // R-128: motor safety cutoff (30s = ~10 turns)
#define SENSOR_POLL_MS            10  // R-128: IR sensor read interval during spin
#define PRODUCT_SELECTION_TIMEOUT 15
#define FLAP_RELOCK_TIMEOUT     3000  // R-129: max wait for proximity before force-lock

// ── Flap proximity switch (R-129) ─────────────────────────────────────────
// Wired to MCP2 GPA2–GPA7. Assign pin number (0–5 = GPA2–GPA7) when wired.
// -1 = not yet assigned. Firmware stubs safely: uses FLAP_RELOCK_TIMEOUT only.
#define FLAP_PROXIMITY_MCP_PIN   -1   // TODO: assign MCP2 GPA pin when wired

// ── Speaker (R-134) ────────────────────────────────────────────────────────
// TODO: assign GPIO when speaker is wired. -1 = not yet assigned.
#define SPEAKER_PIN              -1

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
