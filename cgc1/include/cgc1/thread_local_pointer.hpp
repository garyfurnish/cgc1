#pragma once
namespace cgc1
{
  /**
   * \brief Header only implementation of thread local pointer.
   * 
   * This is like a boost::thread_specific_ptr except header only and faster.
   **/
  template<typename Pointer>
  class thread_local_pointer_t
  {
  public:
    using pointer_type = Pointer*;
    using value_type = Pointer;
#ifdef __APPLE__
    thread_local_pointer_t()
    {
      pthread_key_create(&s_pkey, [](void *) {});
      set(nullptr);
    }
    ~thread_local_pointer_t()
    {
      pthread_key_delete(s_pkey);
    }
    auto get() const noexcept -> pointer_type
    {
      return unsafe_cast<value_type>(pthread_getspecific(s_pkey));
    }
    void  set(pointer_type ptr) noexcept
    {
      pthread_setspecific(s_pkey, ptr);
    }
#else
    thread_local_pointer_t() = default;
    ~thread_local_pointer_t() = default;
    auto get() const noexcept -> pointer_type
    {
      return s_tlks;
    }
    void  set(pointer_type ptr) noexcept
    {
      s_tlks = ptr;
    }
#endif
    thread_local_pointer_t(const thread_local_pointer_t&) = delete;
    thread_local_pointer_t(thread_local_pointer_t&&) = delete;
    thread_local_pointer_t& operator=(const thread_local_pointer_t&) = delete;
    thread_local_pointer_t& operator=(thread_local_pointer_t&&) = delete;
    thread_local_pointer_t& operator=(pointer_type ptr)
    {
      set(ptr);
      return *this;
    }
    operator pointer_type() const noexcept
    {
      return get();
    }
  private:
#ifdef __APPLE__
    static pthread_key_t s_pkey;
#elif defined(_WIN32)
    static __declspec(thread) pointer_type s_tlks = nullptr;
#else
    static thread_local pointer_type *s_tlks = nullptr;
#endif

  };
}
