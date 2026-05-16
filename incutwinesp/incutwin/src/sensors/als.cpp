// =============================================================================
// als.cpp — Implementación Sensor de Luz Ambiente
// =============================================================================
#include "als.h"
#include "../serial_debug/serial_debug.h"
#include "config.h"

static const char* TAG = "ALS";

namespace ALS {

void init() {
    // IO35 es solo entrada — NO configurar como OUTPUT
    // analogReadResolution ya configurada en proximity.cpp
    LOG_INFO(TAG, "Inicializado (IO%d)", PIN_ALS);
}

int readAmbientLight() {
    long sum = 0;
    for (int i = 0; i < PROX_ADC_SAMPLES; i++) {
        sum += analogRead(PIN_ALS);
        delayMicroseconds(200);
    }
    int val = (int)(sum / PROX_ADC_SAMPLES);
    LOG_INFO(TAG, "Ambient: %d", val);
    return val;
}

bool isSaturated() {
    return readAmbientLight() > ALS_SATURATION_THRESHOLD;
}

} // namespace ALS
