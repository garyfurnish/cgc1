#pragma once
namespace cgc1
{
  template <class T, size_t Alignment>
  class aligned_allocator_t;
  template <size_t Alignment>
  class aligned_allocator_t<void, Alignment>
  {
  public:
    static constexpr size_t alignment = Alignment;
    using other = aligned_allocator_t<void, alignment>;
    using value_type = void;
    using pointer = void *;
    using const_pointer = const void *;
    using propogate_on_container_move_assignment = ::std::true_type;
    template <class U>
    struct rebind {
      typedef aligned_allocator_t<U, alignment> other;
    };
    aligned_allocator_t() = default;
    template <class _Other>
    aligned_allocator_t(const aligned_allocator_t<_Other, alignment> &) noexcept
    {
    }
    template <class _Other>
    aligned_allocator_t<void, alignment> &operator=(const aligned_allocator_t<_Other, alignment> &) noexcept
    {
      return *this;
    }
    aligned_allocator_t(aligned_allocator_t<void, Alignment> &&) noexcept = default;
  };
  template <size_t Alignment>
  using aligned_allocator_generic_t = aligned_allocator_t<void, Alignment>;
  using default_aligned_allocator_t = aligned_allocator_generic_t<8>;
  template <class T, size_t Alignment>
  class aligned_allocator_t
  {
  public:
    static constexpr size_t alignment = Alignment;
    using other = aligned_allocator_t<T, alignment>;
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = ::std::size_t;
    using difference_type = ::std::ptrdiff_t;
    using propogate_on_container_move_assignment = ::std::true_type;
    aligned_allocator_t() noexcept = default;
    aligned_allocator_t(const aligned_allocator_t<T, Alignment> &) noexcept = default;
    template <class _Other>
    aligned_allocator_t(const aligned_allocator_t<_Other, Alignment> &) noexcept
    {
    }
    template <class _Other>
    aligned_allocator_t<T, alignment> &operator=(const aligned_allocator_t<_Other, Alignment> &) noexcept
    {
      return *this;
    }
    aligned_allocator_t<T, alignment> &operator=(const aligned_allocator_t<T, Alignment> &) noexcept
    {
      return *this;
    }
    aligned_allocator_t(aligned_allocator_t<T, Alignment> &&) noexcept = default;
    aligned_allocator_t &operator=(aligned_allocator_t<T, Alignment> &&) noexcept = default;
    bool operator==(aligned_allocator_t) noexcept
    {
      return true;
    }
    bool operator!=(aligned_allocator_t) noexcept
    {
      return false;
    }
    template <class U>
    struct rebind {
      typedef aligned_allocator_t<U, alignment> other;
    };

    pointer address(reference x) const
    {
      return ::std::addressof(x);
    }
    const_pointer address(const_reference x) const
    {
      return ::std::addressof(x);
    }
    pointer allocate(size_type n, typename aligned_allocator_t<void, alignment>::const_pointer = nullptr)
    {
#ifdef _WIN32
      return reinterpret_cast<pointer>(::_aligned_malloc(n * sizeof(T), alignment));
#elif defined(__APPLE__)
      static_assert(alignment <= 16, "On OSX, alignment must be <=16.");
      return reinterpret_cast<pointer>(::malloc(n * sizeof(T)));
#else
      return reinterpret_cast<pointer>(::aligned_alloc(alignment, n * sizeof(T)));
#endif
    }
    void deallocate(pointer p, size_type)
    {
#ifdef _WIN32
      ::_aligned_free(p);
#else
      free(p);
#endif
    }
    size_type max_size() const noexcept
    {
      return ::std::numeric_limits<size_type>::max();
    }
    template <class U, class... Args>
    void construct(U *p, Args &&... args)
    {
      ::new ((void *)p) U(::std::forward<Args>(args)...);
    }
    template <class U>
    void destroy(U *p)
    {
      p->~U();
    }
  };
  /**
  Deleter for aligned allocator.
  Use for unique/shared ptrs.
  **/
  template <size_t Alignment>
  struct aligned_deleter_t {
    template <typename T>
    void operator()(T *t)
    {
      aligned_allocator_t<T, Alignment> allocator;
      allocator.destroy(t);
      allocator.deallocate(t, 1);
    }
  };
  /**
  Tag for dispatch of getting deleter.
  **/
  template <typename T, size_t Alignment>
  struct cgc_allocator_deleter_t<T, aligned_allocator_t<void, Alignment>> {
    using type = aligned_deleter_t<Alignment>;
  };
}
