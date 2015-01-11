#pragma once
#include "internal_declarations.hpp"
#include <thread>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <cgc1/concurrency.hpp>
#include "allocator.hpp"
#include "internal_allocator.hpp"
#include "gc_allocator.hpp"
namespace cgc1
{
  namespace details
  {
    /**
    Internal thread that performs garbage collection
    **/
    class gc_thread_t
    {
    public:
      gc_thread_t();
      gc_thread_t(const gc_thread_t &) = delete;
      gc_thread_t(gc_thread_t &&) = delete;
      gc_thread_t &operator=(const gc_thread_t &) = delete;
      gc_thread_t &operator=(gc_thread_t &&) = delete;
      ~gc_thread_t();
      void add_thread(::std::thread::id id) REQUIRES(!m_mutex);
      void reset() REQUIRES(!m_mutex);
      void set_allocator_blocks(gc_allocator_t::this_allocator_block_handle_t *begin,
                                gc_allocator_t::this_allocator_block_handle_t *end) REQUIRES(!m_mutex);
      void set_root_iterators(void ***begin, void ***end) REQUIRES(!m_mutex);
      void wake_up() REQUIRES(!m_mutex);
      /**
      Start Clearing marks.
      **/
      void start_clear() REQUIRES(!m_mutex);
      /**
      Wait until clearing finished.
      **/
      void wait_until_clear_finished();
      /**
      Start marking.
      **/
      void start_mark();
      /**
      Wait until mark phase finished.
      **/
      void wait_until_mark_finished();
      /**
      Start sweeping.
      **/
      void start_sweep();
      /**
      Wait until sweep phase finished.
      **/
      void wait_until_sweep_finished();
      /**
      Notify the thread that all threads have resumed from garbage collection and that it is safe to finalize.
      **/
      void notify_all_threads_resumed();
      /**
      Wait until finalization finishes.
      **/
      void wait_until_finalization_finished();
      /**
      Returns true if finalization finished, false otherwise.
      **/
      bool finalization_finished() const;

    private:
      void _u_remove_thread(::std::thread::id id) REQUIRES(m_mutex);
      bool handle_thread(::std::thread::id id) REQUIRES(m_mutex);
      /**
      Inner thread loop.
      **/
      void _run() REQUIRES(!m_mutex);
      /**
      Clear marks.
      **/
      void _clear_marks() REQUIRES(m_mutex);
      void _mark() REQUIRES(m_mutex);
      /**
      Mark a given address.
      **/
      void _mark_addrs(void *addr, size_t depth, bool ignore_skip_marked = false) REQUIRES(m_mutex);
      /**
      Function called at end to mark any addresses that would have caused stack overflows.
      **/
      void _mark_mark_vector() REQUIRES(m_mutex);
      /**
      Sweep for unaccessible memory.
      **/
      void _sweep() REQUIRES(m_mutex);
      /**
      Finalize.
      **/
      void _finalize() REQUIRES(m_mutex);
      /**
      Mutex for condition variables and protection.
      **/
      mutex_t m_mutex;
      /**
      Vector of threads whose stack this thread is responsible for.
      **/
      cgc_internal_vector_t<::std::thread::id> m_watched_threads GUARDED_BY(m_mutex);
      /**
      Variable for waking up.
      **/
      condition_variable_any_t m_wake_up;
      /**
      Variable for done with clearing.
      **/
      condition_variable_any_t m_done_clearing;
      /**
      Variable for start mark.
      **/
      condition_variable_any_t m_start_mark;
      /**
      Variable for done mark.
      **/
      condition_variable_any_t m_done_mark;
      /**
      Variable for starting sweeping.
      **/
      condition_variable_any_t m_start_sweep;
      /**
      Variable for when done with collection.
      **/
      condition_variable_any_t m_done_sweep;
      /**
      Variable for all threads resumed.
      **/
      condition_variable_any_t m_all_threads_resumed;
      /**
      Variable for finalization done.
      **/
      condition_variable_any_t m_done_finalization;
      /**
      Thread that this runs in.
      **/
      ::std::thread m_thread;
      ::std::atomic<bool> m_clear_done, m_mark_done, m_sweep_done, m_finalization_done;
      ::std::atomic<bool> m_run;
      ::std::atomic<bool> m_do_clear, m_do_mark, m_do_sweep, m_do_all_threads_resumed;
      gc_allocator_t::this_allocator_block_handle_t *m_block_begin GUARDED_BY(m_mutex);
      gc_allocator_t::this_allocator_block_handle_t *m_block_end GUARDED_BY(m_mutex);
      void ***m_root_begin GUARDED_BY(m_mutex);
      void ***m_root_end GUARDED_BY(m_mutex);
      /**
      Normally we recurse, but if the recursion is excessive, do it here instead.
      **/
      cgc_internal_vector_t<void *> m_addresses_to_mark GUARDED_BY(m_mutex);
      /**
      Roots from stack.
      **/
      cgc_internal_vector_t<uint8_t **> m_stack_roots GUARDED_BY(m_mutex);
      /**
      List of roots to be freed.
      **/
      cgc_internal_vector_t<object_state_t *> m_to_be_freed GUARDED_BY(m_mutex);
    };
  }
}
