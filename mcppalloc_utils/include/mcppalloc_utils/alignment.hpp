#pragma once
namespace mcppalloc
{
  /**
   * \brief Align size to alignment.
   **/
  inline constexpr size_t align(size_t sz, size_t alignment)
  {
    return ((sz + alignment - 1) / alignment) * alignment;
  }
  /**
   * \brief Align pointer to alignment.
  **/
  inline void *align(void *iptr, size_t alignment)
  {
    return reinterpret_cast<void *>(align(reinterpret_cast<size_t>(iptr), alignment));
  }
  /**
   * \brief Align size to a given power of 2.
  **/
  inline constexpr size_t align_pow2(size_t size, size_t align_pow)
  {
    return ((size + (static_cast<size_t>(1) << align_pow) - 1) >> align_pow) << align_pow;
  }
  /**
   * \brief Align pointer to a given power of 2.
  **/
  template <typename T>
  inline constexpr T *align_pow2(T *iptr, size_t align_pow)
  {
    return reinterpret_cast<T *>(align_pow2(reinterpret_cast<size_t>(iptr), align_pow));
  }
}
