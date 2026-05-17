#include "firebase_rtdb.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"
#include "../serial_debug/serial_debug.h"

static const char* TAG = "FBRTDB";

static String   gIdToken        = "";
static uint32_t gTokenObtainedAt = 0;

// =============================================================================
// Obtiene un idToken via email+password
// =============================================================================
static String fetchToken() {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=";
    url += FIREBASE_API_KEY;

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    String body = "{\"email\":\"" FIREBASE_EMAIL "\","
                  "\"password\":\"" FIREBASE_PASS "\","
                  "\"returnSecureToken\":true}";
    int code = http.POST(body);

    String token = "";
    if (code == 200) {
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, http.getString());
        token = doc["idToken"].as<String>();
        LOG_INFO(TAG, "Autenticacion OK");
    } else {
        LOG_WARN(TAG, "Auth fallida HTTP=%d", code);
    }

    http.end();
    return token;
}

// =============================================================================

namespace FirebaseRTDB {

void init() {
    gIdToken         = fetchToken();
    gTokenObtainedAt = millis();
}

void update() {
    if (gIdToken.isEmpty() ||
        (millis() - gTokenObtainedAt) >= FIREBASE_TOKEN_REFRESH_MS) {
        LOG_INFO(TAG, "Renovando token...");
        gIdToken         = fetchToken();
        gTokenObtainedAt = millis();
    }
}

String findFirstOnlineId() {
    if (WiFi.status() != WL_CONNECTED) return "";

    // Renovar token si está vacío
    if (gIdToken.isEmpty()) {
        gIdToken = fetchToken();
        gTokenObtainedAt = millis();
    }
    if (gIdToken.isEmpty()) return "";

    for (int intento = 0; intento < 2; intento++) {
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;

        String url = String(FIREBASE_RTDB_URL) + "/incutwins.json?auth=" + gIdToken;
        http.begin(client, url);

        String foundId = "";
        int code = http.GET();
        if (code == 200) {
            DynamicJsonDocument doc(4096);
            if (!deserializeJson(doc, http.getString())) {
                for (JsonPair kv : doc.as<JsonObject>()) {
                    if (kv.value()["enLinea"] == true && kv.value()["conBebe"] == true) {
                        foundId = kv.key().c_str();
                        break;
                    }
                }
            }
            http.end();
            if (foundId.isEmpty()) LOG_WARN(TAG, "Ninguna incutwin en linea con bebe");
            else                   LOG_INFO(TAG, "Primera activa con bebe: %s", foundId.c_str());
            return foundId;
        } else {
            LOG_WARN(TAG, "findFirstOnlineId HTTP=%d (intento %d)", code, intento + 1);
            http.end();
            if (code == 401) {
                // Renovar token y reintentar una vez
                gIdToken = fetchToken();
                gTokenObtainedAt = millis();
            } else {
                break;
            }
        }
    }
    return "";
}

void setHoldDetected(const String& incutwinId, bool value) {
    if (incutwinId.isEmpty()) return;
    if (WiFi.status() != WL_CONNECTED || gIdToken.isEmpty()) {
        LOG_WARN(TAG, "Sin WiFi/token — holdDetected no enviado");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = String(FIREBASE_RTDB_URL)
               + "/incutwins/" + incutwinId
               + "/holdDetected.json?auth=" + gIdToken;

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    int code = http.PUT(value ? "true" : "false");
    if (code == 200) {
        LOG_INFO(TAG, "holdDetected=%s OK → %s", value ? "true" : "false", incutwinId.c_str());
    } else {
        LOG_WARN(TAG, "holdDetected HTTP=%d", code);
        if (code == 401) gIdToken = "";
    }
    http.end();
}

} // namespace FirebaseRTDB
