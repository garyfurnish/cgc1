#pragma once
#ifdef _WIN32
#define _ITERATOR_DEBUG_LEVEL 0
#endif
#include <stdlib.h>
#include <stdint.h>
#include <cstddef>
#include <algorithm>
#include <memory>
#include <assert.h>
#ifndef CGC1_NO_INLINES
#define CGC1_INLINES
#define CGC1_OPT_INLINE inline
#else
#define CGC1_OPT_INLINE
#endif
#ifndef _WIN32
#define CGC1_POSIX
#define cgc1_builtin_prefetch(ADDR) __builtin_prefetch(ADDR)
#define cgc1_builtin_clz1(X) __builtin_clzl(X)
#define cgc1_builtin_current_stack(...) __builtin_frame_address(0)
#define _NoInline_ __attribute__((noinline))
#else
#define cgc1_builtin_prefetch(ADDR) _m_prefetch(ADDR)
#define cgc1_builtin_clz1(X) (63 - __lzcnt64(X))
#define cgc1_builtin_current_stack() _AddressOfReturnAddress()
// spurious error generation in nov ctp.
#pragma warning(disable : 4592)
#pragma warning(disable : 4100)
#define _NoInline_ __declspec(noinline)
#endif
namespace cgc1
{
  /**
  Insert val into a sorted container if val would be unique.
  **/
  template <typename Container, typename T, typename Compare>
  bool insert_unique_sorted(Container &c, const T &val, Compare comp)
  {
    if (c.empty()) {
      c.push_back(val);
      return true;
    }
    auto lb = ::std::lower_bound(c.begin(), c.end(), val, comp);
    while (lb != c.end() && !comp(val, *lb)) {
      if (*lb == val)
        return false;
      lb++;
    }
    c.insert(lb, val);
    return true;
  }
  /**
  Align size to alignment.
  **/
  inline constexpr size_t align(size_t sz, size_t alignment)
  {
    return ((sz + alignment + 1) / alignment) * alignment;
  }
  /**
  Align pointer to alignment.
  **/
  inline const void *align(void *iptr, size_t alignment)
  {
    return reinterpret_cast<void *>(align(reinterpret_cast<size_t>(iptr), alignment));
  }
  /**
  Align size to a given power of 2.
  **/
  inline constexpr size_t align_pow2(size_t size, size_t align_pow)
  {
    return ((size + (((size_t)1) << align_pow) - 1) >> align_pow) << align_pow;
  }
  /**
  Align pointer to a given power of 2.
  **/
  template <typename T>
  inline constexpr T *align_pow2(T *iptr, size_t align_pow)
  {
    return reinterpret_cast<T *>(align_pow2(reinterpret_cast<size_t>(iptr), align_pow));
  }
  /**
  Forward iterator for type T that takes a functional that defines how to advance the iterator.
  The rule is that Current = Advance(Current).
  **/
  template <typename T, typename Advance>
  class functional_iterator_t
  {
  public:
    template <typename In_Advance>
    functional_iterator_t(T *t, In_Advance &&advance)
        : m_t(t), m_advance(::std::forward<In_Advance>(advance))
    {
    }
    functional_iterator_t(const functional_iterator_t<T, Advance> &) = default;
    functional_iterator_t(functional_iterator_t<T, Advance> &&) = default;
    functional_iterator_t<T, Advance> &operator=(const functional_iterator_t &) = default;
    functional_iterator_t<T, Advance> &operator=(functional_iterator_t &&) = default;
    T &operator*() const
    {
      return *m_t;
    }
    T *operator->() const
    {
      return m_t;
    }
    operator T *() const
    {
      return m_t;
    }
    functional_iterator_t<T, Advance> &operator++(int)
    {
      auto ret = functional_iterator_t<T, Advance>(m_t, m_advance);
      m_t = m_advance(m_t);
      return ret;
    }
    functional_iterator_t<T, Advance> &operator++()
    {
      m_t = m_advance(m_t);
      return *this;
    }
    bool operator==(const functional_iterator_t<T, Advance> &it) const
    {
      return m_t == it.m_t;
    }
    bool operator!=(const functional_iterator_t<T, Advance> &it) const
    {
      return m_t != it.m_t;
    }

  public:
    T *m_t;
    Advance m_advance;
  };
  /**
  Function to make functional iterators that uses type deduction to avoid end user templates.
  **/
  template <typename T, typename Advance>
  auto make_functional_iterator(T *t, Advance &&advance) -> functional_iterator_t<T, Advance>
  {
    return functional_iterator_t<T, Advance>(t, ::std::forward<Advance>(advance));
  }
  struct iterator_next_advancer_t {
    template <typename T>
    auto operator()(T t) -> decltype(t -> next())
    {
      return t->next();
    }
  };
  template <typename T>
  using next_iterator = functional_iterator_t<T, iterator_next_advancer_t>;
  /**
  Function to make next iterators that uses type deduction to avoid end user templates.
  **/
  template <typename T>
  auto make_next_iterator(T *t) -> next_iterator<T>
  {
    return make_functional_iterator(t, iterator_next_advancer_t{});
  }
  /**
  Alignment on this system in bytes.
  **/
  static constexpr size_t c_alignment = 16;
  /**
  Power of two to align to on this system.
  **/
  static constexpr size_t c_align_pow2 = 4;
  static_assert((1 << c_align_pow2) == c_alignment, "Alignment not valid.");
  inline constexpr size_t align(size_t size)
  {
    return ((size + c_alignment - 1) >> c_align_pow2) << c_align_pow2;
  }
  /**
  Tag based dispatch for finding a deleter for a given allocator.
  **/
  template <typename T, typename Allocator>
  struct cgc_allocator_deleter_t {
    using type = cgc_allocator_deleter_t<T, typename Allocator::template rebind<void>::other>;
  };
  template <typename T>
  struct cgc_allocator_deleter_t<T, ::std::allocator<void>> {
    using type = ::std::default_delete<T>;
  };
  template <class Iterator>
  ::std::reverse_iterator<Iterator> make_reverse_iterator(Iterator i)
  {
    return ::std::reverse_iterator<Iterator>(i);
  }
  template <typename Iterator, typename Val>
  void insert_replace(Iterator replace, Iterator new_location, Val &&val)
  {
    assert(replace <= new_location);
    ::std::rotate(replace, replace + 1, new_location + 1);
    *new_location = ::std::forward<Val>(val);
  }
#ifdef _DEBUG
#ifndef _CGC1_DEBUG_LEVEL
#define _CGC1_DEBUG_LEVEL 1
#endif
#endif
#ifndef _CGC1_DEBUG_LEVEL
#define _CGC1_DEBUG_LEVEL 0
#endif
  constexpr int c_debug_level = _CGC1_DEBUG_LEVEL;
  constexpr size_t size_inverse(size_t t)
  {
    return ~t;
  }
  inline size_t hide_pointer(void *v)
  {
    return ~reinterpret_cast<size_t>(v);
  }
  inline void *unhide_pointer(size_t sz)
  {
    return reinterpret_cast<void *>(~sz);
  }
  inline void secure_zero(void *s, size_t n)
  {
    volatile char *p = reinterpret_cast<volatile char *>(s);

    while (n--)
      *p++ = 0;
  }
  template <typename T>
  inline void secure_zero_pointer(T *&s)
  {
    secure_zero(&s, sizeof(s));
  }
}
