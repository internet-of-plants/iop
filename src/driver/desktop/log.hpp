#include "core/panic.hpp"
#include <iostream>
#include <pthread.h>

static pthread_mutex_t lock;

void logSetup(const iop::LogLevel &level) noexcept {
    iop_assert(pthread_mutex_init(&lock, NULL) == 0, FLASH("Mutex init failed"));
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