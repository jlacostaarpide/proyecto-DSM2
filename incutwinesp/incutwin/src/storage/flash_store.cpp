// =============================================================================
// flash_store.cpp — Almacenamiento NVS con Preferences ESP32
// =============================================================================
#include "flash_store.h"
#include "../serial_debug/serial_debug.h"
#include "config.h"
#include <Preferences.h>

static const char* TAG = "NVS";
static Preferences _prefs;

namespace FlashStore {

void init() {
    if (!_prefs.begin(NVS_NAMESPACE, false)) {
        LOG_ERROR(TAG, "Error inicializando NVS — intentando borrar y reiniciar");
        _prefs.clear();
        _prefs.begin(NVS_NAMESPACE, false);
    }
    LOG_INFO(TAG, "NVS inicializado (namespace: %s)", NVS_NAMESPACE);
}

void clear() {
    _prefs.clear();
    LOG_WARN(TAG, "NVS borrado completamente");
}

String getString(const char* key, const String& defaultVal) {
    return _prefs.getString(key, defaultVal);
}

bool setString(const char* key, const String& val) {
    size_t written = _prefs.putString(key, val);
    if (written == 0) {
        LOG_ERROR(TAG, "Error escribiendo clave '%s'", key);
        return false;
    }
    LOG_INFO(TAG, "setString('%s') OK", key);
    return true;
}

bool getBool(const char* key, bool defaultVal) {
    return _prefs.getBool(key, defaultVal);
}

bool setBool(const char* key, bool val) {
    _prefs.putBool(key, val);
    LOG_INFO(TAG, "setBool('%s', %d) OK", key, val);
    return true;
}

int getInt(const char* key, int defaultVal) {
    return _prefs.getInt(key, defaultVal);
}

bool setInt(const char* key, int val) {
    _prefs.putInt(key, val);
    LOG_INFO(TAG, "setInt('%s', %d) OK", key, val);
    return true;
}

} // namespace FlashStore
