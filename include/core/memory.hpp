#ifndef IOP_CORE_MEMORY_HPP
#define IOP_CORE_MEMORY_HPP

#include "core/tracer.hpp"
#include <memory>

namespace iop {

template <typename T, typename = std::enable_if_t<std::is_array<T>::value>>
auto try_make_unique(size_t size) noexcept -> std::unique_ptr<T> {
  IOP_TRACE();
  return std::unique_ptr<T>(new typename std::remove_extent<T>::type[size]());
}

template <typename T, typename = std::enable_if_t<!std::is_array<T>::value>,
          typename... Args>
auto try_make_unique(Args &&...args) noexcept -> std::unique_ptr<T> {
  IOP_TRACE();
  return std::unique_ptr<T>(new (std::nothrow) T(std::forward<Args>(args)...));
}

// TODO: OOm can still cause problems because refcount is still allocated
// separately and can fail
template <typename _Tp, typename... _Args>
inline auto try_make_shared(_Args &&...__args) noexcept
    -> std::shared_ptr<_Tp> {
  IOP_TRACE();
  typedef typename std::remove_const<_Tp>::type _Tp_nc;
  return std::allocate_shared<_Tp>(std::allocator<_Tp_nc>(),
                                   std::forward<_Args>(__args)...);
}
} // namespace iop

#endif