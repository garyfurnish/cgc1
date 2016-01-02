#pragma once
#include "integer_block.hpp"
#include <mcppalloc/mcppalloc_utils/alignment.hpp>
#include <mcppalloc/mcppalloc_utils/unsafe_cast.hpp>
namespace mcppalloc
{
  namespace bitmap
  {
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const>::dynamic_bitmap_ref_t(bits_array_type array, size_t sz) noexcept : m_array(array),
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
    inline dynamic_bitmap_ref_t<is_const> &dynamic_bitmap_ref_t<is_const>::deep_copy(const dynamic_bitmap_ref_t<is_const> &rhs)
    {
      if (mcppalloc_unlikely(rhs.size() != size()))
        throw ::std::runtime_error("mcppalloc: dynamic_bitmap_ref_t: bad operator=: 19f0d9b7-200f-46ae-9b73-9b874ed11045");
      for (size_t i = 0; i < rhs.size(); ++i)
        m_array[i] = rhs.m_array[i];
      return *this;
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::size() const noexcept -> size_t
    {
      return m_size;
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::size_in_bytes() const noexcept -> size_t
    {
      return m_size * sizeof(bits_type);
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::size_in_bits() const noexcept -> size_t
    {
      return m_size * sizeof(bits_type) * 8;
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::array() noexcept -> bits_array_type
    {
      return m_array;
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::array() const noexcept -> const bits_array_type
    {
      return m_array;
    }
    template <bool is_const>
    template <bool E>
    typename ::std::enable_if<E, void>::type dynamic_bitmap_ref_t<is_const>::clear() noexcept
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
      auto pos = i / bits_type::size_in_bits();
      auto sub_pos = i - (pos * bits_type::size_in_bits());
      m_array[pos].set_bit(sub_pos, value);
    }
    template <bool is_const>
    inline void dynamic_bitmap_ref_t<is_const>::set_bit_atomic(size_t i, bool value, ::std::memory_order ordering) noexcept
    {
      auto pos = i / bits_type::size_in_bits();
      auto sub_pos = i - (pos * bits_type::size_in_bits());
      m_array[pos].set_bit_atomic(sub_pos, value, ordering);
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::get_bit(size_t i) const noexcept -> bool
    {
      auto pos = i / bits_type::size_in_bits();
      auto sub_pos = i - (pos * bits_type::size_in_bits());
      return m_array[pos].get_bit(sub_pos);
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::get_bit_atomic(size_t i, ::std::memory_order ordering) const noexcept -> bool
    {
      auto pos = i / bits_type::size_in_bits();
      auto sub_pos = i - (pos * bits_type::size_in_bits());
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
          return i * bits_type::size_in_bits() + ret;
      }
      return ::std::numeric_limits<size_t>::max();
    }
    template <bool is_const>
    inline auto dynamic_bitmap_ref_t<is_const>::first_not_set() const noexcept -> size_t
    {
      for (size_t i = 0; i < size(); ++i) {
        auto ret = m_array[i].first_not_set();
        if (ret != ::std::numeric_limits<size_t>::max())
          return i * bits_type::size_in_bits() + ret;
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
    inline auto make_dynamic_bitmap_ref(dynamic_bitmap_ref_t<false>::bits_array_type array, size_t sz) noexcept
    {
      return dynamic_bitmap_ref_t<false>(array, sz);
    }
    inline auto make_dynamic_bitmap_ref(dynamic_bitmap_ref_t<true>::bits_array_type array, size_t sz) noexcept
    {
      return dynamic_bitmap_ref_t<true>(array, sz);
    }
    inline auto make_dynamic_bitmap_ref_from_alloca(void *memory, size_t array_size, size_t alloca_size)
    {
      using bits_array_type = typename dynamic_bitmap_ref_t<false>::bits_array_type;
      bits_array_type array = reinterpret_cast<bits_array_type>(memory);
      bits_array_type new_array =
          reinterpret_cast<bits_array_type>(align(array, dynamic_bitmap_ref_t<false>::bits_type::cs_alignment));
      if (mcppalloc_unlikely(static_cast<size_t>(unsafe_cast<uint8_t>(new_array) - unsafe_cast<uint8_t>(array)) >
                             alloca_size - array_size * sizeof(dynamic_bitmap_ref_t<false>::bits_type))) {
        throw ::std::runtime_error("mcppalloc: out of bounds: e788e46f-2e1d-4afb-af86-d48e068d3d5c");
      }
      return dynamic_bitmap_ref_t<false>(new_array, array_size);
    }
  }
}
