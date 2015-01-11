#pragma once
#include "declarations.hpp"
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
// This file provides wrapped stl mutex functionality that interacts with
// clang's static analyzer to provide thread safety checks.
#include "clang_concurrency.hpp"
namespace cgc1
{
  template <typename T1, typename T2>
  void lock(T1 &t1, T2 &t2) ACQUIRE(t1, t2) NO_THREAD_SAFETY_ANALYSIS
  {
    ::std::lock(t1, t2);
  }

  template <typename T1, typename T2, typename T3, typename T4>
  void lock(T1 &t1, T2 &t2, T3 &t3, T4 &t4) ACQUIRE(t1, t2, t3, t4) NO_THREAD_SAFETY_ANALYSIS
  {
    ::std::lock(t1, t2, t3, t4);
  }
  template <typename T1, typename T2, typename T3, typename T4, typename T5>
  void lock(T1 &t1, T2 &t2, T3 &t3, T4 &t4, T5 &t5) ACQUIRE(t1, t2, t3, t4, t5) NO_THREAD_SAFETY_ANALYSIS
  {
    ::std::lock(t1, t2, t3, t4, t5);
  }
  template <typename T1, typename T2, typename T3, typename T4, typename T5>
  void unlock(T1 &t1, T2 &t2, T3 &t3, T4 &t4, T5 &t5) RELEASE(t1, t2, t3, t4, t5) NO_THREAD_SAFETY_ANALYSIS
  {
    t1.unlock();
    t2.unlock();
    t3.unlock();
    t4.unlock();
    t5.unlock();
  }

  class lock_assume_t
  {
  public:
    template <typename T>
    lock_assume_t(T &t) ACQUIRE(t)
    {
      (void)t;
    }
    ~lock_assume_t() RELEASE()
    {
    }
  } SCOPED_CAPABILITY;
}
#ifndef __APPLE__
namespace cgc1
{
  template <typename T>
  using unique_lock_t = ::std::unique_lock<T>;
  template <typename T>
  class lock_guard_t
  {
  public:
    lock_guard_t(T &t) ACQUIRE(t) : m_T(t)
    {
      t.lock();
    }
    lock_guard_t(const lock_guard_t<T> &) = delete;
    ~lock_guard_t() RELEASE()
    {
      m_T.unlock();
    }
    lock_guard_t<T> &operator=(const lock_guard_t<T> &) = delete;
    T &m_T;
  } SCOPED_CAPABILITY;

  class CAPABILITY("mutex") mutex_t
  {
  public:
    void lock() ACQUIRE()
    {
      m_mutex.lock();
    }
    void unlock() RELEASE()
    {
      m_mutex.unlock();
    }
    bool try_lock() TRY_ACQUIRE(true)
    {
      return m_mutex.try_lock();
    }
    const mutex_t &operator!() const
    {
      return *this;
    }

  protected:
    ::std::mutex m_mutex;
  };
  class CAPABILITY("mutex") spinlock_t
  {
  public:
    spinlock_t() noexcept : m_lock(0)
    {
    }
    spinlock_t(const spinlock_t &) = delete;
    spinlock_t(spinlock_t &&) = default;
#ifndef __clang__
    void lock()
    {
      while (!try_lock())
        _mm_pause();
    }
#else
    void lock()
    {
      while (!try_lock())
			__asm__ volatile("pause");
    }
#endif
    void unlock() noexcept RELEASE()
    {
      m_lock = false;
    }
    bool try_lock() noexcept TRY_ACQUIRE(true)
    {
      bool expected = false;
      bool desired = true;
      return m_lock.compare_exchange_strong(expected, desired);
    }
    const spinlock_t &operator!() const
    {
      return *this;
    }

  protected:
    ::std::atomic<bool> m_lock;
  };
}
using condition_variable_any_t = ::std::condition_variable_any;
#endif
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
#else
namespace cgc1
{
  using condition_variable_any_t = ::std::condition_variable_any;
}
#endif
#define CGC1_CONCURRENCY_LOCK_GUARD(x) cgc1::lock_guard_t<decltype(x)> _cgc1_macro_lock(x);
#define CGC1_CONCURRENCY_LOCK_ASSUME(x) cgc1::lock_assume_t _cgc1_macro_lock(x);
  namespace cgc1
  {
    template <typename T1, typename T2>
    class double_lock_t
    {
    public:
      double_lock_t(T1 &t1, T2 &t2) : m_t1(&t1), m_t2(&t2)
      {
        ::std::lock(*m_t1, *m_t2);
      }
      ~double_lock_t() NO_THREAD_SAFETY_ANALYSIS
      {
        if (m_t1 && m_t2) {
          m_t1->unlock();
          m_t2->unlock();
        }
      }
      void lock()
      {
        if (m_t1 && m_t2)
          ::std::lock(*m_t1, *m_t2);
        else
          abort();
      }
      void unlock() NO_THREAD_SAFETY_ANALYSIS
      {
        m_t1->unlock();
        m_t2->unlock();
      }
      void release()
      {
        m_t1 = nullptr;
        m_t2 = nullptr;
      }
      bool try_lock()
      {
        if (m_t1->try_lock()) {
          if (m_t2->try_lock())
            return true;
          else
            m_t1->unlock();
        }
        return false;
      }

    private:
      T1 *m_t1;
      T2 *m_t2;
    };
  }
