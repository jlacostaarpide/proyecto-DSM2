// =============================================================================
// wifi_manager.cpp — WiFi con Captive Portal (WiFiManager)
// =============================================================================
#include "wifi_manager.h"
#include "../serial_debug/serial_debug.h"
#include "../storage/flash_store.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiManager.h>

static const char* TAG = "WIFI";

static WiFiManager _wm;
static bool        _connected    = false;
static uint8_t     _retryCount   = 0;
static uint32_t    _retryTimer   = 0;
static uint32_t    _backoffMs    = WIFI_BACKOFF_BASE_MS;

// Genera el SSID del AP a partir de los últimos 6 bytes de la MAC
static String _buildAPName() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buf[24];
    snprintf(buf, sizeof(buf), "%s%02X%02X%02X",
             WIFI_SSID_PREFIX, mac[3], mac[4], mac[5]);
    return String(buf);
}

namespace WifiMgr {

void init() {
    WiFi.mode(WIFI_STA);
    _wm.setConfigPortalTimeout(WIFI_CAPTIVE_TIMEOUT_S);
    _wm.setSaveConfigCallback([]() {
        LOG_INFO(TAG, "Credenciales recibidas — guardando en NVS");
        // WiFiManager guarda en su propia NVS; aquí actualizamos la nuestra
        FlashStore::setString(NVS_KEY_WIFI_SSID, WiFi.SSID());
        FlashStore::setString(NVS_KEY_WIFI_PASS, WiFi.psk());
    });
    LOG_INFO(TAG, "Inicializado");
}

void connect() {
    String ssid = FlashStore::getString(NVS_KEY_WIFI_SSID);
    String pass = FlashStore::getString(NVS_KEY_WIFI_PASS);

    if (ssid.length() > 0) {
        LOG_INFO(TAG, "Conectando a SSID: %s", ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(200);
        }

        if (WiFi.status() == WL_CONNECTED) {
            _connected  = true;
            _retryCount = 0;
            _backoffMs  = WIFI_BACKOFF_BASE_MS;
            LOG_INFO(TAG, "Conectado. IP: %s, RSSI: %d dBm",
                     WiFi.localIP().toString().c_str(), WiFi.RSSI());
            return;
        }

        LOG_WARN(TAG, "No se pudo conectar a '%s' — iniciando Captive Portal", ssid.c_str());
    } else {
        LOG_INFO(TAG, "Sin credenciales — iniciando Captive Portal");
    }

    startCaptivePortal();
}

void startCaptivePortal() {
    String apName = _buildAPName();
    LOG_INFO(TAG, "AP activo: '%s' — IP: 192.168.4.1", apName.c_str());
    bool res = _wm.startConfigPortal(apName.c_str());
    if (res && WiFi.status() == WL_CONNECTED) {
        _connected = true;
        LOG_INFO(TAG, "Captive Portal: conectado. IP: %s", WiFi.localIP().toString().c_str());
    } else {
        LOG_WARN(TAG, "Captive Portal: timeout sin configuración");
    }
}

void clearCredentials() {
    FlashStore::setString(NVS_KEY_WIFI_SSID, "");
    FlashStore::setString(NVS_KEY_WIFI_PASS, "");
    _wm.resetSettings();
    LOG_WARN(TAG, "Credenciales WiFi borradas");
}

void disconnect() {
    WiFi.disconnect();
    _connected = false;
    LOG_INFO(TAG, "Desconectado");
}

bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void update() {
    if (!isConnected() && _connected) {
        _connected = false;
        LOG_WARN(TAG, "Conexión WiFi perdida — intentando reconectar (intento %d)", _retryCount + 1);
    }

    if (!isConnected()) {
        uint32_t now = millis();
        if (now - _retryTimer >= _backoffMs) {
            _retryTimer = now;
            _retryCount++;
            LOG_INFO(TAG, "Reintento WiFi #%d (backoff %ums)", _retryCount, _backoffMs);
            WiFi.reconnect();
            // Backoff exponencial
            _backoffMs = min((uint32_t)(_backoffMs * 2), (uint32_t)WIFI_BACKOFF_MAX_MS);
            if (_retryCount >= WIFI_RECONNECT_MAX) {
                LOG_WARN(TAG, "Máx reintentos alcanzados — iniciando Captive Portal de emergencia");
                startCaptivePortal();
                _retryCount = 0;
                _backoffMs  = WIFI_BACKOFF_BASE_MS;
            }
        }
    } else {
        if (!_connected) {
            _connected  = true;
            _retryCount = 0;
            _backoffMs  = WIFI_BACKOFF_BASE_MS;
            LOG_INFO(TAG, "Reconectado. IP: %s, RSSI: %d dBm",
                     WiFi.localIP().toString().c_str(), WiFi.RSSI());
        }
    }
}

String getIP() {
    return WiFi.localIP().toString();
}

int getRSSI() {
    return WiFi.RSSI();
}

} // namespace WifiMgr
