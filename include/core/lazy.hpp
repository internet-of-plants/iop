#ifndef IOP_LAZY_HPP
#define IOP_LAZY_HPP

#include <optional>
#include <functional>
#include "core/utils.hpp"

namespace iop {
template <typename T>
class Lazy {
    std::function<T()> getter;
    std::optional<T> container;
public:
    Lazy(std::function<T()> getter_) noexcept: getter(getter_) {}
    T& operator*() noexcept {
        if (!this->container.has_value()) {
            T value = getter();
            this->container.emplace(value);
        }
        return iop::unwrap_mut(this->container, IOP_CTX());
    }
    T* operator->() noexcept {
        return &**this;
    }
    ~Lazy() noexcept = default;
};
}

#endif