// =============================================================================
// test_proximity.cpp — Tests unitarios sensores de proximidad
// UT-01: readLeft/readRight en rango [0, 4095]
// =============================================================================
#include <unity.h>
#include <Arduino.h>
#include "sensors/proximity.h"

void setUp(void) {
    Proximity::init();
}

void tearDown(void) {}

void test_readLeft_in_range(void) {
    int val = Proximity::readLeft();
    TEST_ASSERT_GREATER_OR_EQUAL(0,    val);
    TEST_ASSERT_LESS_OR_EQUAL(4095, val);
}

void test_readRight_in_range(void) {
    int val = Proximity::readRight();
    TEST_ASSERT_GREATER_OR_EQUAL(0,    val);
    TEST_ASSERT_LESS_OR_EQUAL(4095, val);
}

void test_led_left_toggle(void) {
    // No debe provocar excepción — verificación visual
    Proximity::setLeft(true);
    delay(5);
    Proximity::setLeft(false);
    TEST_PASS();
}

void test_led_right_toggle(void) {
    Proximity::setRight(true);
    delay(5);
    Proximity::setRight(false);
    TEST_PASS();
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_readLeft_in_range);
    RUN_TEST(test_readRight_in_range);
    RUN_TEST(test_led_left_toggle);
    RUN_TEST(test_led_right_toggle);
    return UNITY_END();
}
