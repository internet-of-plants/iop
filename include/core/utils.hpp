#ifndef IOP_CORE_UTILS_HPP
#define IOP_CORE_UTILS_HPP

#include <array>
#include "core/log.hpp"
#include "core/panic.hpp"
#include <variant>

namespace iop {
using MD5Hash = std::array<char, 32>;
using MacAddress = std::array<char, 17>;

void logMemory(const Log &logger) noexcept;

template <typename T, typename E>
inline auto is_ok(std::variant<T, E> const &res) -> bool {
    return res.index() == 0;
}
template <typename T, typename E>
inline auto is_err(std::variant<T, E> const &res) -> bool {
    return res.index() == 1;
}

template <typename T, typename E>
inline auto unwrap_ok(std::variant<T, E> &res, const CodePoint point) -> T {
    if (iop::is_err(res))
        iop::panicHandler(F("unwrap_ok_ref found Error"), point); // TODO: can we improve this message?
    return T(std::move(std::get<T>(res)));
}
template <typename T, typename E>
auto unwrap_ok_ref(std::variant<T, E> const &res, const CodePoint point) -> T const & {
    if (iop::is_err(res))
        iop::panicHandler(F("unwrap_ok_ref found Error"), point); // TODO: can we improve this message?
    return std::get<T>(res);
}
template <typename T, typename E>
inline auto unwrap_ok_mut(std::variant<T, E> &res, const CodePoint point) -> T & {
    if (iop::is_err(res))
        iop::panicHandler(F("unwrap_ok_mut found Error"), point); // TODO: can we improve this message?
    return std::get<T>(res);
}
template <typename T, typename E>
inline auto unwrap_err_ref(std::variant<T, E> const &res, const CodePoint point) -> E const & {
    if (iop::is_ok(res))
        iop::panicHandler(F("unwrap_err_ref found Ok"), point); // TODO: can we improve this message?
    return std::get<E>(res);
}
template <typename T, typename E>
inline auto unwrap_err_mut(std::variant<T, E> &res, const CodePoint point) -> E & {
    if (iop::is_ok(res))
        iop::panicHandler(F("unwrap_err_mut found Ok"), point); // TODO: can we improve this message?
    return std::get<E>(res);
}
template <typename T, typename E>
inline auto ok(std::variant<T, E> res) -> std::optional<T> {
    if (std::holds_alternative<T>(res))
        return iop::unwrap_ok(res, IOP_CTX());
    return std::optional<T>();
}

template <typename T>
inline auto unwrap_ref(std::optional<T> const &opt, const CodePoint point) -> T const & {
    if (!opt.has_value()) {
        iop::panicHandler(F("Option::unwrap_ref found None"), point); // TODO: can we improve this message?
    }
    return opt.value();
}

template <typename T>
inline auto unwrap_mut(std::optional<T> &opt, const CodePoint point) -> T& {
    if (!opt.has_value()) {
        iop::panicHandler(F("Option::unwrap_mut found None"), point); // TODO: can we improve this message?
    }
    return opt.value();
}

template <typename T>
inline auto unwrap(std::optional<T> &opt, const CodePoint point) -> T {
    if (!opt.has_value()) {
        iop::panicHandler(F("Option::unwrap found None"), point); // TODO: can we improve this message?
    }
    std::optional<T> other;
    opt.swap(other);
    return std::move(other.value());
}
} // namespace iop

#endif