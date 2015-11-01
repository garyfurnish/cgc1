#pragma once
#include "condition_variable.hpp"
namespace cgc1
{
  template <typename Allocator>
  void internal_condition_variable_t<Allocator>::notify_all()
  {
    CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
    // set all flags to true.
    for (auto &&queue_struct : m_queued) {
      queue_struct.m_flag.unlock();
    }
  }
  template <typename Allocator>
  template <typename Lock, typename Predicate>
  void internal_condition_variable_t<Allocator>::wait(Lock &&lock, Predicate &&pred, bool _precheck_pred)
  {
    CGC1_CONCURRENCY_LOCK_ASSUME(lock);
    typename queue_type::iterator it;
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      // this could theoretically throw from oom but we can't do anything about it.
      // throwing here would have no negative consequences, so don't do anything.
      // add a new waiter to queue.
      m_queued.emplace_back();
      it = (--m_queued.end());
      // acquire the lock to wait on.
      it->m_flag.lock();
    }
    bool pred_result;
    // if we should check the predicate first
    if (_precheck_pred) {
      // check the predicate.
      // The predicate could throw an exception.
      try {
        pred_result = pred();
      } catch (...) {
        // if it throws an exception we need to cleanup the queue.
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
        it->m_flag.unlock();
        m_queued.erase(it);
        // rethrow.
        throw;
      }
      if (pred_result) {
        {
          // if the predicate is true, we should remove the waiter from the queue.
          CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
          it->m_flag.unlock();
          m_queued.erase(it);
        }
        // return.
        return;
      }
    }
    // do this forever
    while (1) {
      // we are going to unlock and sleep.
      lock.unlock();
      it->m_flag.lock();
      lock.lock();
      // now we have the lock.
      // if the predicate is true
      // this can throw while we have the lock.
      // correct behavior is to let it throw while keeping lock.
      // but we need to remove from queue.
      try {
        pred_result = pred();
      } catch (...) {
        // if it throws an exception we need to cleanup the queue.
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
        it->m_flag.unlock();
        m_queued.erase(it);
        // rethrow.
        throw;
      }
      if (pred()) {
        {
          // if the predicate is true, we should remove the waiter from the queue.
          CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
          it->m_flag.unlock();
          m_queued.erase(it);
        }
        // return.
        return;
      }
      it->m_flag.lock();
    }
  }
  template <typename Allocator>
  template <typename Lock, typename Predicate>
  void internal_condition_variable_t<Allocator>::wait(Lock &&lock, Predicate &&pred)
  {
    // forward to internal wait.
    wait(::std::forward<Lock>(lock), ::std::forward<Predicate>(pred), true);
  }

  template <typename Allocator>
  template <typename Lock>
  void internal_condition_variable_t<Allocator>::wait(Lock &&lock)
  {
    // forward to internal wait.
    wait(::std::forward<Lock>(lock), []() { return true; }, false);
  }
}
