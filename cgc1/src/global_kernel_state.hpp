#pragma once
#include "gc_allocator.hpp"
#include "gc_thread.hpp"
#include "global_kernel_state_param.hpp"
#include "internal_allocator.hpp"
#include "internal_declarations.hpp"
#include "root_collection.hpp"
#include <atomic>
#include <cgc1/cgc_internal_malloc_allocator.hpp>
#include <condition_variable>
#include <mcppalloc/mcppalloc_bitmap_allocator/bitmap_allocator.hpp>
#include <mcppalloc/mcppalloc_slab_allocator/slab_allocator.hpp>
#include <mcppalloc/mcppalloc_sparse/allocator.hpp>
#include <mcpputil/mcpputil/concurrency.hpp>
#include <vector>

#include <boost/property_tree/ptree_fwd.hpp>
namespace cgc1::details
{
  class thread_local_kernel_state_t;
  /**
   * \brief Class that encapsulates all garbage collection state.
   **/
  class global_kernel_state_t
  {
  public:
    /**
     * \brief Lock that waits for collection safety.
     **/
    class collection_lock_t
    {
    public:
      void lock() const ACQUIRE() NO_THREAD_SAFETY_ANALYSIS;
      void unlock() const RELEASE() NO_THREAD_SAFETY_ANALYSIS;
    };
    using cgc_internal_allocator_allocator_t = cgc_internal_slab_allocator_t<void>;
    using internal_allocator_policy_type = ::mcppalloc::default_allocator_policy_t<cgc_internal_allocator_allocator_t>;
    using internal_allocator_t = ::mcppalloc::sparse::allocator_t<internal_allocator_policy_type>;
    using bitmap_allocator_type = ::mcppalloc::bitmap_allocator::bitmap_allocator_t<gc_bitmap_allocator_policy_t>;
    using internal_slab_allocator_type = mcppalloc::slab_allocator::details::slab_allocator_t;
    using duration_type = ::std::chrono::duration<double>;
#ifdef __APPLE__
    using mutex_type = ::mcpputil::spinlock_t;
#else
    using mutex_type = ::mcpputil::mutex_t;
#endif
    /**
     * \brief Constructor
     * @param param Initialization Parameters.
     **/
    global_kernel_state_t(const global_kernel_state_param_t &param);
    global_kernel_state_t(const global_kernel_state_t &) = delete;
    global_kernel_state_t(global_kernel_state_t &&) = delete;
    global_kernel_state_t &operator=(const global_kernel_state_t &) = delete;
    global_kernel_state_t &operator=(global_kernel_state_t &&) = delete;
    ~global_kernel_state_t();
    /**
     * \brief Perform initialization work that can not be performed in constructor.
     **/
    void initialize() REQUIRES(!m_mutex, !m_thread_mutex);
    /**
     * \brief Perform shutdown work that must be done before destruction.
    **/
    void shutdown() REQUIRES(!m_mutex, !m_thread_mutex);
    /**
     * \brief Enable garbage collection.
     *
     * Works using a incremented variable.
    **/
    void enable();
    /**
     * \brief Disable garbage collection.
     *
     * Works using a incremented variable.
    **/
    void disable();
    /**
     * \brief Return if garbage collection is enabled.
    **/
    bool enabled() const;
    /**
     * \brief Hint to perform a garbage collection.
    **/
    void collect() REQUIRES(!m_mutex, !m_thread_mutex, !m_allocators_unavailable_mutex, !m_start_world_condition_mutex);
    /**
     * \brief Force a garbage collection.
     *
     * @param do_local_finalization Should the thread do local finalization.
    **/
    void force_collect(bool do_local_finalization = true)
        REQUIRES(!m_mutex, !m_thread_mutex, !m_allocators_unavailable_mutex, !m_start_world_condition_mutex);
    /**
     * \brief Return the number of collections that have happened.
     *
     * May wrap around.
    **/
    size_t num_collections() const;
    /**
     * \brief Initialize the current thread for garbage collection.
     *
     * @param top_of_stack Top of the current thread's stack.
    **/
    void initialize_current_thread(void *top_of_stack) REQUIRES(!m_mutex, !m_thread_mutex);
    /**
     * Destroy the current thread state for garbage collection.
    **/
    void destroy_current_thread() REQUIRES(!m_mutex, !m_thread_mutex);
    /**
     * \brief Master collect function for a given thread.
     *
     * This calls into the thread kernel state's collect.
    **/
    void _collect_current_thread() REQUIRES(!m_mutex, !m_thread_mutex, !m_allocators_unavailable_mutex);
    /**
     * \brief Return the GC allocator.
    **/
    gc_allocator_t &gc_allocator() const noexcept;
    /**
     * \brief Return the packed object allocator.
     **/
    auto _bitmap_allocator() const noexcept -> bitmap_allocator_type &;
    /**
     * \brief Return the internal allocator that can be touched during GC.
    **/
    internal_allocator_t &_internal_allocator() const noexcept;
    /**
     * \brief Return the internal slab allocator.
    **/
    auto _internal_slab_allocator() const noexcept -> internal_slab_allocator_type &;
    /**
     * \brief Return the thread local kernel state for the current thread.
     *
     * @return nullptr on error.
     **/
    thread_local_kernel_state_t *tlks(::std::thread::id id) REQUIRES(!m_thread_mutex);

    auto allocate(size_t sz) -> details::gc_allocator_t::block_type;
    auto allocate_atomic(size_t sz) -> details::gc_allocator_t::block_type;
    auto allocate_raw(size_t sz) -> details::gc_allocator_t::block_type;
    auto allocate_sparse(size_t sz) -> details::gc_allocator_t::block_type;
    void deallocate(void *v);

    auto root_collection();
    auto root_collection() const;
    /**
     * \brief Wait for finalization of the last collection to finish.
    **/
    void wait_for_finalization(bool do_local_finalization = true) REQUIRES(!m_mutex, !m_thread_mutex);
    /**
     * \brief Attempt to do some GKS finalization in this thread.
     **/
    void local_thread_finalization() REQUIRES(!m_mutex);
    /**
     * \brief Wait for ongoing collection to finish.
     *
     * Waits on main mutex only.
     * Used for things such as adding roots.
     * Returns with lock on mutex held.
    **/
    void wait_for_collection() REQUIRES(!m_mutex) ACQUIRE(m_mutex) NO_THREAD_SAFETY_ANALYSIS;
    /**
     * \brief Wait for ongoing collection to finish.
     *
     * Waits for both main mutex and thread mutex.
     * Used when need to alter thread data.
     * Returns with lock on mutex and thread mutex held.
    **/
    void wait_for_collection2() ACQUIRE(m_mutex, m_thread_mutex) NO_THREAD_SAFETY_ANALYSIS;
    /**
     * \brief Add object states that were freed in the last collection.
    **/
    template <typename Container>
    void _add_freed_in_last_collection(Container &container) REQUIRES(!m_mutex);
    /**
     * \brief Add object states that need special finalizing.
     **/
    template <typename Container>
    void _add_need_special_finalizing_collection(Container &container) REQUIRES(!m_mutex);
    /**
     * \brief Add number of pointers that were freed in last collection.
     *
     * This is always valid in both release and debug modes.
     **/
    void _add_num_freed_in_last_collection(size_t num_freed) noexcept;
    /**
     * \brief Return number of pointers that were freed in last collection.
     *
     * This is always valid in both release and debug modes.
     **/
    size_t num_freed_in_last_collection() const noexcept;
    /**
     * \brief Return pointers that were freed in the last collection.
     *
     * In non-debug mode may be empty.
     * Returns hidden pointers.
    **/
    cgc_internal_vector_t<uintptr_t> _d_freed_in_last_collection() const REQUIRES(!m_mutex);
    /**
     * \brief Return true if the object state is valid, false otherwise.
    **/
    bool is_valid_object_state(const gc_sparse_object_state_t *os) const;
    /**
     * \brief Find a valid object state for the given addr.
     *
     * This does not need a mutex held if this is called during garbage collection because all of the relevant state is frozen.
     * @return nullptr if not found.
    **/
    gc_sparse_object_state_t *_u_find_valid_object_state(void *addr) const REQUIRES(m_mutex, gc_allocator()._mutex());
    /**
     * \brief Find a valid object state for the given addr.
     *
     * @return nullptr if not found.
    **/
    gc_sparse_object_state_t *find_valid_object_state(void *addr) const REQUIRES(!m_mutex);

    /**
     * \brief Return time for clear phase of gc.
     **/
    auto clear_mark_time_span() const -> duration_type;
    /**
     * \brief Return time for mark phase of gc.
     **/
    auto mark_time_span() const -> duration_type;
    /**
     * \brief Return time for sweep phase of gc.
     **/
    auto sweep_time_span() const -> duration_type;
    /**
     * \brief Return time for notify phase of gc.
     **/
    auto notify_time_span() const -> duration_type;
    /**
     * \brief Return total gc collect time.
     **/
    auto total_collect_time_span() const -> duration_type;

    RETURN_CAPABILITY(m_mutex) auto _mutex() const -> mutex_type &;

    /**
     * \brief Return reference to cached initialization parameters.
     **/
    auto initialization_parameters_ref() const noexcept -> const global_kernel_state_param_t &;

    /**
     * \brief Wait for collection safe point.
     **/
    static const constexpr collection_lock_t sc_collection_lock;

  private:
    /**
     * \brief Initialize the global kernel state.
     *
    * This may be called multiple times, but will be a nop if already called.
    **/
    void _u_initialize() REQUIRES(m_mutex);
    /**
     * \brief Pause all threads.
     *
     * Abort on error because usually these errors are unrecoverable.
    **/
    void _u_suspend_threads() REQUIRES(m_thread_mutex);
    /**
     * \brief Resume all threads.
     *
     * Abort on error because usually these errors are unrecoverable.
    **/
    void _u_resume_threads() REQUIRES(m_thread_mutex);
    /**
     * \brief Do per collection gc thread setup.
     *
     * When called during collection, does not require m_thread_mutex as that data is frozen.
    **/
    void _u_setup_gc_threads() REQUIRES(m_mutex, m_thread_mutex);
    /**
     * \brief Get a vector of sparse states that need to be finalized by this thread.
     **/
    REQUIRES(m_mutex) auto _u_get_local_finalization_vector_sparse() -> cgc_internal_vector_t<gc_sparse_object_state_t *>;
    /**
     * \brief Internal slab allocator used for internal allocator.
    **/
    mutable internal_slab_allocator_type m_slab_allocator;
    /**
     * \brief Internal allocator that can be touched during GC.
    **/
    mutable internal_allocator_t m_cgc_allocator;

    /**
     * \brief Sparse allocator for gc.
    **/
    mutable gc_allocator_t m_gc_allocator;
    /**
     * \brief Packed object allocator for fast allocation.
     **/
    mutable bitmap_allocator_type m_bitmap_allocator;
    /**
     * \brief Main mutex for state.
     **/
    mutable mutex_type m_mutex;
    /**
     * \brief Mutex for resuming world.
     *
     **/
    mutable ::mcpputil::mutex_t m_start_world_condition_mutex;
    /**
     * Number of paused threads in a collect cycle.
    **/
    std::atomic<size_t> m_num_paused_threads;
    /**
     * Number of paused threads that have finished garbage collection.
     **/
    std::atomic<size_t> m_num_resumed_threads;
    /**
     * \brief Number of collections that have happened.
     * May wrap around.
    **/
    mutable ::std::atomic<size_t> m_num_collections{0};
#ifndef _WIN32
    /**
     * \brief Condition variable used to broadcast
     *
     * 1) When a thread has stopped and is in the signal handler.
     * 2) When a thread should resume normal operations.
    **/
    //      mutable condition_variable_any_t m_stop_world_condition;
    /**
     * \brief Condition variable used to broadcast when threads should start garbage collection.
    **/
    mutable condition_variable_any_t m_start_world_condition;
#endif
    /**
     * \brief Mutex for protecting m_threads.
    **/
    mutable mutex_type m_thread_mutex;
    /**
     * \brief This mutex is locked when mutexes are unavailable.
     **/
    mutable ::mcpputil::mutex_t m_allocators_unavailable_mutex;
    using root_collection_policy_type =
        default_root_collection_policy_t<decltype(sc_collection_lock), cgc_internal_malloc_allocator_t<void **>>;
    mutable root_collection_t<root_collection_policy_type> m_roots{sc_collection_lock,
                                                                   cgc_internal_malloc_allocator_t<void **>()};
    /**
     * \brief Vector of all threads registered with the kernel.
     *
     * This is a vector for cache locality.
    **/
    cgc_internal_vector_t<details::thread_local_kernel_state_t *> m_threads GUARDED_BY(m_thread_mutex);
    /**
     * \brief Threads that do the actual garbage collection.
     *
     * Not necesarily a one to one map.
    **/
    ::mcpputil::rebind_vector_t<::std::unique_ptr<gc_thread_t, cgc_internal_malloc_deleter_t>,
                                cgc_internal_malloc_allocator_t<void>>
        m_gc_threads;
    /**
     * \brief List of pointers freed in last collection.
     *
     * In debug mode this is all pointers.
     * Otherwise this is just ones to be finalized.
    **/
    cgc_internal_vector_t<uintptr_t> m_freed_in_last_collection GUARDED_BY(m_mutex);
    /**
     * \brief List of pointers that need special finalizing.
     *
     * The idea is that the finalizers for these need to run in a special thread(s).
     **/
    cgc_internal_vector_t<gc_sparse_object_state_t *> m_need_special_finalizing_collection_sparse GUARDED_BY(m_mutex);
    /**
     * \brief True while a garbage collection is running or trying to run, otherwise false.
    **/
    ::std::atomic<bool> m_collect{false};
    /**
     * \brief True if last GC is finished, false otherwise.
     **/
    ::std::atomic<bool> m_collect_finished{true};
    /**
     * \brief Increment variable for enabling or disabling
    **/
    ::std::atomic<long> m_enabled_count{1};
    /**
     * \brief True if the kernel has been initialized, false otherwise.
    **/
    bool m_initialized GUARDED_BY(m_mutex) = false;
    //
    //
    // Debug information under here.
    //
    //
    /**
     * \brief Absolute number of pointers freed in last collection.
     **/
    ::std::atomic<size_t> m_num_freed_in_last_collection{0};
    /**
     * \brief Time for clear phase of gc.
     **/
    duration_type m_clear_mark_time_span = duration_type::zero();
    /**
     * \brief Time for mark phase of gc.
     **/
    duration_type m_mark_time_span = duration_type::zero();
    /**
     * \brief Time for sweep phase of gc.
     **/
    duration_type m_sweep_time_span = duration_type::zero();
    /**
     * \brief Time for notify phase of gc.
     **/
    duration_type m_notify_time_span = duration_type::zero();
    /**
     * \brief Total gc collect time.
     **/
    duration_type m_total_collect_time_span = duration_type::zero();
    /**
     * \brief Saved initialization parameters.
     **/
    const global_kernel_state_param_t m_initialization_parameters;
    /**
     * \brief Has initialize been called yet?
     **/
    bool m_initialize_called{false};

  public:
    ::std::atomic<bool> m_in_destructor{false};

  private:
  };
  extern global_kernel_state_t *g_gks;
  extern auto _real_gks() -> global_kernel_state_t *;
}
#include "global_kernel_state_impl.hpp"
