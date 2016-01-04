#pragma once
#include <emmintrin.h>
#include <random>
namespace mcppalloc
{
  namespace details
  {
    /**
     * \brief Securely zero a pointer, guarenteed not to be optimized out.
     *
     * This has no alignment restrictions.
     **/
    inline void secure_zero_no_vector(volatile void *s, size_t n)
    {
      volatile size_t *p_sz = reinterpret_cast<volatile size_t *>(s);
      while (n >= sizeof(size_t)) {
        *p_sz++ = 0;
        n -= sizeof(size_t);
      }
      volatile char *p = reinterpret_cast<volatile char *>(p_sz);

      while (n--)
        *p++ = 0;
    }
  }
  inline void secure_zero_stream(void *s, size_t n)
  {
#ifdef __AVX__
    __m256i zero256 = _mm256_setzero_si256();
    __m256i *p_m256 = reinterpret_cast<__m256i *>(s);
    if (!s % 32) {
      while (n >= sizeof(__m256i)) {
        _mm256_stream_si256(p_m256++, zero256);
        n -= sizeof(__m256i);
      }
    }
    volatile size_t *p_sz = reinterpret_cast<volatile size_t *>(p_m256);
#elif defined(__SSE2__)
    const __m128i zero = _mm_setzero_si128();
    __m128i *p_m128 = reinterpret_cast<__m128i *>(s);
    if (!s % 16) {
      while (n >= sizeof(__m128i)) {
        _mm_stream_si128(p_m128++, zero);
        *p_m128++ = zero;
        n -= sizeof(__m128i);
      }
    }
    volatile size_t *p_sz = reinterpret_cast<volatile size_t *>(p_m128);
#else
    volatile size_t *p_sz = reinterpret_cast<volatile size_t *>(s);
    while (n >= sizeof(size_t) * 8) {
      *p_sz++ = 0;
      *p_sz++ = 0;
      *p_sz++ = 0;
      *p_sz++ = 0;
      n -= sizeof(size_t) * 4;
    }
#endif
    details::secure_zero_no_vector(p_sz, n);
  }
  /**
   * \brief Securely zero a pointer, guarenteed to not be optimized out.
   **/
  inline void secure_zero(void *s, size_t n)
  {
#ifdef __AVX__
    __m256i zero256 = _mm256_setzero_si256();
    volatile __m256i *p_m256 = reinterpret_cast<volatile __m256i *>(s);
    volatile __m128i *p_m128 = reinterpret_cast<volatile __m128i *>(p_m256);
    if (!s % 32) {
      while (n >= sizeof(__m256i)) {
        *p_m256++ = zero256;
        n -= sizeof(__m256i);
      }
      p_m128 = reinterpret_cast<volatile __m128i *>(p_m256);
      if (n >= sizeof(__m128i)) {
        *p_m128++ = _mm_setzero_si128();
        n -= sizeof(__m128i);
      }
    }
    volatile size_t *p_sz = reinterpret_cast<volatile size_t *>(p_m128);
#elif defined(__SSE2__)
    const __m128i zero = _mm_setzero_si128();
    volatile __m128i *p_m128 = reinterpret_cast<volatile __m128i *>(s);
    if (!s % 16) {
      while (n >= sizeof(__m128i)) {
        *p_m128++ = zero;
        n -= sizeof(__m128i);
      }
    }
    volatile size_t *p_sz = reinterpret_cast<volatile size_t *>(p_m128);
#else
    volatile size_t *p_sz = reinterpret_cast<volatile size_t *>(s);
    while (n >= sizeof(size_t) * 8) {
      *p_sz++ = 0;
      *p_sz++ = 0;
      *p_sz++ = 0;
      *p_sz++ = 0;
      n -= sizeof(size_t) * 4;
    }
#endif
    details::secure_zero_no_vector(p_sz, n);
  }

  inline bool is_zero(void *v, size_t sz)
  {
    auto ptr = reinterpret_cast<uint8_t *>(v);
    const auto end = ptr + sz;
    return ::std::all_of(ptr, end, [](auto &c) { return c == 0; });
  }

  inline void put_unique_seeded_random(void *v, size_t sz)
  {
    if (sz % sizeof(uint32_t))
      throw ::std::runtime_error("Put unique seeded random size must be divisible by  sizeof(uint32_t)");
    sz /= sizeof(uint32_t);
    std::mt19937 mt;
    auto ptr = reinterpret_cast<uint32_t *>(v);
    const auto end = ptr + sz;
    while (ptr != end) {
      *ptr++ = static_cast<uint32_t>(mt());
    }
  }
  inline bool is_unique_seeded_random(void *v, size_t sz)
  {
    if (sz % sizeof(uint32_t))
      throw ::std::runtime_error("Put unique seeded random size must be divisible by  sizeof(uint32_t)");
    sz /= sizeof(uint32_t);
    std::mt19937 mt;
    auto ptr = reinterpret_cast<uint32_t *>(v);
    const auto end = ptr + sz;
    while (ptr != end) {
      if (*ptr++ != static_cast<uint32_t>(mt()))
        return false;
    }
    return true;
  }
  inline size_t is_unique_seeded_random_failure_loc(void *v, size_t sz)
  {
    if (sz % sizeof(uint32_t))
      throw ::std::runtime_error("Put unique seeded random size must be divisible by  sizeof(uint32_t)");
    sz /= sizeof(uint32_t);
    std::mt19937 mt;
    auto ptr = reinterpret_cast<uint32_t *>(v);
    const auto begin = ptr;
    const auto end = ptr + sz;
    while (ptr != end) {
      if (*ptr++ != static_cast<uint32_t>(mt())) {
        --ptr;
        return sizeof(uint32_t) * static_cast<size_t>((ptr - begin));
      }
    }
    return ::std::numeric_limits<size_t>::max();
  }

  /**
   * \brief Secure zero a pointer (the address, not the pointed to object).
   **/
  template <typename T>
  inline void secure_zero_pointer(T *&s)
  {
    secure_zero(&s, sizeof(s));
  }
}
