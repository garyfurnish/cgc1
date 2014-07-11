#pragma once
#include "internal_declarations.hpp"
#include <atomic>
#include <condition_variable>
#include <vector>
#include <cgc1/concurrency.hpp>
#include <cgc1/cgc_internal_malloc_allocator.hpp>
#include "internal_allocator.hpp"
#include "allocator.hpp"
#include "gc_allocator.hpp"
#include "gc_thread.hpp"
#include "slab_allocator.hpp"
namespace cgc1
{
  namespace details
  {
    class thread_local_kernel_state_t;
    struct cgc_allocator_traits
    {
      do_nothing_t on_create_allocator_block;
      do_nothing_t on_destroy_allocator_block;
      template <typename Allocator>
      inline void on_creation(Allocator &a)
      {
        a.initialize(pow2(24), pow2(27));
      }
      using allocator_block_user_data_type = user_data_base_t;
    };
    class global_kernel_state_t
    {
    public:
      using cgc_internal_allocator_allocator_t = cgc_internal_slab_allocator_t<void>;
      using internal_allocator_t = allocator_t<cgc_internal_allocator_allocator_t, cgc_allocator_traits>;
      global_kernel_state_t();
      global_kernel_state_t(const global_kernel_state_t &) = delete;
      global_kernel_state_t(global_kernel_state_t &&) = delete;
      global_kernel_state_t &operator=(const global_kernel_state_t &) = delete;
      global_kernel_state_t &operator=(global_kernel_state_t &&) = delete;
      /**
      Perform shutdown work that must be done before destruction.
      **/
      void shutdown();
      /**
      Enable garbage collection.
      Works using a incremented variable.
      **/
      void enable();
      /**
      Disable garbage collection.
      Works using a incremented variable.
      **/
      void disable();
      /**
      Return if garbage collection is enabled.
      **/
      bool enabled() const;
      /**
      Hint to perform a garbage collection.
      **/
      void collect() _Locks_Excluded_(m_mutex, m_thread_mutex);
      /**
      Force a garbage collection.
      **/
      void force_collect() _Locks_Excluded_(m_mutex, m_thread_mutex);
      /**
      Return the number of collections that have happened.
      May wrap around.
      **/
      size_t num_collections() const;
      /**
      Initialize the current thread for garbage collection.
      @param top_of_stack Top of the current thread's stack.
      **/
      void initialize_current_thread(void *top_of_stack) _No_Thread_Safety_Analysis_; //_Locks_Excluded_(m_mutex);
      /**
      Destroy the current thread state for garbage collection.
      **/
      void destroy_current_thread() _No_Thread_Safety_Analysis_; //_Locks_Excluded_(m_mutex);
      /**
      Master collect function for a given thread.
      This calls into the thread kernel state's collect.
      **/
      void _collect_current_thread() _Locks_Excluded_(m_mutex);
      /**
      Return the GC allocator.
      **/
      gc_allocator_t &gc_allocator() const;
      /**
      Return the internal allocator that can be touched during GC.
      **/
      internal_allocator_t &_internal_allocator() const;
      /**
      Return the internal slab allocator.
      **/
      slab_allocator_t &_internal_slab_allocator() const;

      thread_local_kernel_state_t *tlks(::std::thread::id id);
      /**
      Add a root the global kernel state.
      **/
      void add_root(void **) _Locks_Excluded_(m_mutex) _No_Thread_Safety_Analysis_;
      /**
      Remove a root from the global kernel state.
      **/
      void remove_root(void **) _Locks_Excluded_(m_mutex) _No_Thread_Safety_Analysis_;
      /**
      Wait for finalization of the last collection to finish.
      **/
      void wait_for_finalization() _Locks_Excluded_(m_thread_mutex);
      /**
      Wait for ongoing collection to finish.
      **/
      void wait_for_collection() _Locks_Excluded_(m_mutex) _Exclusive_Lock_Function_(m_mutex) _No_Thread_Safety_Analysis_;
      /**
      Wait for ongoing collection to finish.
      **/
      void wait_for_collection2() _Exclusive_Lock_Function_(m_mutex, m_thread_mutex) _No_Thread_Safety_Analysis_;
      /**
      Add pointers that were freed in the last collection.
      **/
      template <typename Container>
      void _add_freed_in_last_collection(Container &container) _Locks_Excluded_(m_mutex);
      /**
      Return pointers that were freed in the last collection.
      In non-debug mode may be empty.
      **/
      cgc_internal_vector_t<void *> _d_freed_in_last_collection() const _Locks_Excluded_(m_mutex);
      /**
      Return true if the object state is valid, false otherwise.
      **/
      bool is_valid_object_state(const object_state_t* os) const;
      /**
      Find a valid object state for the given addr.
      Return nullptr if not found.
      **/
      object_state_t* _u_find_valid_object_state(void* addr) const _Exclusive_Locks_Required_(m_mutex);
      /**
      Find a valid object state for the given addr.
      Return nullptr if not found.
      **/
      object_state_t* find_valid_object_state(void* addr) const _Locks_Excluded_(m_mutex);

    private:
      /**
      Initialize the global kernel state.
      This may be called multiple times, but will be a nop if already called.
      **/
      void _u_initialize() _No_Thread_Safety_Analysis_; // _Exclusive_Locks_Required_(m_mutex);
      /**
      Pause all threads.
      Abort on error because usually these errors are unrecoverable.
      **/
      void _u_suspend_threads() _Exclusive_Locks_Required_(m_thread_mutex);
      /**
      Resume all threads.
      Abort on error because usually these errors are unrecoverable.
      **/
      void _u_resume_threads() _Exclusive_Locks_Required_(m_thread_mutex);
      /**
      Do per collection gc thread setup.
      **/
      void _u_setup_gc_threads() _Exclusive_Locks_Required_(m_mutex);
      /**
      Do GC setup.
      Clean all existing marks.
      **/
      void _u_gc_setup() _Exclusive_Locks_Required_(m_mutex);
      /**
      Do GC mark phase.
      **/
      void _u_gc_mark() _Exclusive_Locks_Required_(m_mutex);
      /**
      Do GC sweep phase.
      **/
      void _u_gc_sweep() _Exclusive_Locks_Required_(m_mutex);
      /**
      Start finalization phase for gc.
      **/
      void _u_gc_finalize() _Exclusive_Locks_Required_(m_mutex);
      /**
      Internal slab allocator used for internal allocator.
      **/
      mutable slab_allocator_t m_slab_allocator;
      /**
      Internal allocator that can be touched during GC.
      **/
      mutable internal_allocator_t m_cgc_allocator;
      /**
      Allocator for gc.
      **/
      mutable gc_allocator_t m_gc_allocator;
      mutable mutex_t m_mutex;
      /**
      Number of paused threads in a collect cycle.
      **/
      std::atomic<size_t> m_num_paused_threads; //_Guarded_by_(m_mutex)
      /**
      Number of paused threads that have finished garbage collection.
      **/
      std::atomic<size_t> m_num_resumed_threads; //_Guarded_by_(m_mutex)
      /**
      Number of collections that have happened.
      May wrap around.
      **/
      mutable ::std::atomic<size_t> m_num_collections;
#ifndef _WIN32
      /**
      Condition variable used to broadcast
      1) When a thread has stopped and is in the signal handler.
      2) When a thread should resume normal operations.
      **/
      mutable ::std::condition_variable_any m_stop_world_condition;
      /**
      Condition variable used to broadcast when threads should start garbage collection.
      **/
      mutable ::std::condition_variable_any m_start_world_condition;
#endif
      /**
      Mutex for protecting m_threads.
      **/
      mutable mutex_t m_thread_mutex;
      mutable mutex_t m_block_mutex;
      rebind_vector_t<void **, cgc_internal_malloc_allocator_t<void>> m_roots _Guarded_by_(m_mutex);
      /**
      Vector of all threads registered with the kernel.
      **/
      cgc_internal_vector_t<details::thread_local_kernel_state_t *> m_threads _Guarded_by_(m_thread_mutex);
      /**
      Threads that do the actual garbage collection.
      Not necesarily a one to one map.
      **/
      rebind_vector_t<::std::unique_ptr<gc_thread_t>, cgc_internal_malloc_allocator_t<void>> m_gc_threads;
      /**
      List of pointers freed in last collection.
      **/
      cgc_internal_vector_t<void *> m_freed_in_last_collection _Guarded_by_(m_mutex);
      /**
      True while a garbage collection is running, otherwise false.
      **/
      std::atomic<bool> m_collect;
      /**
      Increment variable for enabling or disabling
      **/
      std::atomic<long> m_enabled_count;
      /**
      True if the kernel has been initialized, false otherwise.
      **/
      bool m_initialized _Guarded_by_(m_mutex) = false;
    };
    extern global_kernel_state_t g_gks;
  }
}
#include "global_kernel_state_impl.hpp"
