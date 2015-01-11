#pragma once
#include "declarations.hpp"
#include <assert.h>
#include <memory>
#include <vector>
namespace cgc1
{
  extern bool in_signal_handler();
  template <class T>
  class cgc_internal_malloc_allocator_t;
  template <>
  class cgc_internal_malloc_allocator_t<void>
  {
  public:
    using other = cgc_internal_malloc_allocator_t<void>;
    using value_type = void;
    using pointer = void *;
    using const_pointer = const void *;
    using propogate_on_container_move_assignment = ::std::true_type;
    template <class U>
    struct rebind {
      typedef cgc_internal_malloc_allocator_t<U> other;
    };
    cgc_internal_malloc_allocator_t() = default;
    template <class _Other>
    cgc_internal_malloc_allocator_t(const cgc_internal_malloc_allocator_t<_Other> &) noexcept
    {
    }
    template <class _Other>
    cgc_internal_malloc_allocator_t<void> &operator=(const cgc_internal_malloc_allocator_t<_Other> &) noexcept
    {
      return *this;
    }
    cgc_internal_malloc_allocator_t(cgc_internal_malloc_allocator_t<void> &&) noexcept = default;
  };
  /**
  Allocator for use with standard library that uses malloc internally.
  **/
  using cgc_internal_malloc_allocator_generic_t = cgc_internal_malloc_allocator_t<void>;
  template <class T>
  class cgc_internal_malloc_allocator_t
  {
  public:
    using other = cgc_internal_malloc_allocator_t<T>;
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = ::std::size_t;
    using difference_type = ::std::ptrdiff_t;
    using propogate_on_container_move_assignment = ::std::true_type;
    cgc_internal_malloc_allocator_t() noexcept = default;
    cgc_internal_malloc_allocator_t(const cgc_internal_malloc_allocator_t<T> &) noexcept = default;
    template <class _Other>
    cgc_internal_malloc_allocator_t(const cgc_internal_malloc_allocator_t<_Other> &) noexcept
    {
    }
    template <class _Other>
    cgc_internal_malloc_allocator_t<void> &operator=(const cgc_internal_malloc_allocator_t<_Other> &) noexcept
    {
      return *this;
    }
    cgc_internal_malloc_allocator_t(cgc_internal_malloc_allocator_t<T> &&) noexcept = default;
    cgc_internal_malloc_allocator_t &operator=(cgc_internal_malloc_allocator_t<T> &&) noexcept = default;
    bool operator==(cgc_internal_malloc_allocator_t) noexcept
    {
      return true;
    }
    bool operator!=(cgc_internal_malloc_allocator_t) noexcept
    {
      return false;
    }
    template <class U>
    struct rebind {
      typedef cgc_internal_malloc_allocator_t<U> other;
    };

    pointer address(reference x) const
    {
      return ::std::addressof(x);
    }
    const_pointer address(const_reference x) const
    {
      return ::std::addressof(x);
    }
    pointer allocate(size_type n, cgc_internal_malloc_allocator_t<void>::const_pointer = nullptr)
    {
      assert(!in_signal_handler());
      return reinterpret_cast<pointer>(::malloc(n * sizeof(T)));
    }
    void deallocate(pointer p, size_type)
    {
      assert(!in_signal_handler());
      ::free(p);
    }
    size_type max_size() const noexcept
    {
      return ::std::numeric_limits<size_type>::max();
    }
    template <class U, class... Args>
    void construct(U *p, Args &&... args)
    {
      ::new (static_cast<void *>(p)) U(::std::forward<Args>(args)...);
    }
    template <class U>
    void destroy(U *p)
    {
      p->~U();
    }
  };
  /**
  Deleter for malloc allocator.
  Use for unique/shared ptrs.
  **/
  struct cgc_internal_malloc_deleter_t {
    template <typename T>
    void operator()(T *t)
    {
      cgc_internal_malloc_allocator_t<T> allocator;
      allocator.destroy(t);
      allocator.deallocate(t, 1);
    }
  };
  /**
  Tag for dispatch of getting deleter.
  **/
  template <typename T>
  struct cgc_allocator_deleter_t<T, cgc_internal_malloc_allocator_t<void>> {
    using type = cgc_internal_malloc_deleter_t;
  };
}
