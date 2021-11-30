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
void logPrint(const std::string_view msg) noexcept {
    pthread_mutex_lock(&lock);
    std::cout << msg;
    pthread_mutex_unlock(&lock);
}
void logPrint(const iop::StaticString msg) noexcept {
    pthread_mutex_lock(&lock);
    std::cout << msg.asCharPtr();
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

//HardwareSerial Serial(UART0);

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
void logPrint(const iop::StaticString msg) noexcept {
    Serial.print(msg.get());
}
void logPrint(const std::string_view msg) noexcept {
    Serial.write(reinterpret_cast<const uint8_t*>(msg.begin()), msg.length());
}
void logFlush() noexcept {
    Serial.flush();
}
#endif
