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
#include <tuple>
#ifndef CGC1_NO_INLINES
#define CGC1_INLINES
#define CGC1_OPT_INLINE inline
#define CGC1_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define CGC1_OPT_INLINE
#define CGC1_ALWAYS_INLINE
#endif
#ifndef _WIN32
#define CGC1_POSIX
#define cgc1_builtin_prefetch(ADDR) __builtin_prefetch(ADDR)
#define cgc1_builtin_clz1(X) __builtin_clzl(X)
#ifndef cgc1_builtin_current_stack
#define cgc1_builtin_current_stack(...) __builtin_frame_address(0)
#endif
#define _NoInline_ __attribute__((noinline))
#define likely(x) __builtin_expect(static_cast<bool>(x), 1)
#define unlikely(x) __builtin_expect(static_cast<bool>(x), 0)
#else
#define cgc1_builtin_prefetch(ADDR) _m_prefetch(ADDR)
#define cgc1_builtin_clz1(X) (63 - __lzcnt64(X))
#ifndef cgc1_builtin_current_stack
#define cgc1_builtin_current_stack() _AddressOfReturnAddress()
#endif
// spurious error generation in nov ctp.
#pragma warning(disable : 4592)
#pragma warning(disable : 4100)
#define _NoInline_ __declspec(noinline)
#endif
namespace cgc1
{
  namespace literals
  {
    constexpr inline ::std::size_t operator"" _sz(unsigned long long x)
    {
      return static_cast<::std::size_t>(x);
    }
  }
  /**
   * \brief Insert val into a sorted container if val would be unique.
  **/
  template <typename Container, typename T, typename Compare>
  bool insert_unique_sorted(Container &c, const T &val, Compare comp)
  {
    // if empty container, just add it!
    if (c.empty()) {
      c.push_back(val);
      return true;
    }
    // otherwise find an iterator to insert before.
    // TODO: THIS deserves further testing.
    // Changing this code doesn't seem to do anything.
    auto lb = ::std::lower_bound(c.begin(), c.end(), val, comp);
    auto origlb = lb;
    while (lb != c.end() && comp(*lb, val)) {
      if (*lb == val)
        return false;
      lb++;
    }
    // insert before lb.
    c.insert(origlb, val);
    return true;
  }
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
  /**
   * \brief Forward iterator for type T that takes a functional that defines how to advance the iterator.
   *
   * The rule is that Current = Advance(Current).
  **/
  template <typename T, typename Advance>
  class functional_iterator_t
  {
  public:
    template <typename In_Advance>
    /**
     * \brief Constructor.
     * @param ptr Start object.
     * @param advance Functional used to advance to next object.
     **/
    functional_iterator_t(T *t, In_Advance &&advance) noexcept : m_t(t), m_advance(::std::forward<In_Advance>(advance))
    {
    }
    functional_iterator_t(const functional_iterator_t<T, Advance> &) noexcept = default;
    functional_iterator_t(functional_iterator_t<T, Advance> &&) noexcept = default;
    functional_iterator_t<T, Advance> &operator=(const functional_iterator_t &) noexcept = default;
    functional_iterator_t<T, Advance> &operator=(functional_iterator_t &&) noexcept = default;
    T &operator*() const noexcept
    {
      return *m_t;
    }
    T *operator->() const noexcept
    {
      return m_t;
    }
    operator T *() const noexcept
    {
      return m_t;
    }
    functional_iterator_t<T, Advance> &operator++(int) noexcept
    {
      auto ret = functional_iterator_t<T, Advance>(m_t, m_advance);
      m_t = m_advance(m_t);
      return ret;
    }
    functional_iterator_t<T, Advance> &operator++() noexcept
    {
      m_t = m_advance(m_t);
      return *this;
    }
    bool operator==(const functional_iterator_t<T, Advance> &it) const noexcept
    {
      return m_t == it.m_t;
    }
    bool operator!=(const functional_iterator_t<T, Advance> &it) const noexcept
    {
      return m_t != it.m_t;
    }

  public:
    T *m_t;
    Advance m_advance;
  };
  /**
   * \brief Function to make functional iterators that uses type deduction to avoid end user templates.
  **/
  template <typename T, typename Advance>
  auto make_functional_iterator(T *t, Advance &&advance) noexcept -> functional_iterator_t<T, Advance>
  {
    return functional_iterator_t<T, Advance>(t, ::std::forward<Advance>(advance));
  }
  /**
   * \brief Functional that gets the next element from a type.
   **/
  struct iterator_next_advancer_t {
    template <typename T>
    auto operator()(T &&t) const noexcept -> decltype(t->next())
    {
      return ::std::forward<T>(t)->next();
    }
  };
  /**
   * \brief Iterator that works by obtaining the next element.
   **/
  template <typename T>
  using next_iterator = functional_iterator_t<T, iterator_next_advancer_t>;
  /**
   * \brief Function to make next iterators that uses type deduction to avoid end user templates.
  **/
  template <typename T>
  auto make_next_iterator(T *t) noexcept -> next_iterator<T>
  {
    return make_functional_iterator(t, iterator_next_advancer_t{});
  }
  /**
   * \brief Alignment on this system in bytes.
  **/
  static constexpr size_t c_alignment = 16;
  /**
   * \brief Power of two to align to on this system.
  **/
  static constexpr size_t c_align_pow2 = 4;
  static_assert((1 << c_align_pow2) == c_alignment, "Alignment not valid.");
  /**
   * \brief Align size to system alignment.
   **/
  inline constexpr size_t align(size_t size)
  {
    return ((size + c_alignment - 1) >> c_align_pow2) << c_align_pow2;
  }
  /**
   * \brief Tag based dispatch for finding a deleter for a given allocator.
   *
   * TODO: Needs more documentation.
  **/
  template <typename T, typename Allocator>
  struct cgc_allocator_deleter_t {
    using type = cgc_allocator_deleter_t<T, typename Allocator::template rebind<void>::other>;
  };
  template <typename T>
  struct cgc_allocator_deleter_t<T, ::std::allocator<void>> {
    using type = ::std::default_delete<T>;
  };
  template <typename T, typename Allocator>
  using allocator_unique_ptr = ::std::unique_ptr<T, typename cgc_allocator_deleter_t<T, Allocator>::type>;
  template <typename T, typename Allocator, typename... Ts>
  allocator_unique_ptr<T, Allocator> make_unique_allocator(Ts &&... ts)
  {
    typename Allocator::template rebind<T>::other allocator;
    auto ptr = allocator.allocate(1);
    // if T throws, we don't want to leak.
    try {
      allocator.construct(ptr, ::std::forward<Ts>(ts)...);
    } catch (...) {
      allocator.deallocate(ptr, 1);
      throw;
    }
    return ::std::unique_ptr<T, typename cgc_allocator_deleter_t<T, Allocator>::type>(ptr);
  }

  /**
   * \brief Make a reverse iterator from an iterator.
   **/
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
#ifndef NDEBUG
#define _DEBUG
#endif
#ifdef _DEBUG
#ifndef _CGC1_DEBUG_LEVEL
#define _CGC1_DEBUG_LEVEL 2
#define CGC1_DEBUG_VERBOSE_TRACK
#endif
#endif
#ifndef _CGC1_DEBUG_LEVEL
#define _CGC1_DEBUG_LEVEL 0
#endif
  /**
   * \brief Current debug level.
   **/
  static const constexpr int c_debug_level = _CGC1_DEBUG_LEVEL;
  /**
   * \brief Return inverse for size.
   **/
  __attribute__((always_inline)) inline constexpr size_t size_inverse(size_t t)
  {
    return ~t;
  }
  /**
   * \brief Hide a pointer from garbage collection in a unspecified way.
   **/
  __attribute__((always_inline)) inline uintptr_t hide_pointer(void *v)
  {
    return ~reinterpret_cast<size_t>(v);
  }
  /**
   * \brief Unide a pointer from garbage collection in a unspecified way.
   **/
  __attribute__((always_inline)) inline void *unhide_pointer(uintptr_t sz)
  {
    return reinterpret_cast<void *>(~sz);
  }
  /**
   * \brief Securely zero a pointer, guarenteed to not be optimized out.
   **/
  inline void secure_zero(void *s, size_t n)
  {
    volatile char *p = reinterpret_cast<volatile char *>(s);

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
  /**
   * \brief Unsafe cast of references.
   *
   * Contain any undefined behavior here here.
   * @tparam Type New type.
   **/
  template <typename Type, typename In>
  Type &unsafe_reference_cast(In &in)
  {
    union {
      Type *t;
      In *i;
    } u;
    u.i = &in;
    return *u.t;
  }
  /**
   * \brief Unsafe cast of pointers
   *
   * Contain any undefined behavior here here.
   * @tparam Type New type.
   **/
  template <typename Type, typename In>
  Type *unsafe_cast(In *in)
  {
    union {
      Type *t;
      In *i;
    } u;
    u.i = in;
    return u.t;
  }

  template <size_t bytes = 5000>
  extern void clean_stack(size_t, size_t, size_t, size_t, size_t);
  namespace details
  {
    /**
     * \brief Return 2^n.
    **/
    inline constexpr size_t pow2(int n)
    {
      return static_cast<size_t>(2) << (n - 1);
    }
  }
  template <typename Begin, typename End, typename Val, typename Comparator>
  auto last_greater_equal_than(Begin &&begin, End &&end, Val &&val, Comparator &&comparator)
  {
    if (begin == end)
      return end;
    const auto ub = ::std::upper_bound(::std::forward<Begin>(begin), ::std::forward<End>(end), ::std::forward<Val>(val),
                                       ::std::forward<Comparator>(comparator));
    const auto plb = ub - 1;
    if (ub == begin || comparator(*plb, val)) {
      return ub;
    } else {
      return plb;
    }
  }
}
namespace std
{
  template <typename F, typename Tuple, size_t... I>
  CGC1_ALWAYS_INLINE auto apply_impl(F &&f, Tuple &&t, integer_sequence<size_t, I...>)
      -> decltype(::std::forward<F>(f)(::std::get<I>(::std::forward<Tuple>(t))...))
  {
    return t, ::std::forward<F>(f)(::std::get<I>(::std::forward<Tuple>(t))...);
  }
  template <typename F, typename Tuple>
  CGC1_ALWAYS_INLINE auto apply(F &&f, Tuple &&t)
  {
    return apply_impl(::std::forward<F>(f), ::std::forward<Tuple>(t),
                      make_index_sequence<::std::tuple_size<typename ::std::decay<Tuple>::type>::value>());
  }
}
