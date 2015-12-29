#pragma once
#include <map>
#include <vector>
namespace mcppalloc
{
  template <typename T, typename allocator>
  using rebind_vector_t = typename ::std::vector<T, typename allocator::template rebind<T>::other>;
  template <typename K, typename V, typename Compare, typename allocator>
  using rebind_map_t = typename ::std::map<K, V, Compare, typename allocator::template rebind<::std::pair<K, V>>::other>;
}
