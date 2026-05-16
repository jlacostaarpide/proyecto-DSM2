#pragma once
// =============================================================================
// flash_store.h — Almacenamiento persistente en NVS (ESP32 Preferences)
// INT-201
// =============================================================================
#include <Arduino.h>

namespace FlashStore {
    void init();
    void clear();

    String getString(const char* key, const String& defaultVal = "");
    bool   setString(const char* key, const String& val);

    bool   getBool(const char* key, bool defaultVal = false);
    bool   setBool(const char* key, bool val);

    int    getInt(const char* key, int defaultVal = 0);
    bool   setInt(const char* key, int val);
}
