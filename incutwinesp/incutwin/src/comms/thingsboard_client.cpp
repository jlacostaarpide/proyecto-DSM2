// =============================================================================
// thingsboard_client.cpp — Cliente ThingsBoard MQTT
// =============================================================================
#include "thingsboard_client.h"
#include "../serial_debug/serial_debug.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static const char* TAG = "TB";

static WiFiClient   _wifiClient;
static PubSubClient _mqtt(_wifiClient);
static String       _token;
static String       _deviceName;
static uint32_t     _heartbeatTimer = 0;
static uint32_t     _reconnectTimer = 0;

// Estado extra para heartbeat (actualizado por main.cpp vía setHeartbeatState)
static bool        _hbProxLeft  = false;
static bool        _hbProxRight = false;
static bool        _hbHandDet   = false;
static int         _hbAlsRaw    = 0;
static const char* _hbState     = "BOOT";
static String      _hbLinkedId  = "";

static void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    LOG_INFO(TAG, "Mensaje en topic: %s (%u bytes)", topic, length);
}

namespace TBClient {

void init(const String& token, const String& deviceName) {
    _token      = token;
    _deviceName = deviceName;
    _mqtt.setServer(TB_SERVER, TB_PORT);
    _mqtt.setCallback(onMqttMessage);
    _mqtt.setBufferSize(TB_MQTT_BUFFER_SIZE);
    LOG_INFO(TAG, "Inicializado. Dispositivo: %s", deviceName.c_str());
}

bool connect() {
    if (_mqtt.connected()) return true;
    if (_token.length() == 0) {
        LOG_ERROR(TAG, "Sin token — no se puede conectar");
        return false;
    }
    LOG_INFO(TAG, "Conectando a %s:%d como '%s'", TB_SERVER, TB_PORT, _deviceName.c_str());
    bool ok = _mqtt.connect(_deviceName.c_str(), _token.c_str(), nullptr);
    if (ok) {
        LOG_INFO(TAG, "Conectado a ThingsBoard ✓");
        // Publicar atributo de arranque
        StaticJsonDocument<128> attr;
        attr["firmware_version"] = FIRMWARE_VERSION;
        char buf[128];
        serializeJson(attr, buf);
        _mqtt.publish("v1/devices/me/attributes", buf);
    } else {
        LOG_ERROR(TAG, "Error conectando. Estado MQTT: %d", _mqtt.state());
    }
    return ok;
}

void disconnect() {
    _mqtt.disconnect();
    LOG_INFO(TAG, "Desconectado de ThingsBoard");
}

bool isConnected() {
    return _mqtt.connected();
}

bool publishTelemetry(const JsonObject& data) {
    if (!_mqtt.connected()) {
        LOG_WARN(TAG, "MQTT desconectado — telemetría descartada");
        return false;
    }
    char buf[TB_MQTT_BUFFER_SIZE];
    serializeJson(data, buf);
    bool ok = _mqtt.publish("v1/devices/me/telemetry", buf);
    if (ok) LOG_INFO(TAG, "Telemetry published");
    else    LOG_WARN(TAG, "Error publicando telemetría");
    return ok;
}

bool publishEvent(const char* key, bool val) {
    if (!_mqtt.connected()) return false;
    StaticJsonDocument<128> doc;
    doc[key] = val;
    char buf[128];
    serializeJson(doc, buf);
    return _mqtt.publish("v1/devices/me/telemetry", buf);
}

bool sendSharedAttributes(const JsonObject& data) {
    if (!_mqtt.connected()) {
        LOG_WARN(TAG, "MQTT desconectado — shared attrs descartados");
        return false;
    }
    char buf[TB_MQTT_BUFFER_SIZE];
    serializeJson(data, buf);
    bool ok = _mqtt.publish("v1/devices/me/attributes", buf);
    if (ok) LOG_INFO(TAG, "Shared attrs publicados");
    else    LOG_WARN(TAG, "Error publicando shared attrs");
    return ok;
}

void setHeartbeatState(bool proxL, bool proxR, bool handDet, int alsRaw,
                       const char* stateName, const char* linkedId) {
    _hbProxLeft  = proxL;
    _hbProxRight = proxR;
    _hbHandDet   = handDet;
    _hbAlsRaw    = alsRaw;
    _hbState     = stateName;
    _hbLinkedId  = linkedId ? linkedId : "";
}

bool publishLinkEvent(bool linked, const String& incunestId, const char* reason) {
    if (!_mqtt.connected()) return false;
    StaticJsonDocument<256> doc;
    doc["linked"] = linked;
    if (linked) {
        doc["incunest_id"] = incunestId;
        LOG_INFO(TAG, "Evento: incunest_linked → %s", incunestId.c_str());
    } else {
        doc["reason"] = reason ? reason : "unknown";
        LOG_INFO(TAG, "Evento: incunest_unlinked (reason: %s)", reason ? reason : "unknown");
    }
    char buf[256];
    serializeJson(doc, buf);
    return _mqtt.publish("v1/devices/me/telemetry", buf);
}

void reconnect() {
    if (_mqtt.connected() || _token.length() == 0) return;
    uint32_t now = millis();
    if (now - _reconnectTimer < 5000) return;
    _reconnectTimer = now;
    LOG_INFO(TAG, "Intentando reconexión MQTT...");
    connect();
}

void update() {
    if (!_mqtt.connected()) {
        reconnect();
        return;
    }
    _mqtt.loop();

    // Heartbeat cada TB_HEARTBEAT_INTERVAL_MS
    uint32_t now = millis();
    if (now - _heartbeatTimer >= TB_HEARTBEAT_INTERVAL_MS) {
        _heartbeatTimer = now;
        StaticJsonDocument<384> doc;
        doc["uptime_s"]        = now / 1000;
        doc["rssi"]            = WiFi.RSSI();
        doc["heap_free"]       = esp_get_free_heap_size();
        doc["proximity_left"]  = _hbProxLeft;
        doc["proximity_right"] = _hbProxRight;
        doc["hand_detected"]   = _hbHandDet;
        doc["als_raw"]         = _hbAlsRaw;
        doc["device_state"]    = _hbState;
        JsonObject obj = doc.as<JsonObject>();
        publishTelemetry(obj);
    }
}

} // namespace TBClient
