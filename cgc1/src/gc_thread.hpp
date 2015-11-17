#pragma once
#include "internal_declarations.hpp"
#include <mutex>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <mcppalloc/mcppalloc_utils/concurrency.hpp>
#include <cgc1/allocated_thread.hpp>
#include <cgc1/cgc_internal_malloc_allocator.hpp>
#include <mcppalloc/mcppalloc_utils/boost/container/flat_set.hpp>
#include <mcppalloc/mcppalloc_sparse/allocator.hpp>
#include "internal_allocator.hpp"
#include "gc_allocator.hpp"
namespace cgc1
{
  namespace details
  {
    /**
     * \brief Internal thread that performs garbage collection.
     *
     * Multiple of these can run to make gc faster.
    **/
    class gc_thread_t
    {
    public:
      /**
       * \brief Constructor.
       **/
      gc_thread_t();
      gc_thread_t(const gc_thread_t &) = delete;
      gc_thread_t(gc_thread_t &&) = delete;
      gc_thread_t &operator=(const gc_thread_t &) = delete;
      gc_thread_t &operator=(gc_thread_t &&) = delete;
      /**
       * \brief Destructor.
       **/
      ~gc_thread_t();
      /**
       * \brief Explicit shutdown.
       **/
      void shutdown() REQUIRES(!m_mutex);
      /**
       * \brief Reset all GC thread state.
       **/
      void reset() REQUIRES(!m_mutex);
      /**
       * \brief Make this gc_thread responsible for a given thread.
       *
       * This gets called every garbage collection cycle.
       **/
      void add_thread(::std::thread::id id) REQUIRES(!m_mutex);
      /**
       * \brief Set the allocator blocks that this thread is responsible for marking.
       **/
      void set_allocator_blocks(gc_allocator_t::this_allocator_block_handle_t *begin,
                                gc_allocator_t::this_allocator_block_handle_t *end) REQUIRES(!m_mutex);
      /**
       * \brief Set the root pointers that this thread is responsible for marking.
       *
       * Note this takes a void***, that is an iterator to a pointer to an unknown memory location.
       **/
      void set_root_iterators(void ***begin, void ***end) REQUIRES(!m_mutex);
      /**
       * \brief Wake up thread from sleeping.
       **/
      void wake_up() REQUIRES(!m_mutex);
      /**
       * \brief Start Clearing marks.
      **/
      void start_clear() REQUIRES(!m_mutex);
      /**
       * \brief Wait until clearing finished.
      **/
      void wait_until_clear_finished();
      /**
       * \brief Start marking.
      **/
      void start_mark();
      /**
       * \brief Wait until mark phase finished.
      **/
      void wait_until_mark_finished();
      /**
       * \brief Start sweeping.
      **/
      void start_sweep();
      /**
       * \brief Wait until sweep phase finished.
      **/
      void wait_until_sweep_finished();
      /**
       * \brief Notify the thread that all threads have resumed from garbage collection and that it is safe to finalize.
      **/
      void notify_all_threads_resumed();
      /**
       * \brief Wait until finalization finishes.
      **/
      void wait_until_finalization_finished();
      /**
       * \brief Returns true if finalization finished, false otherwise.
      **/
      bool finalization_finished() const;

    private:
      /**
       * \brief This handles setting up/marking per thread data.
       *
       * This handles stack/registers.
       **/
      bool handle_thread(::std::thread::id id) REQUIRES(m_mutex);
      /**
       * \brief Inner thread loop.
      **/
      void _run() REQUIRES(!m_mutex);
      /**
       * \brief Clear marks of all blocks assigned to this thread.
      **/
      void _clear_marks() REQUIRES(m_mutex);
      /**
       * \brief Mark all roots and additional vector of stuff to mark.
       **/
      void _mark() REQUIRES(m_mutex);
      /**
       * \brief Mark a given address.
       *
       * @param addr Address to mark.
       * @param depth Current depth for controlling infinite recursion.
       * @param ignore_skip_marked Remark even if marked if true.
      **/
      void _mark_addrs(void *addr, size_t depth, bool ignore_skip_marked = false) REQUIRES(m_mutex);
      /**
       * \brief Function called at end to mark any addresses that would have caused stack overflows.
       *
       * This is how infinite recursion is prevented.
      **/
      void _mark_mark_vector() REQUIRES(m_mutex);
      /**
       * \brief Sweep for unaccessible memory.
      **/
      void _sweep() REQUIRES(m_mutex);
      /**
       * \brief Finalize sweeped objects.
      **/
      void _finalize() REQUIRES(m_mutex);
      /**
       * \brief Mutex for condition variables and protection.
      **/
      ::mcppalloc::mutex_t m_mutex;
      /**
       * \brief Vector of threads whose stack this thread is responsible for.
      **/
      cgc_internal_vector_t<::std::thread::id> m_watched_threads GUARDED_BY(m_mutex);
      /**
       * \brief Variable for waking up.
      **/
      condition_variable_any_t m_wake_up;
      /**
       * \brief Variable for done with clearing.
      **/
      condition_variable_any_t m_done_clearing;
      /**
       * \brief Variable for start marking.
      **/
      condition_variable_any_t m_start_mark;
      /**
       * \brief Variable for done marking.
      **/
      condition_variable_any_t m_done_mark;
      /**
       * \brief Variable for starting sweeping.
      **/
      condition_variable_any_t m_start_sweep;
      /**
       * \brief Variable for when done with collection.
      **/
      condition_variable_any_t m_done_sweep;
      /**
       * \brief Variable for all threads resumed.
      **/
      condition_variable_any_t m_all_threads_resumed;
      /**
       * \brief Variable for finalization done.
      **/
      condition_variable_any_t m_done_finalization;
      /**
       * \brief Thread that this runs in.
      **/
      allocated_thread_t<cgc_internal_malloc_allocator_t<void>> m_thread;
      /**
       * \brief State variables.
       **/
      ::std::atomic<bool> m_clear_done, m_mark_done, m_sweep_done, m_finalization_done;
      /**
       * \brief Should the gc thread keep running.
       **/
      ::std::atomic<bool> m_run;
      /**
       * \brief State variables.
       **/
      ::std::atomic<bool> m_do_clear, m_do_mark, m_do_sweep, m_do_all_threads_resumed;
      /**
       * \brief Start block iterator.
       **/
      gc_allocator_t::this_allocator_block_handle_t *m_block_begin GUARDED_BY(m_mutex);
      /**
       * \brief End block iterator.
       **/
      gc_allocator_t::this_allocator_block_handle_t *m_block_end GUARDED_BY(m_mutex);
      /**
       * \brief Start root iterator.
       **/
      void ***m_root_begin GUARDED_BY(m_mutex);
      /**
       * \brief End root iterator.
       **/
      void ***m_root_end GUARDED_BY(m_mutex);
      /**
       * \brief Hold addresses to mark that would have otherwise caused excessive recursion.
      **/
      ::boost::container::flat_set<void *, ::std::less<>, cgc_internal_malloc_allocator_t<void *>>
          m_addresses_to_mark GUARDED_BY(m_mutex);
      /**
       * \brief Roots from stack.
      **/
      cgc_internal_vector_t<uint8_t **> m_stack_roots GUARDED_BY(m_mutex);
      /**
       * \brief List of objects to be freed.
      **/
      cgc_internal_vector_t<gc_sparse_object_state_t *> m_to_be_freed GUARDED_BY(m_mutex);
    };
  }
}
