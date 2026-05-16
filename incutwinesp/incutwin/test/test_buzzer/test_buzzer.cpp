// =============================================================================
// test_buzzer.cpp — Tests unitarios buzzer (UT-04)
// =============================================================================
#include <unity.h>
#include <Arduino.h>
#include "buzzer/buzzer.h"

void setUp(void) {
    Buzzer::init();
}

void tearDown(void) {}

void test_beep_no_crash(void) {
    Buzzer::beep(1000, 50);
    TEST_PASS();
}

void test_beep_high_freq(void) {
    Buzzer::beep(4000, 50);
    TEST_PASS();
}

void test_beep_low_freq(void) {
    Buzzer::beep(500, 50);
    TEST_PASS();
}

void test_rate_limit(void) {
    // 4 beeps rápidos — el 4º debe ser suprimido (limite = 3 en 5s)
    Buzzer::beep(1000, 20);
    Buzzer::beep(1000, 20);
    Buzzer::beep(1000, 20);
    Buzzer::beep(1000, 20);  // suprimido
    TEST_PASS();  // Solo verificamos que no crashea
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_beep_no_crash);
    RUN_TEST(test_beep_high_freq);
    RUN_TEST(test_beep_low_freq);
    RUN_TEST(test_rate_limit);
    return UNITY_END();
}
