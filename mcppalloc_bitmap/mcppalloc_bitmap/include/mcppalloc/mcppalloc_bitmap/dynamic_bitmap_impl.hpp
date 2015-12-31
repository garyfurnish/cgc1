#pragma once
#include "integer_block.hpp"
namespace mcppalloc
{
  namespace bitmap
  {
    template <bool is_const>
    template <bool E>
    dynamic_bitmap_ref_t<is_const>::dynamic_bitmap_ref_t(typename ::std::enable_if<E, const bits_array_type *>::type array,
                                                         size_t sz) noexcept : m_array(array),
                                                                               m_size(sz)
    {
    }
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const>::dynamic_bitmap_ref_t(bits_array_type *array, size_t sz) noexcept : m_array(array),
                                                                                                              m_size(sz)
    {
    }
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const>::dynamic_bitmap_ref_t(const dynamic_bitmap_ref_t &) noexcept = default;
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const>::dynamic_bitmap_ref_t(dynamic_bitmap_ref_t &&) noexcept = default;
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const> &dynamic_bitmap_ref_t<is_const>::
    operator=(const dynamic_bitmap_ref_t &) noexcept = default;
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const> &dynamic_bitmap_ref_t<is_const>::operator=(dynamic_bitmap_ref_t &&) noexcept = default;
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::size() const noexcept -> size_t
    {
      return m_size;
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::size_in_bytes() const noexcept -> size_t
    {
      return m_size * sizeof(bits_array_type);
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::size_in_bits() const noexcept -> size_t
    {
      return m_size * sizeof(bits_array_type) * 8;
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::array() noexcept -> bits_array_type *
    {
      return m_array;
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::array() const noexcept -> const bits_array_type *
    {
      return m_array;
    }
    template <bool is_const>
    template <bool E>
    typename ::std::enable_if<E, void>::type dynamic_bitmap_ref_t<is_const>::clear() noexcept

    //    inline void dynamic_bitmap_ref_t<is_const>::clear() noexcept
    {
      for (size_t i = 0; i < size(); ++i)
        m_array[i].clear();
    }
    template <bool is_const>
    inline void dynamic_bitmap_ref_t<is_const>::fill(uint64_t word) noexcept
    {
      for (size_t i = 0; i < size(); ++i)
        m_array[i].fill(word);
    }
    template <bool is_const>
    inline void dynamic_bitmap_ref_t<is_const>::set_bit(size_t i, bool value) noexcept
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      m_array[pos].set_bit(sub_pos, value);
    }
    template <bool is_const>
    inline void dynamic_bitmap_ref_t<is_const>::set_bit_atomic(size_t i, bool value, ::std::memory_order ordering) noexcept
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      m_array[pos].set_bit_atomic(sub_pos, value, ordering);
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::get_bit(size_t i) const noexcept -> bool
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      return m_array[pos].get_bit(sub_pos);
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::get_bit_atomic(size_t i, ::std::memory_order ordering) const noexcept -> bool
    {
      auto pos = i / bits_array_type::size_in_bits();
      auto sub_pos = i - (pos * bits_array_type::size_in_bits());
      return m_array[pos].get_bit_atomic(sub_pos, ordering);
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::any_set() const noexcept -> bool
    {
      for (size_t i = 0; i < size(); ++i) {
        auto &&it = m_array[i];
        if (it.any_set())
          return true;
      }
      return false;
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::all_set() const noexcept -> bool
    {
      for (size_t i = 0; i < size(); ++i) {
        auto &&it = m_array[i];
        if (!it.all_set())
          return false;
      }
      return true;
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::none_set() const noexcept -> bool
    {
      for (size_t i = 0; i < size(); ++i) {
        auto &&it = m_array[i];
        if (it.any_set())
          return false;
      }
      return true;
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::first_set() const noexcept -> size_t
    {
      for (size_t i = 0; i < size(); ++i) {
        auto ret = m_array[i].first_set();
        if (ret != ::std::numeric_limits<size_t>::max())
          return i * bits_array_type::size_in_bits() + ret;
      }
      return ::std::numeric_limits<size_t>::max();
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::first_not_set() const noexcept -> size_t
    {
      for (size_t i = 0; i < size(); ++i) {
        auto ret = m_array[i].first_not_set();
        if (ret != ::std::numeric_limits<size_t>::max())
          return i * bits_array_type::size_in_bits() + ret;
      }
      return ::std::numeric_limits<size_t>::max();
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::popcount() const noexcept -> size_t
    {
      return ::std::accumulate(m_array, m_array + size(), static_cast<size_t>(0),
                               [](size_t b, auto &&x) { return x.popcount() + b; });
    }
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const> &dynamic_bitmap_ref_t<is_const>::negate() noexcept
    {
      for (size_t i = 0; i < size(); ++i) {
        m_array[i].negate();
      }
      return *this;
    }
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const> &dynamic_bitmap_ref_t<is_const>::operator|=(const dynamic_bitmap_ref_t &rhs) noexcept
    {
      for (size_t i = 0; i < size(); ++i) {
        m_array[i] |= rhs.m_array[i];
      }
      return *this;
    }
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const> &dynamic_bitmap_ref_t<is_const>::operator&=(const dynamic_bitmap_ref_t &rhs) noexcept
    {
      for (size_t i = 0; i < size(); ++i) {
        m_array[i] &= rhs.m_array[i];
      }
      return *this;
    }
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const> &dynamic_bitmap_ref_t<is_const>::operator^=(const dynamic_bitmap_ref_t &rhs) noexcept
    {
      for (size_t i = 0; i < size(); ++i) {
        m_array[i] ^= rhs.m_array[i];
      }
      return *this;
    }
  }
}
