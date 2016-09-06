#pragma once
#include "integer_block.hpp"
#include <mcpputil/mcpputil/unsafe_cast.hpp>
#include <numeric>
namespace mcppalloc
{
  namespace bitmap
  {
    namespace details
    {
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE void integer_block_t<Quads>::clear() noexcept
      {
        ::std::fill(m_array.begin(), m_array.end(), 0);
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE void integer_block_t<Quads>::fill(uint64_t word) noexcept
      {
        ::std::fill(m_array.begin(), m_array.end(), word);
      }

      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE void integer_block_t<Quads>::set_bit(size_t i, bool value) noexcept
      {
        auto pos = i / 64;
        auto sub_pos = i - (pos * 64);
        m_array[pos] = (m_array[pos] & (~(1ll << sub_pos))) | (static_cast<size_t>(value) << sub_pos);
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE void
      integer_block_t<Quads>::set_bit_atomic(size_t i, bool value, ::std::memory_order ordering) noexcept
      {
        auto pos = i / 64;
        auto sub_pos = i - (pos * 64);
        auto &bits = mcpputil::unsafe_reference_cast<::std::atomic<uint64_t>>(m_array[pos]);
        bool success = false;
        while (!success) {
          auto cur = bits.load(::std::memory_order_relaxed);
          auto new_val = (cur & (~(1ll << sub_pos))) | (static_cast<uint64_t>(value) << sub_pos);
          success = bits.compare_exchange_weak(cur, new_val, ordering);
        }
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::get_bit(size_t i) const noexcept -> bool
      {
        auto pos = i / 64;
        auto sub_pos = i - (pos * 64);
        return (m_array[pos] & (1ll << sub_pos)) > sub_pos;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE bool integer_block_t<Quads>::get_bit_atomic(size_t i, ::std::memory_order ordering) const noexcept
      {
        auto pos = i / 64;
        auto sub_pos = i - (pos * 64);
        auto &bits = mcpputil::unsafe_reference_cast<::std::atomic<uint64_t>>(m_array[pos]);
        return (bits.load(ordering) & (1ll << sub_pos)) > sub_pos;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::all_set() const noexcept -> bool
      {
        for (auto &&val : m_array) {
          if (val != ::std::numeric_limits<value_type>::max())
            return false;
        }
        return true;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::any_set() const noexcept -> bool
      {
        for (auto &&it : m_array)
          if (it)
            return true;
        return false;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::none_set() const noexcept -> bool
      {
        for (auto &&it : m_array)
          if (it)
            return false;
        return true;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::first_set() const noexcept -> size_t
      {
#if defined(__AVX__) && !defined(__APPLE__)
        __m256i m = *unsafe_cast<__m256i>(&m_array[0]);
        static_assert(size() >= 8, "");
        __m256i m2 = *unsafe_cast<__m256i>(&m_array[4]);
        if (!_mm256_testz_si256(m, m)) {
          for (size_t i = 0; i < 4; ++i) {
            const uint64_t &it = m_array[i];
            auto first = static_cast<size_t>(__builtin_ffsll(static_cast<long long>(it)));
            if (first)
              return (64 * i) + (first - 1);
          }
        } else if (!_mm256_testz_si256(m2, m2)) {
          for (size_t i = 4; i < 8; ++i) {
            const uint64_t &it = m_array[i];
            auto first = static_cast<size_t>(__builtin_ffsll(static_cast<long long>(it)));
            if (first)
              return (64 * i) + (first - 1);
          }
        } else {
          for (size_t i = 8; i < m_array.size(); ++i) {
            const uint64_t &it = m_array[i];
            auto first = static_cast<size_t>(__builtin_ffsll(static_cast<long long>(it)));
            if (first)
              return (64 * i) + (first - 1);
          }
        }
#else
        for (size_t i = 0; i < m_array.size(); ++i) {
          const uint64_t &it = m_array[i];
          auto first = static_cast<size_t>(__builtin_ffsll(static_cast<long long>(it)));
          if (first)
            return (64 * i) + (first - 1);
        }
#endif
        return ::std::numeric_limits<size_t>::max();
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::first_not_set() const noexcept -> size_t
      {
        return (~*this).first_set();
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::popcount() const noexcept -> size_t
      {
        return ::std::accumulate(m_array.begin(), m_array.end(), static_cast<size_t>(0),
                                 [](size_t b, auto x) { return static_cast<size_t>(__builtin_popcountll(x)) + b; });
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::operator~() const noexcept -> integer_block_t
      {
        integer_block_t ret = *this;
        for (auto &&i : ret.m_array)
          i = ~i;
        return ret;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::negate() noexcept -> integer_block_t &
      {
        for (auto &&i : m_array)
          i = ~i;
        return *this;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::operator|(const integer_block_t &b) const noexcept -> integer_block_t
      {
        integer_block_t ret;
        for (size_t i = 0; i < m_array.size(); ++i) {
          ret.m_array[i] = m_array[i] | b.m_array[i];
        }
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::operator|=(const integer_block_t &b) noexcept -> integer_block_t &
      {
        for (size_t i = 0; i < m_array.size(); ++i) {
          m_array[i] |= b.m_array[i];
        }
        return *this;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::operator&(const integer_block_t &b) const noexcept -> integer_block_t
      {
        integer_block_t ret;
        for (size_t i = 0; i < m_array.size(); ++i) {
          ret.m_array[i] = m_array[i] & b.m_array[i];
        }
        return ret;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::operator&=(const integer_block_t &b) noexcept -> integer_block_t &
      {
        for (size_t i = 0; i < m_array.size(); ++i) {
          m_array[i] &= b.m_array[i];
        }
        return *this;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::operator^(const integer_block_t &b) const noexcept -> integer_block_t
      {
        integer_block_t ret;
        for (size_t i = 0; i < m_array.size(); ++i) {
          ret.m_array[i] = m_array[i] ^ b.m_array[i];
        }
        return ret;
      }
      template <size_t Quads>
      MCPPALLOC_ALWAYS_INLINE auto integer_block_t<Quads>::operator^=(const integer_block_t &b) noexcept -> integer_block_t &
      {
        for (size_t i = 0; i < m_array.size(); ++i) {
          m_array[i] ^= b.m_array[i];
        }
        return *this;
      }
      template <size_t Quads>
      template <typename Func>
      void integer_block_t<Quads>::for_set_bits(size_t offset, size_t limit, Func &&func)
      {
        limit = ::std::min(limit, size_in_bits());
        for (size_t i = 0; i < limit; ++i) {
          if (i % 64 == 0 && m_array[i / 64] == 0)
            continue;
          if (get_bit(i))
            func(offset + i);
        }
      }

      template <size_t Quads>
      template <typename Func>
      void integer_block_t<Quads>::for_some_contiguous_bits_flip(size_t offset, Func &&func)
      {
#if defined(__AVX2__) && !defined(__APPLE__)
        const __m256i ones = _mm256_set1_epi64x(-1);
        for (size_t i = 0; i + 3 < size(); i += 4) {
          __m256i m = *unsafe_cast<__m256i>(&m_array[i]);
          m = _mm256_andnot_si256(m, ones);
          if (_mm256_testz_si256(m, m)) {
            func(offset + i * 64, offset + ((i + 4) * 64));
          }
        }
#elif defined(__SSE4__)
        for (size_t i = 0; i + 1 < size(); i += 2) {
          __m128i *m = unsafe_cast<__m128i>(&m_array[i]);
          if (_mm_test_all_ones(*m)) {
            func(offset + i * 64, offset + ((i + 2) * 64));
          }
        }
#else
        for (size_t i = 0; i + 1 < size(); i += 2) {
          if (m_array[i] == ::std::numeric_limits<uint64_t>::max()) {
            func(offset + i * 64, offset + ((i + 1) * 64));
          }
        }
#endif
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
