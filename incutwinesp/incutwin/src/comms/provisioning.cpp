// =============================================================================
// provisioning.cpp — Provisionamiento ThingsBoard vía MQTT
// =============================================================================
#include "provisioning.h"
#include "../serial_debug/serial_debug.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static const char* TAG = "PROV";

// Estado compartido entre callback y función principal
static volatile bool _responseReceived = false;
static String        _receivedToken    = "";

// Topics MQTT para provisionamiento (estándar ThingsBoard)
static const char* TOPIC_PROV_REQUEST  = "/provision/request";
static const char* TOPIC_PROV_RESPONSE = "/provision/response";

// MQTT client temporal para provisionamiento
static WiFiClient   _wifiClient;
static PubSubClient _mqttClient(_wifiClient);

static void onProvisionResponse(char* topic, byte* payload, unsigned int length) {
    LOG_INFO(TAG, "Respuesta recibida en topic: %s", topic);
    
    // Loguear payload raw para depuración si es necesario
    String rawPayload = "";
    for (unsigned int i = 0; i < length; i++) rawPayload += (char)payload[i];
    LOG_INFO(TAG, "Payload: %s", rawPayload.c_str());

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        LOG_ERROR(TAG, "Error parseando JSON de provision: %s", err.c_str());
        return;
    }
    const char* status = doc["status"] | "";
    if (strcmp(status, "SUCCESS") == 0) {
        const char* token = doc["credentialsValue"] | "";
        _receivedToken    = String(token);
        LOG_INFO(TAG, "Token recibido correctamente ✓");
    } else {
        const char* errorMsg = doc["errorMsg"] | "unknown error";
        LOG_ERROR(TAG, "Provision fallido. Status: %s. Error: %s", status, errorMsg);
    }
    _responseReceived = true;
}

namespace Provisioning {

String buildDeviceName() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buf[20];
    snprintf(buf, sizeof(buf), "INCUTWIN-%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(buf);
}

String provision(const String& deviceName) {
    LOG_INFO(TAG, "Iniciando provisionamiento para: %s", deviceName.c_str());

    _mqttClient.setServer(TB_SERVER, TB_PORT);
    _mqttClient.setCallback(onProvisionResponse);
    _mqttClient.setBufferSize(TB_MQTT_BUFFER_SIZE);

    // Conectar con credenciales de provision
    LOG_INFO(TAG, "Conectando a %s:%d", TB_SERVER, TB_PORT);
    if (!_mqttClient.connect(deviceName.c_str(), "provision", "provision")) {
        LOG_ERROR(TAG, "Error conectando para provision (estado=%d)", _mqttClient.state());
        return "";
    }
    LOG_INFO(TAG, "Conectado al broker de provision");

    // Suscribirse al topic de respuesta
    LOG_INFO(TAG, "Suscribiendo a %s...", TOPIC_PROV_RESPONSE);
    if (!_mqttClient.subscribe(TOPIC_PROV_RESPONSE)) {
        LOG_ERROR(TAG, "Error suscribiendo al topic de respuesta");
        _mqttClient.disconnect();
        return "";
    }

    // Publicar solicitud
    StaticJsonDocument<256> req;
    req["deviceName"]           = deviceName;
    req["provisionDeviceKey"]   = TB_PROVISION_KEY;
    req["provisionDeviceSecret"]= TB_PROVISION_SECRET;
    char reqBuf[256];
    serializeJson(req, reqBuf);

    LOG_INFO(TAG, "Publicando solicitud en %s...", TOPIC_PROV_REQUEST);
    if (!_mqttClient.publish(TOPIC_PROV_REQUEST, reqBuf)) {
        LOG_ERROR(TAG, "Error publicando solicitud de provision");
        _mqttClient.disconnect();
        return "";
    }

    // Esperar respuesta con timeout
    uint32_t start = millis();
    _responseReceived = false;
    while (!_responseReceived && (millis() - start < TB_PROVISION_TIMEOUT_MS)) {
        _mqttClient.loop();
        delay(50);
    }

    _mqttClient.disconnect();

    if (!_responseReceived) {
        LOG_ERROR(TAG, "Timeout esperando respuesta de provision");
        return "";
    }

    return _receivedToken;
}

} // namespace Provisioning
