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
#include "state_machine/state_machine.h"
#include "incunest_link/incunest_link.h"
#endif

// =============================================================================
// ESTADO GLOBAL
// =============================================================================
#if PHASE >= 3
static StateMachine fsm;
static SystemState  _prevFsmState  = SystemState::BOOT;
static bool         _handDetected  = false;
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
    // FASE 3: Máquina de estados y lógica principal
    // -----------------------------------------------------------------------
    LOG_INFO("BOOT", "Iniciando Fase 3 — Logica principal");
    IncuNestLink::init();
    fsm.begin();

    if (WifiMgr::isConnected()) {
        fsm.onWifiConnected();
        if (TBClient::isConnected()) {
            fsm.onServerConnected();
        }
    }
    LOG_INFO("BOOT", "Fase 3 OK — FSM activa");
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
        fsm.onProximityLeft(detL);
        fsm.onProximityRight(detR);

        // Feedback sonoro en eventos de detección
        if (detL && !prevDetL) {
            Buzzer::beep(BUZZ_FREQ_PROX, BUZZ_DUR_PROX);
        }
        if (detR && !prevDetR) {
            Buzzer::beep(BUZZ_FREQ_PROX, BUZZ_DUR_PROX);
        }

        // Desenlace al perder presencia
        if ((!detL || !detR) && (prevDetL && prevDetR)) {
            if (IncuNestLink::isLinked()) {
                IncuNestLink::unlink("no_proximity");
                fsm.onIncuNestUnlinked("no_proximity");
                Buzzer::beepPattern(BUZZ_FREQ_DISCONNECT, BUZZ_DUR_DISCONNECT, 2, 120);
            }
        }

        // --- Modo "Coge mi mano" — detectar cambios y publicar shared attrs (FR-304/307/309) ---
        bool handDet = detL && detR;
        bool proxChanged = (detL != prevDetL) || (detR != prevDetR);
        bool handChanged = (handDet != _handDetected);

        if ((proxChanged || handChanged) && TBClient::isConnected()) {
            _handDetected = handDet;
            StaticJsonDocument<256> attrs;
            attrs["proximity_left"]     = detL;
            attrs["proximity_right"]    = detR;
            attrs["hand_detected"]      = _handDetected;
            attrs["linked_incunest_id"] = IncuNestLink::getLinkedId();
            attrs["device_state"]       = fsm.getStateName();
            JsonObject attrObj = attrs.as<JsonObject>();
            TBClient::sendSharedAttributes(attrObj);
            LOG_INFO("MAIN", "Shared attrs: hand=%d proxL=%d proxR=%d",
                     _handDetected, detL, detR);
        } else if (handChanged) {
            _handDetected = handDet;
        }

        prevDetL = detL;
        prevDetR = detR;

        // Iniciar búsqueda de IncuNest en STATE_SEARCHING
        if (fsm.getState() == SystemState::SEARCHING && !IncuNestLink::isSearching()) {
            IncuNestLink::startSearch();
        }
        IncuNestLink::update();

        // Avanzar FSM
        fsm.update();

        // --- Publicar device_state cuando cambia el estado FSM (FR-309) ---
        {
            SystemState currState = fsm.getState();
            if (currState != _prevFsmState) {
                _prevFsmState = currState;
                if (TBClient::isConnected()) {
                    StaticJsonDocument<192> stateAttrs;
                    stateAttrs["device_state"] = fsm.getStateName();
                    if (currState == SystemState::IDLE ||
                        currState == SystemState::SERVER_CONNECTED) {
                        stateAttrs["hand_detected"]      = false;
                        stateAttrs["linked_incunest_id"] = "";
                    }
                    JsonObject sObj = stateAttrs.as<JsonObject>();
                    TBClient::sendSharedAttributes(sObj);
                    LOG_INFO("MAIN", "device_state publicado: %s", fsm.getStateName());
                }
            }
        }

        // Actualizar estado extra del heartbeat
        TBClient::setHeartbeatState(detL, detR, _handDetected, _alsValue,
                                    fsm.getStateName(),
                                    IncuNestLink::getLinkedId().c_str());

        // Actualizar LED ring según estado FSM
        LedRing::update(fsm.getState());

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
