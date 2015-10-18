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
  /**
   * \brief Locks the given Lockable objects lock1, lock2, ..., lockn using a deadlock avoidance algorithm to avoid deadlock.
   **/
  template <typename Lockable1, typename Lockable2>
  void lock(Lockable1 &lock1, Lockable2 &lock2) ACQUIRE(lock1, lock2) NO_THREAD_SAFETY_ANALYSIS
  {
    ::std::lock(lock1, lock2);
  }
  /**
   * \brief Locks the given Lockable objects lock1, lock2, ..., lockn using a deadlock avoidance algorithm to avoid deadlock.
   **/
  template <typename Lockable1, typename Lockable2, typename Lockable3>
  void lock(Lockable1 &lock1, Lockable2 &lock2, Lockable3 &lock3) ACQUIRE(lock1, lock2, lock3) NO_THREAD_SAFETY_ANALYSIS
  {
    ::std::lock(lock1, lock2, lock3);
  }
  /**
   * \brief Locks the given Lockable objects lock1, lock2, ..., lockn using a deadlock avoidance algorithm to avoid deadlock.
   **/
  template <typename Lockable1, typename Lockable2, typename Lockable3, typename Lockable4>
  void lock(Lockable1 &lock1, Lockable2 &lock2, Lockable3 &lock3, Lockable4 &lock4)
      ACQUIRE(lock1, lock2, lock3, lock4) NO_THREAD_SAFETY_ANALYSIS
  {
    ::std::lock(lock1, lock2, lock3, lock4);
  }
  /**
   * \brief Locks the given Lockable objects lock1, lock2, ..., lockn using a deadlock avoidance algorithm to avoid deadlock.
   **/
  template <typename Lockable1, typename Lockable2, typename Lockable3, typename Lockable4, typename Lockable5>
  void lock(Lockable1 &lock1, Lockable2 &lock2, Lockable3 &lock3, Lockable4 &lock4, Lockable5 &lock5)
      ACQUIRE(lock1, lock2, lock3, lock4, lock5) NO_THREAD_SAFETY_ANALYSIS
  {
    ::std::lock(lock1, lock2, lock3, lock4, lock5);
  }
  /**
   * \brief Locks the given Lockable objects lock1, lock2, ..., lockn using a deadlock avoidance algorithm to avoid deadlock.
   **/
  template <typename Lockable1,
            typename Lockable2,
            typename Lockable3,
            typename Lockable4,
            typename Lockable5,
            typename Lockable6>
  void lock(Lockable1 &lock1, Lockable2 &lock2, Lockable3 &lock3, Lockable4 &lock4, Lockable5 &lock5, Lockable6 &lock6)
      ACQUIRE(lock1, lock2, lock3, lock4, lock5, lock6) NO_THREAD_SAFETY_ANALYSIS
  {
    ::std::lock(lock1, lock2, lock3, lock4, lock5, lock6);
  }
  /**
   * \brief Locks the given Lockable objects lock1, lock2, ..., lockn using a deadlock avoidance algorithm to avoid deadlock.
   **/
  template <typename Lockable1,
            typename Lockable2,
            typename Lockable3,
            typename Lockable4,
            typename Lockable5,
            typename Lockable6,
            typename Lockable7>
  void lock(Lockable1 &lock1,
            Lockable2 &lock2,
            Lockable3 &lock3,
            Lockable4 &lock4,
            Lockable5 &lock5,
            Lockable6 &lock6,
            Lockable7 &lock7) ACQUIRE(lock1, lock2, lock3, lock4, lock5, lock6, lock7) NO_THREAD_SAFETY_ANALYSIS
  {
    ::std::lock(lock1, lock2, lock3, lock4, lock5, lock6, lock7);
  }

  /**
   * \brief For static analysis, assume lock1 is unlocked.
   **/
  template <typename Lockable1>
  void assume_unlock(Lockable1 &lock1) RELEASE(lock1) NO_THREAD_SAFETY_ANALYSIS
  {
    (void)lock1;
  }
  /**
   * \brief Unlock lock1, lock2, ..., lockn in order.
   **/
  template <typename Lockable1, typename Lockable2>
  void unlock(Lockable1 &lock1, Lockable2 &lock2) RELEASE(lock1, lock2) NO_THREAD_SAFETY_ANALYSIS
  {
    lock1.unlock();
    lock2.unlock();
  }
  /**
   * \brief Unlock lock1, lock2, ..., lockn in order.
   **/
  template <typename Lockable1, typename Lockable2, typename Lockable3>
  void unlock(Lockable1 &lock1, Lockable2 &lock2, Lockable3 &lock3) RELEASE(lock1, lock2, lock3) NO_THREAD_SAFETY_ANALYSIS
  {
    lock1.unlock();
    lock2.unlock();
    lock3.unlock();
  }
  /**
   * \brief Unlock lock1, lock2, ..., lockn in order.
   **/
  template <typename Lockable1, typename Lockable2, typename Lockable3, typename Lockable4, typename Lockable5>
  void unlock(Lockable1 &lock1, Lockable2 &lock2, Lockable3 &lock3, Lockable4 &lock4, Lockable5 &lock5)
      RELEASE(lock1, lock2, lock3, lock4, lock5) NO_THREAD_SAFETY_ANALYSIS
  {
    lock1.unlock();
    lock2.unlock();
    lock3.unlock();
    lock4.unlock();
    lock5.unlock();
  }
  /**
   * \brief Unlock lock1, lock2, ..., lockn in order.
   **/
  template <typename Lockable1,
            typename Lockable2,
            typename Lockable3,
            typename Lockable4,
            typename Lockable5,
            typename Lockable6>
  void unlock(Lockable1 &lock1, Lockable2 &lock2, Lockable3 &lock3, Lockable4 &lock4, Lockable5 &lock5, Lockable6 &lock6)
      RELEASE(lock1, lock2, lock3, lock4, lock5, lock6) NO_THREAD_SAFETY_ANALYSIS
  {
    lock1.unlock();
    lock2.unlock();
    lock3.unlock();
    lock4.unlock();
    lock5.unlock();
    lock6.unlock();
  }

  /**
   * \brief Scoped variable for assuming that lock is locked.
   **/
  class lock_assume_t
  {
  public:
    template <typename Lockable>
    lock_assume_t(Lockable &lock1) ACQUIRE(lock1)
    {
      (void)lock1;
    }
    template <typename Lockable1, typename Lockable2>
    lock_assume_t(Lockable1 &lock1, Lockable2 &lock2) ACQUIRE(lock1, lock2)
    {
      (void)lock1;
      (void)lock2;
    }
    ~lock_assume_t() RELEASE()
    {
    }
  } SCOPED_CAPABILITY;
}
#ifndef __APPLE__
namespace cgc1
{
  /**
   * \brief The class unique_lock_t is a general-purpose mutex ownership wrapper.
   **/
  template <typename Lock>
  using unique_lock_t = ::std::unique_lock<Lock>;
  /**
   * \brief Scoped variable for locking lock.
   **/
  template <typename Lock1>
  class lock_guard_t
  {
  public:
    lock_guard_t(Lock1 &lock1) ACQUIRE(lock1) : m_lock(lock1)
    {
      lock1.lock();
    }
    lock_guard_t(const lock_guard_t<Lock1> &) = delete;
    ~lock_guard_t() RELEASE()
    {
      m_lock.unlock();
    }
    lock_guard_t<Lock1> &operator=(const lock_guard_t<Lock1> &) = delete;

  private:
    /**
     * \brief Underlying lock.
     **/
    Lock1 &m_lock;
  } SCOPED_CAPABILITY;
  /**
   * \brief Lockable that is an operating system mutex.
   **/
  class CAPABILITY("mutex") mutex_t
  {
  public:
    /**
     * \brief Lock lock.
     **/
    void lock() ACQUIRE()
    {
      m_mutex.lock();
    }
    /**
     * \brief Unlock lock.
     **/
    void unlock() RELEASE()
    {
      m_mutex.unlock();
    }
    /**
     * \brief Try to acquire lock.
     *
     * Does not block.
     * @return True if acquired lock, false otherwise.
     **/
    bool try_lock() TRY_ACQUIRE(true)
    {
      return m_mutex.try_lock();
    }
    const mutex_t &operator!() const
    {
      return *this;
    }

  protected:
    /**
     * \brief Underlying OS mutex.
     **/
    ::std::mutex m_mutex;
  };
}
#endif
#include "concurrency_apple.hpp"
#define CGC1_CONCURRENCY_LOCK_GUARD_MERGE_(a, b) a##b
#define CGC1_CONCURRENCY_LOCK_GUARD_LABEL_(a) CGC1_CONCURRENCY_LOCK_GUARD_MERGE_(cgc1_concurrency_lock_guard_, a)
#define CGC1_CONCURRENCY_LOCK_GUARD_VARIABLE CGC1_CONCURRENCY_LOCK_GUARD_LABEL_(__LINE__)
#define CGC1_CONCURRENCY_LOCK_ASSUME_MERGE_(a, b) a##b
#define CGC1_CONCURRENCY_LOCK_ASSUME_LABEL_(a) CGC1_CONCURRENCY_LOCK_ASSUME_MERGE_(cgc1_concurrency_lock_assume_, a)
#define CGC1_CONCURRENCY_LOCK_ASSUME_VARIABLE CGC1_CONCURRENCY_LOCK_ASSUME_LABEL_(__LINE__)
#define CGC1_CONCURRENCY_LOCK_GUARD(x) cgc1::lock_guard_t<decltype(x)> CGC1_CONCURRENCY_LOCK_GUARD_VARIABLE(x);
#define CGC1_CONCURRENCY_LOCK_ASSUME(...) cgc1::lock_assume_t CGC1_CONCURRENCY_LOCK_GUARD_VARIABLE(__VA_ARGS__);
#define CGC1_CONCURRENCY_LOCK_GUARD_TAKE(x)                                                                                      \
  ::std::unique_lock<decltype(x)> CGC1_CONCURRENCY_LOCK_GUARD_VARIABLE(x, ::std::adopt_lock);                                    \
  cgc1::assume_unlock(x);                                                                                                        \
  cgc1::lock_assume_t CGC1_CONCURRENCY_LOCK_ASSUME_VARIABLE(x);
namespace cgc1
{
  /**
   * \brief Lockable that is a hardware spinlock.
   **/
  class CAPABILITY("mutex") spinlock_t
  {
  public:
    spinlock_t() noexcept : m_lock(0)
    {
    }
    spinlock_t(const spinlock_t &) = delete;
    spinlock_t(spinlock_t &&) = default;
#ifndef __clang__
    /**
     * \brief Lock lock.
     **/
    void lock()
    {
      while (!try_lock())
        _mm_pause();
    }
#else
    /**
     * \brief Lock lock.
     **/
    void lock() ACQUIRE()
    {
      while (!try_lock())
        __asm__ volatile("pause");
    }
#endif
    /**
     * \brief Unlock lock.
     **/
    void unlock() noexcept RELEASE()
    {
      ::std::atomic_thread_fence(::std::memory_order_release);
      m_lock = false;
    }
    /**
     * \brief Try to acquire lock.
     *
     * Does not block.
     * @return True if acquired lock, false otherwise.
     **/
    bool try_lock() noexcept TRY_ACQUIRE(true)
    {
      bool expected = false;
      bool desired = true;
      if (m_lock.compare_exchange_strong(expected, desired)) {
        ::std::atomic_thread_fence(::std::memory_order_acquire);
        return true;
      }
      return false;
    }
    const spinlock_t &operator!() const
    {
      return *this;
    }

  protected:
    /**
     * \brief Underlying atomic value.
     **/
    ::std::atomic<bool> m_lock;
  };
  /**
   * \brief The class double_lock_t is a general-purpose mutex ownership wrapper for two Lockables.
   *
   * This is not compatible with clang thread safety analysis yet.
   * Uses a deadlock avoidance algorithm to avoid deadlock.
   **/
  template <typename Lockable1, typename Lockable2>
  class double_lock_t
  {
  public:
    double_lock_t(Lockable1 &t1, Lockable2 &t2) : m_lock1(&t1), m_lock2(&t2)
    {
      ::std::lock(*m_lock1, *m_lock2);
    }
    ~double_lock_t() NO_THREAD_SAFETY_ANALYSIS
    {
      if (m_lock1 && m_lock2) {
        m_lock1->unlock();
        m_lock2->unlock();
      }
    }
    /**
     * \brief Lock locks.
     *
     * Aborts if lock has been released.
     **/
    void lock()
    {
      if (m_lock1 && m_lock2)
        ::std::lock(*m_lock1, *m_lock2);
      else
        abort();
    }
    /**
     * \brief Unlock locks.
     *
     * Aborts if lock has been released.
     **/
    void unlock() NO_THREAD_SAFETY_ANALYSIS
    {
      if (m_lock1 && m_lock2) {
        m_lock1->unlock();
        m_lock2->unlock();
      } else {
        abort();
      }
    }
    /**
     * \brief Disassociates the associated mutexes without unlocking them.
     **/
    void release()
    {
      m_lock1 = nullptr;
      m_lock2 = nullptr;
    }
    /**
     * \brief Try to acquire locks.
     *
     * Does not block.
     * @return True if acquired lock, false otherwise.
     **/
    bool try_lock() NO_THREAD_SAFETY_ANALYSIS
    {
      if (m_lock1->try_lock()) {
        if (m_lock2->try_lock())
          return true;
        else
          m_lock1->unlock();
      }
      return false;
    }

  private:
    /**
     * \brief Underlying lockable.
     **/
    Lockable1 *m_lock1, *m_lock2;
  };
}
