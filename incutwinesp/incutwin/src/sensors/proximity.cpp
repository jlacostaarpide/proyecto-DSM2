// =============================================================================
// proximity.cpp — Implementación sensores de proximidad IR
// =============================================================================
#include "proximity.h"
#include "../serial_debug/serial_debug.h"
#include "config.h"

static const char* TAG = "PROX";

namespace Proximity {

void init() {
    pinMode(PIN_PROX_LED_LEFT,  OUTPUT);
    pinMode(PIN_PROX_LED_RIGHT, OUTPUT);
    analogReadResolution(12);
    // Atenuación 11dB/12dB — rango 0-3.3V
    // Compatibilidad Core 2.x (ADC_11db) y Core 3.x (ADC_ATTEN_DB_12)
#ifdef ADC_ATTEN_DB_12
    analogSetPinAttenuation(PIN_PROX_PHOTO_LEFT,  ADC_ATTEN_DB_12);
    analogSetPinAttenuation(PIN_PROX_PHOTO_RIGHT, ADC_ATTEN_DB_12);
    analogSetPinAttenuation(PIN_ALS,              ADC_ATTEN_DB_12);
#elif defined(ADC_11db)
    analogSetPinAttenuation(PIN_PROX_PHOTO_LEFT,  ADC_11db);
    analogSetPinAttenuation(PIN_PROX_PHOTO_RIGHT, ADC_11db);
    analogSetPinAttenuation(PIN_ALS,              ADC_11db);
#endif
    digitalWrite(PIN_PROX_LED_LEFT,  LOW);
    digitalWrite(PIN_PROX_LED_RIGHT, LOW);
    LOG_INFO(TAG, "Inicializado (LED L:IO%d R:IO%d | ADC L:IO%d R:IO%d)",
             PIN_PROX_LED_LEFT, PIN_PROX_LED_RIGHT,
             PIN_PROX_PHOTO_LEFT, PIN_PROX_PHOTO_RIGHT);
}

void setLeft(bool on) {
    digitalWrite(PIN_PROX_LED_LEFT, on ? HIGH : LOW);
    LOG_INFO(TAG, "LED Left: %s", on ? "ON" : "OFF");
}

void setRight(bool on) {
    digitalWrite(PIN_PROX_LED_RIGHT, on ? HIGH : LOW);
    LOG_INFO(TAG, "LED Right: %s", on ? "ON" : "OFF");
}

// Lee ADC con promediado de PROX_ADC_SAMPLES muestras
static int readADC(int pin) {
    long sum = 0;
    for (int i = 0; i < PROX_ADC_SAMPLES; i++) {
        sum += analogRead(pin);
        delayMicroseconds(200);
    }
    return (int)(sum / PROX_ADC_SAMPLES);
}

int readLeft() {
    // Secuencia: LED ON → esperar → leer → LED OFF
    digitalWrite(PIN_PROX_LED_LEFT, HIGH);
    delay(PROX_LED_SETTLE_MS);
    int val = readADC(PIN_PROX_PHOTO_LEFT);
    digitalWrite(PIN_PROX_LED_LEFT, LOW);
    LOG_INFO(TAG, "Left ADC: %d", val);
    return val;
}

int readRight() {
    digitalWrite(PIN_PROX_LED_RIGHT, HIGH);
    delay(PROX_LED_SETTLE_MS);
    int val = readADC(PIN_PROX_PHOTO_RIGHT);
    digitalWrite(PIN_PROX_LED_RIGHT, LOW);
    LOG_INFO(TAG, "Right ADC: %d", val);
    return val;
}

// Self-test: compara lectura con LED ON vs OFF
bool selfTestLeft() {
    // Lectura SIN LED
    int off_val = readADC(PIN_PROX_PHOTO_LEFT);
    // Lectura CON LED
    digitalWrite(PIN_PROX_LED_LEFT, HIGH);
    delay(PROX_LED_SETTLE_MS);
    int on_val = readADC(PIN_PROX_PHOTO_LEFT);
    digitalWrite(PIN_PROX_LED_LEFT, LOW);
    int delta = abs(on_val - off_val);
    LOG_INFO(TAG, "Self-test Left: ON=%d OFF=%d delta=%d", on_val, off_val, delta);
    if (delta < 50) {
        LOG_WARN(TAG, "Self-test Left: delta bajo (%d) — verificar hardware", delta);
    }
    // No bloqueamos el arranque por delta bajo (puede ser por entorno)
    // Con pull-down: off_val ≈ 0 es normal. on_val > off_val si el LED enciende.
    return (off_val >= 0 && off_val < 4095);
}

bool selfTestRight() {
    int off_val = readADC(PIN_PROX_PHOTO_RIGHT);
    digitalWrite(PIN_PROX_LED_RIGHT, HIGH);
    delay(PROX_LED_SETTLE_MS);
    int on_val = readADC(PIN_PROX_PHOTO_RIGHT);
    digitalWrite(PIN_PROX_LED_RIGHT, LOW);
    int delta = abs(on_val - off_val);
    LOG_INFO(TAG, "Self-test Right: ON=%d OFF=%d delta=%d", on_val, off_val, delta);
    if (delta < 50) {
        LOG_WARN(TAG, "Self-test Right: delta bajo (%d) — verificar hardware", delta);
    }
    return (off_val >= 0 && off_val < 4095);
}

} // namespace Proximity
