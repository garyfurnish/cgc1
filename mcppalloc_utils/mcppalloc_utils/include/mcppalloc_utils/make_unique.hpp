#pragma once
namespace mcppalloc
{
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
  using allocator_unique_ptr_t = ::std::unique_ptr<T, typename cgc_allocator_deleter_t<T, Allocator>::type>;
  template <typename T, typename Allocator, typename... Ts>
  allocator_unique_ptr_t<T, Allocator> make_unique_allocator(Ts &&... ts)
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
}
