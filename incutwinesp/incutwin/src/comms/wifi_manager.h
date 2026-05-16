#pragma once
// =============================================================================
// wifi_manager.h — Gestión WiFi con Captive Portal y reconexión
// FR-201 a FR-212 / INT-202
// =============================================================================
#include <Arduino.h>

namespace WifiMgr {
    void init();

    // Intenta conectar con credenciales en NVS. Si no hay, abre Captive Portal.
    void connect();
    void disconnect();
    bool isConnected();

    // Abre el Captive Portal manualmente
    void startCaptivePortal();

    // Resetea credenciales (borra NVS wifi)
    void clearCredentials();

    // Llamar en el loop principal — gestiona reconexión no bloqueante
    void update();

    // Devuelve la IP actual como String
    String getIP();
    int    getRSSI();
}
