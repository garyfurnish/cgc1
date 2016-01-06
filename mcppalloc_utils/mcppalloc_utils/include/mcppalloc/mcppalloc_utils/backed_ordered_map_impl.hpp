#pragma once
#include "backed_ordered_map.hpp"
#include <algorithm>
#include <mcppalloc/mcppalloc_utils/unsafe_cast.hpp>
namespace mcppalloc
{
  namespace containers
  {
    template <typename K, typename V, typename Less>
    backed_ordered_multimap<K, V, Less>::backed_ordered_multimap() = default;
    template <typename K, typename V, typename Less>
    template <typename KC>
    backed_ordered_multimap<K, V, Less>::backed_ordered_multimap(backing_pointer_type v, size_t capacity, KC &&kc)
        : m_v(v), m_capacity(capacity / sizeof(value_type)), m_compare(::std::forward<KC>(kc))
    {
    }
    template <typename K, typename V, typename Less>
    backed_ordered_multimap<K, V, Less>::backed_ordered_multimap(backed_ordered_multimap &&bom)
        : m_v(::std::move(bom.m_v)), m_size(::std::move(bom.m_size)), m_capacity(::std::move(bom.m_capacity)),
          m_compare(::std::move(bom.m_compare))
    {
      bom.m_v = nullptr;
      bom.m_size = 0;
      bom.m_capacity = 0;
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::operator=(backed_ordered_multimap &&rhs) -> backed_ordered_multimap &
    {
      m_v = rhs.m_v;
      m_size = rhs.m_size;
      m_capacity = rhs.m_capacity;
      m_compare = ::std::move(rhs.m_compare);
      rhs.m_v = nullptr;
      rhs.m_size = 0;
      rhs.m_capacity = 0;
    }
    template <typename K, typename V, typename Less>
    backed_ordered_multimap<K, V, Less>::~backed_ordered_multimap()
    {
      clear();
      m_v = nullptr;
      m_size = 0;
      m_capacity = 0;
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::size() noexcept -> size_type
    {
      return m_size;
    }
    template <typename K, typename V, typename Less>

    auto backed_ordered_multimap<K, V, Less>::capacity() noexcept -> size_type
    {
      return m_capacity;
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::data() noexcept -> value_type *
    {
      return unsafe_cast<value_type>(m_v);
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::data() const noexcept -> const value_type *
    {
      return unsafe_cast<const value_type>(m_v);
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::begin() noexcept -> iterator
    {
      return data();
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::begin() const noexcept -> const_iterator
    {
      return data();
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::cbegin() const noexcept -> const_iterator
    {
      return data();
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::end() noexcept -> iterator
    {
      return data() + size();
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::end() const noexcept -> const_iterator
    {
      return data() + size();
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::cend() const noexcept -> const_iterator
    {
      return data() + size();
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::find(const key_type &k) -> iterator
    {
      auto lb = lower_bound(k);
      if (!m_compare(k, lb->first))
        return lb;
      return end();
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::find(const key_type &k) const -> const_iterator
    {
      auto lb = lower_bound(k);
      if (!m_compare(k, lb->first))
        return lb;
      return end();
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::operator[](const key_type &k) -> mapped_type &
    {
      auto lb = find(k);
      if (lb != end())
        return lb->second;
      auto ii = insert(k, value_type())->second;
      if (!ii->second)
        throw ::std::out_of_range("mcppalloc: backed_ordered_multimap full: d7a2ae32-ba84-426c-a5ff-f2d7eb33f735");
      return ii->first->second;
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::operator[](key_type &&k) -> mapped_type &
    {
      auto lb = find(k);
      if (lb != end())
        return lb->second;
      auto ii = insert(k, value_type())->second;
      if (!ii->second)
        throw ::std::out_of_range("mcppalloc: backed_ordered_multimap full: f1620786-17eb-4f0d-b582-31396b9fe1c3");
      return ii->first->second;
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::at(const key_type &k) -> mapped_type &
    {
      auto lb = find(k);
      if (lb != end())
        return lb->second;
      throw ::std::out_of_range("mcppalloc: key does not exist: 6a3c90e5-c060-4394-94ad-77b7ad2ee987");
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::at(const key_type &k) const -> const mapped_type &
    {
      auto lb = find(k);
      if (lb != end())
        return lb->second;
      throw ::std::out_of_range("mcppalloc: key does not exist: cae44ae9-4ecb-4436-939a-c185c05b804b");
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::insert(const value_type &k) -> std::pair<iterator, bool>
    {
      if (size() == capacity())
        return ::std::make_pair(end(), false);
      auto ub = upper_bound(k.first);
      new (end()) value_type(k);
      ++m_size;
      ::std::rotate(ub, end() - 1, end());
      return ::std::make_pair(ub, true);
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::insert(value_type &&k) -> ::std::pair<iterator, bool>
    {
      if (size() == capacity())
        return ::std::make_pair(end(), false);
      auto ub = upper_bound(k.first);
      new (end()) value_type(::std::move(k));
      ++m_size;
      ::std::rotate(ub, end() - 1, end());
      return ::std::make_pair(ub, true);
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::erase(const key_type &k) -> size_type
    {
      auto it = find(k);
      if (it == end())
        return 0;
      auto it_erase_end = it + 1;
      while (it_erase_end != end() && !m_compare(k, it_erase_end->first))
        it_erase_end++;
      const size_t num_erased = static_cast<size_t>(it_erase_end - it);
      erase(it, it_erase_end);
      return num_erased;
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::erase(const_iterator cit) -> iterator
    {
      if (cit >= end())
        return 0;
      const auto it = begin() + (cit - begin());
      ::std::rotate(it, it + 1, end());
      (end() - 1)->~value_type();
      return it;
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::erase(const_iterator cit_erase_begin, const_iterator cit_erase_end) -> iterator
    {
      const auto it_erase_begin = begin() + (cit_erase_begin - begin());
      const auto it_erase_end = begin() + (cit_erase_end - begin());
      const size_t num_erased = static_cast<size_t>(it_erase_end - it_erase_begin);
      if (!num_erased)
        return it_erase_end;
      ::std::rotate(it_erase_begin, it_erase_end, end());
      for (size_t i = 0; i < num_erased; ++i)
        (end() - 1 - i)->~value_type();
      m_size -= num_erased;
      return it_erase_end;
    }

    template <typename K, typename V, typename Less>
    void backed_ordered_multimap<K, V, Less>::clear()
    {
      for (size_type i = 0; i < size(); ++i) {
        data()[i].~value_type();
      }
      m_size = 0;
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::lower_bound(const key_type &k) -> iterator
    {
      return ::std::lower_bound(begin(), end(), k, [this](auto &&a, auto &&b) { return m_compare(a.first, b); });
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::lower_bound(const key_type &k) const -> const_iterator
    {
      return ::std::lower_bound(begin(), end(), k, [this](auto &&a, auto &&b) { return m_compare(a.first, b); });
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::upper_bound(const key_type &k) -> iterator
    {
      return ::std::upper_bound(begin(), end(), k, [this](auto &&a, auto &&b) { return m_compare(a, b.first); });
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::upper_bound(const key_type &k) const -> const_iterator
    {
      return ::std::upper_bound(begin(), end(), k, [this](auto &&a, auto &&b) { return m_compare(a, b.first); });
    }
    template <typename K, typename V, typename Less>
    auto backed_ordered_multimap<K, V, Less>::move(const key_type &k, value_type &&v) -> ::std::pair<iterator, bool>
    {
      auto insert_it = upper_bound(v.first);
      auto remove_it = find(k);
      if (remove_it == end())
        return insert(::std::move(v));
      if (remove_it < insert_it) {
        ::std::rotate(remove_it, remove_it + 1, insert_it);
        *(insert_it - 1) = ::std::move(v);
        return ::std::make_pair(insert_it, true);
      } else if (remove_it > insert_it) {
        ::std::rotate(insert_it, remove_it, remove_it + 1);
        *insert_it = ::std::move(v);
        return ::std::make_pair(insert_it, true);
      } else {
        // equal
        *remove_it = ::std::move(v);
        return ::std::make_pair(remove_it, true);
      }
    }
  }
}
