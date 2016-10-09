#pragma once
#include "declarations.hpp"
#include <thread>
namespace cgc1
{
  namespace details
  {
#ifdef _WIN32
    using native_thread_handle_type = ::std::thread::native_handle_type;
#else
    using native_thread_handle_type = ::pthread_t;
#endif
    constexpr const static native_thread_handle_type c_native_thread_handle_initializer{0};
    template <typename Allocator>
    class allocated_thread_details_base_t
    {
    public:
      using allocator = Allocator;
      using native_handle_type = native_thread_handle_type;
      using this_type = allocated_thread_details_base_t<allocator>;
      allocated_thread_details_base_t(const allocator &allocator);
      virtual ~allocated_thread_details_base_t() = default;
      static void *_s_run(void *user_data);
      virtual void *_run() = 0;

    protected:
      allocator m_allocator;
    };
    template <typename Allocator, typename Function, typename... Args>
    class allocated_thread_details_t : public allocated_thread_details_base_t<Allocator>
    {
    public:
      using allocator = Allocator;
      allocated_thread_details_t(const allocator &allocator, Function &&function);
      allocated_thread_details_t(const allocated_thread_details_t &) = delete;
      allocated_thread_details_t(allocated_thread_details_t &&) = default;
      allocated_thread_details_t &operator=(const allocated_thread_details_t &) = delete;
      allocated_thread_details_t &operator=(allocated_thread_details_t &&) = default;
      ~allocated_thread_details_t() = default;
      virtual void *_run() override;

    private:
      Function m_function;
    };
  }
  template <typename Allocator>
  class allocated_thread_t
  {
  public:
    using allocator = Allocator;
    using native_handle_type = details::native_thread_handle_type;
    allocated_thread_t() = default;
    template <typename Function, typename... Args>
    allocated_thread_t(const allocator &alloc, Function &&f, Args &&... args);
    allocated_thread_t(const allocated_thread_t &) = delete;
    allocated_thread_t(allocated_thread_t &&t) noexcept = default;
    allocated_thread_t &operator=(const allocated_thread_t &) = delete;
    allocated_thread_t &operator=(allocated_thread_t &&) noexcept = default;

    auto joinable() const noexcept -> bool;
    void join();
    void detach();
    auto get_id() const noexcept -> ::std::thread::id;
    auto native_handle() -> native_handle_type;
    static auto hardware_concurrency() noexcept -> unsigned;

  private:
    void _start(void *details_ptr);
    using details_type = details::allocated_thread_details_base_t<Allocator>;
    native_handle_type m_native_handle{details::c_native_thread_handle_initializer};
  };
}
#include "allocated_thread_timpl.hpp"
