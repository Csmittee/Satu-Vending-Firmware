// ============================================================
// SATU VENDING MACHINE - MAIN FIRMWARE
// Version: 1.0
// Hardware: ESP32-S3 + 2x MCP23017
// Author: Satu Team
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// ============================================================
// PIN MAPPING
// ============================================================

#define I2C_SDA 19
#define I2C_SCL 20
#define UART_RX 44
#define UART_TX 43
#define LED_DATA_PIN 5

// MCP23017 Addresses
#define MCP1_ADDR 0x20
#define MCP2_ADDR 0x21

// MCP1 Pin Mapping (Sensors 1-8, Relays 1-6)
const int mcp1_sensors[] = {0, 1, 2, 3, 4, 5, 6, 7};  // A0-A7
const int mcp1_relays[] = {8, 9, 10, 11, 12, 13};      // B0-B5

// MCP2 Pin Mapping (Sensors 9-10, Relays 7-10, Pump, Lock)
const int mcp2_sensors[] = {0, 1};                      // A0-A1
const int mcp2_relays[] = {8, 9, 10, 11, 12, 13};       // B0-B5

#define RELAY_ON  HIGH
#define RELAY_OFF LOW
#define SENSOR_TRIGGERED LOW
#define SENSOR_CLEAR HIGH

// ============================================================
// WiFi Configuration
// ============================================================

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiEndpoint = "https://your-backend.workers.dev/api";

// ============================================================
// LED Strip Configuration
// ============================================================

#define NUM_LEDS 40
#define LED_BRIGHTNESS 128

CRGB leds[NUM_LEDS];

// LED Zones
const int ZONE_TOP_START = 0;
const int ZONE_TOP_END = 9;
const int ZONE_FLOOR1_START = 10;
const int ZONE_FLOOR1_END = 19;
const int ZONE_FLOOR2_START = 20;
const int ZONE_FLOOR2_END = 29;
const int ZONE_DOOR_START = 30;
const int ZONE_DOOR_END = 39;

// ============================================================
// State Machine
// ============================================================

enum MachineState {
  STATE_STARTUP,
  STATE_IDLE,
  STATE_ID_SCANNING,
  STATE_AUTHENTICATING,
  STATE_PRODUCT_SELECTION,
  STATE_AWAITING_PAYMENT,
  STATE_VENDING,
  STATE_WAITING_DROP,
  STATE_DISPENSING,
  STATE_WAITING_REMOVAL,
  STATE_COMPLETING,
  STATE_ERROR,
  STATE_OFFLINE
};

MachineState currentState = STATE_STARTUP;
MachineState lastState = STATE_STARTUP;

// Transaction Data
String currentOrderId = "";
int selectedProduct = -1;
String currentUserId = "";
unsigned long stateStartTime = 0;
unsigned long lastHeartbeat = 0;

// Timeouts (milliseconds)
const unsigned long PAYMENT_TIMEOUT = 120000;  // 2 minutes
const unsigned long VEND_TIMEOUT = 10000;      // 10 seconds
const unsigned long DROP_TIMEOUT = 5000;       // 5 seconds
const unsigned long REMOVAL_TIMEOUT = 30000;   // 30 seconds
const unsigned long HEARTBEAT_INTERVAL = 300000; // 5 minutes

// Error Counters (for lane disabling)
int laneErrorCount[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool laneDisabled[10] = {false, false, false, false, false, false, false, false, false, false};

// ============================================================
// Global Objects
// ============================================================

Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;
WiFiClientSecure client;

// ============================================================
// Function Declarations
// ============================================================

void setupHardware();
void connectWiFi();
void sendHeartbeat();
bool readIdCard(String &userId);
void authenticateUser(String userId, int product);
bool checkPaymentStatus(String orderId);
void displayQrCode(String qrData);
void setRelay(int relayNum, bool state);
bool readSensor(int sensorNum);
void vendProduct(int lane);
void unlockDoor();
void lockDoor();
void setLEDColor(int start, int end, CRGB color);
void celebrationAnimation();
void errorAnimation();
void idleAnimation();

// ============================================================
// Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("SATU VENDING MACHINE FIRMWARE v1.0");
  Serial.println("========================================\n");
  
  setupHardware();
  connectWiFi();
  
  // Initialize LED strip
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  idleAnimation();
  
  currentState = STATE_IDLE;
  stateStartTime = millis();
  lastHeartbeat = millis();
  
  Serial.println("[SYSTEM] Ready. Waiting for ID card...");
}

// ============================================================
// Hardware Setup
// ============================================================

void setupHardware() {
  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);  // 100kHz for reliable communication
  Serial.println("[I2C] Initialized at 100kHz");
  
  // Initialize MCP23017 #1
  if (!mcp1.begin(MCP1_ADDR)) {
    Serial.println("[ERROR] MCP23017 #1 not found!");
    currentState = STATE_ERROR;
  } else {
    Serial.println("[MCP1] Found at address 0x20");
    
    // Configure sensor pins as inputs with pull-up
    for (int i = 0; i < 8; i++) {
      mcp1.pinMode(mcp1_sensors[i], INPUT_PULLUP);
    }
    
    // Configure relay pins as outputs, start OFF
    for (int i = 0; i < 6; i++) {
      mcp1.pinMode(mcp1_relays[i], OUTPUT);
      mcp1.digitalWrite(mcp1_relays[i], RELAY_OFF);
    }
  }
  
  // Initialize MCP23017 #2
  if (!mcp2.begin(MCP2_ADDR)) {
    Serial.println("[ERROR] MCP23017 #2 not found!");
    currentState = STATE_ERROR;
  } else {
    Serial.println("[MCP2] Found at address 0x21");
    
    // Configure sensor pins as inputs with pull-up
    for (int i = 0; i < 2; i++) {
      mcp2.pinMode(mcp2_sensors[i], INPUT_PULLUP);
    }
    
    // Configure relay pins as outputs, start OFF
    for (int i = 0; i < 6; i++) {
      mcp2.pinMode(mcp2_relays[i], OUTPUT);
      mcp2.digitalWrite(mcp2_relays[i], RELAY_OFF);
    }
  }
  
  // Initialize UART for ID card reader
  Serial1.begin(9600, SERIAL_8N1, UART_RX, UART_TX);
  Serial.println("[UART] ID Card Reader initialized on GPIO43/44");
}

// ============================================================
// WiFi Connection
// ============================================================

void connectWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected!");
    Serial.print("[WiFi] IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] Connection failed! Entering offline mode.");
    currentState = STATE_OFFLINE;
  }
}

// ============================================================
// Heartbeat
// ============================================================

void sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  http.begin(client, String(apiEndpoint) + "/heartbeat");
  http.addHeader("Content-Type", "application/json");
  
  StaticJsonDocument<256> doc;
  doc["device_id"] = "SATU-001";
  doc["state"] = currentState;
  doc["free_heap"] = ESP.getFreeHeap();
  
  String jsonStr;
  serializeJson(doc, jsonStr);
  
  int httpCode = http.POST(jsonStr);
  http.end();
  
  lastHeartbeat = millis();
}

// ============================================================
// ID Card Reader
// ============================================================

bool readIdCard(String &userId) {
  if (Serial1.available()) {
    String data = Serial1.readString();
    data.trim();
    
    // RDM6300 sends 14 characters: STX + 10 digits + checksum + ETX
    if (data.length() >= 12) {
      userId = data.substring(1, 11);  // Extract the 10-digit ID
      return true;
    }
  }
  return false;
}

// ============================================================
// Relay Control
// ============================================================

void setRelay(int relayNum, bool state) {
  // relayNum: 0-9 for relays 1-10
  // 10 = water pump, 11 = door lock
  
  if (relayNum >= 0 && relayNum <= 5) {
    mcp1.digitalWrite(mcp1_relays[relayNum], state ? RELAY_ON : RELAY_OFF);
  } else if (relayNum >= 6 && relayNum <= 9) {
    mcp2.digitalWrite(mcp2_relays[relayNum - 6], state ? RELAY_ON : RELAY_OFF);
  } else if (relayNum == 10) {
    mcp2.digitalWrite(mcp2_relays[5], state ? RELAY_ON : RELAY_OFF);  // Water pump on B5
  } else if (relayNum == 11) {
    mcp2.digitalWrite(mcp2_relays[4], state ? RELAY_ON : RELAY_OFF);  // Door lock on B4
  }
}

// ============================================================
// Sensor Reading
// ============================================================

bool readSensor(int sensorNum) {
  // sensorNum: 0-9 for sensors 1-10
  // Returns true if product detected (sensor triggered)
  
  bool triggered = false;
  
  if (sensorNum >= 0 && sensorNum <= 7) {
    triggered = (mcp1.digitalRead(mcp1_sensors[sensorNum]) == SENSOR_TRIGGERED);
  } else if (sensorNum >= 8 && sensorNum <= 9) {
    triggered = (mcp2.digitalRead(mcp2_sensors[sensorNum - 8]) == SENSOR_TRIGGERED);
  }
  
  return triggered;
}

// ============================================================
// Vending Logic
// ============================================================

void vendProduct(int lane) {
  Serial.printf("[VEND] Lane %d selected. Starting motor...\n", lane + 1);
  
  // Check if lane is disabled
  if (laneDisabled[lane]) {
    Serial.printf("[ERROR] Lane %d is disabled due to multiple failures\n", lane + 1);
    currentState = STATE_ERROR;
    return;
  }
  
  // Turn on relay
  setRelay(lane, true);
  
  unsigned long startTime = millis();
  bool dropDetected = false;
  
  // Wait for drop detection or timeout
  while ((millis() - startTime) < DROP_TIMEOUT) {
    if (readSensor(lane)) {
      dropDetected = true;
      Serial.printf("[SENSOR] Drop detected on lane %d after %d ms\n", 
                    lane + 1, millis() - startTime);
      break;
    }
    delay(10);
  }
  
  // Turn off relay
  setRelay(lane, false);
  
  if (dropDetected) {
    // Success - reset error counter
    laneErrorCount[lane] = 0;
    Serial.printf("[VEND] Lane %d vend successful\n", lane + 1);
  } else {
    // No drop detected - increment error counter
    laneErrorCount[lane]++;
    Serial.printf("[ERROR] Lane %d no drop detected. Error count: %d\n", 
                  lane + 1, laneErrorCount[lane]);
    
    if (laneErrorCount[lane] >= 2) {
      laneDisabled[lane] = true;
      Serial.printf("[ERROR] Lane %d has been DISABLED\n", lane + 1);
    }
  }
}

// ============================================================
// Door Lock Control
// ============================================================

void unlockDoor() {
  Serial.println("[LOCK] Unlocking door...");
  setRelay(11, true);   // Door lock relay ON
  delay(500);           // Keep unlocked for 500ms
  setRelay(11, false);  // Door lock relay OFF (lock re-engages)
  Serial.println("[LOCK] Door unlocked");
}

void lockDoor() {
  Serial.println("[LOCK] Door locked");
  // Lock is fail-safe - it locks when relay is off
  // So nothing to do here
}

// ============================================================
// LED Control
// ============================================================

void setLEDColor(int start, int end, CRGB color) {
  for (int i = start; i <= end; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

void celebrationAnimation() {
  // Gold pulse effect on all LEDs
  for (int brightness = 0; brightness < 255; brightness += 5) {
    CRGB gold = CHSV(32, 255, brightness);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = gold;
    }
    FastLED.show();
    delay(5);
  }
  delay(200);
  
  for (int brightness = 255; brightness > 0; brightness -= 5) {
    CRGB gold = CHSV(32, 255, brightness);
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = gold;
    }
    FastLED.show();
    delay(5);
  }
  
  idleAnimation();
}

void errorAnimation() {
  // Red flash 3 times
  for (int f = 0; f < 3; f++) {
    setLEDColor(0, NUM_LEDS - 1, CRGB::Red);
    delay(200);
    setLEDColor(0, NUM_LEDS - 1, CRGB::Black);
    delay(200);
  }
  idleAnimation();
}

void idleAnimation() {
  // Warm white ambient lighting
  setLEDColor(0, NUM_LEDS - 1, CRGB(255, 200, 150));
}

void waterPumpAnimation() {
  // Blue wave
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Blue;
    FastLED.show();
    delay(10);
  }
  delay(300);
  idleAnimation();
}

// ============================================================
// Main Loop - State Machine
// ============================================================

void loop() {
  // Handle LED animation (non-blocking)
  FastLED.show();
  
  // Send heartbeat periodically
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
  }
  
  // State Machine
  switch (currentState) {
    
    case STATE_IDLE:
      // Wait for ID card
      if (readIdCard(currentUserId)) {
        Serial.printf("[ID] Card detected: %s\n", currentUserId.c_str());
        currentState = STATE_ID_SCANNING;
        stateStartTime = millis();
      }
      break;
      
    case STATE_ID_SCANNING:
      // Send ID to backend for authentication
      // (Simplified - will be expanded)
      currentState = STATE_PRODUCT_SELECTION;
      break;
      
    case STATE_PRODUCT_SELECTION:
      // Wait for product selection from touchscreen
      // (Handled by display code - simplified here)
      // For now, simulate selection of lane 0
      selectedProduct = 0;
      currentState = STATE_AWAITING_PAYMENT;
      break;
      
    case STATE_AWAITING_PAYMENT:
      // Check payment status
      if (checkPaymentStatus(currentOrderId)) {
        currentState = STATE_VENDING;
        celebrationAnimation();
      } else if (millis() - stateStartTime > PAYMENT_TIMEOUT) {
        Serial.println("[ERROR] Payment timeout");
        currentState = STATE_IDLE;
        errorAnimation();
      }
      break;
      
    case STATE_VENDING:
      vendProduct(selectedProduct);
      currentState = STATE_WAITING_DROP;
      stateStartTime = millis();
      break;
      
    case STATE_WAITING_DROP:
      if (readSensor(selectedProduct)) {
        unlockDoor();
        currentState = STATE_DISPENSING;
        stateStartTime = millis();
      } else if (millis() - stateStartTime > DROP_TIMEOUT) {
        Serial.println("[ERROR] Drop timeout - product may be stuck");
        currentState = STATE_ERROR;
        errorAnimation();
      }
      break;
      
    case STATE_DISPENSING:
      // Wait for product removal (door sensor)
      // For now, assume removed after 3 seconds
      if (millis() - stateStartTime > 3000) {
        lockDoor();
        currentState = STATE_COMPLETING;
      }
      break;
      
    case STATE_COMPLETING:
      // Send completion to backend
      currentState = STATE_IDLE;
      idleAnimation();
      Serial.println("[COMPLETE] Transaction finished. Ready for next.\n");
      break;
      
    case STATE_ERROR:
      // Wait 10 seconds then recover
      if (millis() - stateStartTime > 10000) {
        currentState = STATE_IDLE;
        idleAnimation();
      }
      break;
      
    case STATE_OFFLINE:
      // Attempt to reconnect
      if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
      }
      if (WiFi.status() == WL_CONNECTED) {
        currentState = STATE_IDLE;
      }
      delay(5000);
      break;
      
    default:
      break;
  }
  
  delay(10);  // Small delay to prevent watchdog issues
}

// ============================================================
// Backend API Functions (Stubs - to be implemented)
// ============================================================

bool checkPaymentStatus(String orderId) {
  // This will call your Cloudflare API
  // Returns true when payment is confirmed
  // For now, return true after 3 seconds (simulation)
  static unsigned long startTime = 0;
  static bool firstCall = true;
  
  if (firstCall) {
    startTime = millis();
    firstCall = false;
    return false;
  }
  
  if (millis() - startTime > 3000) {
    firstCall = true;
    return true;
  }
  
  return false;
}
