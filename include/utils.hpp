#ifndef IOP_UTILS_HPP
#define IOP_UTILS_HPP

#include <cstdint>

// (Un)Comment this line to toggle wifi dependency
#define IOP_ONLINE

// (Un)Comment this line to toggle credentials server dependency
#define IOP_SERVER

// (Un)Comment this line to toggle monitor server dependency
#define IOP_MONITOR

// (Un)Comment this line to toggle serial dependency
#define IOP_SERIAL

// (Un)Comment this line to toggle sensors dependency
#define IOP_SENSORS

// (Un)Comment this line to toggle flash memory dependency
#define IOP_FLASH

// (Un)Comment this line to toggle factory reset dependency
#define IOP_FACTORY_RESET

// (Un)Comment this line to toggle over the air updates (OTA) dependency
#define IOP_OTA

// (Un)Comment this line to toggle memory stats logging
//#define LOG_MEMORY

// If IOP_MONITOR is not defined the Api methods will be short-circuited
// If IOP_MOCK_MONITOR is defined, then the methods will run normally
// and pretend the request didn't fail
// If IOP_MONITOR is defined, then it doesn't matter whether IOP_MOCK_MONITOR is
// defined
#define IOP_MOCK_MONITOR

using esp_time = unsigned long; // NOLINT google-runtime-int

// If you change the number of interrupt types, please update interruptVariant's
// to the correct size
using InterruptEvent = enum class interruptEvent {
  NONE,
  FACTORY_RESET,
  ON_CONNECTION,
  MUST_UPGRADE
};
constexpr static const uint8_t interruptVariants = 4;

#define MAYBE_PROGMEM_STRING_EMPTY(name) static const Option<StaticString> name;
#define MAYBE_PROGMEM_STRING(name, msg)                                        \
  PROGMEM_STRING(name_##storage, msg);                                         \
  static const Option<StaticString> name(name_##storage);

// This is a sanity check, we override CertStore by hijacking the header guard
// That means if someone breaks the include order HTTPS will break silently
static volatile auto certStoreOverrideWorked = false;

#include <memory>

template <bool B, class T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template <typename T, typename = enable_if_t<std::is_array<T>::value>>
auto try_make_unique(size_t size) noexcept -> std::unique_ptr<T> {
  return std::unique_ptr<T>(new typename std::remove_extent<T>::type[size]());
}

template <typename T, typename = enable_if_t<!std::is_array<T>::value>,
          typename... Args>
auto try_make_unique(Args &&...args) noexcept -> std::unique_ptr<T> {
  return std::unique_ptr<T>(new (std::nothrow) T(std::forward<Args>(args)...));
}

// TODO: OOm can still cause problems because refcount is still allocated
// separately and can fail
template <typename _Tp, typename... _Args>
inline auto try_make_shared(_Args &&...__args) noexcept
    -> std::shared_ptr<_Tp> {
  typedef typename std::remove_const<_Tp>::type _Tp_nc;
  return std::allocate_shared<_Tp>(std::allocator<_Tp_nc>(),
                                   std::forward<_Args>(__args)...);
}

#endif