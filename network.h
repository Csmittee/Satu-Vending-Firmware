// ============================================================
// network.h — Satu Vending Machine Network Layer  R3
// Board: ESP32-S3 (ESP32-8048S070C)
// ============================================================
// CHANGE LOG:
//   R3 — initWiFi(JsonDocument&) now passes back full /hello
//          response so satu_vending.ino can call loadSlotsFromJson()
//        createOrder() accepts sacredWater bool, passes to backend
//        reloadHello() added — called on "reload_slots" command
//        donor_name + donor_id added to createOrder payload
//        Firmware version string moved to constant FW_VERSION
// ============================================================
// RESPONSIBILITIES:
//   WiFi connect with retry → STATE_OFFLINE fallback
//   NVS: device_id + device_secret survive reboot
//   POST /v1/machine/hello      → boot + returns slot config
//   POST /v1/machine/heartbeat  → every 5 min
//   GET  /v1/machine/commands   → every 30s
//   POST /v1/order              → creates PromptPay order
//   GET  /v1/order/{id}/status  → payment poll fallback
//   POST /v1/machine/completion → vend complete report
// SECURITY:
//   device_secret in NVS only — never hardcoded
//   X-Device-Secret header on all authenticated calls
// ============================================================

#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"

// ── Firmware version ─────────────────────────────────────────────────────────
#define FW_VERSION  "v1.0.0-r3"

// ── NVS keys ─────────────────────────────────────────────────────────────────
#define NVS_NAMESPACE         "satu"
#define NVS_KEY_DEVICE_ID     "device_id"
#define NVS_KEY_DEVICE_SECRET "dev_secret"

// ── Runtime globals ───────────────────────────────────────────────────────────
static String      g_deviceId     = "";
static String      g_deviceSecret = "";
static Preferences g_prefs;

// ── Command list ──────────────────────────────────────────────────────────────
#define MAX_COMMANDS 8
struct CommandList {
  String commands[MAX_COMMANDS];
  int    count = 0;
};

// ════════════════════════════════════════════════════════════════════════════
//  INTERNAL HELPERS
// ════════════════════════════════════════════════════════════════════════════

static void addAuthHeaders(HTTPClient& http) {
  if (g_deviceSecret.length() > 0)
    http.addHeader("X-Device-Secret", g_deviceSecret);
}

static int doPost(const String& url, const String& body, String& resp) {
  if (WiFi.status() != WL_CONNECTED) return -1;
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  addAuthHeaders(http);
  http.setTimeout(10000);
  int code = http.POST(body);
  resp = (code > 0) ? http.getString() : "";
  http.end();
  return code;
}

static int doGet(const String& url, String& resp) {
  if (WiFi.status() != WL_CONNECTED) return -1;
  HTTPClient http;
  http.begin(url);
  addAuthHeaders(http);
  http.setTimeout(10000);
  int code = http.GET();
  resp = (code > 0) ? http.getString() : "";
  http.end();
  return code;
}

static void saveCredentialsToNVS(const String& id, const String& secret) {
  g_prefs.begin(NVS_NAMESPACE, false);
  g_prefs.putString(NVS_KEY_DEVICE_ID,     id);
  g_prefs.putString(NVS_KEY_DEVICE_SECRET, secret);
  g_prefs.end();
  g_deviceId     = id;
  g_deviceSecret = secret;
  Serial.printf("[NET] NVS saved: device_id=%s\n", id.c_str());
}

static void loadCredentialsFromNVS() {
  g_prefs.begin(NVS_NAMESPACE, true);
  g_deviceId     = g_prefs.getString(NVS_KEY_DEVICE_ID,     "");
  g_deviceSecret = g_prefs.getString(NVS_KEY_DEVICE_SECRET, "");
  g_prefs.end();
  if (!g_deviceId.isEmpty())
    Serial.printf("[NET] NVS loaded: device_id=%s\n", g_deviceId.c_str());
}

// ════════════════════════════════════════════════════════════════════════════
//  SEND /hello  (internal — fills outDoc with full response)
//  R3: returns full JsonDocument so caller can extract slots[]
// ════════════════════════════════════════════════════════════════════════════
static bool _sendHello(JsonDocument& outDoc) {
  String mac = WiFi.macAddress();
  Serial.printf("[NET] /hello — MAC: %s  FW: %s\n", mac.c_str(), FW_VERSION);

  JsonDocument reqDoc;
  reqDoc["mac"]              = mac;
  reqDoc["firmware_version"] = FW_VERSION;
  reqDoc["num_slots"]        = NUM_SLOTS;
  reqDoc["capabilities"]     = "qr,water,ir,led";
  String body;
  serializeJson(reqDoc, body);

  String resp;
  int code = doPost(String(API_BASE_URL) + "/v1/machine/hello", body, resp);

  if (code != 200 && code != 201) {
    Serial.printf("[NET] /hello failed: HTTP %d\n", code);
    return false;
  }

  DeserializationError err = deserializeJson(outDoc, resp);
  if (err) {
    Serial.printf("[NET] /hello JSON error: %s\n", err.c_str());
    return false;
  }

  String deviceId     = outDoc["device_id"]     | "";
  String deviceSecret = outDoc["device_secret"] | "";

  if (deviceId.isEmpty()) {
    Serial.println("[NET] /hello: no device_id in response");
    return false;
  }

  saveCredentialsToNVS(deviceId, deviceSecret);
  Serial.printf("[NET] /hello OK: device_id=%s status=%s slots=%d\n",
                deviceId.c_str(),
                (const char*)(outDoc["status"] | "unknown"),
                outDoc["slots"].size());
  return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  INIT WiFi
//  R3: accepts JsonDocument& — passes back full /hello response
//      so satu_vending.ino can call loadSlotsFromJson(doc["slots"])
// ════════════════════════════════════════════════════════════════════════════
void initWiFi(JsonDocument& helloDoc) {
  Serial.printf("[NET] Connecting WiFi: %s\n", WIFI_SSID);
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
    Serial.println("[NET] WiFi failed — entering OFFLINE state");
    return;   // state machine handles STATE_OFFLINE
  }

  Serial.printf("[NET] WiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
  loadCredentialsFromNVS();

  bool ok = _sendHello(helloDoc);
  if (!ok) {
    Serial.println("[NET] /hello failed — using cached credentials");
    // populate helloDoc minimally so caller doesn't crash
    helloDoc["device_id"] = g_deviceId;
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  RELOAD HELLO  (called on "reload_slots" command from backend)
// ════════════════════════════════════════════════════════════════════════════
void reloadHello(JsonDocument& outDoc) {
  Serial.println("[NET] reloadHello called");
  _sendHello(outDoc);
}

// ════════════════════════════════════════════════════════════════════════════
//  HEARTBEAT
// ════════════════════════════════════════════════════════════════════════════
void sendHeartbeat() {
  if (g_deviceId.isEmpty()) { Serial.println("[NET] Heartbeat skipped: no device_id"); return; }

  JsonDocument doc;
  doc["device_id"] = g_deviceId;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["uptime"]    = millis() / 1000;
  doc["firmware"]  = FW_VERSION;
  String body;
  serializeJson(doc, body);

  String resp;
  int code = doPost(String(API_BASE_URL) + "/v1/machine/heartbeat", body, resp);
  Serial.printf("[NET] Heartbeat: HTTP %d\n", code);
}

// ════════════════════════════════════════════════════════════════════════════
//  POLL COMMANDS
// ════════════════════════════════════════════════════════════════════════════
CommandList pollCommands() {
  CommandList result;
  if (g_deviceId.isEmpty()) return result;

  String url  = String(API_BASE_URL) + "/v1/machine/commands?device_id=" + g_deviceId;
  String resp;
  int code = doGet(url, resp);

  if (code != 200) { Serial.printf("[NET] /commands: HTTP %d\n", code); return result; }

  JsonDocument doc;
  if (deserializeJson(doc, resp)) return result;

  JsonArray arr = doc["commands"].as<JsonArray>();
  for (JsonObject cmd : arr) {
    if (result.count >= MAX_COMMANDS) break;
    result.commands[result.count++] = cmd["command"] | "";
  }

  if (result.count) Serial.printf("[NET] %d commands pending\n", result.count);
  return result;
}

// ════════════════════════════════════════════════════════════════════════════
//  CREATE ORDER
//  R3: added sacredWater, donorName, donorId parameters
//      slot is 0-indexed (backend receives 1-indexed product_id)
// ════════════════════════════════════════════════════════════════════════════
bool createOrder(int slotIdx, bool sacredWater,
                 String& outQrUrl, int& outAmount, String& outOrderId,
                 String donorName = "", String donorId = "") {
  if (g_deviceId.isEmpty()) { Serial.println("[NET] createOrder: no device_id"); return false; }

  JsonDocument reqDoc;
  reqDoc["device_id"]   = g_deviceId;
  reqDoc["product_id"]  = slotIdx + 1;   // 1-indexed
  reqDoc["sacred_water"]= sacredWater;
  if (donorName.length()) reqDoc["donor_name"] = donorName;
  if (donorId.length())   reqDoc["donor_id"]   = donorId;
  String body;
  serializeJson(reqDoc, body);

  String resp;
  int code = doPost(String(API_BASE_URL) + "/v1/order", body, resp);

  if (code != 200 && code != 201) {
    Serial.printf("[NET] /order failed: HTTP %d\n", code);
    return false;
  }

  JsonDocument rdoc;
  if (deserializeJson(rdoc, resp)) { Serial.println("[NET] /order JSON error"); return false; }

  outOrderId = rdoc["order_id"]    | "";
  outQrUrl   = rdoc["qr_code_url"] | "";
  outAmount  = rdoc["amount"]      | 0;

  if (outOrderId.isEmpty() || outQrUrl.isEmpty()) {
    Serial.println("[NET] /order: missing order_id or qr_code_url");
    return false;
  }

  Serial.printf("[NET] Order %s — %d THB — water=%d\n",
                outOrderId.c_str(), outAmount, sacredWater);
  return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  CHECK PAYMENT STATUS  (fallback poll — primary is webhook→command)
// ════════════════════════════════════════════════════════════════════════════
String checkPaymentStatus(String orderId) {
  if (orderId.isEmpty()) return "pending";

  String url  = String(API_BASE_URL) + "/v1/order/" + orderId + "/status";
  String resp;
  int code = doGet(url, resp);

  if (code != 200) { Serial.printf("[NET] /status HTTP %d\n", code); return "pending"; }

  JsonDocument doc;
  deserializeJson(doc, resp);
  String status = doc["status"] | "pending";
  Serial.printf("[NET] Order %s: %s\n", orderId.c_str(), status.c_str());
  return status;
}

// ════════════════════════════════════════════════════════════════════════════
//  REPORT COMPLETION
// ════════════════════════════════════════════════════════════════════════════
void reportCompletion(String orderId, bool success) {
  if (orderId.isEmpty() || g_deviceId.isEmpty()) return;

  JsonDocument doc;
  doc["device_id"] = g_deviceId;
  doc["order_id"]  = orderId;
  doc["success"]   = success;
  String body;
  serializeJson(doc, body);

  String resp;
  int code = doPost(String(API_BASE_URL) + "/v1/machine/completion", body, resp);
  Serial.printf("[NET] Completion report: HTTP %d\n", code);
}

#endif // NETWORK_H
