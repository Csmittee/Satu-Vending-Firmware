#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Initialization
void initWiFi();
void sendHeartbeat();

// API calls (Phase 3+)
bool authenticateUser(String userId, int product, String &orderId, String &qrCode);
bool checkPaymentStatus(String orderId);
void reportCompletion(String orderId, bool success);

#endif
