#pragma once
namespace cgc1
{
  /**
   * \brief Allocator that returns memory that is aligned to Alignment.
   * @tparam T Type to allocate.
   * @tparam Alignment alignment to request.
   **/
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
    /**
     * \brief Class whose member other is a typedef of allocator for type Type.
     * @tparam Type for allocator.
     **/
    template <class Type>
    struct rebind {
      typedef aligned_allocator_t<Type, alignment> other;
    };
    aligned_allocator_t() = default;
    aligned_allocator_t(const aligned_allocator_t<void, Alignment> &) noexcept = default;
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
    aligned_allocator_t(aligned_allocator_t<T, Alignment> &&) noexcept = default;
    template <class _Other>
    aligned_allocator_t<T, alignment> &operator=(const aligned_allocator_t<_Other, Alignment> &) noexcept
    {
      return *this;
    }
    aligned_allocator_t<T, alignment> &operator=(const aligned_allocator_t<T, Alignment> &) noexcept = default;
    aligned_allocator_t &operator=(aligned_allocator_t<T, Alignment> &&) noexcept = default;
    /**
     * \brief Operator==
     * All aligned allocators of same size are equal.
     **/
    bool operator==(aligned_allocator_t) noexcept
    {
      return true;
    }
    /**
     * \brief Operator!=
     * All aligned allocators of same size are equal.
     **/
    bool operator!=(aligned_allocator_t) noexcept
    {
      return false;
    }
    /**
     * \brief Class whose member other is a typedef of allocator for type Type.
     * @tparam Type for allocator.
     **/
    template <class Type>
    struct rebind {
      typedef aligned_allocator_t<Type, alignment> other;
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
    /**
     * \brief Release block of storage
     **/
    void deallocate(pointer p, size_type)
    {
#ifdef _WIN32
      ::_aligned_free(p);
#else
      free(p);
#endif
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
   * \brief Deleter for aligned allocator.
   * Use for unique/shared ptrs.
   * \tparam Alignment alignment of allocator that allocated memory.
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
   * \brief Tag for dispatch of getting deleter.
   * \tparam Type type to delete.
   * \tparam Alignment Alignment of allocator for type.
  **/
  template <typename Type, size_t Alignment>
  struct cgc_allocator_deleter_t<Type, aligned_allocator_t<void, Alignment>> {
    using type = aligned_deleter_t<Alignment>;
  };
}
