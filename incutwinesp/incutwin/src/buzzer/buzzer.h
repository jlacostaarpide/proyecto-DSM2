#pragma once
// =============================================================================
// buzzer.h — Control del buzzer piezoeléctrico
// FR-107 / INT-104
// =============================================================================
#include <Arduino.h>

namespace Buzzer {
    void init();

    // Genera un tono de frecuencia freq_hz durante duration_ms
    void beep(uint16_t freq_hz, uint32_t duration_ms);

    // Patrón de N beeps
    void beepPattern(uint16_t freq_hz, uint32_t duration_ms, uint8_t count, uint32_t gap_ms);

    // Actualización rate limiter — llamar en cada ciclo del loop
    void update();

    bool selfTest();
}
