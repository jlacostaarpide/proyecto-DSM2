#include "firebase_rtdb.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "config.h"
#include "../serial_debug/serial_debug.h"

static const char* TAG = "FBRTDB";

namespace FirebaseRTDB {

void setHoldDetected(bool value) {
    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARN(TAG, "Sin WiFi — holdDetected no enviado");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String url = String(FIREBASE_RTDB_URL)
               + "/incutwins/" + FIREBASE_INCUTWIN_ID
               + "/holdDetected.json";

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    int code = http.PUT(value ? "true" : "false");

    if (code == 200) {
        LOG_INFO(TAG, "holdDetected=%s OK", value ? "true" : "false");
    } else {
        LOG_WARN(TAG, "holdDetected=%s HTTP=%d", value ? "true" : "false", code);
    }

    http.end();
}

} // namespace FirebaseRTDB
