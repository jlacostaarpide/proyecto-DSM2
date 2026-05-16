// =============================================================================
// led_ring.cpp — Control WS2812B con FastLED
// =============================================================================
#include "led_ring.h"
#include "../serial_debug/serial_debug.h"
#include "config.h"
#include <FastLED.h>

static const char* TAG = "RING";

static CRGB _leds[LED_RING_COUNT];
static uint8_t _brightness = LED_RING_BRIGHTNESS;  // 0-255
static uint32_t _animTimer = 0;
static uint8_t  _animStep  = 0;
static SystemState _lastState = SystemState::BOOT;

namespace LedRing {

void init() {
    FastLED.addLeds<WS2812B, PIN_LED_RING, GRB>(_leds, LED_RING_COUNT);
    FastLED.setBrightness(_brightness);
    FastLED.clear(true);
    LOG_INFO(TAG, "Inicializado (%d LEDs, IO%d, brillo=%d)", LED_RING_COUNT, PIN_LED_RING, _brightness);
}

void setAll(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < LED_RING_COUNT; i++) {
        _leds[i] = CRGB(r, g, b);
    }
    LOG_INFO(TAG, "SetAll: R=%d G=%d B=%d", r, g, b);
}

void setPixel(uint8_t n, uint8_t r, uint8_t g, uint8_t b) {
    if (n < LED_RING_COUNT) {
        _leds[n] = CRGB(r, g, b);
        LOG_INFO(TAG, "Set pixel %d: R=%d G=%d B=%d", n, r, g, b);
    }
}

void clear() {
    FastLED.clear();
    FastLED.show();
}

void setBrightness(uint8_t pct) {
    _brightness = (uint8_t)(pct * 255 / 100);
    FastLED.setBrightness(_brightness);
}

void show() {
    FastLED.show();
}

// Barrido blanco de arranque — BLOQUEANTE (solo se usa en setup)
void runBootSweep() {
    LOG_INFO(TAG, "Boot sweep");
    FastLED.clear();
    for (int i = 0; i < LED_RING_COUNT; i++) {
        _leds[i] = CRGB::White;
        FastLED.show();
        delay(LED_BOOT_SWEEP_MS);
        _leds[i] = CRGB::Black;
    }
    FastLED.show();
}

// Test colores — BLOQUEANTE (solo para autotest)
void runRainbowTest() {
    LOG_INFO(TAG, "Rainbow test: Rojo");
    setAll(255, 0, 0); FastLED.show(); delay(400);
    LOG_INFO(TAG, "Rainbow test: Verde");
    setAll(0, 255, 0); FastLED.show(); delay(400);
    LOG_INFO(TAG, "Rainbow test: Azul");
    setAll(0, 0, 255); FastLED.show(); delay(400);
    clear();
}

bool selfTest() {
    runRainbowTest();
    LOG_INFO(TAG, "Self-test completado");
    return true;
}

// Calcula brillo sinusoidal para animaciones de pulso
static uint8_t sineWave(uint32_t period_ms, uint8_t minBri, uint8_t maxBri) {
    float t   = (float)(millis() % period_ms) / (float)period_ms;
    float val = (sinf(t * 2.0f * PI) + 1.0f) / 2.0f;
    return (uint8_t)(minBri + val * (maxBri - minBri));
}

// Actualización no bloqueante según estado
void update(SystemState state) {
    if (state != _lastState) {
        _animStep  = 0;
        _animTimer = millis();
        _lastState = state;
        LOG_INFO(TAG, "Estado: %d", (int)state);
    }

    uint32_t now = millis();

    switch (state) {
        case SystemState::BOOT:
            // Barrido ya ejecutado en setup; aquí: blanco fijo muy bajo
            setAll(20, 20, 20);
            FastLED.show();
            break;

        case SystemState::WIFI_CONNECTING: {
            // Respiración azul 1 Hz
            uint8_t bri = sineWave(1000, 10, 80);
            FastLED.setBrightness(bri);
            setAll(0, 0, 255);
            FastLED.show();
            FastLED.setBrightness(_brightness);
            break;
        }

        case SystemState::WIFI_CONNECTED:
            // Azul sólido tenue
            setAll(0, 0, 76);
            FastLED.show();
            break;

        case SystemState::PROVISIONING: {
            // Parpadeo cian 2 Hz
            uint8_t on = ((now / 250) % 2 == 0) ? 1 : 0;
            if (on) setAll(0, 255, 255);
            else    clear();
            FastLED.show();
            break;
        }

        case SystemState::SERVER_CONNECTED:
            // Cian sólido 50% brillo
            setAll(0, 127, 127);
            FastLED.show();
            break;

        case SystemState::IDLE: {
            // Respiración muy lenta blanco cálido (0.2 Hz, brillo 5%)
            uint8_t bri = sineWave(5000, 2, 13);
            FastLED.setBrightness(bri);
            setAll(255, 200, 100);
            FastLED.show();
            FastLED.setBrightness(_brightness);
            break;
        }

        case SystemState::PROXIMITY_LEFT:
            // Semianillo izquierdo naranja (LEDs 4–7), resto apagado
            for (int i = 0; i < LED_RING_COUNT; i++) {
                if (i >= 4 && i <= 7) _leds[i] = CRGB(255, 80, 0);
                else                  _leds[i] = CRGB::Black;
            }
            FastLED.show();
            break;

        case SystemState::PROXIMITY_RIGHT:
            // Semianillo derecho naranja (LEDs 0–3), resto apagado
            for (int i = 0; i < LED_RING_COUNT; i++) {
                if (i >= 0 && i <= 3) _leds[i] = CRGB(255, 80, 0);
                else                  _leds[i] = CRGB::Black;
            }
            FastLED.show();
            break;

        case SystemState::PROXIMITY_BOTH: {
            // Anillo amarillo completo, pulso suave
            uint8_t bri = sineWave(500, 40, 120);
            FastLED.setBrightness(bri);
            setAll(255, 200, 0);
            FastLED.show();
            FastLED.setBrightness(_brightness);
            break;
        }

        case SystemState::SEARCHING: {
            // Perseguidor circular amarillo (chase 2 Hz)
            uint8_t pos = (uint8_t)((now / (LED_CHASE_PERIOD_MS / LED_RING_COUNT)) % LED_RING_COUNT);
            for (int i = 0; i < LED_RING_COUNT; i++) {
                _leds[i] = (i == pos) ? CRGB(255, 200, 0) : CRGB::Black;
            }
            FastLED.show();
            break;
        }

        case SystemState::LINKED: {
            // Verde latido sinusoidal ~800 ms
            uint8_t bri = sineWave(LED_HEARTBEAT_PERIOD_MS, 5, 80);
            FastLED.setBrightness(bri);
            setAll(0, 255, 80);
            FastLED.show();
            FastLED.setBrightness(_brightness);
            break;
        }

        case SystemState::ERROR_STATE: {
            // Rojo parpadeo 2 Hz
            uint8_t on = ((now / 250) % 2 == 0) ? 1 : 0;
            if (on) setAll(255, 0, 0);
            else    clear();
            FastLED.show();
            break;
        }

        default:
            break;
    }
}

// Demo Fase 1: fade cíclico no bloqueante R→B→G→W (0 al máximo)
void runPhase1FadeLoop() {
    static uint8_t  _fadeColor   = 0;   // 0=R, 1=B, 2=G, 3=W
    static uint8_t  _fadeVal     = 0;
    static uint32_t _fadeTimer   = 0;
    const  uint32_t STEP_MS      = 4;   // ~1 s por color (256 pasos × 4 ms)

    uint32_t now = millis();
    if (now - _fadeTimer < STEP_MS) return;
    _fadeTimer = now;

    switch (_fadeColor) {
        case 0: setAll(_fadeVal, 0,        0       ); break;  // rojo
        case 1: setAll(0,        0,        _fadeVal); break;  // azul
        case 2: setAll(0,        _fadeVal, 0       ); break;  // verde
        case 3: setAll(_fadeVal, _fadeVal, _fadeVal); break;  // blanco
    }
    FastLED.show();

    if (_fadeVal == 255) {
        _fadeVal   = 0;
        _fadeColor = (_fadeColor + 1) % 4;
    } else {
        _fadeVal++;
    }
}

} // namespace LedRing
