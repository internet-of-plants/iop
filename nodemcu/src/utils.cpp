#include <utils.hpp>
#include <configuration.h>
#include <log.hpp>

String clone(const String& value) {
  return String((const String&)value);
}

// FNV hash
uint64_t hashString(const String string) {
  const uint8_t * bytes = (uint8_t*) string.c_str();
  const uint64_t p = 16777619;
  uint64_t hash = 2166136261;

  for (uint32_t i = 0; i < string.length(); ++i) {
    hash = (hash ^ bytes[i]) * p;
  }

  hash += hash << 13;
  hash ^= hash >> 7;
  hash += hash << 3;
  hash ^= hash >> 17;
  hash += hash << 5;
  return hash;
}