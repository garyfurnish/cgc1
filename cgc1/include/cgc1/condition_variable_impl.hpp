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
      queue_struct.m_flag = true;
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
      m_queued.emplace_back();
      it = (--m_queued.end());
    }
    bool pred_result;
    if (_precheck_pred) {
      try {
        pred_result = pred();
      } catch (...) {
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
        m_queued.erase(it);
        // rethrow.
        throw;
      }
      if (pred_result) {
        {
          CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
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
      while (!it->m_flag) {
        // sleep using OS until we get notified.
        ::std::this_thread::yield();
      }
      // we have been notified, try to get lock.
      while (!lock.try_lock()) {
        // sleep using OS until we get the lock.
        ::std::this_thread::yield();
      }
      // now we have the lock.
      // if the predicate is true
      // this can throw while we have the lock.
      // correct behavior is to let it throw while keeping lock.
      // but we need to remove from queue.
      try {
        pred_result = pred();
      } catch (...) {
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
        m_queued.erase(it);
        // rethrow.
        throw;
      }
      if (pred()) {
        {
          CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
          m_queued.erase(it);
        }
        // return.
        return;
      }
      it->m_flag = false;
    }
  }
  template <typename Allocator>
  template <typename Lock, typename Predicate>
  void internal_condition_variable_t<Allocator>::wait(Lock &&lock, Predicate &&pred)
  {
    wait(::std::forward<Lock>(lock), ::std::forward<Predicate>(pred), true);
  }

  template <typename Allocator>
  template <typename Lock>
  void internal_condition_variable_t<Allocator>::wait(Lock &&lock)
  {
    wait(::std::forward<Lock>(lock), []() { return true; }, false);
  }
}
