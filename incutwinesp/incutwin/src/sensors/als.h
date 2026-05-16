#pragma once
// =============================================================================
// als.h — Sensor de luz ambiente (Ambient Light Sensor)
// FR-106 / INT-102
// =============================================================================
#include <Arduino.h>

namespace ALS {
    void init();
    int readAmbientLight();  // Devuelve valor ADC 0–4095
    bool isSaturated();      // true si la luz ambiente es excesiva
}
