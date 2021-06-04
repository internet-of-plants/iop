#ifndef IOP_CORE_UTILS_HPP
#define IOP_CORE_UTILS_HPP

#include "core/string/fixed.hpp"

// (Un)Comment this line to toggle wifi dependency
#define IOP_ONLINE

// (Un)Comment this line to toggle serial dependency
#define IOP_SERIAL

// (Un)Comment this line to toggle SSL dependency
#ifndef IOP_DESKTOP
#define IOP_SSL
#endif

// (Un)Comment this line to toggle memory stats logging
// #define LOG_MEMORY

namespace iop {
using esp_time = unsigned long; // NOLINT google-runtime-int

TYPED_STORAGE(MD5Hash, 32);
TYPED_STORAGE(MacAddress, 17);

auto hashString(const std::string_view txt) noexcept -> uint64_t; // FNV hash
auto isPrintable(const char ch) noexcept -> bool;
auto isAllPrintable(const std::string_view txt) noexcept -> bool;
auto scapeNonPrintable(const std::string_view txt) noexcept -> CowString;
auto hashSketch() noexcept -> const MD5Hash &;
auto macAddress() noexcept -> const MacAddress &;
void logMemory(const Log &logger) noexcept;
#ifndef IOP_DESKTOP
auto to_view(const String& str) -> std::string_view;
#endif
auto to_view(const std::string& str) -> std::string_view;
auto to_view(const CowString& str) -> std::string_view;
template <uint16_t SIZE>
auto to_view(const FixedString<SIZE>& str) -> std::string_view;

template <typename T, typename E>
auto is_ok(std::variant<T, E> const &res) -> bool {
    return std::holds_alternative<T>(res);
}
template <typename T, typename E>
auto is_err(std::variant<T, E> const &res) -> bool {
    return std::holds_alternative<E>(res);
}

template <typename T, typename E>
auto unwrap_ok_ref(std::variant<T, E> const &res, const CodePoint point) -> T const & {
    if (iop::is_err(res))
        iop::panicHandler(F("unwrap_ok_ref found Error"), point); // TODO: can we improve this message?
    return std::get<T>(res);
}
template <typename T, typename E>
auto unwrap_ok_mut(std::variant<T, E> &res, const CodePoint point) -> T & {
    if (iop::is_err(res))
        iop::panicHandler(F("unwrap_ok_mut found Error"), point); // TODO: can we improve this message?
    return std::get<T>(res);
}
template <typename T, typename E>
auto unwrap_err_ref(std::variant<T, E> const &res, const CodePoint point) -> E const & {
    if (iop::is_ok(res))
        iop::panicHandler(F("unwrap_err_ref found Ok"), point); // TODO: can we improve this message?
    return std::get<E>(res);
}
template <typename T, typename E>
auto unwrap_err_mut(std::variant<T, E> &res, const CodePoint point) -> E & {
    if (iop::is_ok(res))
        iop::panicHandler(F("unwrap_err_mut found Ok"), point); // TODO: can we improve this message?
    return std::get<E>(res);
}

template <typename T>
auto unwrap_ref(std::optional<T> const &opt, const CodePoint point) -> T const & {
    if (!opt.has_value()) {
        iop::panicHandler(F("Option::unwrap_ref found None"), point); // TODO: can we improve this message?
    }
    return opt.value();
}

template <typename T>
auto unwrap_mut(std::optional<T> &opt, const CodePoint point) -> T& {
    if (!opt.has_value()) {
        iop::panicHandler(F("Option::unwrap_mut found None"), point); // TODO: can we improve this message?
    }
    return opt.value();
}

template <typename T>
auto unwrap(std::optional<T> &opt, const CodePoint point) -> T {
    if (!opt.has_value()) {
        iop::panicHandler(F("Option::unwrap found None"), point); // TODO: can we improve this message?
    }
    std::optional<T> other;
    opt.swap(other);
    return std::move(other.value());
}
} // namespace iop

#endif