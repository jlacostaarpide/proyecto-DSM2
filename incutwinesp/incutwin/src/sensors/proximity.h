#pragma once
// =============================================================================
// proximity.h — Sensores de proximidad IR (LED emisor + fototransistor)
// FR-102, FR-103, FR-104, FR-105 / INT-101, INT-103
// =============================================================================
#include <Arduino.h>

namespace Proximity {
    void init();

    // Control LEDs emisores IR
    void setLeft(bool on);
    void setRight(bool on);

    // Lectura ADC promediada con encendido/apagado automático del LED
    int readLeft();
    int readRight();

    // Test periférico — devuelve true si delta > umbral
    bool selfTestLeft();
    bool selfTestRight();
}
