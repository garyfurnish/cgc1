#pragma once
#include <mutex>
#include <thread>
#include <atomic>
// This file provides wrapped stl mutex functionality that interacts with
// clang's static analyzer to provide thread safety checks.
#ifdef __clang__
#define _Guarded_by_(x) __attribute__((guarded_by(x)))
#define _Exclusive_Locks_Required_(...) __attribute__((exclusive_locks_required(__VA_ARGS__)))
#define _Exclusive_Lock_Function_(...) __attribute__((exclusive_lock_function(__VA_ARGS__)))
#define _Unlock_Function_(...) __attribute__((unlock_function(__VA_ARGS__)))
#define _Locks_Excluded_(...) __attribute__((locks_excluded(__VA_ARGS__)))
#define _No_Thread_Safety_Analysis_ __attribute__((no_thread_safety_analysis))
namespace cgc1
{
  template <typename T>
  using unique_lock_t = ::std::unique_lock<T>;
  template <typename T>
  class lock_guard_t
  {
  public:
    lock_guard_t(T &t) __attribute__((exclusive_lock_function(t))) : m_T(t)
    {
      t.lock();
    }
    ~lock_guard_t() __attribute__((unlock_function))
    {
      m_T.unlock();
    }
    T &m_T;
  } __attribute__((scoped_lockable));

  class mutex_t
  {
  public:
    void lock() __attribute__((exclusive_lock_function))
    {
      m_mutex.lock();
    }
    void unlock() __attribute__((unlock_function))
    {
      m_mutex.unlock();
    }
    bool try_lock() __attribute__((exclusive_trylock_function(true)))
    {
      return m_mutex.try_lock();
    }

  protected:
    ::std::mutex m_mutex;
  } __attribute__((lockable));
  class spinlock_t
  {
  public:
    spinlock_t() : m_lock(0)
    {
    }
    spinlock_t(const spinlock_t &) = delete;
    spinlock_t(spinlock_t &&) = default;
    void lock() __attribute__((exclusive_lock_function))
    {
      while (!try_lock())
        __asm__ volatile("pause");
    }
    void unlock() __attribute__((unlock_function))
    {
      m_lock = false;
    }
    bool try_lock() __attribute__((exclusive_trylock_function(true)))
    {
      bool expected = false;
      bool desired = true;
      return m_lock.compare_exchange_strong(expected, desired);
    }

  protected:
    ::std::atomic<bool> m_lock;
  } __attribute__((lockable));
}
#else
namespace cgc1
{
  template <typename T>
  using lock_guard_t = ::std::lock_guard<T>;
  using mutex_t = ::std::mutex;
  template <typename T>
  using unique_lock_t = ::std::unique_lock<T>;
  class spinlock_t
  {
  public:
    spinlock_t() : m_lock(0)
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
    void unlock()
    {
      m_lock = false;
    }
    bool try_lock()
    {
      bool expected = false;
      bool desired = true;
      return m_lock.compare_exchange_strong(expected, desired);
    }

  protected:
    ::std::atomic<bool> m_lock;
  };
}
#define _Guarded_by_(x)
#define _Exclusive_Locks_Required_(...)
#define _Exclusive_Lock_Function_(...)
#define _Unlock_Function_(...)
#define _No_Thread_Safety_Analysis_
#define _Locks_Excluded_(...)
#endif
#define CGC1_CONCURRENCY_LOCK_GUARD(x) cgc1::lock_guard_t<decltype(x)> _cgc1_macro_lock(x);
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
    ~double_lock_t() _No_Thread_Safety_Analysis_
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
    void unlock() _No_Thread_Safety_Analysis_
    {
      m_t1->unlock();
      m_t2->unlock();
    }
    void release()
    {
      m_t1 = m_t2 = nullptr;
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
