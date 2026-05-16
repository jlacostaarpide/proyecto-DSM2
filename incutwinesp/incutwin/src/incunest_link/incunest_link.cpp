// =============================================================================
// incunest_link.cpp — Lógica de emparejamiento IncuTwin ↔ IncuNest
//
// AD-301: Comunicación vía MQTT broker ThingsBoard.
// AD-302: Atributo "baby_inside" en shared attributes del IncuNest.
// AD-303: Emparejamiento 1-a-1 con primer IncuNest respondiente.
//
// NOTA: El protocolo exacto IncuNest ↔ IncuTwin está pendiente de
// validación con el equipo de firmware de IncuNest (ver AD-301).
// La implementación actual es un placeholder funcional que puede
// adaptarse una vez definido el protocolo definitivo.
// =============================================================================
#include "incunest_link.h"
#include "../serial_debug/serial_debug.h"
#include "../comms/thingsboard_client.h"
#include "config.h"

static const char* TAG = "INCUNEST";

static bool     _linked       = false;
static bool     _searching    = false;
static String   _linkedId     = "";
static uint32_t _searchStart  = 0;
static uint32_t _lastPollMs   = 0;
static uint32_t _lastCommMs   = 0;

namespace IncuNestLink {

void init() {
    _linked    = false;
    _searching = false;
    _linkedId  = "";
    LOG_INFO(TAG, "Inicializado");
}

void startSearch() {
    if (_searching || _linked) return;
    _searching   = true;
    _searchStart = millis();
    LOG_INFO(TAG, "Iniciando busqueda de IncuNest...");
    TBClient::publishEvent("searching_incunest", true);
}

void unlink(const char* reason) {
    if (!_linked && !_searching) return;
    LOG_INFO(TAG, "Desenlazando IncuNest (razon: %s)", reason);
    if (_linked) {
        TBClient::publishLinkEvent(false, _linkedId, reason);
    }
    _linked    = false;
    _searching = false;
    _linkedId  = "";
}

bool isLinked()    { return _linked;    }
bool isSearching() { return _searching; }
String getLinkedId() { return _linkedId; }

void update() {
    uint32_t now = millis();

    if (_searching) {
        // Timeout de búsqueda
        if (now - _searchStart > INCUNEST_SEARCH_TIMEOUT_MS) {
            LOG_WARN(TAG, "Timeout buscando IncuNest");
            _searching = false;
            TBClient::publishEvent("searching_incunest", false);
            return;
        }

        // Sondeo periódico: en implementación real, suscribirse a
        // shared attributes de dispositivos IncuNest vía MQTT.
        // Por ahora se publica el estado de búsqueda como telemetría.
        if (now - _lastPollMs > INCUNEST_POLL_INTERVAL_MS) {
            _lastPollMs = now;
            LOG_INFO(TAG, "Buscando IncuNest... (%lus transcurridos)",
                     (now - _searchStart) / 1000);
            // TODO: suscribirse a v1/devices/INCUNEST-XXXXXX/attributes
            // y filtrar por baby_inside=true cuando el protocolo esté definido.
        }
    }

    if (_linked) {
        // Timeout de comunicación con IncuNest
        if (now - _lastCommMs > INCUNEST_LINK_TIMEOUT_MS) {
            LOG_WARN(TAG, "Timeout comunicacion IncuNest — enlace perdido");
            unlink("timeout");
        }
    }
}

// Función para ser llamada externamente cuando se detecte un IncuNest
// con baby_inside=true (desde callback MQTT)
void _onIncuNestFound(const String& id) {
    if (!_searching) return;
    LOG_INFO(TAG, "IncuNest encontrado: %s — estableciendo enlace", id.c_str());
    _searching  = false;
    _linked     = true;
    _linkedId   = id;
    _lastCommMs = millis();
    TBClient::publishLinkEvent(true, id);
}

} // namespace IncuNestLink
