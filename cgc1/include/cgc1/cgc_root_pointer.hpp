#pragma once
#include "cgc_root.hpp"
namespace cgc1
{
  template <typename T>
  class CGC1_DLL_PUBLIC cgc_root_pointer
  {
    cgc_root_pointer() noexcept;
    cgc_root_pointer(T *t) noexcept;
    cgc_root_pointer(const cgc_root_pointer<T> &) noexcept;
    cgc_root_pointer(cgc_root_pointer<T> &&) noexcept;
    cgc_root_pointer &operator=(const cgc_root_pointer<T> &) noexcept;
    cgc_root_pointer &operator=(cgc_root_pointer<T> &&) noexcept;
  };
}
