// =============================================================================
// buzzer.cpp — Implementación control buzzer
// =============================================================================
#include "buzzer.h"
#include "../serial_debug/serial_debug.h"
#include "config.h"

static const char* TAG = "BUZZ";

// Rate limiter
static uint8_t  _beepCount   = 0;
static uint32_t _windowStart = 0;

namespace Buzzer {

void init() {
    ledcSetup(BUZZER_LEDC_CHANNEL, 1000, BUZZER_LEDC_RES);
    ledcAttachPin(PIN_BUZZER, BUZZER_LEDC_CHANNEL);
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);  // silencio
    _windowStart = millis();
    LOG_INFO(TAG, "Inicializado (IO%d, canal LEDC %d)", PIN_BUZZER, BUZZER_LEDC_CHANNEL);
}

void beep(uint16_t freq_hz, uint32_t duration_ms) {
    // Rate limiting: máx BUZZ_RATE_LIMIT_N beeps en BUZZ_RATE_LIMIT_MS
    uint32_t now = millis();
    if (now - _windowStart > BUZZ_RATE_LIMIT_MS) {
        _beepCount   = 0;
        _windowStart = now;
    }
    if (_beepCount >= BUZZ_RATE_LIMIT_N) {
        LOG_WARN(TAG, "Rate limit alcanzado — beep suprimido");
        return;
    }
    _beepCount++;

    LOG_INFO(TAG, "Beep: %uHz, %ums", freq_hz, duration_ms);
    ledcSetup(BUZZER_LEDC_CHANNEL, freq_hz, BUZZER_LEDC_RES);
    ledcAttachPin(PIN_BUZZER, BUZZER_LEDC_CHANNEL);
    ledcWrite(BUZZER_LEDC_CHANNEL, 128);  // duty 50%
    delay(duration_ms);
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

void beepPattern(uint16_t freq_hz, uint32_t duration_ms, uint8_t count, uint32_t gap_ms) {
    for (uint8_t i = 0; i < count; i++) {
        beep(freq_hz, duration_ms);
        if (i < count - 1) delay(gap_ms);
    }
}

void update() {
    uint32_t now = millis();
    if (now - _windowStart > BUZZ_RATE_LIMIT_MS) {
        _beepCount   = 0;
        _windowStart = now;
    }
}

bool selfTest() {
    LOG_INFO(TAG, "Self-test: emitiendo tono de arranque");
    beep(BUZZ_FREQ_BOOT, BUZZ_DUR_BOOT);
    return true;  // Si no hubo excepción, OK
}

} // namespace Buzzer
