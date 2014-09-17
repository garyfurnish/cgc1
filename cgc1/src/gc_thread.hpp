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
      void add_thread(::std::thread::id id) _Locks_Excluded_(m_mutex);
      void reset() _Locks_Excluded_(m_mutex);
      void set_allocator_blocks(gc_allocator_t::this_allocator_block_handle_t *begin,
                                gc_allocator_t::this_allocator_block_handle_t *end) _Locks_Excluded_(m_mutex);
      void set_root_iterators(void ***begin, void ***end) _Locks_Excluded_(m_mutex);
      void wake_up() _Locks_Excluded_(m_mutex);
      /**
      Start Clearing marks.
      **/
      void start_clear();
      /**
      Wait until clearing finished.
      **/
      void wait_until_clear_finished() _No_Thread_Safety_Analysis_;
      /**
      Start marking.
      **/
      void start_mark();
      /**
      Wait until mark phase finished.
      **/
      void wait_until_mark_finished() _No_Thread_Safety_Analysis_;
      /**
      Start sweeping.
      **/
      void start_sweep();
      /**
      Wait until sweep phase finished.
      **/
      void wait_until_sweep_finished() _No_Thread_Safety_Analysis_;
      /**
      Notify the thread that all threads have resumed from garbage collection and that it is safe to finalize.
      **/
      void notify_all_threads_resumed();
      /**
      Wait until finalization finishes.
      **/
      void wait_until_finalization_finished() _No_Thread_Safety_Analysis_;
      /**
      Returns true if finalization finished, false otherwise.
      **/
      bool finalization_finished() const;

    private:
      void _u_remove_thread(::std::thread::id id) _Exclusive_Locks_Required_(m_mutex);
      bool handle_thread(::std::thread::id id) _Exclusive_Locks_Required_(m_mutex);
      /**
      Inner thread loop.
      **/
      void _run();
      /**
      Clear marks.
      **/
      void _clear_marks() _Exclusive_Locks_Required_(m_mutex);
      void _mark() _Exclusive_Locks_Required_(m_mutex);
      /**
      Mark a given address.
      **/
      void _mark_addrs(void *addr, size_t depth, bool ignore_skip_marked = false) _Exclusive_Locks_Required_(m_mutex);
      /**
      Function called at end to mark any addresses that would have caused stack overflows.
      **/
      void _mark_mark_vector() _Exclusive_Locks_Required_(m_mutex);
      /**
      Sweep for unaccessible memory.
      **/
      void _sweep() _Exclusive_Locks_Required_(m_mutex);
      /**
      Finalize.
      **/
      void _finalize() _Exclusive_Locks_Required_(m_mutex);
      /**
      Mutex for condition variables and protection.
      **/
      mutex_t m_mutex;
      /**
      Vector of threads whose stack this thread is responsible for.
      **/
      cgc_internal_vector_t<::std::thread::id> m_watched_threads _Guarded_by_(m_mutex);
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
      gc_allocator_t::this_allocator_block_handle_t *m_block_begin _Guarded_by_(m_mutex);
      gc_allocator_t::this_allocator_block_handle_t *m_block_end _Guarded_by_(m_mutex);
      void ***m_root_begin _Guarded_by_(m_mutex);
      void ***m_root_end _Guarded_by_(m_mutex);
      /**
      Normally we recurse, but if the recursion is excessive, do it here instead.
      **/
      cgc_internal_vector_t<void *> m_addresses_to_mark _Guarded_by_(m_mutex);
      /**
      Roots from stack.
      **/
      cgc_internal_vector_t<uint8_t **> m_stack_roots _Guarded_by_(m_mutex);
      /**
      List of roots to be freed.
      **/
      cgc_internal_vector_t<object_state_t *> m_to_be_freed _Guarded_by_(m_mutex);
    };
  }
}
