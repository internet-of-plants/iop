#ifndef IOP_DRIVER_STRING_STATIC_HPP
#define IOP_DRIVER_STRING_STATIC_HPP

#ifdef IOP_ESP32
#include <experimental/optional>
template<typename T>
using optional = experimental::optional<T>;
}
#else
#include <optional>
#endif

#include <array>
#include <string>
#include <cstring>

class __FlashStringHelper;

#if defined(IOP_ESP8266) || (defined(IOP_NOOP) && defined(ESP8266))
#define INTERNAL_IOP__STRINGIZE_NX(A) #A
#define INTERNAL_IOP__STRINGIZE(A) INTERNAL_IOP__STRINGIZE_NX(A)\

// Arduino's ICACHE_RAM_ATTR
#define IOP_RAM __attribute__((section("\".iram.text." __FILE__ "." INTERNAL_IOP__STRINGIZE(__LINE__) "." INTERNAL_IOP__STRINGIZE(__COUNTER__) "\"")))
// Arduino's PROGMEM
#define IOP_ROM __attribute__((section( "\".irom.text." __FILE__ "." INTERNAL_IOP__STRINGIZE(__LINE__) "."  INTERNAL_IOP__STRINGIZE(__COUNTER__) "\"")))

#define IOP_INTERNAL_FLASH_RAW_N(s,n) (__extension__({static const char __pstr__[] __attribute__((__aligned__(n))) __attribute__((section( "\".irom0.pstr." __FILE__ "." INTERNAL_IOP__STRINGIZE(__LINE__) "."  INTERNAL_IOP__STRINGIZE(__COUNTER__) "\", \"aSM\", @progbits, 1 #"))) = (s); &__pstr__[0];}))
#define IOP_FLASH_RAW(s) IOP_INTERNAL_FLASH_RAW_N(s, 4)

#elif (defined(IOP_NOOP) && !defined(ESP8266)) || defined(IOP_ESP32) || defined(IOP_DESKTOP)
#define IOP_RAM
#define IOP_ROM
#define IOP_FLASH_RAW(x) x
#else
#error "Target not supported"
#endif

#define IOP_STATIC_STRING(string_literal) iop::StaticString(reinterpret_cast<const __FlashStringHelper *>(IOP_FLASH_RAW(string_literal)))

namespace iop {
// C++11 iop::StringView
class StringView {
  const char* start;
  size_t size;

public:
  StringView() noexcept: start(""), size(0) {}
  StringView(const std::string str): start(str.c_str()), size(str.length()) {}
  StringView(const char* start): start(start), size(strlen(start)) {}
  StringView(const char* start, size_t size): start(start), size(size) {}
  auto begin() const noexcept -> const char* { return this->start; }
  auto length() const noexcept -> size_t { return this->size; }
  auto substr(size_t size) const noexcept -> StringView { return this->substr(0, size); }
  auto substr(size_t start, size_t size) const noexcept -> StringView { return StringView(this->start + start, size)}
};

/// Allows for possibly destructive operations, like scaping non printable characters
///
/// If the operation needs to modify the data it converts the `iop::StringView` into the `std::string` variant
/// Otherwise it can keep storing the reference
///
/// Analogous to std::variant<iop::StringView, std::string> but C++11 compliant
class CowString {
  bool is_owned;
  union {
    iop::StringView borrowed;
    std::string owned;
  };

public:
  explicit CowString(iop::StringView str) noexcept: is_owned(false), borrowed(str) {}
  explicit CowString(std::string str) noexcept: is_owned(true), owned(str) {}

  auto isOwned() const noexcept -> bool { return this->is_owned; }
  auto isBorrowed() const noexcept -> bool { return !this->is_owned; }

  auto borrow() const noexcept -> iop::StringView {
    if (this->isOwned()) return iop::StringView(this->owned);
    return this->borrowed;
  }
  auto own() const noexcept -> std::string {
    if (this->isOwned()) return this->owned;
    return std::string(this->borrowed);
  }

  CowString(CowString &&other) noexcept = default;
  CowString(CowString &other) noexcept = default;
  auto operator=(CowString &&other) noexcept -> CowString & = default;
  auto operator=(CowString &other) noexcept -> CowString & = default;
  ~CowString() noexcept {
    if (this->isOwned()) {
      using string = std::string;
      this->owned.~string();
    }
  }
};

/// Applies FNV hasing to convert a string to a `uint64_t`
auto hashString(const iop::StringView txt) noexcept -> uint64_t;

/// Checks if a character is ASCII
auto isPrintable(const char ch) noexcept -> bool;

/// Checks if all characters are ASCII
auto isAllPrintable(const iop::StringView txt) noexcept -> bool;

/// If some character isn't ASCII it returns a `std::string` based `CowString` that scapes them
/// If all characters are ASCII it returns a `iop::StringView` based `CowString`
///
/// They are scaped by converting the `char` to `uint8_t` and then to string, and surrounding it by `<\` and `>`
///
/// For example the code: `iop::scapeNonPrintable("ABC \1")` returns `std::string("ABC <\1>")`
auto scapeNonPrintable(const iop::StringView txt) noexcept -> CowString;

using MD5Hash = std::array<char, 32>;
using MacAddress = std::array<char, 17>;

/// Helper string that holds a pointer to a string stored in PROGMEM
///
/// It's here to provide a typesafe way to handle IOP_ROM data and to avoid
/// defaulting to the String(__FlashStringHelper*) constructor that allocates implicitly.
///
/// It is not compatible with other strings because it requires 32 bits aligned reads, and
/// things like `strlen` will cause a crash, use `strlen_P`. So we separate them.
///
/// Either call a `_P` functions using `.getCharPtr()` to get the regular
/// pointer. Or construct a String with it using `.get()` so it knows to read
/// from PROGMEM.
class StaticString {
private:
  const __FlashStringHelper *str;

public:
  StaticString() noexcept: str(nullptr) { this->str = nullptr; }
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StaticString(const __FlashStringHelper *str) noexcept: str(str) {}
  
  /// Creates a `std::string` from the compile time string
  auto toString() const noexcept -> std::string;

  // Be careful when calling this function, if you pass a progmem stored `const char *`
  // to a function that expects a RAM stored `const char *`, a hardware exception _may_ happen
  // As IOP_ROM data needs to be read with 32 bits of alignment
  //
  // This has caused trouble in the past and will again.
  auto asCharPtr() const noexcept -> const char * { return reinterpret_cast<const char *>(this->get()); }

  auto get() const noexcept -> const __FlashStringHelper * { return this->str; }

  auto length() const noexcept -> size_t;
};

auto to_view(const std::string& str) -> iop::StringView;
auto to_view(const CowString& str) -> iop::StringView;
template <size_t SIZE>
auto to_view(const std::array<char, SIZE>& str, const size_t size) -> iop::StringView {
  return iop::StringView(str.data(), size);
}
template <size_t SIZE>
auto to_view(const std::array<char, SIZE>& str) -> iop::StringView {
  return to_view(str, strnlen(str.begin(), str.max_size()));
}
template <size_t SIZE>
auto to_view(const std::reference_wrapper<std::array<char, SIZE>> &str, const size_t size) -> iop::StringView {
  return to_view(str.get(), size);
}
template <size_t SIZE>
auto to_view(const std::reference_wrapper<std::array<char, SIZE>> &str) -> iop::StringView {
  return to_view(str.get());
}
} // namespace iop

#endif