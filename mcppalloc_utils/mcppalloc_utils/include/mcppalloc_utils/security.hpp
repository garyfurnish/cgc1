#pragma once
namespace mcppalloc
{
  /**
   * \brief Securely zero a pointer, guarenteed to not be optimized out.
   **/
  inline void secure_zero(void *s, size_t n)
  {
    volatile size_t *p_sz = reinterpret_cast<volatile size_t *>(s);
    while (n >= sizeof(size_t) * 8) {
      *p_sz++ = 0;
      *p_sz++ = 0;
      *p_sz++ = 0;
      *p_sz++ = 0;
      *p_sz++ = 0;
      *p_sz++ = 0;
      *p_sz++ = 0;
      *p_sz++ = 0;
      n -= sizeof(size_t) * 8;
    }
    while (n >= sizeof(size_t)) {
      *p_sz++ = 0;
      n -= sizeof(size_t);
    }
    volatile char *p = reinterpret_cast<volatile char *>(p_sz);

    while (n--)
      *p++ = 0;
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
