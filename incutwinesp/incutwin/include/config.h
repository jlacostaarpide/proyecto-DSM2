#pragma once
// =============================================================================
// config.h — Constantes globales IncuTwin
// Medicina Abierta al Mundo (MOW)
// =============================================================================

// --- Versión de firmware ---
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0.0"
#endif

// --- Fase de compilación ---
#ifndef PHASE
#define PHASE 1
#endif

// =============================================================================
// PINES GPIO
// =============================================================================
#define PIN_PROX_LED_LEFT 27  // LED IR proximidad izquierdo (OUTPUT)
#define PIN_PROX_LED_RIGHT 15 // LED IR proximidad derecho (OUTPUT)
#define PIN_PROX_PHOTO_LEFT                                                    \
  39 // Fototransistor izquierdo — SENSOR_VN (ADC1_CH3)
#define PIN_PROX_PHOTO_RIGHT 33 // Fototransistor derecho (ADC1_CH5)
#define PIN_ALS 35              // Sensor luz ambiente (ADC1_CH7, solo entrada)
#define PIN_BUZZER 13           // Buzzer piezoeléctrico (OUTPUT PWM)
#define PIN_LED_RING 19         // WS2812B anillo (NeoPixel / FastLED)

// --- LEDs anillo ---
#define LED_RING_COUNT 8 // Número de LEDs WS2812B
#define LED_RING_BRIGHTNESS                                                    \
  60 // Brillo máximo (0-255). ~24% — apto entorno clínico

// --- LEDC (buzzer PWM) ---
#define BUZZER_LEDC_CHANNEL 0
#define BUZZER_LEDC_RES 8 // Resolución 8 bits

// =============================================================================
// DETECCIÓN DE PROXIMIDAD
// =============================================================================
// Circuito: fototransistor con pull-down (R11=100k a GND).
// Sin objeto → ADC ≈ 0.  Con objeto cerca → ADC SUBE (más corriente por
// R10+R11). PROX_THRESHOLD_DETECT : ADC > este valor → presencia detectada
// PROX_THRESHOLD_RELEASE: ADC < este valor → presencia liberada (histéresis)
#define PROX_THRESHOLD_DETECT  15  // ADC > 15   → objeto detectado
#define PROX_THRESHOLD_RELEASE  8  // ADC < 8    → objeto retirado
// Alias invertidos eliminados — usar DETECT/RELEASE en lugar de LOW/HIGH
#define PROX_THRESHOLD_LOW PROX_THRESHOLD_RELEASE
#define PROX_THRESHOLD_HIGH PROX_THRESHOLD_DETECT
#define PROX_DEBOUNCE_MS 100 // Tiempo de debounce en ms
#define PROX_ADC_SAMPLES 4   // Número de muestras para promediado ADC
#define PROX_LED_SETTLE_MS 2 // Tiempo de estabilización LED emisor (ms)
#define ALS_SATURATION_THRESHOLD 4000 // ALS > este valor → suprimir detección

// =============================================================================
// BUZZER
// =============================================================================
#define BUZZ_FREQ_BOOT 1000      // Hz — beep de arranque
#define BUZZ_DUR_BOOT 100        // ms
#define BUZZ_FREQ_PROX 1500      // Hz — detección de presencia
#define BUZZ_DUR_PROX 50         // ms
#define BUZZ_FREQ_DISCONNECT 800 // Hz — desconexión IncuNest
#define BUZZ_DUR_DISCONNECT 80   // ms
#define BUZZ_RATE_LIMIT_N 3      // máx beeps por ventana
#define BUZZ_RATE_LIMIT_MS 5000  // ventana de rate limiting

// =============================================================================
// CONECTIVIDAD
// =============================================================================
#define WIFI_SSID_PREFIX "IncuTwin-"
#define WIFI_CAPTIVE_TIMEOUT_S 120
#define WIFI_RECONNECT_MAX 5
#define WIFI_BACKOFF_BASE_MS 1000
#define WIFI_BACKOFF_MAX_MS 30000

// ThingsBoard
#define TB_SERVER "mon.medicalopenworld.org"
#define TB_PORT 1883U
#define TB_PROVISION_KEY "9l35qc4g5ejvwl9cs0sc"
#define TB_PROVISION_SECRET "uqm7j8f5jmpu3lztruku"
#define TB_HEARTBEAT_INTERVAL_MS 30000
#define TB_PROVISION_TIMEOUT_MS 15000
#define TB_MQTT_BUFFER_SIZE 512

// NVS keys
#define NVS_NAMESPACE "incutwin"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASS "wifi_pass"
#define NVS_KEY_TB_TOKEN "tb_token"
#define NVS_KEY_TB_DEVICE "tb_device_name"
#define NVS_KEY_PROVISIONED "provisioned"
#define NVS_KEY_LINKED_INCUNEST "linked_incunest"

// =============================================================================
// INCUNEST LINK
// =============================================================================
#define INCUNEST_LINK_TIMEOUT_MS 10000   // ms sin comunicación → enlace perdido
#define INCUNEST_SEARCH_TIMEOUT_MS 30000 // ms buscando → volver a IDLE
#define INCUNEST_POLL_INTERVAL_MS 2000   // Frecuencia de sondeo estado IncuNest

// =============================================================================
// LED RING — Animaciones
// =============================================================================
#define LED_FADE_PERIOD_MS 500      // ms — duración transición entre estados
#define LED_HEARTBEAT_PERIOD_MS 800 // ms — periodo del pulso STATE_LINKED
#define LED_CHASE_PERIOD_MS 500 // ms — periodo del perseguidor STATE_SEARCHING
#define LED_BREATH_PERIOD_MS 5000 // ms — respiración lenta STATE_IDLE
#define LED_BOOT_SWEEP_MS 300     // ms por LED en barrido de arranque

// =============================================================================
// FIREBASE REALTIME DATABASE (demo)
// =============================================================================
#define FIREBASE_RTDB_URL    "https://incutwinapp-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_INCUTWIN_ID "INCUTWIN-A1B2C3"

// =============================================================================
// WATCHDOG
// =============================================================================
#define WDT_TIMEOUT_S 30
