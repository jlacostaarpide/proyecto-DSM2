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

void setHoldDetected(bool value) {
    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARN(TAG, "Sin WiFi — holdDetected no enviado");
        return;
    }
    if (gIdToken.isEmpty()) {
        LOG_WARN(TAG, "Sin token — holdDetected no enviado");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String url = String(FIREBASE_RTDB_URL)
               + "/incutwins/" + FIREBASE_INCUTWIN_ID
               + "/holdDetected.json?auth=" + gIdToken;

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    int code = http.PUT(value ? "true" : "false");

    if (code == 200) {
        LOG_INFO(TAG, "holdDetected=%s OK", value ? "true" : "false");
    } else {
        LOG_WARN(TAG, "holdDetected=%s HTTP=%d", value ? "true" : "false", code);
        if (code == 401) gIdToken = ""; // forzar renovación en próximo update()
    }

    http.end();
}

} // namespace FirebaseRTDB
