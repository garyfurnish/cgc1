#pragma once
#include <mcppalloc_utils/unsafe_cast.hpp>
#include <mcppalloc_utils/std.hpp>
namespace cgc1
{
  namespace details
  {
    template <typename Allocator>
    allocated_thread_details_base_t<Allocator>::allocated_thread_details_base_t(const allocator &allocator)
        : m_allocator(allocator)
    {
    }
    template <typename Allocator>
    void *allocated_thread_details_base_t<Allocator>::_s_run(void *user_data)
    {
      auto unique = ::mcppalloc::allocator_unique_ptr_t<this_type, allocator>(::mcppalloc::unsafe_cast<this_type>(user_data));
      unique->_run();
      return nullptr;
    }

    template <typename Allocator, typename Function, typename... Args>
    allocated_thread_details_t<Allocator, Function, Args...>::allocated_thread_details_t(const allocator &allocator,
                                                                                         Function &&function)
        : allocated_thread_details_base_t<Allocator>(allocator), m_function(::std::forward<Function>(function))

    {
    }
    template <typename Allocator, typename Function, typename... Args>
    void *allocated_thread_details_t<Allocator, Function, Args...>::_run()
    {
      return m_function();
    }
  }
  template <typename Allocator>
  template <typename Function, typename... Args>
  allocated_thread_t<Allocator>::allocated_thread_t(const allocator &alloc, Function &&f, Args &&... args)
  {
    auto args_tuple = ::std::make_tuple(::std::forward<Args>(args)...);
    auto nf = [ f = ::std::forward<Function>(f), args_tuple = ::std::move(args_tuple) ]()->void *
    {
      return ::std::apply(f, args_tuple);
    };
    using actual_details_type = details::allocated_thread_details_t<allocator, decltype(nf), Args...>;
    auto details = ::mcppalloc::make_unique_allocator<actual_details_type, allocator>(alloc, ::std::move(nf));
    auto details_ptr = details.release();
    try {
      _start(details_ptr);
    } catch (...) {
      details.reset(details_ptr);
      details.reset();
      throw;
    }
  }
#ifndef _WIN32
  template <typename Allocator>
  void allocated_thread_t<Allocator>::_start(void *details_ptr)
  {
    int ec = pthread_create(&m_native_handle, 0, &details_type::_s_run, details_ptr);
    if (ec)
      throw ::std::system_error(::std::error_code(ec, ::std::system_category()), "thread constructor failed");
  }

  template <typename Allocator>
  auto allocated_thread_t<Allocator>::joinable() const noexcept -> bool
  {
    return m_native_handle != details::c_native_thread_handle_initializer;
  }
  template <typename Allocator>
  void allocated_thread_t<Allocator>::join()
  {
    int ec = ::pthread_join(m_native_handle, 0);
    if (ec)
      throw ::std::system_error(::std::error_code(ec, ::std::system_category()), "thread::join failed");
  }
  template <typename Allocator>
  void allocated_thread_t<Allocator>::detach()
  {
    int ec = ::pthread_detach(m_native_handle);
    if (ec)
      throw ::std::system_error(::std::error_code(ec, ::std::system_category()), "thread::detach failed");
  }
#endif
  template <typename Allocator>
  auto allocated_thread_t<Allocator>::get_id() const noexcept -> ::std::thread::id
  {
    return ::std::thread::id(native_handle);
  }
  template <typename Allocator>
  auto allocated_thread_t<Allocator>::native_handle() -> native_handle_type
  {
    return m_native_handle;
  }
  template <typename Allocator>
  static auto hardware_concurrency() noexcept -> unsigned
  {
    return ::std::thread::hardware_concurrency();
  }
}
