#pragma once
#include <list>
#include "concurrency.hpp"
namespace cgc1
{
  /**
   * \brief Replacement for ::std::condition_variable_any that uses a given allocator.
   * This condition variable is guarenteed to be reenterant at the expense of possibly performance.
   * It can not use kernel functions because they may not be reenterant with the exception of yield.
   **/
  template <typename Allocator>
  class internal_condition_variable_t
  {
  public:
    using allocator = Allocator;
    internal_condition_variable_t() = default;
    internal_condition_variable_t(const internal_condition_variable_t &) = delete;
    internal_condition_variable_t(internal_condition_variable_t &&) = delete;
    internal_condition_variable_t &operator=(const internal_condition_variable_t &) = delete;
    internal_condition_variable_t &operator=(internal_condition_variable_t &&) = delete;
    // notify all waiting threads.
    void notify_all() REQUIRES(!m_mutex);
    template <typename Lock>
    void wait(Lock &&lock) REQUIRES(!m_mutex);
    template <typename Lock, typename Predicate>
    void wait(Lock &&lock, Predicate &&pred) REQUIRES(!m_mutex);

  private:
    template <typename Lock, typename Predicate>
    void wait(Lock &&lock, Predicate &&pred, bool _precheck_pred) REQUIRES(!m_mutex);

    struct queue_struct_t {
      ::std::atomic<bool> m_flag{false};
    };
    using queue_type = ::std::list<queue_struct_t, typename allocator::template rebind<queue_struct_t>::other>;
    // queue to hold waiting threads.
    queue_type m_queued GUARDED_BY(m_mutex);
    spinlock_t m_mutex;
  };
}
#include "condition_variable_impl.hpp"
