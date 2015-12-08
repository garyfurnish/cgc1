#pragma once
#ifdef __APPLE__
namespace mcppalloc
{
  /**
   * \brief Scoped variable for locking lock.
   **/
  template <typename T>
  using lock_guard_t = ::std::lock_guard<T>;
  /**
   * \brief Lockable that is a pthread mutex.
   **/
  class pthread_mutex_t
  {
  public:
    /**
     * \brief Constructor.
     *
     * Aborts on error.
     **/
    pthread_mutex_t() noexcept
    {
      if (pthread_mutex_init(&m_mutex, 0))
        :: ::std::terminate();
    }
    pthread_mutex_t(const pthread_mutex_t &) = delete;
    pthread_mutex_t(pthread_mutex_t &&) = default;
    /**
     * \brief Destructor.
     *
     * Aborts on error.
     **/
    ~pthread_mutex_t()
    {
      if (::pthread_mutex_destroy(&m_mutex))
        :: ::std::terminate();
    }
    /**
     * \brief Locks lock.
     *
     * Aborts on error.
     **/
    void lock() noexcept
    {
      if (::pthread_mutex_lock(&m_mutex))
        :: ::std::terminate();
    }
    /**
     * \brief Unlocks lock.
     *
     * Aborts on error.
     **/
    void unlock() noexcept
    {
      if (::pthread_mutex_unlock(&m_mutex))
        :: ::std::terminate();
    }
    /**
     * \brief Try to acquire locks.
     *
     * Does not block.
     * @return True if acquired lock, false otherwise.
     **/
    bool try_lock() noexcept
    {
      auto result = ::pthread_mutex_trylock(&m_mutex);
      if (result == 0)
        return true;
      return false;
    }
    /**
     * \brief Type of native handle.
     **/
    using native_handle_type = ::pthread_mutex_t *;
    /**
     * \brief Return native handle.
     **/
    native_handle_type native_handle() noexcept
    {
      return &m_mutex;
    }

  private:
    /**
     * \brief Underlying pthread mutex.
     **/
    ::pthread_mutex_t m_mutex;
  };
  /**
   * \brief Conditional variable using pthreads.
   **/
  class pthread_condition_variable_any_t
  {
  public:
    /**
     * \brief Constructor.
     *
     * Aborts on error.
     **/
    pthread_condition_variable_any_t() noexcept
    {
      if (::pthread_cond_init(&m_cond, 0))
        :: ::std::terminate();
    }
    pthread_condition_variable_any_t(const pthread_condition_variable_any_t &) = delete;
    pthread_condition_variable_any_t(pthread_condition_variable_any_t &&) = default;
    /**
     * \brief Destructor.
     *
     * Aborts on error.
     **/
    ~pthread_condition_variable_any_t()
    {
      if (::pthread_cond_destroy(&m_cond))
        :: ::std::terminate();
    }
    /**
     * \brief Wait until signaled and pred is true.
     *
     * Aborts on error.
     * @param lock Lock to unlock while waiting and reacquire afterwards.
     * @param pred Predicate to test.
     **/
    template <typename Lock, typename Pred>
    void wait(Lock &lock, const Pred &pred)
    {
      while (!pred())
        wait(lock);
    }
    /**
     * \brief Wait until signaled.
     *
     * Aborts on error.
     * @param lock Lock to unlock while waiting and reacquire afterwards.
     **/
    template <typename Lock>
    void wait(Lock &lock)
    {
      m_mutex.lock();
      lock.unlock();
      if (::pthread_cond_wait(&m_cond, m_mutex.native_handle()))
        :: ::std::terminate();
      m_mutex.unlock();
      lock.lock();
    }
    /**
     * \brief Notify all variables waiting.
     *
     * Aborts on error.
     **/
    void notify_all() noexcept
    {
      lock_guard_t<decltype(m_mutex)> lg(m_mutex);
      if (::pthread_cond_broadcast(&m_cond))
        :: ::std::terminate();
    }

  private:
    /**
     * \brief Underlying pthread conditional variable.
     **/
    ::pthread_cond_t m_cond;
    /**
     * \brief Internal mutex needed for protection since pthread requires a pthread mutex.
     **/
    pthread_mutex_t m_mutex;
  };
  /**
   * \brief Lockable that is an operating system mutex.
   **/
  using mutex_t = pthread_mutex_t;
  /**
   * \brief The class unique_lock_t is a general-purpose mutex ownership wrapper.
   **/
  template <typename T>
  using unique_lock_t = ::std::unique_lock<T>;
}
#endif
