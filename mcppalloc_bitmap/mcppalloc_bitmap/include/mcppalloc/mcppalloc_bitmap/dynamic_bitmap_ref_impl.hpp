#pragma once
#include "integer_block.hpp"
#include <mcpputil/mcpputil/alignment.hpp>
#include <mcpputil/mcpputil/unsafe_cast.hpp>
namespace mcppalloc
{
  namespace bitmap
  {
    template <bool is_const>
    inline dynamic_bitmap_ref_t<is_const>::dynamic_bitmap_ref_t(bits_array_type array, size_t sz) noexcept
        : m_array(array), m_size(sz)
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
      if (mcpputil_unlikely(rhs.size() != size()))
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
      size_t i = 0;
      const size_t rounded_max = (size() / 4) * 4;
      while (i < rounded_max) {
        auto ret = m_array[i].first_set();
        if (ret != ::std::numeric_limits<size_t>::max())
          return i * bits_type::size_in_bits() + ret;
        ret = m_array[i + 1].first_set();
        if (ret != ::std::numeric_limits<size_t>::max())
          return (i + 1) * bits_type::size_in_bits() + ret;
        ret = m_array[i + 2].first_set();
        if (ret != ::std::numeric_limits<size_t>::max())
          return (i + 2) * bits_type::size_in_bits() + ret;
        ret = m_array[i + 3].first_set();
        if (ret != ::std::numeric_limits<size_t>::max())
          return (i + 3) * bits_type::size_in_bits() + ret;
        i += 4;
      }
      for (; i < size(); ++i) {
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
    template <bool is_const>
    template <typename Func>
    void dynamic_bitmap_ref_t<is_const>::for_some_contiguous_bits_flip(size_t limit, Func &&func)
    {
      limit = ::std::min(limit, size_in_bits());
      const auto max_block = limit / bits_type::size_in_bits();
      for (size_t block = 0; block < max_block; ++block) {
        m_array[block].for_some_contiguous_bits_flip(block * bits_type::size_in_bits(), func);
      }
    }
    template <bool is_const>
    template <typename Func>
    void dynamic_bitmap_ref_t<is_const>::for_set_bits(size_t limit, Func &&func) const
    {
      limit = ::std::min(limit, size_in_bits());
      for (size_t block = 0; block < size(); ++block) {
        size_t internal_limit = bits_type::size_in_bits();
        bool done = false;
        if ((block + 1) * bits_type::size_in_bits() > limit) {
          internal_limit = limit % bits_type::size_in_bits() ? limit % bits_type::size_in_bits() : internal_limit;
          done = true;
        }
        m_array[block].for_set_bits(block * bits_type::size_in_bits(), internal_limit, ::std::move(func));
        if (done)
          break;
      }
      /*      for (size_t i = first_set(); i < limit; ++i) {
  if (get_bit(i))
    func(i);
    }*/
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
          reinterpret_cast<bits_array_type>(mcpputil::align(array, dynamic_bitmap_ref_t<false>::bits_type::cs_alignment));
      if (mcpputil_unlikely(
              static_cast<size_t>(mcpputil::unsafe_cast<uint8_t>(new_array) - mcpputil::unsafe_cast<uint8_t>(array)) >
              alloca_size - array_size * sizeof(dynamic_bitmap_ref_t<false>::bits_type))) {
        throw ::std::runtime_error("mcppalloc: out of bounds: e788e46f-2e1d-4afb-af86-d48e068d3d5c");
      }
      return dynamic_bitmap_ref_t<false>(new_array, array_size);
    }
  }
}
