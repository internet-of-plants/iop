#include "utils.hpp"

#include <unity.h>

int main(int argc, char** argv) {
    UNITY_BEGIN();
    const std::string data = "asdasdasdadsasdasdas";
    const std::string expectedHash = "YXNkYXNkYXNkYWRzYXNkYXNkYXM=";
    const std::string hash = utils::base64Encode(reinterpret_cast<const uint8_t*>(data.c_str()), data.length()).c_str();
    TEST_ASSERT_EQUAL_CHAR_ARRAY(hash.c_str(), expectedHash.c_str(), std::min(hash.length(), expectedHash.length()));
    UNITY_END();
    return 0;
}