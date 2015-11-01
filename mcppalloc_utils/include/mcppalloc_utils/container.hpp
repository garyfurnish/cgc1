#pragma once
#include <vector>
namespace mcppalloc
{
  template <typename T, typename allocator>
  using rebind_vector_t = typename ::std::vector<T, typename allocator::template rebind<T>::other>;
}
