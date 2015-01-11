#pragma once
#ifdef __APPLE__
namespace cgc1
{
  template <typename T>
  using lock_guard_t = ::std::lock_guard<T>;
  class pthread_mutex_t
  {
  public:
    pthread_mutex_t() noexcept
    {
      if (pthread_mutex_init(&m_mutex, 0))
        ::abort();
    }
    pthread_mutex_t(const pthread_mutex_t &) = delete;
    pthread_mutex_t(pthread_mutex_t &&) = default;
    ~pthread_mutex_t()
    {
      if (::pthread_mutex_destroy(&m_mutex))
        ::abort();
    }
    void lock() noexcept
    {
      if (::pthread_mutex_lock(&m_mutex))
        ::abort();
    }
    void unlock() noexcept
    {
      if (::pthread_mutex_unlock(&m_mutex))
        ::abort();
    }
    bool try_lock() noexcept
    {
      auto result = ::pthread_mutex_trylock(&m_mutex);
      if (result == 0)
        return true;
      return false;
    }
    using native_handle_type = ::pthread_mutex_t *;
    native_handle_type native_handle() noexcept
    {
      return &m_mutex;
    }

  private:
    ::pthread_mutex_t m_mutex;
  };
  class pthread_condition_variable_any_t
  {
  public:
    pthread_condition_variable_any_t() noexcept
    {
      if (::pthread_cond_init(&m_cond, 0))
        ::abort();
    }
    pthread_condition_variable_any_t(const pthread_condition_variable_any_t &) = delete;
    pthread_condition_variable_any_t(pthread_condition_variable_any_t &&) = default;
    ~pthread_condition_variable_any_t()
    {
      if (::pthread_cond_destroy(&m_cond))
        ::abort();
    }
    template <typename Lock, typename Pred>
    void wait(Lock &lock, const Pred &pred)
    {
      while (!pred())
        wait(lock);
    }
    template <typename Lock>
    void wait(Lock &lock)
    {
      m_mutex.lock();
      lock.unlock();
      if (::pthread_cond_wait(&m_cond, m_mutex.native_handle()))
        ::abort();
      m_mutex.unlock();
      lock.lock();
    }
    void notify_all() noexcept
    {
      lock_guard_t<decltype(m_mutex)> lg(m_mutex);
      if (::pthread_cond_broadcast(&m_cond))
        ::abort();
    }

  private:
    ::pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;
  };
  using mutex_t = pthread_mutex_t;
  using condition_variable_any_t = pthread_condition_variable_any_t;

  template <typename T>
  using unique_lock_t = ::std::unique_lock<T>;
}
#endif
