#pragma once
// =============================================================================
// led_ring.h — Control del anillo WS2812B (8 LEDs)
// FR-108, FR-111 / INT-105
// =============================================================================
#include <Arduino.h>

// Estados del sistema para la máquina de animaciones
enum class SystemState : uint8_t {
    BOOT = 0,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    PROVISIONING,
    SERVER_CONNECTED,
    IDLE,
    PROXIMITY_LEFT,
    PROXIMITY_RIGHT,
    PROXIMITY_BOTH,
    SEARCHING,
    LINKED,
    ERROR_STATE
};

namespace LedRing {
    void init();
    void setAll(uint8_t r, uint8_t g, uint8_t b);
    void setPixel(uint8_t n, uint8_t r, uint8_t g, uint8_t b);
    void clear();
    void setBrightness(uint8_t pct);  // 0–100%
    void show();

    // Animaciones específicas de estado
    void update(SystemState state);
    void runBootSweep();            // Animación de arranque
    void runRainbowTest();          // Test de colores

    bool selfTest();
    void runPhase1FadeLoop();  // Bucle de demo Fase 1: fade R→B→G→W
}
