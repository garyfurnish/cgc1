#pragma once
#include <cgc1/declarations.hpp>
#include <mcpputil/mcpputil/make_unique.hpp>
namespace cgc1
{
  template <typename T, typename Allocator>
  using unique_ptr_allocated = ::std::unique_ptr<T, typename ::mcpputil::mcpputil_allocator_deleter_t<T, Allocator>::type>;
}
