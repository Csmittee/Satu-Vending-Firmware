// ============================================================
// network.h — Satu Vending Machine Network Layer
// Board: ESP32-S3 (ESP32-8048S070C)
// ============================================================
// RESPONSIBILITIES:
//   - WiFi connect (with retry + fallback to STATE_OFFLINE)
//   - NVS storage: device_id, device_secret (survives reboot)
//   - POST /v1/machine/hello   → on boot
//   - POST /v1/machine/heartbeat → every HEARTBEAT_INTERVAL
//   - GET  /v1/machine/commands  → every 30s
//   - POST /v1/order             → on product selection
//   - GET  /v1/order/{id}/status → payment poll
//   - POST /v1/machine/completion → on vend complete
//
// SECURITY:
//   - device_secret is read from NVS only — never hardcoded
//   - X-Device-Secret header sent on heartbeat + commands (live mode)
//   - Fake mode: backend accepts without secret header
// ============================================================

#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>    // NVS — ESP32 key-value store, survives reboot
#include "config.h"

// ── NVS namespace ────────────────────────────────────────────────────────────
#define NVS_NAMESPACE    "satu"
#define NVS_KEY_DEVICE_ID     "device_id"
#define NVS_KEY_DEVICE_SECRET "dev_secret"

// ── Runtime globals (set once on boot, read everywhere) ──────────────────────
static String g_deviceId     = "";
static String g_deviceSecret = "";
static Preferences g_prefs;

// ── Command list (returned from pollCommands) ─────────────────────────────────
#define MAX_COMMANDS 8
struct CommandList {
  String commands[MAX_COMMANDS];
  int    count = 0;
};

// ════════════════════════════════════════════════════════════════════════════
//  INTERNAL HELPERS
// ════════════════════════════════════════════════════════════════════════════

// Add X-Device-Secret header if we have a secret stored
static void addAuthHeaders(HTTPClient& http) {
  if (g_deviceSecret.length() > 0) {
    http.addHeader("X-Device-Secret", g_deviceSecret);
  }
}

// Common POST helper — returns HTTP status code, fills responseBody
static int doPost(const String& url, const String& jsonBody, String& responseBody) {
  if (WiFi.status() != WL_CONNECTED) return -1;

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  addAuthHeaders(http);
  http.setTimeout(10000);

  int code = http.POST(jsonBody);
  responseBody = (code > 0) ? http.getString() : "";
  http.end();
  return code;
}

// Common GET helper
static int doGet(const String& url, String& responseBody) {
  if (WiFi.status() != WL_CONNECTED) return -1;

  HTTPClient http;
  http.begin(url);
  addAuthHeaders(http);
  http.setTimeout(10000);

  int code = http.GET();
  responseBody = (code > 0) ? http.getString() : "";
  http.end();
  return code;
}

// ════════════════════════════════════════════════════════════════════════════
//  NVS — Load / Save device credentials
// ════════════════════════════════════════════════════════════════════════════

static void loadCredentialsFromNVS() {
  g_prefs.begin(NVS_NAMESPACE, true);   // read-only
  g_deviceId     = g_prefs.getString(NVS_KEY_DEVICE_ID,     "");
  g_deviceSecret = g_prefs.getString(NVS_KEY_DEVICE_SECRET, "");
  g_prefs.end();
  Serial.printf("[NVS] Loaded: device_id=%s secret=%s\n",
                g_deviceId.c_str(),
                g_deviceSecret.length() > 0 ? "(present)" : "(empty)");
}

static void saveCredentialsToNVS(const String& deviceId, const String& deviceSecret) {
  g_prefs.begin(NVS_NAMESPACE, false);  // read-write
  g_prefs.putString(NVS_KEY_DEVICE_ID,     deviceId);
  g_prefs.putString(NVS_KEY_DEVICE_SECRET, deviceSecret);
  g_prefs.end();
  g_deviceId     = deviceId;
  g_deviceSecret = deviceSecret;
  Serial.printf("[NVS] Saved device_id=%s\n", deviceId.c_str());
}

// ════════════════════════════════════════════════════════════════════════════
//  POST /v1/machine/hello
//  Called on every boot. Backend returns device_id + device_secret.
//  If NVS already has credentials, they are confirmed/refreshed.
//  CRITICAL: device_secret must be persisted in NVS after this call.
// ════════════════════════════════════════════════════════════════════════════
static bool sendHello() {
  String mac = WiFi.macAddress();
  Serial.printf("[NET] Sending /hello — MAC: %s\n", mac.c_str());

  StaticJsonDocument<128> doc;
  doc["mac"]      = mac;
  doc["firmware"] = "v1.0.0";
  String body;
  serializeJson(doc, body);

  String resp;
  int code = doPost(String(API_BASE_URL) + "/v1/machine/hello", body, resp);

  if (code != 200 && code != 201) {
    Serial.printf("[NET] /hello failed: HTTP %d\n", code);
    return false;
  }

  StaticJsonDocument<256> rdoc;
  DeserializationError err = deserializeJson(rdoc, resp);
  if (err) {
    Serial.printf("[NET] /hello JSON parse error: %s\n", err.c_str());
    return false;
  }

  // Extract and persist credentials
  String deviceId     = rdoc["device_id"]     | "";
  String deviceSecret = rdoc["device_secret"] | "";
  String status       = rdoc["status"]        | "unknown";

  if (deviceId.length() == 0) {
    Serial.println("[NET] /hello: no device_id in response");
    return false;
  }

  saveCredentialsToNVS(deviceId, deviceSecret);

  Serial.printf("[NET] /hello OK: device_id=%s status=%s\n",
                deviceId.c_str(), status.c_str());
  return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  INIT WiFi
//  Connects to WiFi. Retries 20 times (10s). Falls back to offline state.
//  After connect: loads NVS creds → sends /hello → saves new creds.
// ════════════════════════════════════════════════════════════════════════════
void initWiFi() {
  Serial.printf("[NET] Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NET] WiFi connect failed — machine will enter OFFLINE state");
    // State machine will handle OFFLINE — don't block here
    return;
  }

  Serial.printf("[NET] WiFi connected — IP: %s\n", WiFi.localIP().toString().c_str());

  // Load any previously saved credentials
  loadCredentialsFromNVS();

  // Always send /hello on boot (refreshes device_secret, updates firmware version)
  bool ok = sendHello();
  if (!ok) {
    Serial.println("[NET] /hello failed — continuing with cached credentials if available");
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  POST /v1/machine/heartbeat
//  Sent every HEARTBEAT_INTERVAL (5 min). Includes free heap and uptime.
//  Requires: device_id (from NVS)
//  Auth: X-Device-Secret header (added by addAuthHeaders)
// ════════════════════════════════════════════════════════════════════════════
void sendHeartbeat() {
  if (g_deviceId.isEmpty()) {
    Serial.println("[NET] Heartbeat skipped: no device_id");
    return;
  }

  StaticJsonDocument<256> doc;
  doc["device_id"] = g_deviceId;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["uptime"]    = millis() / 1000;
  doc["firmware"]  = "v1.0.0";
  String body;
  serializeJson(doc, body);

  String resp;
  int code = doPost(String(API_BASE_URL) + "/v1/machine/heartbeat", body, resp);
  Serial.printf("[NET] Heartbeat: HTTP %d\n", code);
}

// ════════════════════════════════════════════════════════════════════════════
//  GET /v1/machine/commands
//  Called every 30 seconds. Returns array of pending commands.
//  Backend marks commands as executed when returned.
//  Auth: X-Device-Secret header
// ════════════════════════════════════════════════════════════════════════════
CommandList pollCommands() {
  CommandList result;

  if (g_deviceId.isEmpty()) {
    Serial.println("[NET] pollCommands skipped: no device_id");
    return result;
  }

  String url = String(API_BASE_URL) + "/v1/machine/commands?device_id=" + g_deviceId;
  String resp;
  int code = doGet(url, resp);

  if (code != 200) {
    Serial.printf("[NET] /commands: HTTP %d\n", code);
    return result;
  }

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, resp);
  if (err) {
    Serial.printf("[NET] /commands JSON error: %s\n", err.c_str());
    return result;
  }

  JsonArray arr = doc["commands"].as<JsonArray>();
  for (JsonObject cmd : arr) {
    if (result.count >= MAX_COMMANDS) break;
    result.commands[result.count++] = cmd["command"] | "";
  }

  Serial.printf("[NET] /commands: %d pending\n", result.count);
  return result;
}

// ════════════════════════════════════════════════════════════════════════════
//  POST /v1/order
//  Creates a PromptPay order. Returns QR URL and amount.
//  Called immediately after product is selected.
//
//  product 0-9 → maps to product_id 1-10 (backend is 1-indexed)
// ════════════════════════════════════════════════════════════════════════════
bool createOrder(int productIndex, String& outQrUrl, int& outAmount, String& outOrderId) {
  if (g_deviceId.isEmpty()) {
    Serial.println("[NET] createOrder: no device_id");
    return false;
  }

  StaticJsonDocument<256> doc;
  doc["device_id"]  = g_deviceId;
  doc["product_id"] = productIndex + 1;  // backend uses 1-based product IDs
  // user_id: omitted — anonymous donation flow (backend handles null)
  String body;
  serializeJson(doc, body);

  String resp;
  int code = doPost(String(API_BASE_URL) + "/v1/order", body, resp);

  if (code != 200 && code != 201) {
    Serial.printf("[NET] /order failed: HTTP %d\n", code);
    return false;
  }

  StaticJsonDocument<512> rdoc;
  DeserializationError err = deserializeJson(rdoc, resp);
  if (err) {
    Serial.printf("[NET] /order JSON error: %s\n", err.c_str());
    return false;
  }

  outOrderId = rdoc["order_id"]    | "";
  outQrUrl   = rdoc["qr_code_url"] | "";
  outAmount  = rdoc["amount"]      | 0;

  if (outOrderId.isEmpty() || outQrUrl.isEmpty()) {
    Serial.println("[NET] /order: missing order_id or qr_code_url");
    return false;
  }

  Serial.printf("[NET] Order created: %s, %d THB\n", outOrderId.c_str(), outAmount);
  return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  GET /v1/order/{id}/status
//  Fallback polling — primary confirmation is via /commands (webhook → queue)
//  Returns: "pending" | "paid" | "expired" | "failed"
// ════════════════════════════════════════════════════════════════════════════
String checkPaymentStatus(String orderId) {
  if (orderId.isEmpty()) return "pending";

  String url = String(API_BASE_URL) + "/v1/order/" + orderId + "/status";
  String resp;
  int code = doGet(url, resp);

  if (code != 200) {
    Serial.printf("[NET] /order/status HTTP %d\n", code);
    return "pending";
  }

  StaticJsonDocument<128> doc;
  deserializeJson(doc, resp);
  String status = doc["status"] | "pending";
  Serial.printf("[NET] Order %s status: %s\n", orderId.c_str(), status.c_str());
  return status;
}

// ════════════════════════════════════════════════════════════════════════════
//  POST /v1/machine/completion   (reportCompletion)
//  Tells backend the vending completed. Logs success/failure.
//  Backend uses this to mark orders as completed.
// ════════════════════════════════════════════════════════════════════════════
void reportCompletion(String orderId, bool success) {
  if (orderId.isEmpty() || g_deviceId.isEmpty()) return;

  StaticJsonDocument<256> doc;
  doc["device_id"] = g_deviceId;
  doc["order_id"]  = orderId;
  doc["success"]   = success;
  String body;
  serializeJson(doc, body);

  String resp;
  // Note: this endpoint may not exist yet in backend — gracefully ignored if 404
  int code = doPost(String(API_BASE_URL) + "/v1/machine/completion", body, resp);
  Serial.printf("[NET] Completion report: HTTP %d\n", code);
}

#endif // NETWORK_H
