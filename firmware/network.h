// ============================================================
// network.h — Satu Vending Machine Network Layer  R5
// Board: ESP32-S3 (ESP32-8048S070C)
// ============================================================
// CHANGE LOG:
//   R3  — initWiFi(JsonDocument&) passes back full /hello response
//          createOrder() accepts sacredWater bool
//          reloadHello() added for reload_slots command
//   R4  — FW_VERSION → v1.0.0-r4
//          Added: fetchImageBytes() — downloads PNG QR to PSRAM buffer
//          Added: loadConfigFromNVS() — restores grid/config from NVS
//          Added: factoryResetBackend() — POST /v1/machine/factory-reset
//          Updated: _sendHello() caches config{} block to NVS
//          Updated: initWiFi() calls loadConfigFromNVS() before hello
//          Updated: reportCompletion() adds slotIdx param
//   R5  — FW_VERSION → v1.0.0-r5
//          initWiFi() NVS-first: reads nvs_ssid/nvs_pass before config.h
//          Falls back to config.h WIFI_SSID/WIFI_PASSWORD if NVS empty
//          Empty credentials → returns without connecting (caller checks
//          WiFi.status() and transitions to STATE_WIFI_SETUP)
//          Added: saveWifiAndReboot() — saves to NVS then ESP.restart()
//   R5.1 — fetchImageBytes() upgraded to WiFiClientSecure + setInsecure()
//          External HTTPS (e.g. api.qrserver.com) now works — R-97
//          Added entry serial log and HTTPC_STRICT_FOLLOW_REDIRECTS
//   R5.2 — fetchImageBytes() chunked-safe read loop — R-103
//          Detects stream close (!http.connected()) for chunked EOF
//          Per-packet idle timeout 5000ms replaces 15s global wall-clock
// SECURITY:
//   device_secret in NVS only — never hardcoded
//   WiFi credentials in NVS (nvs_ssid/nvs_pass) — never in git
//   X-Device-Secret header on all authenticated calls
// ============================================================

#ifndef NETWORK_H
#define NETWORK_H
#include "esp_event.h"
#include <esp_wifi.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <PNGdec.h>
#include "config.h"

// ── Firmware version ────────────────────────────────────────────────────────────────
#define FW_VERSION  "v1.0.0-r5"

// ── NVS keys ──────────────────────────────────────────────────────────────────
#define NVS_NAMESPACE         "satu"
#define NVS_KEY_DEVICE_ID     "device_id"
#define NVS_KEY_DEVICE_SECRET "dev_secret"
#define NVS_KEY_WIFI_SSID     "nvs_ssid"
#define NVS_KEY_WIFI_PASS     "nvs_pass"

// ── Runtime globals ─────────────────────────────────────────────────────────────
static String      g_deviceId     = "";
static String      g_deviceSecret = "";
String             g_setupCode    = "";   // setup code from /hello (status=pending)
static Preferences g_prefs;

// ── UI runtime grid globals (defined in ui.h, declared here so network.h
//    functions can use them — network.h is included before ui.h) ──────────
extern int  g_grid_rows;
extern int  g_grid_cols;
extern int  g_cfg_idle;
extern int  g_cfg_sel;
extern bool g_cfg_water;
extern bool g_cfg_lucky;

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
//  LOAD CONFIG FROM NVS  (R4)
//  Restores grid dimensions and feature flags from NVS so the
//  machine boots with the correct grid if /hello is unreachable.
// ════════════════════════════════════════════════════════════════════════════
void loadConfigFromNVS() {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, true);
  int rows = prefs.getInt("nvs_grow", 2);
  int cols = prefs.getInt("nvs_gcol", 5);
  int idle = prefs.getInt("cfg_idle", 60);
  int sel  = prefs.getInt("cfg_sel",  15);
  bool water = prefs.getBool("cfg_water", true);
  bool lucky = prefs.getBool("cfg_lucky", true);
  prefs.end();

  if (rows >= 1 && rows <= 3) g_grid_rows = rows;
  if (cols >= 1 && cols <= 7) g_grid_cols = cols;
  if (idle > 0)  g_cfg_idle  = idle;
  if (sel  > 0)  g_cfg_sel   = sel;
  g_cfg_water = water;
  g_cfg_lucky = lucky;

  Serial.printf("[NET] Config from NVS: grid=%dx%d idle=%d sel=%d\n",
                g_grid_cols, g_grid_rows, g_cfg_idle, g_cfg_sel);
}

// ════════════════════════════════════════════════════════════════════════════
//  SAVE WIFI AND REBOOT  (R5)
//  Saves WiFi credentials to NVS then restarts.
//  Called by drawWifiSetupScreen() on CONNECT tap.
//  R-85 compliance: credentials go to NVS, never to source files.
// ════════════════════════════════════════════════════════════════════════════
void saveWifiAndReboot(const String& ssid, const String& pass) {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putString(NVS_KEY_WIFI_SSID, ssid);
  prefs.putString(NVS_KEY_WIFI_PASS, pass);
  prefs.end();
  Serial.printf("[NET] WiFi credentials saved: ssid=%s — rebooting\n", ssid.c_str());
  delay(200);
  ESP.restart();
}

// ════════════════════════════════════════════════════════════════════════════
//  SEND /hello  (internal)
//  R4: also caches config{} block to NVS
// ════════════════════════════════════════════════════════════════════════════
static bool _sendHello(JsonDocument& outDoc) {
  String mac = WiFi.macAddress();
  Serial.printf("[NET] /hello — MAC: %s  FW: %s\n", mac.c_str(), FW_VERSION);

  JsonDocument reqDoc;
  reqDoc["mac"]          = mac;
  reqDoc["firmware"]     = FW_VERSION;
  reqDoc["num_slots"]    = NUM_SLOTS;
  reqDoc["capabilities"] = "qr,water,ir,led";
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
  String setupCode    = outDoc["setup_code"]    | "";

  if (!setupCode.isEmpty()) g_setupCode = setupCode;  // cache for debug screen

  if (deviceId.isEmpty()) {
    Serial.println("[NET] /hello: no device_id in response");
    return false;
  }

  saveCredentialsToNVS(deviceId, deviceSecret);

  // Cache config{} block to NVS so it survives reboot without WiFi
  if (outDoc.containsKey("config") && !outDoc["config"].isNull()) {
    JsonObject cfg = outDoc["config"];
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putInt( "cfg_idle",  cfg["idle_timeout"]      | 60);
    prefs.putInt( "cfg_sel",   cfg["selection_timeout"] | 15);
    prefs.putBool("cfg_water", cfg["sacred_water"]      | true);
    prefs.putBool("cfg_lucky", cfg["lucky_number"]      | true);
    if (cfg.containsKey("grid_rows")) {
      int rows = (int)(cfg["grid_rows"] | 2);
      int cols = (int)(cfg["grid_cols"] | 5);
      if (rows >= 1 && rows <= 3) prefs.putInt("nvs_grow", rows);
      if (cols >= 1 && cols <= 7) prefs.putInt("nvs_gcol", cols);
    }
    prefs.end();
    Serial.println("[NET] Config cached to NVS");
  }

  Serial.printf("[NET] /hello OK: device_id=%s status=%s\n",
                deviceId.c_str(),
                (const char*)(outDoc["status"] | "unknown"));
  return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  INIT WiFi  (R5 — NVS-first credential resolution)
//
//  Priority order:
//    1. NVS (nvs_ssid / nvs_pass) — set by touchscreen provisioning
//    2. config.h WIFI_SSID / WIFI_PASSWORD — only if non-empty
//    3. No credentials → returns without connecting
//       Caller checks WiFi.status() != WL_CONNECTED and transitions
//       to STATE_WIFI_SETUP → drawWifiSetupScreen()
// ════════════════════════════════════════════════════════════════════════════
void initWiFi(JsonDocument& helloDoc) {
  // Load cached grid/config first so UI is ready before network
  loadConfigFromNVS();

  // 1. Read credentials from NVS
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, true);
  String ssid = prefs.getString(NVS_KEY_WIFI_SSID, "");
  String pass = prefs.getString(NVS_KEY_WIFI_PASS, "");
  prefs.end();

  // 2. Fall back to config.h if NVS is empty
  if (ssid.isEmpty() && strlen(WIFI_SSID) > 0) {
    ssid = String(WIFI_SSID);
    pass = String(WIFI_PASSWORD);
    Serial.println("[NET] WiFi: using config.h credentials");
  }

  // 3. No credentials available → signal caller to show setup screen
  if (ssid.isEmpty()) {
    Serial.println("[NET] No WiFi credentials — setup screen required");
    return;  // WiFi.status() != WL_CONNECTED, caller handles
  }

  Serial.printf("[NET] Connecting WiFi: %s\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NET] WiFi failed — entering OFFLINE state");
    return;
  }

  Serial.printf("[NET] WiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
  loadCredentialsFromNVS();

  bool ok = _sendHello(helloDoc);
  if (!ok) {
    Serial.println("[NET] /hello failed — using cached credentials");
    helloDoc["device_id"] = g_deviceId;
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  RELOAD HELLO  (called on reload_slots command)
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
// ════════════════════════════════════════════════════════════════════════════
bool createOrder(int slotIdx, bool sacredWater,
                 String& outQrUrl, int& outAmount, String& outOrderId,
                 String donorName = "", String donorId = "") {
  if (g_deviceId.isEmpty()) { Serial.println("[NET] createOrder: no device_id"); return false; }

  JsonDocument reqDoc;
  reqDoc["device_id"]    = g_deviceId;
  reqDoc["product_id"]   = slotIdx + 1;
  reqDoc["sacred_water"] = sacredWater;
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
//  CHECK PAYMENT STATUS  (fallback poll)
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
//  R4: slotIdx added (sent as slot 1-indexed to backend)
// ════════════════════════════════════════════════════════════════════════════
void reportCompletion(String orderId, bool success, int slotIdx = -1) {
  if (orderId.isEmpty() || g_deviceId.isEmpty()) return;

  JsonDocument doc;
  doc["device_id"] = g_deviceId;
  doc["order_id"]  = orderId;
  doc["success"]   = success;
  if (slotIdx >= 0) doc["slot"] = slotIdx + 1;
  String body;
  serializeJson(doc, body);

  String resp;
  int code = doPost(String(API_BASE_URL) + "/v1/machine/completion", body, resp);
  Serial.printf("[NET] Completion report: HTTP %d\n", code);
}

// ════════════════════════════════════════════════════════════════════════════
//  FACTORY RESET BACKEND  (R4)
//  Calls /v1/machine/factory-reset. Returns true on HTTP 200.
//  Machine MUST receive true before wiping NVS — R-74 compliance.
// ════════════════════════════════════════════════════════════════════════════
bool factoryResetBackend() {
  if (g_deviceId.isEmpty()) {
    Serial.println("[NET] factoryReset: no device_id");
    return false;
  }

  JsonDocument doc;
  doc["device_id"] = g_deviceId;
  String body;
  serializeJson(doc, body);

  String resp;
  int code = doPost(String(API_BASE_URL) + "/v1/machine/factory-reset", body, resp);
  Serial.printf("[NET] Factory reset backend: HTTP %d\n", code);
  return (code == 200);
}

// ════════════════════════════════════════════════════════════════════════════
//  FETCH IMAGE BYTES  (R5.2 — chunked-safe read loop)
//  Downloads a PNG from url into a caller-provided PSRAM buffer.
//  Returns byte count on success, 0 on failure.
//  Caller must ps_malloc(200*1024) and free after use.
//
//  R-97: Uses WiFiClientSecure + setInsecure() for external HTTPS URLs.
//  R-103: Handles chunked transfer encoding (Content-Length=-1).
//         Uses !http.connected() to detect EOF. Per-packet idle timeout 5s.
// ════════════════════════════════════════════════════════════════════════════
size_t fetchImageBytes(const String& url, uint8_t* buf, size_t bufSize) {
  Serial.printf("[NET] fetchImageBytes: url=%s\n", url.c_str());

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NET] fetchImageBytes: WiFi not connected");
    return 0;
  }
  if (!buf || bufSize == 0) {
    Serial.println("[NET] fetchImageBytes: invalid buffer");
    return 0;
  }

  WiFiClientSecure client;
  client.setInsecure();  // QR image only — not sensitive data — R-97

  HTTPClient http;
  http.begin(client, url);
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int code = http.GET();
  Serial.printf("[NET] fetchImageBytes: HTTP %d\n", code);

  if (code != 200) {
    http.end();
    return 0;
  }

  int contentLen = http.getSize();
  Serial.printf("[NET] fetchImageBytes: Content-Length=%d\n", contentLen);

  if (contentLen > 0 && (size_t)contentLen > bufSize) {
    Serial.printf("[NET] fetchImageBytes: too large (%d > %u)\n", contentLen, bufSize);
    http.end();
    return 0;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t bytesRead = 0;
  unsigned long lastDataMs = millis();
  const unsigned long IDLE_TIMEOUT_MS = 5000;

  while (bytesRead < bufSize) {
    if (stream->available()) {
      size_t toRead = min((size_t)stream->available(), bufSize - bytesRead);
      size_t got    = stream->readBytes(buf + bytesRead, toRead);
      bytesRead    += got;
      lastDataMs    = millis();
    } else {
      if (!http.connected()) {
        Serial.println("[NET] fetchImageBytes: stream closed — transfer complete");
        break;
      }
      if (millis() - lastDataMs > IDLE_TIMEOUT_MS) {
        Serial.printf("[NET] fetchImageBytes: idle timeout after %ums — %u bytes so far\n",
                      IDLE_TIMEOUT_MS, bytesRead);
        break;
      }
      delay(5);
    }
    if (contentLen > 0 && bytesRead >= (size_t)contentLen) break;
  }

  http.end();
  Serial.printf("[NET] fetchImageBytes: %u bytes read\n", bytesRead);
  return bytesRead;
}

#endif // NETWORK_H
