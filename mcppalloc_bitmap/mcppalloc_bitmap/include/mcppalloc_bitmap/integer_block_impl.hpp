#pragma once
#include "integer_block.hpp"
#include <mcppalloc_utils/unsafe_cast.hpp>
#include <numeric>
namespace mcppalloc
{
  namespace bitmap
  {
    namespace details
    {
      template <size_t Quads>
      CGC1_ALWAYS_INLINE void integer_block_t<Quads>::clear() noexcept
      {
        ::std::fill(m_array.begin(), m_array.end(), 0);
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE void integer_block_t<Quads>::fill(uint64_t word) noexcept
      {
        ::std::fill(m_array.begin(), m_array.end(), word);
      }

      template <size_t Quads>
      CGC1_ALWAYS_INLINE void integer_block_t<Quads>::set_bit(size_t i, bool value) noexcept
      {
        auto pos = i / 64;
        auto sub_pos = i - (pos * 64);
        m_array[pos] = (m_array[pos] & (~(1ll << sub_pos))) | (static_cast<size_t>(value) << sub_pos);
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE void integer_block_t<Quads>::set_bit_atomic(size_t i, bool value, ::std::memory_order ordering) noexcept
      {
        auto pos = i / 64;
        auto sub_pos = i - (pos * 64);
        auto &bits = unsafe_reference_cast<::std::atomic<uint64_t>>(m_array[pos]);
        bool success = false;
        while (!success) {
          auto cur = bits.load(::std::memory_order_relaxed);
          auto new_val = (cur & (~(1ll << sub_pos))) | (static_cast<uint64_t>(value) << sub_pos);
          success = bits.compare_exchange_weak(cur, new_val, ordering);
        }
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::get_bit(size_t i) const noexcept -> bool
      {
        auto pos = i / 64;
        auto sub_pos = i - (pos * 64);
        return (m_array[pos] & (1ll << sub_pos)) > sub_pos;
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE void integer_block_t<Quads>::get_bit_atomic(size_t i, ::std::memory_order ordering) const noexcept
      {
        auto pos = i / 64;
        auto sub_pos = i - (pos * 64);
        auto &bits = unsafe_reference_cast<::std::atomic<uint64_t>>(m_array[pos]);
        return (bits.load(ordering) & (1ll << sub_pos)) > sub_pos;
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::all_set() const noexcept -> bool
      {
        for (auto &&val : m_array) {
          if (val != ::std::numeric_limits<value_type>::max())
            return false;
        }
        return true;
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::any_set() const noexcept -> bool
      {
        for (auto &&it : m_array)
          if (it)
            return true;
        return false;
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::none_set() const noexcept -> bool
      {
        for (auto &&it : m_array)
          if (it)
            return false;
        return true;
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::first_set() const noexcept -> size_t
      {
        for (size_t i = 0; i < m_array.size(); ++i) {
          const uint64_t &it = m_array[i];
          auto first = static_cast<size_t>(__builtin_ffsll(static_cast<long long>(it)));
          if (first)
            return (64 * i) + (first - 1);
        }
        return ::std::numeric_limits<size_t>::max();
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::first_not_set() const noexcept -> size_t
      {
        return (~*this).first_set();
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::popcount() const noexcept -> size_t
      {
        return ::std::accumulate(m_array.begin(), m_array.end(), static_cast<size_t>(0),
                                 [](size_t b, auto x) { return static_cast<size_t>(__builtin_popcountll(x)) + b; });
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::operator~() const noexcept -> integer_block_t
      {
        integer_block_t ret = *this;
        for (auto &&i : ret.m_array)
          i = ~i;
        return ret;
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::negate() noexcept -> integer_block_t &
      {
        for (auto &&i : m_array)
          i = ~i;
        return *this;
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::operator|(const integer_block_t &b) const noexcept -> integer_block_t
      {
        integer_block_t ret;
        for (size_t i = 0; i < m_array.size(); ++i) {
          ret.m_array[i] = m_array[i] | b.m_array[i];
        }
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::operator|=(const integer_block_t &b) noexcept -> integer_block_t &
      {
        for (size_t i = 0; i < m_array.size(); ++i) {
          m_array[i] |= b.m_array[i];
        }
        return *this;
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::operator&(const integer_block_t &b) const noexcept -> integer_block_t
      {
        integer_block_t ret;
        for (size_t i = 0; i < m_array.size(); ++i) {
          ret.m_array[i] = m_array[i] & b.m_array[i];
        }
        return ret;
      }
      template <size_t Quads>
      CGC1_ALWAYS_INLINE auto integer_block_t<Quads>::operator&=(const integer_block_t &b) noexcept -> integer_block_t &
      {
        for (size_t i = 0; i < m_array.size(); ++i) {
          m_array[i] &= b.m_array[i];
        }
        return *this;
      }

      template <size_t Quads>
      constexpr size_t integer_block_t<Quads>::size() noexcept
      {
        return cs_quad_words;
      }
      template <size_t Quads>
      constexpr size_t integer_block_t<Quads>::size_in_bytes() noexcept
      {
        return sizeof(m_array);
      }
      template <size_t Quads>
      constexpr size_t integer_block_t<Quads>::size_in_bits() noexcept
      {
        return size_in_bytes() * 8;
      }
    }
  }
}
