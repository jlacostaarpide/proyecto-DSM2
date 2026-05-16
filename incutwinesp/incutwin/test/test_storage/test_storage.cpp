// =============================================================================
// test_storage.cpp — Tests unitarios NVS (UT-03)
// =============================================================================
#include <unity.h>
#include <Arduino.h>
#include "storage/flash_store.h"

void setUp(void) {
    FlashStore::init();
}

void tearDown(void) {}

void test_string_write_read(void) {
    FlashStore::setString("test_str", "hello");
    String val = FlashStore::getString("test_str");
    TEST_ASSERT_EQUAL_STRING("hello", val.c_str());
}

void test_bool_write_read(void) {
    FlashStore::setBool("test_bool", true);
    TEST_ASSERT_TRUE(FlashStore::getBool("test_bool"));
    FlashStore::setBool("test_bool", false);
    TEST_ASSERT_FALSE(FlashStore::getBool("test_bool"));
}

void test_default_value(void) {
    String val = FlashStore::getString("nonexistent_key_xyz", "default");
    TEST_ASSERT_EQUAL_STRING("default", val.c_str());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_string_write_read);
    RUN_TEST(test_bool_write_read);
    RUN_TEST(test_default_value);
    return UNITY_END();
}
