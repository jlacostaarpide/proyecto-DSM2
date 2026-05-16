#pragma once
// =============================================================================
// state_machine.h — Máquina de estados principal IncuTwin
// FUN-301
// =============================================================================
#include <Arduino.h>
#include "../leds/led_ring.h"  // SystemState definido aquí

class StateMachine {
public:
    StateMachine();

    void begin();
    void update();

    SystemState getState() const { return _state; }
    const char* getStateName() const;
    void        setState(SystemState s);

    // Eventos externos
    void onWifiConnected();
    void onWifiDisconnected();
    void onProvisioned();
    void onServerConnected();
    void onProximityLeft(bool detected);
    void onProximityRight(bool detected);
    void onIncuNestLinked(const String& id);
    void onIncuNestUnlinked(const char* reason);
    void onError(const char* msg);

private:
    SystemState _state;
    SystemState _prevState;
    bool        _proxLeft;
    bool        _proxRight;
    uint32_t    _stateEnteredAt;

    void _transition(SystemState next);
    void _executeState();
};
