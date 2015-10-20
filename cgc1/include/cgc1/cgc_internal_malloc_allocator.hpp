#pragma once
#include "declarations.hpp"
#include <assert.h>
#include <memory>
#include <vector>
namespace cgc1
{
  /**
   * \brief Return true if called from inside signal handler registered by CGC1, false otherwise.
   **/
  extern bool in_signal_handler() noexcept;
  /**
   * \brief Allocator for standard library that uses cgc's internal allocation mechanisms.
   * This typically corresponds to an allocator over some slab allocator.
   * @tparam T Type to allocate.
   **/
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
    /**
     * \brief Class whose member other is a typedef of allocator for type Type.
     * @tparam Type for allocator.
     **/
    template <class Type>
    struct rebind {
      typedef cgc_internal_malloc_allocator_t<Type> other;
    };
    cgc_internal_malloc_allocator_t() = default;
    cgc_internal_malloc_allocator_t(const cgc_internal_malloc_allocator_t &) noexcept = default;
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
    cgc_internal_malloc_allocator_t &operator=(const cgc_internal_malloc_allocator_t &) noexcept = default;
    cgc_internal_malloc_allocator_t &operator=(cgc_internal_malloc_allocator_t &&) noexcept = default;
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
    /**
     * \brief Operator==
     * All allocators of same type are equal.
     **/
    bool operator==(cgc_internal_malloc_allocator_t) noexcept
    {
      return true;
    }
    /**
     * \brief Operator!=
     * All allocators of same type are equal.
     **/
    bool operator!=(cgc_internal_malloc_allocator_t) noexcept
    {
      return false;
    }
    /**
     * \brief Class whose member other is a typedef of allocator for type Type.
     * @tparam Type for allocator.
     **/
    template <class Type>
    struct rebind {
      typedef cgc_internal_malloc_allocator_t<Type> other;
    };
    /**
     * \brief Return address of reference.
     **/
    pointer address(reference x) const
    {
      return ::std::addressof(x);
    }
    /**
     * \brief Return address of reference.
     **/
    const_pointer address(const_reference x) const
    {
      return ::std::addressof(x);
    }
    /**
     * \brief Allocate block of storage
     **/
    pointer allocate(size_type n, cgc_internal_malloc_allocator_t<void>::const_pointer = nullptr)
    {
      //      assert(!in_signal_handler());
      return reinterpret_cast<pointer>(::malloc(n * sizeof(T)));
    }
    /**
     * \brief Release block of storage
     **/
    void deallocate(pointer p, size_type)
    {
      assert(!in_signal_handler());
      ::free(p);
    }
    /**
     * \brief Maximum size possible to allocate
     **/
    size_type max_size() const noexcept
    {
      return ::std::numeric_limits<size_type>::max();
    }
    /**
     * \brief Construct an object
     *
     **/
    template <class... Args>
    void construct(pointer p, Args &&... args)
    {
      ::new (static_cast<void *>(p)) value_type(::std::forward<Args>(args)...);
    }
    /**
     * \brief Destroy an object.
     **/
    void destroy(pointer p)
    {
      p->~value_type();
    }
  };
  /**
   * \brief Deleter for malloc allocator.
   * Use for unique/shared ptrs.
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
  template <typename T>
  using unique_ptr_malloc_t = ::std::unique_ptr<T, cgc_internal_malloc_deleter_t>;
  template <typename T, typename... Args>
  auto make_unique_malloc(Args &&... args) -> unique_ptr_malloc_t<T>
  {
    cgc_internal_malloc_allocator_t<T> allocator;
    auto ptr = allocator.allocate(1);
    allocator.construct(ptr, ::std::forward<Args>(args)...);
    return ::std::unique_ptr<T, cgc_internal_malloc_deleter_t>(ptr);
  }
  /**
   * Tag for dispatch of getting deleter.
  **/
  template <typename T>
  struct cgc_allocator_deleter_t<T, cgc_internal_malloc_allocator_t<void>> {
    using type = cgc_internal_malloc_deleter_t;
  };
}
