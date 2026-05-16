#pragma once
// =============================================================================
// serial_debug.h — Macros de log con timestamp y módulo
// =============================================================================
#include <Arduino.h>

#define LOG_INFO(module, fmt, ...)  \
    Serial.printf("[%07lu][%s] " fmt "\n", millis(), module, ##__VA_ARGS__)

#define LOG_WARN(module, fmt, ...)  \
    Serial.printf("[%07lu][WARN][%s] " fmt "\n", millis(), module, ##__VA_ARGS__)

#define LOG_ERROR(module, fmt, ...)  \
    Serial.printf("[%07lu][ERR][%s] " fmt "\n", millis(), module, ##__VA_ARGS__)

#define LOG_PASS(test)  \
    Serial.printf("[%07lu][TEST] PASS: %s\n", millis(), test)

#define LOG_FAIL(test, reason)  \
    Serial.printf("[%07lu][TEST] FAIL: %s — %s\n", millis(), test, reason)
