#pragma once
// =============================================================================
// thingsboard_client.h — Cliente ThingsBoard MQTT
// FR-203 a FR-210 / INT-203
// =============================================================================
#include <Arduino.h>
#include <ArduinoJson.h>

namespace TBClient {
    void init(const String& token, const String& deviceName);

    bool connect();
    void disconnect();
    bool isConnected();

    // Publica telemetría (objeto JSON)
    bool publishTelemetry(const JsonObject& data);

    // Publica un evento simple clave:valor bool
    bool publishEvent(const char* key, bool val);

    // Publica un evento con incunest_id
    bool publishLinkEvent(bool linked, const String& incunestId, const char* reason = nullptr);

    // Publica shared attributes (clave-valor persistente, topic attributes)
    bool sendSharedAttributes(const JsonObject& data);

    // Actualiza estado extra para el próximo heartbeat (llamar desde main loop)
    void setHeartbeatState(bool proxL, bool proxR, bool handDet, int alsRaw,
                           const char* stateName, const char* linkedId = "");

    // Llamar en cada loop — mantiene MQTT activo y heartbeat
    void update();

    // Reconexión no bloqueante
    void reconnect();
}
