#include <utils.hpp>

#include <esp8266_peri.h>
#include <string>
#include <string.h>
#include <WString.h>
#include <bits/basic_string.h>
#include <models.hpp>

namespace utils {
  // FNV hash
  uint64_t hashString(const StringView string) {
    const auto bytes = reinterpret_cast<const uint8_t*>(string.get());
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

  Option<size_t> search(const StringView haystack, const StaticString& needle) {
    return search(haystack, needle, 0);
  }

  Option<size_t> search(const StringView haystack, const StaticString& needle, const size_t fromIndex) {
    if (fromIndex >= haystack.length()) {
      return Option<size_t>();
    }

    const char *found = strstr_P(haystack.get() + fromIndex, needle.asCharPtr());
    if (found == NULL) {
      return Option<size_t>();
    }
    return Option<size_t>(found - haystack.get());
  }


  Option<size_t> search(const StringView haystack, const StringView needle) {
    return search(haystack, needle, 0);
  }

  Option<size_t> search(const StringView haystack, const StringView needle, const size_t fromIndex) {
    if (fromIndex >= haystack.length()) {
      return Option<size_t>();
    }

    const char *found = strstr(haystack.get() + fromIndex, needle.get());
    if (found == NULL) {
      return Option<size_t>();
    }
    return Option<size_t>(found - haystack.get());
  }

  uint64_t random() {
    const uint64_t first = ((uint64_t)RANDOM_REG32) << 32 | RANDOM_REG32;
    const uint64_t second = ((uint64_t)RANDOM_REG32) << 32 | RANDOM_REG32;
    const uint64_t third = ((uint64_t)RANDOM_REG32) << 32 | RANDOM_REG32;
    return hashString(std::to_string(first ^ second ^ third));
  }

    Option<CsrfToken> parseCsrfTokenCookie(std::string header) {
    auto maybeTokenStart = utils::search(header, F("csrf="));
    if (maybeTokenStart.isSome()) {
      const auto tokenStart = maybeTokenStart.expect(F("maybeTokenStart is none"));
      auto maybeTokenEnd = utils::search(header, F(";"), tokenStart);
      if (maybeTokenEnd.isSome()) {
        const auto tokenEnd = maybeTokenEnd.expect(F("maybeTokenEnd is none"));
        header[tokenEnd] = 0;
      }
      auto tokenRes = CsrfToken::fromStringTruncating(UnsafeRawString(header.c_str() + tokenStart));
      return tokenRes;
    }
    return Option<CsrfToken>();
  }

  Option<CsrfToken> parseCsrfTokenCookie(String header) {
    auto maybeTokenStart = utils::search(header, F("csrf="));
    if (maybeTokenStart.isSome()) {
      const auto tokenStart = maybeTokenStart.expect(F("maybeTokenStart is none"));
      auto maybeTokenEnd = utils::search(header, F(";"), tokenStart);
      if (maybeTokenEnd.isSome()) {
        const auto tokenEnd = maybeTokenEnd.expect(F("maybeTokenEnd is none"));
        header.setCharAt(tokenEnd, 0);
      }
      auto tokenRes = CsrfToken::fromStringTruncating(UnsafeRawString(header.c_str() + tokenStart));
      return tokenRes;
    }
    return Option<CsrfToken>();
  }

  CsrfToken randomCsrfToken(uint64_t secretKey) {
    const auto token = std::to_string(random() ^ secretKey);
    return CsrfToken::fromStringTruncating(token);
  }

  uint64_t u64fromString(const StringView str) {
    uint64_t result = 0;
    char const* p = str.get();
    char const* q = p + str.length();
    while (p < q) {
      result *= 10;
      result += *(p++) - '0';
    }
    return result;
  }

  bool compareCsrfTokens(const String header, const String form) {
    return false;
  }
}