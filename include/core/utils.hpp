#ifndef IOP_CORE_UTILS_HPP
#define IOP_CORE_UTILS_HPP

#include "storage.hpp"

// (Un)Comment this line to toggle wifi dependency
#define IOP_ONLINE

// (Un)Comment this line to toggle serial dependency
#define IOP_SERIAL

// (Un)Comment this line to toggle SSL dependency
// #define IOP_SSL

// (Un)Comment this line to toggle memory stats logging
// #define LOG_MEMORY

namespace iop {
using esp_time = unsigned long; // NOLINT google-runtime-int

TYPED_STORAGE(MD5Hash, 32);
TYPED_STORAGE(MacAddress, 17);

auto hashSketch() noexcept -> const MD5Hash &;
auto macAddress() noexcept -> const MacAddress &;
void logMemory(const iop::Log &logger) noexcept;

template <typename T>
auto as_ref(std::optional<T> const &opt) -> std::optional<std::reference_wrapper<const T>> {
    if (!opt.has_value())
        return std::optional<std::reference_wrapper<const T>>();
    return std::make_optional(std::ref(opt.value()));
}

template <typename T>
auto as_mut(std::optional<T> const &opt) -> std::optional<std::reference_wrapper<T>> {
    if (!opt.has_value())
        return std::optional<std::reference_wrapper<T>>();
    return std::make_optional(std::ref(opt.value()));
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