#pragma once
#include <cgc1/declarations.hpp>
namespace cgc1
{
  template <typename T, typename Allocator>
  using unique_ptr_allocated = ::std::unique_ptr<T, typename cgc_allocator_deleter_t<T, Allocator>::type>;
}