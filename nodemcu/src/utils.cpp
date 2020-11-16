#include <utils.hpp>

namespace utils {
  // FNV hash
  uint64_t hashString(const StringView string) {
    const auto bytes = reinterpret_cast<const uint8_t*>(string.get());
    const uint64_t p = 16777619;
    uint64_t hash = 2166136261;

    const auto length = string.length();
    for (uint32_t i = 0; i < length; ++i) {
      hash = (hash ^ bytes[i]) * p;
    }

    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;
    return hash;
  }
}
