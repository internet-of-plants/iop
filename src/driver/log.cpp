#include "driver/log.hpp"
#include "core/log.hpp"

#ifdef IOP_DESKTOP
#include <iostream>
#include "core/panic.hpp"

#include <pthread.h>
static pthread_mutex_t lock;

void logSetup(const iop::LogLevel &level) noexcept {
    iop_assert(pthread_mutex_init(&lock, NULL) == 0, F("Mutex init failed"));
}
void logPrint(const char *msg) noexcept {
    pthread_mutex_lock(&lock);
    std::cout << msg;
    pthread_mutex_unlock(&lock);
}
void logPrint(const __FlashStringHelper *msg) noexcept {
    pthread_mutex_lock(&lock);
    std::cout << reinterpret_cast<const char*>(msg);
    pthread_mutex_unlock(&lock);
}
void logFlush() noexcept { 
    pthread_mutex_lock(&lock);
    std::cout << std::flush;
    pthread_mutex_unlock(&lock);
}
#else
#include "Arduino.h"
#include "core/log.hpp"

void logSetup(const iop::LogLevel &level) noexcept {
    constexpr const uint32_t BAUD_RATE = 115200;
    Serial.begin(BAUD_RATE);
    if (level <= iop::LogLevel::DEBUG)
        Serial.setDebugOutput(true);

    constexpr const uint32_t twoSec = 2 * 1000;
    const auto end = millis() + twoSec;
    while (!Serial && millis() < end)
        yield();
}
void logPrint(const __FlashStringHelper *msg) noexcept {
    Serial.print(msg);
}
void logPrint(const char *msg) noexcept {
    Serial.print(msg);
}
void logFlush() noexcept {
    Serial.flush();
}
#endif