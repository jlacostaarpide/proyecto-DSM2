// =============================================================================
// state_machine.cpp — Implementación FSM IncuTwin
// =============================================================================
#include "state_machine.h"
#include "../serial_debug/serial_debug.h"
#include "config.h"

static const char* TAG = "FSM";

static const char* stateNames[] = {
    "BOOT", "WIFI_CONNECTING", "WIFI_CONNECTED", "PROVISIONING",
    "SERVER_CONNECTED", "IDLE", "PROXIMITY_LEFT", "PROXIMITY_RIGHT",
    "PROXIMITY_BOTH", "SEARCHING", "LINKED", "ERROR"
};

StateMachine::StateMachine()
    : _state(SystemState::BOOT)
    , _prevState(SystemState::BOOT)
    , _proxLeft(false)
    , _proxRight(false)
    , _stateEnteredAt(0)
{}

void StateMachine::begin() {
    _stateEnteredAt = millis();
    LOG_INFO(TAG, "FSM iniciada en estado: %s", stateNames[(int)_state]);
}

void StateMachine::_transition(SystemState next) {
    LOG_INFO(TAG, "Transicion: %s -> %s",
             stateNames[(int)_state], stateNames[(int)next]);
    _prevState      = _state;
    _state          = next;
    _stateEnteredAt = millis();
}

void StateMachine::setState(SystemState s) {
    if (s != _state) _transition(s);
}

const char* StateMachine::getStateName() const {
    return stateNames[(int)_state];
}

void StateMachine::update() {
    _executeState();
}

void StateMachine::_executeState() {
    uint32_t elapsed = millis() - _stateEnteredAt;

    switch (_state) {
        case SystemState::BOOT:
            // Transición automática a WIFI_CONNECTING tras boot
            if (elapsed > 100) {
                _transition(SystemState::WIFI_CONNECTING);
            }
            break;

        case SystemState::WIFI_CONNECTING:
            // Espera evento onWifiConnected()
            break;

        case SystemState::WIFI_CONNECTED:
            // Espera evento onProvisioned() o onServerConnected()
            break;

        case SystemState::PROVISIONING:
            // Espera evento onServerConnected()
            break;

        case SystemState::SERVER_CONNECTED:
            if (elapsed > 500) {
                _transition(SystemState::IDLE);
            }
            break;

        case SystemState::IDLE:
            if (_proxLeft && _proxRight) {
                _transition(SystemState::PROXIMITY_BOTH);
            } else if (_proxLeft) {
                _transition(SystemState::PROXIMITY_LEFT);
            } else if (_proxRight) {
                _transition(SystemState::PROXIMITY_RIGHT);
            }
            break;

        case SystemState::PROXIMITY_LEFT:
            if (!_proxLeft) {
                _transition(SystemState::IDLE);
            } else if (_proxRight) {
                _transition(SystemState::PROXIMITY_BOTH);
            }
            break;

        case SystemState::PROXIMITY_RIGHT:
            if (!_proxRight) {
                _transition(SystemState::IDLE);
            } else if (_proxLeft) {
                _transition(SystemState::PROXIMITY_BOTH);
            }
            break;

        case SystemState::PROXIMITY_BOTH:
            if (!_proxLeft && !_proxRight) {
                _transition(SystemState::IDLE);
            } else if (elapsed > 300) {
                // Ambos detectados un tiempo → buscar IncuNest
                _transition(SystemState::SEARCHING);
            }
            break;

        case SystemState::SEARCHING:
            // Timeout búsqueda
            if (!_proxLeft || !_proxRight) {
                _transition(SystemState::IDLE);
            } else if (elapsed > INCUNEST_SEARCH_TIMEOUT_MS) {
                LOG_WARN(TAG, "Timeout buscando IncuNest — volviendo a IDLE");
                _transition(SystemState::IDLE);
            }
            break;

        case SystemState::LINKED:
            if (!_proxLeft || !_proxRight) {
                // onIncuNestUnlinked será llamado externamente
                _transition(SystemState::IDLE);
            }
            break;

        case SystemState::ERROR_STATE:
            // Espera reset manual o reinicio
            break;

        default:
            break;
    }
}

// Eventos externos
void StateMachine::onWifiConnected() {
    if (_state == SystemState::WIFI_CONNECTING) {
        _transition(SystemState::WIFI_CONNECTED);
    }
}

void StateMachine::onWifiDisconnected() {
    if (_state != SystemState::ERROR_STATE &&
        _state != SystemState::WIFI_CONNECTING) {
        _transition(SystemState::WIFI_CONNECTING);
    }
}

void StateMachine::onProvisioned() {
    if (_state == SystemState::WIFI_CONNECTED) {
        _transition(SystemState::PROVISIONING);
    }
}

void StateMachine::onServerConnected() {
    _transition(SystemState::SERVER_CONNECTED);
}

void StateMachine::onProximityLeft(bool detected) {
    _proxLeft = detected;
}

void StateMachine::onProximityRight(bool detected) {
    _proxRight = detected;
}

void StateMachine::onIncuNestLinked(const String& id) {
    if (_state == SystemState::SEARCHING) {
        LOG_INFO(TAG, "IncuNest enlazado: %s", id.c_str());
        _transition(SystemState::LINKED);
    }
}

void StateMachine::onIncuNestUnlinked(const char* reason) {
    if (_state == SystemState::LINKED) {
        LOG_INFO(TAG, "IncuNest desenlazado: %s", reason);
        _transition(SystemState::IDLE);
    }
}

void StateMachine::onError(const char* msg) {
    LOG_ERROR(TAG, "ERROR: %s", msg);
    _transition(SystemState::ERROR_STATE);
}
