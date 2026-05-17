// =============================================================================
// main.cpp — IncuTwin Firmware
// Medicina Abierta al Mundo (MOW)
// Versión: FIRMWARE_VERSION
//
// Fases de compilación controladas por la macro PHASE:
//   PHASE=1 → Bring-up de hardware (sensores, LEDs, buzzer)
//   PHASE=2 → Conectividad WiFi + ThingsBoard
//   PHASE=3 → Lógica principal (FSM, detección, IncuNest)
// =============================================================================
#include <Arduino.h>
#include "config.h"
#include "serial_debug/serial_debug.h"
#include "sensors/proximity.h"
#include "sensors/als.h"
#include "buzzer/buzzer.h"
#include "leds/led_ring.h"
#include "storage/flash_store.h"

#if PHASE >= 2
#include "comms/wifi_manager.h"
#include "comms/provisioning.h"
#include "comms/thingsboard_client.h"
#endif

#if PHASE >= 3
// #include "state_machine/state_machine.h"  // comentado — FSM no usado en demo
// #include "incunest_link/incunest_link.h"   // comentado — IncuNest no usado en demo
#endif

// =============================================================================
// ESTADO GLOBAL
// =============================================================================
#if PHASE >= 3
enum DemoPhase : uint8_t { DEMO_WAIT, DEMO_SWEEP, DEMO_LINKED };
static DemoPhase _demoPhase   = DEMO_WAIT;
static uint8_t   _sweepLed    = 0;
static uint32_t  _sweepTimer  = 0;
static uint32_t  _lastDetTime = 0;
#endif

// Detección de presencia con histéresis y debounce
struct ProxState {
    bool     detected   = false;
    uint32_t lastChange = 0;
};
static ProxState proxLeft, proxRight;
static bool      prevDetL = false;
static bool      prevDetR = false;

static uint32_t _proxPollTimer = 0;
static uint32_t _alsPollTimer  = 0;
static int      _alsValue      = 0;

// =============================================================================
// AUTOTEST FASE 1
// =============================================================================
static void runAutotest() {
    Serial.println("\n=== AUTOTEST INCUTWIN ===");

    bool ledOk = LedRing::selfTest();
    if (ledOk) LOG_PASS("WS2812B ring");
    else       LOG_FAIL("WS2812B ring", "sin respuesta");

    bool buzzOk = Buzzer::selfTest();
    if (buzzOk) LOG_PASS("Buzzer");
    else        LOG_FAIL("Buzzer", "sin respuesta");

    bool proxLOk = Proximity::selfTestLeft();
    if (proxLOk) LOG_PASS("Fototransistor Izquierdo");
    else         LOG_FAIL("Fototransistor Izquierdo", "valor fuera de rango");

    bool proxROk = Proximity::selfTestRight();
    if (proxROk) LOG_PASS("Fototransistor Derecho");
    else         LOG_FAIL("Fototransistor Derecho", "valor fuera de rango");

    int alsVal = ALS::readAmbientLight();
    if (alsVal >= 0 && alsVal <= 4095) LOG_PASS("ALS");
    else                               LOG_FAIL("ALS", "valor fuera de rango");

    FlashStore::setString("_test_key", "ok");
    String v = FlashStore::getString("_test_key");
    if (v == "ok") LOG_PASS("NVS Preferences");
    else           LOG_FAIL("NVS Preferences", "lectura/escritura fallida");
    FlashStore::setString("_test_key", "");

    Serial.println("=========================\n");
}

// =============================================================================
// DETECCIÓN DE PROXIMIDAD (histéresis + debounce)
// =============================================================================
static bool updateProxDetection(ProxState& state, int adcVal) {
    bool newDetected = state.detected;

    // Circuito pull-down: ADC SUBE cuando hay objeto (fototransistor conduce más)
    if (!state.detected && adcVal > PROX_THRESHOLD_DETECT) {
        newDetected = true;
    } else if (state.detected && adcVal < PROX_THRESHOLD_RELEASE) {
        newDetected = false;
    }

    if (newDetected != state.detected) {
        if (millis() - state.lastChange >= (uint32_t)PROX_DEBOUNCE_MS) {
            state.detected   = newDetected;
            state.lastChange = millis();
        }
    } else {
        state.lastChange = millis();
    }
    return state.detected;
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(300);

    Serial.printf("\n[BOOT] IncuTwin v%s — Iniciando...\n", FIRMWARE_VERSION);
    LOG_INFO("BOOT", "Fase de compilacion: %d", PHASE);

    // Inicializar periféricos base (Fase 1)
    FlashStore::init();
    Proximity::init();
    ALS::init();
    Buzzer::init();
    LedRing::init();

    // Arranque visual y sonoro
    LedRing::runBootSweep();
    Buzzer::beep(BUZZ_FREQ_BOOT, BUZZ_DUR_BOOT);

    // Autotest de todos los periféricos
    runAutotest();

    LOG_INFO("BOOT", "Fase 1 OK — Hardware bring-up completado");

#if PHASE >= 2
    // -----------------------------------------------------------------------
    // FASE 2: Conectividad WiFi + ThingsBoard
    // -----------------------------------------------------------------------
    LOG_INFO("BOOT", "Iniciando Fase 2 — Conectividad");
    LedRing::update(SystemState::WIFI_CONNECTING);

    WifiMgr::init();
    WifiMgr::connect();

    if (WifiMgr::isConnected()) {
        LedRing::update(SystemState::WIFI_CONNECTED);
        LedRing::show();

        String deviceName  = Provisioning::buildDeviceName();
        String token       = FlashStore::getString(NVS_KEY_TB_TOKEN);
        bool   provisioned = FlashStore::getBool(NVS_KEY_PROVISIONED, false);

        if (!provisioned || token.length() == 0) {
            LOG_INFO("BOOT", "Provisionando: %s", deviceName.c_str());
            LedRing::update(SystemState::PROVISIONING);
            LedRing::show();
            token = Provisioning::provision(deviceName);
            if (token.length() > 0) {
                FlashStore::setString(NVS_KEY_TB_TOKEN,  token);
                FlashStore::setString(NVS_KEY_TB_DEVICE, deviceName);
                FlashStore::setBool(NVS_KEY_PROVISIONED, true);
                LOG_INFO("BOOT", "Provisionamiento OK");
            } else {
                LOG_ERROR("BOOT", "Provisionamiento fallido");
            }
        } else {
            LOG_INFO("BOOT", "Token NVS existente — omitiendo provisionamiento");
        }

        if (token.length() > 0) {
            TBClient::init(token, deviceName);
            if (TBClient::connect()) {
                LedRing::update(SystemState::SERVER_CONNECTED);
                LedRing::show();
                LOG_INFO("BOOT", "Fase 2 OK — ThingsBoard conectado");
            }
        }
    } else {
        LOG_WARN("BOOT", "Sin WiFi tras setup — continuando offline");
    }
#endif

#if PHASE >= 3
    // -----------------------------------------------------------------------
    // FASE 3: Demo de detección (IncuNest y FSM comentados)
    // -----------------------------------------------------------------------
    LOG_INFO("BOOT", "Iniciando Fase 3 — Demo modo");
    // IncuNestLink::init();  // comentado
    // fsm.begin();           // comentado
    LedRing::clear();
    LOG_INFO("BOOT", "Fase 3 OK — LEDs apagados, esperando deteccion");
#endif

    LOG_INFO("BOOT", "Setup completo. IncuTwin operativo.\n");
}

// =============================================================================
// LOOP PRINCIPAL
// =============================================================================
void loop() {
    uint32_t now = millis();

    Buzzer::update();

#if PHASE >= 2
    WifiMgr::update();
    if (WifiMgr::isConnected()) {
        TBClient::update();
    }
#endif

    // --- Lectura de sensores de proximidad a 20 Hz ---
    if (now - _proxPollTimer >= 50) {
        _proxPollTimer = now;

        bool alsSuppressed = (_alsValue > ALS_SATURATION_THRESHOLD);
        int  adcL = 0, adcR = 0;

        if (!alsSuppressed) {
            adcL = Proximity::readLeft();
            adcR = Proximity::readRight();
        }

        bool detL = alsSuppressed ? false : updateProxDetection(proxLeft,  adcL);
        bool detR = alsSuppressed ? false : updateProxDetection(proxRight, adcR);

#if PHASE >= 3
        // Demo: deteccion ADC > umbral → barrido blanco LED a LED (+beep) → azul LINKED
        bool anyDet = (adcL > PROX_THRESHOLD_DETECT) || (adcR > PROX_THRESHOLD_DETECT);
        if (anyDet) _lastDetTime = now;

        // Timeout 10s sin deteccion → apagar LEDs y resetear
        if (_demoPhase != DEMO_WAIT && (now - _lastDetTime >= 10000)) {
            _demoPhase = DEMO_WAIT;
            LedRing::clear();
            LOG_INFO("MAIN", "Demo: timeout sin deteccion — LEDs apagados");
        }

        if (_demoPhase == DEMO_WAIT && anyDet) {
            _demoPhase  = DEMO_SWEEP;
            _sweepLed   = 0;
            _sweepTimer = now;
            LedRing::setAll(0, 0, 0);
            LedRing::setPixel(0, 255, 255, 255);
            LedRing::show();
            Buzzer::beep(BUZZ_FREQ_PROX, BUZZ_DUR_PROX);
            LOG_INFO("MAIN", "Demo: deteccion L=%d R=%d — barrido iniciado", adcL, adcR);
        } else if (_demoPhase == DEMO_SWEEP) {
            if (now - _sweepTimer >= LED_BOOT_SWEEP_MS) {
                _sweepTimer = now;
                LedRing::setPixel(_sweepLed, 0, 0, 0);
                _sweepLed++;
                if (_sweepLed >= LED_RING_COUNT) {
                    _demoPhase = DEMO_LINKED;
                    LedRing::setAll(0, 80, 255);
                    LedRing::show();
                    LOG_INFO("MAIN", "Demo: conexion simulada — LINKED azul");
                } else {
                    LedRing::setPixel(_sweepLed, 255, 255, 255);
                    LedRing::show();
                    Buzzer::beep(BUZZ_FREQ_PROX, BUZZ_DUR_PROX);
                }
            }
        }
        // DEMO_LINKED: LEDs azules fijos, nada que hacer

#elif PHASE == 2
        // Fase 2: LED ring refleja estado de conectividad
        if (TBClient::isConnected()) {
            LedRing::update(SystemState::SERVER_CONNECTED);
        } else if (WifiMgr::isConnected()) {
            LedRing::update(SystemState::WIFI_CONNECTED);
        } else {
            LedRing::update(SystemState::WIFI_CONNECTING);
        }
#else
        // Fase 1: bucle demo fade R→B→G→W
        LedRing::runPhase1FadeLoop();
#endif
    }

    // --- Lectura ALS cada 500 ms ---
    if (now - _alsPollTimer >= 500) {
        _alsPollTimer = now;
        _alsValue     = ALS::readAmbientLight();
    }
}
