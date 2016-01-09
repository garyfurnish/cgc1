#pragma once
#include "cgc1.hpp"
namespace cgc1
{
  template <typename T>
  class gc_allocator_t
  {
  public:
    using size_type = ::std::size_t;
    using difference_type = ::std::ptrdiff_t;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using value_type = T;
    using propagate_on_container_move_assignment = ::std::true_type;
    using is_always_equal = ::std::true_type;

    template <class Other>
    struct rebind {
      using other = gc_allocator_t<Other>;
    };

    gc_allocator_t() noexcept = default;
    gc_allocator_t(const gc_allocator_t &) noexcept = default;
    gc_allocator_t(gc_allocator_t &&) noexcept = default;
    gc_allocator_t &operator=(const gc_allocator_t &) noexcept = default;
    gc_allocator_t &operator=(gc_allocator_t &&) noexcept = default;
    ~gc_allocator_t() = default;

    auto address(reference ref) const noexcept -> pointer
    {
      return ::std::addressof(ref);
    }
    auto address(const_reference ref) const noexcept -> const_pointer
    {
      return ::std::addressof(ref);
    }

    auto allocate(size_type n, const void * = 0) -> pointer
    {
      return reinterpret_cast<pointer>(cgc_malloc(n * sizeof(T)));
    }

    void deallocate(pointer p, size_type)
    {
      cgc_free(p);
    }

    auto max_size() const noexcept -> size_type
    {
      return ::std::numeric_limits<size_type>::max();
    }

    void construct(pointer p, const T &val)
    {
      new (p) T(val);
    }
    template <typename... Args>
    void construct(pointer p, Args &&... args)
    {
      new (p) T(::std::forward<Args>(args)...);
    }

    void destroy(pointer p)
    {
      p->~T();
    }
    bool operator==(const gc_allocator_t &) noexcept
    {
      return true;
    }
    bool operator!=(const gc_allocator_t &) noexcept
    {
      return false;
    }
  };
  template <typename T, typename... Args>
  auto make_cgc_unique(Args &&... args)
  {
    T *t{nullptr};
    try {
      t = reinterpret_cast<T *>(cgc_malloc(sizeof(T)));
      gc_allocator_t<T>().construct(t, ::std::forward<Args>(args)...);
    } catch (...) {
      cgc_free(t);
      throw;
    }
    static const auto deleter = [](void *lt) { cgc_free(lt); };
    return ::std::unique_ptr<T, decltype(deleter)>(t, deleter);
  }
  template <typename T, typename... Args>
  T *make_cgc(Args &&... args)
  {
    return make_cgc_unique<T>(::std::forward<Args>(args)...).release();
  }
}
