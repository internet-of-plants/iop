#include "utils.hpp"

#include <unity.h>

void base64() {
    const std::string data = "asdasdasdadsasdasdas";
    const std::string expectedHash = "YXNkYXNkYXNkYWRzYXNkYXNkYXM=";
    const std::string hash = utils::base64Encode(reinterpret_cast<const uint8_t*>(data.c_str()), data.length()).c_str();
    TEST_ASSERT_EQUAL(expectedHash.length(), hash.length());
    TEST_ASSERT_EQUAL_CHAR_ARRAY(hash.c_str(), expectedHash.c_str(), std::min(hash.length(), expectedHash.length()));
}
void interrupts() {
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::NONE);
    utils::scheduleInterrupt(InterruptEvent::FACTORY_RESET);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::FACTORY_RESET);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::NONE);

    utils::scheduleInterrupt(InterruptEvent::FACTORY_RESET);
    utils::scheduleInterrupt(InterruptEvent::FACTORY_RESET);
    utils::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
    utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::FACTORY_RESET);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::MUST_UPGRADE);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::ON_CONNECTION);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::NONE);
    TEST_ASSERT(utils::descheduleInterrupt() == InterruptEvent::NONE);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(base64);
    RUN_TEST(interrupts);
    UNITY_END();
    return 0;
}