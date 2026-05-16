// =============================================================================
// test_state_machine.cpp — Tests FSM (UT-05)
// =============================================================================
#include <unity.h>
#include <Arduino.h>
#include "state_machine/state_machine.h"

static StateMachine fsm;

void setUp(void) {
    fsm.begin();
    // Avanzar hasta IDLE normalmente
    fsm.onWifiConnected();
    fsm.onServerConnected();
    // update varias veces para que transite a IDLE
    for (int i = 0; i < 10; i++) { fsm.update(); delay(60); }
}

void tearDown(void) {}

void test_initial_state_reaches_idle(void) {
    TEST_ASSERT_EQUAL((int)SystemState::IDLE, (int)fsm.getState());
}

void test_prox_left_transition(void) {
    fsm.onProximityLeft(true);
    fsm.onProximityRight(false);
    fsm.update();
    TEST_ASSERT_EQUAL((int)SystemState::PROXIMITY_LEFT, (int)fsm.getState());
}

void test_prox_both_transition(void) {
    fsm.onProximityLeft(true);
    fsm.onProximityRight(true);
    fsm.update();
    TEST_ASSERT_EQUAL((int)SystemState::PROXIMITY_BOTH, (int)fsm.getState());
}

void test_prox_lost_returns_idle(void) {
    fsm.onProximityLeft(false);
    fsm.onProximityRight(false);
    for (int i = 0; i < 5; i++) { fsm.update(); delay(10); }
    TEST_ASSERT_EQUAL((int)SystemState::IDLE, (int)fsm.getState());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_state_reaches_idle);
    RUN_TEST(test_prox_left_transition);
    RUN_TEST(test_prox_both_transition);
    RUN_TEST(test_prox_lost_returns_idle);
    return UNITY_END();
}
