#pragma once
#include <assert.h>
#include <cgc1/declarations.hpp>
#include <mcppalloc/mcppalloc_utils/make_unique.hpp>
#include <memory>
#include <vector>
namespace cgc1
{
  namespace details
  {
    extern void *internal_allocate(size_t n);
    extern void internal_deallocate(void *p);
    extern void *internal_slab_allocate(size_t n);
    extern void internal_slab_deallocate(void *p);
  }
  template <class T>
  class cgc_internal_allocator_t;
  template <>
  class cgc_internal_allocator_t<void>
  {
  public:
    using other = cgc_internal_allocator_t<void>;
    using value_type = void;
    using pointer = void *;
    using const_pointer = const void *;
    using propogate_on_container_move_assignment = ::std::true_type;
    template <class U>
    struct rebind {
      typedef cgc_internal_allocator_t<U> other;
    };
    cgc_internal_allocator_t() = default;
    template <class _Other>
    cgc_internal_allocator_t(const cgc_internal_allocator_t<_Other> &) noexcept
    {
    }
    template <class _Other>
    cgc_internal_allocator_t<void> &operator=(const cgc_internal_allocator_t<_Other> &) noexcept
    {
      return *this;
    }
    cgc_internal_allocator_t(cgc_internal_allocator_t<void> &&) noexcept = default;
  };
  using cgc_internal_allocator_generic_t = cgc_internal_allocator_t<void>;
  template <class T>
  class cgc_internal_allocator_t
  {
  public:
    using other = cgc_internal_allocator_t<T>;
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = ::std::size_t;
    using difference_type = ::std::ptrdiff_t;
    using propogate_on_container_move_assignment = ::std::true_type;
    cgc_internal_allocator_t() noexcept = default;
    cgc_internal_allocator_t(const cgc_internal_allocator_t<T> &) noexcept = default;
    template <class _Other>
    cgc_internal_allocator_t(const cgc_internal_allocator_t<_Other> &) noexcept
    {
    }
    template <class Other>
    cgc_internal_allocator_t<T> &operator=(const cgc_internal_allocator_t<Other> &) noexcept
    {
      return *this;
    }
    cgc_internal_allocator_t<T> &operator=(const cgc_internal_allocator_t<T> &) noexcept
    {
      return *this;
    }
    cgc_internal_allocator_t(cgc_internal_allocator_t<T> &&) noexcept = default;
    cgc_internal_allocator_t &operator=(cgc_internal_allocator_t<T> &&) noexcept = default;
    bool operator==(cgc_internal_allocator_t) noexcept
    {
      return true;
    }
    bool operator!=(cgc_internal_allocator_t) noexcept
    {
      return false;
    }
    template <class U>
    struct rebind {
      typedef cgc_internal_allocator_t<U> other;
    };

    pointer address(reference x) const
    {
      return ::std::addressof(x);
    }
    const_pointer address(const_reference x) const
    {
      return ::std::addressof(x);
    }
    pointer allocate(size_type n, cgc_internal_allocator_t<void>::const_pointer = nullptr);
    void deallocate(pointer p, size_type);
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
  template <class T>
  class cgc_internal_slab_allocator_t;
  template <>
  class cgc_internal_slab_allocator_t<void>
  {
  public:
    using other = cgc_internal_slab_allocator_t<void>;
    using value_type = void;
    using pointer = void *;
    using const_pointer = const void *;
    using propogate_on_container_move_assignment = ::std::true_type;
    template <class U>
    struct rebind {
      typedef cgc_internal_slab_allocator_t<U> other;
    };
    cgc_internal_slab_allocator_t() = default;
    template <class _Other>
    cgc_internal_slab_allocator_t(const cgc_internal_slab_allocator_t<_Other> &) noexcept
    {
    }
    template <class _Other>
    cgc_internal_slab_allocator_t<void> &operator=(const cgc_internal_slab_allocator_t<_Other> &) noexcept
    {
      return *this;
    }
    cgc_internal_slab_allocator_t(cgc_internal_slab_allocator_t<void> &&) noexcept = default;
  };
  template <class T>
  class cgc_internal_slab_allocator_t
  {
  public:
    using other = cgc_internal_slab_allocator_t<T>;
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = ::std::size_t;
    using difference_type = ::std::ptrdiff_t;
    using propogate_on_container_move_assignment = ::std::true_type;
    cgc_internal_slab_allocator_t() noexcept = default;
    cgc_internal_slab_allocator_t(const cgc_internal_slab_allocator_t<T> &) noexcept = default;
    template <class _Other>
    cgc_internal_slab_allocator_t(const cgc_internal_slab_allocator_t<_Other> &) noexcept
    {
    }
    template <class _Other>
    cgc_internal_slab_allocator_t<void> &operator=(const cgc_internal_slab_allocator_t<_Other> &) noexcept
    {
      return *this;
    }
    cgc_internal_slab_allocator_t(cgc_internal_slab_allocator_t<T> &&) noexcept = default;
    cgc_internal_slab_allocator_t &operator=(cgc_internal_slab_allocator_t<T> &&) noexcept = default;
    bool operator==(cgc_internal_slab_allocator_t) noexcept
    {
      return true;
    }
    bool operator!=(cgc_internal_slab_allocator_t) noexcept
    {
      return false;
    }
    template <class U>
    struct rebind {
      typedef cgc_internal_slab_allocator_t<U> other;
    };

    pointer address(reference x) const
    {
      return ::std::addressof(x);
    }
    const_pointer address(const_reference x) const
    {
      return ::std::addressof(x);
    }
    pointer allocate(size_type n, cgc_internal_slab_allocator_t<void>::const_pointer = nullptr);
    void deallocate(pointer p, size_type);
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
  struct cgc_internal_deleter_t {
    template <typename T>
    void operator()(T *t) const
    {
      cgc_internal_allocator_t<T> allocator;
      allocator.destroy(t);
      allocator.deallocate(t, 1);
    }
  };
  static_assert(::std::is_nothrow_move_assignable<cgc_internal_deleter_t>::value, "");
  struct cgc_internal_slab_deleter_t {
    template <typename T>
    void operator()(T *t) const
    {
      cgc_internal_slab_allocator_t<T> allocator;
      allocator.destroy(t);
      allocator.deallocate(t, 1);
    }
  };
}
namespace mcppalloc
{
  template <typename T>
  struct cgc_allocator_deleter_t<T, ::cgc1::cgc_internal_allocator_t<void>> {
    using type = ::cgc1::cgc_internal_deleter_t;
  };
  template <typename T>
  struct cgc_allocator_deleter_t<T, ::cgc1::cgc_internal_slab_allocator_t<void>> {
    using type = ::cgc1::cgc_internal_slab_deleter_t;
  };
}
namespace cgc1
{
  template <typename T>
  using cgc_internal_unique_ptr_t = ::std::unique_ptr<T, cgc_internal_deleter_t>;
  template <typename T>
  using cgc_internal_vector_t = typename ::std::vector<T, cgc_internal_allocator_t<T>>;
  template <class T>
  auto cgc_internal_allocator_t<T>::allocate(size_type n, cgc_internal_allocator_t<void>::const_pointer) -> pointer
  {
    return reinterpret_cast<pointer>(details::internal_allocate(n * std::max(alignof(T), sizeof(T))));
  }
  template <class T>
  void cgc_internal_allocator_t<T>::deallocate(pointer p, size_type)
  {
    details::internal_deallocate(p);
  }
  template <class T>
  auto cgc_internal_slab_allocator_t<T>::allocate(size_type n, cgc_internal_slab_allocator_t<void>::const_pointer) -> pointer
  {
    return reinterpret_cast<pointer>(details::internal_slab_allocate(n * std::max(alignof(T), sizeof(T))));
  }
  template <class T>
  void cgc_internal_slab_allocator_t<T>::deallocate(pointer p, size_type)
  {
    details::internal_slab_deallocate(p);
  }
  static_assert(::std::is_same<cgc_internal_deleter_t,
                               ::mcppalloc::cgc_allocator_deleter_t<int, cgc_internal_allocator_t<void>>::type>::value,
                "Error with internal deleter");
  static_assert(::std::is_same<cgc_internal_slab_deleter_t,
                               ::mcppalloc::cgc_allocator_deleter_t<int, cgc_internal_slab_allocator_t<void>>::type>::value,
                "Error with internal deleter");
}
#include <mcppalloc/mcppalloc_utils/condition_variable.hpp>
namespace cgc1
{
  using condition_variable_any_t = internal_condition_variable_t<cgc_internal_allocator_t<void>>;
}
