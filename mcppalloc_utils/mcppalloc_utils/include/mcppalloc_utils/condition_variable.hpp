#pragma once
#include <list>
#include "concurrency.hpp"
namespace cgc1
{
  /**
   * \brief Replacement for ::std::condition_variable_any that uses a given allocator.
   *
   * This condition variable is guarenteed to be reenterant at the expense of possibly performance.
   * It can not use most kernel functions since on many oses conditional variables use mutexes internally.
   * Mutexes and yield are safe to use on osx/linux.
   * Therefore this works by maintaining a queue of mutexes that can be unlocked to notify waiting threads.
   * @tparam Allocator allocator to use for internal memory allocation.
   **/
  template <typename Allocator>
  class internal_condition_variable_t
  {
  public:
    /**
     * \brief Allocator used for this variable.
     **/
    using allocator = Allocator;
    internal_condition_variable_t() = default;
    internal_condition_variable_t(const internal_condition_variable_t &) = delete;
    internal_condition_variable_t(internal_condition_variable_t &&) = delete;
    internal_condition_variable_t &operator=(const internal_condition_variable_t &) = delete;
    internal_condition_variable_t &operator=(internal_condition_variable_t &&) = delete;
    /**
     * \brief Notify all variables waiting.
     *
     * Aborts on error.
     **/
    void notify_all() REQUIRES(!m_mutex) NO_THREAD_SAFETY_ANALYSIS;
    /**
     * \brief Wait until signaled and pred is true.
     *
     * Aborts on error.
     * @param lock Lock to unlock while waiting and reacquire afterwards.
     **/
    template <typename Lock>
    void wait(Lock &&lock) REQUIRES(!m_mutex);
    /**
     * \brief Wait until signaled and pred is true.
     *
     * Aborts on error.
     * @param lock Lock to unlock while waiting and reacquire afterwards.
     * @param pred Predicate to test.
     **/
    template <typename Lock, typename Predicate>
    void wait(Lock &&lock, Predicate &&pred) REQUIRES(!m_mutex);

  private:
    /**
     * \brief Internal implementation of wait.
     *
     * Wait until signaled and pred is true.
     * @param lock Lock to unlock while waiting and reacquire afterwards.
     * @param pred Predicate to test.
     * @param _precheck_pred Should the predicate be tested prior to notification waiting.
     **/
    template <typename Lock, typename Predicate>
    void wait(Lock &&lock, Predicate &&pred, bool _precheck_pred) REQUIRES(!m_mutex) NO_THREAD_SAFETY_ANALYSIS;
    /**
     * \brief Contains information a waiter.
     **/
    struct queue_struct_t {
      /**
       * \brief Mutex used to signal a waiter.
       **/
      ::mcppalloc::mutex_t m_flag;
    };
    /**
     * \brief Type of queue.
     *
     * Uses Allocator for memory.
     **/
    using queue_type = ::std::list<queue_struct_t, typename allocator::template rebind<queue_struct_t>::other>;
    /**
     * \brief Queue to hold waiting threads.
     **/
    queue_type m_queued GUARDED_BY(m_mutex);
    /**
     * \brief Internal protection for queue.
     **/
    ::mcppalloc::mutex_t m_mutex;
  };
}
#include "condition_variable_impl.hpp"
