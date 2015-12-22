#include "new.hpp"
#include "internal_declarations.hpp"
#include <cgc1/declarations.hpp>
#include <algorithm>
#include <signal.h>
#include <cgc1/posix.hpp>
#include <cgc1/cgc1.hpp>
#include <mcppalloc/mcppalloc_utils/concurrency.hpp>
#include <mcppalloc/mcppalloc_utils/aligned_allocator.hpp>
#include "global_kernel_state.hpp"
#include "thread_local_kernel_state.hpp"
#include <chrono>
#include <iostream>
#include <mcppalloc/mcppalloc_utils/boost/property_tree/ptree.hpp>
#include <mcppalloc/mcppalloc_utils/boost/property_tree/json_parser.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

template <>
cgc1::details::gc_user_data_t cgc1::details::gc_allocator_t::allocator_block_type::s_default_user_data{};
template <>
::mcppalloc::details::user_data_base_t mcppalloc::sparse::details::allocator_block_t<
    mcppalloc::default_allocator_policy_t<cgc1::cgc_internal_slab_allocator_t<void>>>::s_default_user_data{};

#ifdef __APPLE__
template <>
pthread_key_t mcppalloc::thread_local_pointer_t<
    mcppalloc::bitmap_allocator::details::bitmap_thread_allocator_t<::cgc1::details::gc_allocator_policy_t>>::s_pkey{0};
#else
template <>
thread_local mcppalloc::thread_local_pointer_t<
    mcppalloc::bitmap_allocator::details::bitmap_thread_allocator_t<::cgc1::details::gc_allocator_policy_t>>::pointer_type
    mcppalloc::thread_local_pointer_t<
        mcppalloc::bitmap_allocator::details::bitmap_thread_allocator_t<::cgc1::details::gc_allocator_policy_t>>::s_tlks =
        nullptr;
#endif
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {
      template <>
      thread_local_pointer_t<typename bitmap_allocator_t<cgc1::details::gc_allocator_policy_t>::thread_allocator_type>
          bitmap_allocator_t<cgc1::details::gc_allocator_policy_t>::t_thread_allocator{};
    }
  }
}

namespace cgc1
{
  namespace details
  {
    auto _real_gks() -> global_kernel_state_t *
    {
      static global_kernel_state_t *gks = nullptr;
      if (mcppalloc_likely(g_gks))
        gks = g_gks.get();
      else {
        if (mcppalloc_unlikely(!gks->m_in_destructor))
          ::std::terminate();
      }
      return gks;
    }
    void *internal_allocate(size_t n)
    {
      auto gks = _real_gks();
      return gks->_internal_allocator().initialize_thread().allocate(n).m_ptr;
    }
    void internal_deallocate(void *p)
    {
      auto gks = _real_gks();
      gks->_internal_allocator().initialize_thread().destroy(p);
    }
    void *internal_slab_allocate(size_t n)
    {
      auto gks = _real_gks();
      return gks->_internal_slab_allocator().allocate_raw(n);
    }
    void internal_slab_deallocate(void *p)
    {
      auto gks = _real_gks();
      gks->_internal_slab_allocator().deallocate_raw(p);
    }
    global_kernel_state_t::global_kernel_state_t(const global_kernel_state_param_t &param)
        : m_slab_allocator(param.slab_allocator_start_size(), param.slab_allocator_expansion_size()),
          m_bitmap_allocator(param.packed_allocator_start_size(), param.packed_allocator_expansion_size()),
          m_initialization_parameters(param)
    {
      m_cgc_allocator.initialize(param.internal_allocator_start_size(), param.internal_allocator_expansion_size());
      details::initialize_tlks();
    }
    struct shutdown_ptr_functional_t {
      template <typename T>
      void operator()(T &&t) const
      {
        t->shutdown();
      }
    };
    static const auto shutdown_ptr_functional = shutdown_ptr_functional_t{};
    global_kernel_state_t::~global_kernel_state_t()
    {
      m_in_destructor = true;
      ::std::for_each(m_gc_threads.begin(), m_gc_threads.end(), shutdown_ptr_functional);
      m_bitmap_allocator.shutdown();
      m_gc_allocator.shutdown();
      {
        m_gc_threads.clear();
        auto a1 = ::std::move(m_gc_threads);
        m_roots.clear();
        auto a2 = ::std::move(m_roots);
        m_threads.clear();
        auto a3 = ::std::move(m_threads);
        m_freed_in_last_collection.clear();
        auto a4 = ::std::move(m_freed_in_last_collection);
      }
      m_cgc_allocator.shutdown();
      assert(!m_gc_threads.capacity());
      assert(!m_roots.capacity());
      assert(!m_threads.capacity());
      assert(!m_freed_in_last_collection.capacity());
      m_in_destructor = false;
    }
    void global_kernel_state_t::shutdown()
    {
      disable();
      wait_for_finalization();
      mcppalloc::double_lock_t<decltype(m_mutex), decltype(m_thread_mutex)> guard(m_mutex, m_thread_mutex);
      m_gc_threads.clear();
    }
    void global_kernel_state_t::enable()
    {
      m_enabled_count++;
    }
    void global_kernel_state_t::disable()
    {
      m_enabled_count--;
    }
    bool global_kernel_state_t::enabled() const
    {
      return m_enabled_count > 0;
    }
    auto global_kernel_state_t::initialization_parameters_ref() const noexcept -> const global_kernel_state_param_t &
    {
      return m_initialization_parameters;
    }
    auto global_kernel_state_t::clear_mark_time_span() const -> duration_type
    {
      return m_clear_mark_time_span;
    }
    auto global_kernel_state_t::mark_time_span() const -> duration_type
    {
      return m_mark_time_span;
    }
    auto global_kernel_state_t::sweep_time_span() const -> duration_type
    {
      return m_sweep_time_span;
    }
    auto global_kernel_state_t::notify_time_span() const -> duration_type
    {
      return m_notify_time_span;
    }
    auto global_kernel_state_t::total_collect_time_span() const -> duration_type
    {
      return m_total_collect_time_span;
    }

    void global_kernel_state_t::to_ptree(::boost::property_tree::ptree &ptree, int level) const
    {
      (void)level;
      {
        ::boost::property_tree::ptree init;
        initialization_parameters_ref().to_ptree(init);
        ptree.put_child("initialization", init);
      }
      {
        ::boost::property_tree::ptree last_collect;
        last_collect.put("clear_mark_time", ::std::to_string(clear_mark_time_span().count()));
        last_collect.put("mark_time", ::std::to_string(mark_time_span().count()));
        last_collect.put("sweep_time", ::std::to_string(sweep_time_span().count()));
        last_collect.put("notify_time", ::std::to_string(notify_time_span().count()));
        last_collect.put("total_time", ::std::to_string(total_collect_time_span().count()));
        ptree.put_child("last_collect", last_collect);
      }
      {
        ::boost::property_tree::ptree slab_allocator;
        m_slab_allocator.to_ptree(slab_allocator, level);
        ptree.put_child("slab_allocator", slab_allocator);
      }
      {
        ::boost::property_tree::ptree cgc_allocator;
        m_cgc_allocator.to_ptree(cgc_allocator, level);
        ptree.put_child("cgc_allocator", cgc_allocator);
      }
      {
        ::boost::property_tree::ptree gc_allocator;
        m_gc_allocator.to_ptree(gc_allocator, level);
        ptree.put_child("gc_allocator", gc_allocator);
      }
      {
        ::boost::property_tree::ptree packed_allocator;
        m_bitmap_allocator.to_ptree(packed_allocator, level);
        ptree.put_child("packed_allocator", packed_allocator);
      }
    }
    auto global_kernel_state_t::to_json(int level) const -> ::std::string
    {
      ::std::stringstream ss;
      ::boost::property_tree::ptree ptree;
      to_ptree(ptree, level);
      ::boost::property_tree::json_parser::write_json(ss, ptree);
      return ss.str();
    }

    gc_sparse_object_state_t *global_kernel_state_t::_u_find_valid_object_state(void *addr) const
    {
      // get handle for block.
      const auto handle = gc_allocator()._u_find_block(addr);
      if (!handle)
        return nullptr;
      // find state for address.
      gc_sparse_object_state_t *const os = handle->m_block->find_address(addr);
      // make sure is in valid and in use.
      if (!os || !os->in_use())
        return nullptr;
      return os;
    }
    gc_sparse_object_state_t *global_kernel_state_t::find_valid_object_state(void *addr) const
    {
      MCPPALLOC_CONCURRENCY_LOCK2_GUARD(m_mutex, gc_allocator()._mutex());
      // forward to thread unsafe version.
      return _u_find_valid_object_state(addr);
    }
    auto global_kernel_state_t::allocate(size_t sz) -> details::gc_allocator_t::block_type
    {
      auto &tlks = *details::get_tlks();
      // check to see if size with user data fits in a bin.
      const auto size_with_user_data =
          ::mcppalloc::align(sz, sizeof(::mcppalloc::details::user_data_alignment_t)) + sizeof(gc_user_data_t);

      if (::mcppalloc::bitmap_allocator::details::fits_in_bins(size_with_user_data)) {
        auto &bitmap_allocator = *tlks.bitmap_thread_allocator();
        return bitmap_allocator.allocate(size_with_user_data, 2);
      }
      else {
        auto &sparse_allocator = *tlks.thread_allocator();
        return sparse_allocator.allocate(sz);
      }
    }
    auto global_kernel_state_t::allocate_atomic(size_t sz) -> details::gc_allocator_t::block_type
    {
      auto &tlks = *details::get_tlks();
      if (::mcppalloc::bitmap_allocator::details::fits_in_bins(sz)) {
        auto &bitmap_allocator = *tlks.bitmap_thread_allocator();
        return bitmap_allocator.allocate(sz, 1);
      }
      else {
        auto &sparse_allocator = *tlks.thread_allocator();
        const auto allocation = sparse_allocator.allocate_detailed(sz);
        ::cgc1::details::set_atomic(get_allocation_object_state(allocation), true);
        return ::std::get<0>(allocation);
      }
    }
    auto global_kernel_state_t::allocate_raw(size_t sz) -> details::gc_allocator_t::block_type
    {
      auto &tlks = *details::get_tlks();
      if (::mcppalloc::bitmap_allocator::details::fits_in_bins(sz)) {
        auto &bitmap_allocator = *tlks.bitmap_thread_allocator();
        return bitmap_allocator.allocate(sz, 0);
      }
      else {
        auto &sparse_allocator = *tlks.thread_allocator();
        return sparse_allocator.allocate(sz);
      }
    }

    auto global_kernel_state_t::allocate_sparse(size_t sz) -> details::gc_allocator_t::block_type
    {
      auto &tlks = *details::get_tlks();
      auto &sparse_allocator = *tlks.thread_allocator();
      return sparse_allocator.allocate(sz);
    }
    void global_kernel_state_t::deallocate(void *v)
    {
      auto &tlks = *details::get_tlks();
      auto &sparse_allocator = *tlks.thread_allocator();
      auto &bitmap_allocator = *tlks.bitmap_thread_allocator();
      if (!bitmap_allocator.deallocate(v))
        sparse_allocator.deallocate(v);
    }

    void global_kernel_state_t::add_root(void **r)
    {
      wait_for_collection();
      MCPPALLOC_CONCURRENCY_LOCK_GUARD_TAKE(m_mutex);
      // make sure not already a root.
      const auto it = find(m_roots.begin(), m_roots.end(), r);
      if (it == m_roots.end())
        m_roots.push_back(r);
    }
    void global_kernel_state_t::remove_root(void **r)
    {
      wait_for_collection();
      MCPPALLOC_CONCURRENCY_LOCK_GUARD_TAKE(m_mutex);
      // find root and erase.
      auto it = find(m_roots.begin(), m_roots.end(), r);
      if (it != m_roots.end())
        m_roots.erase(it);
    }
    size_t global_kernel_state_t::num_collections() const
    {
      return m_num_collections;
    }
    void global_kernel_state_t::_u_setup_gc_threads()
    {
      // if no gc threads, trivially done.
      if (m_gc_threads.empty())
        return;
      size_t cur_gc_thread = 0;
      // reset all threads.
      for (auto &thread : m_gc_threads) {
        thread->reset();
      }
      // for each thread, assign a gc thread to manage it.
      for (auto state : m_threads) {
        m_gc_threads[cur_gc_thread]->add_thread(state->thread_id());
        cur_gc_thread = (cur_gc_thread + 1) % m_gc_threads.size();
      }
      auto &blocks = m_gc_allocator._u_blocks();
      size_t num_gc_threads = m_gc_threads.size();
      size_t num_blocks = blocks.size();
      size_t block_per_thread = num_blocks / num_gc_threads;
      size_t num_roots = m_roots.size();
      size_t roots_per_thread = num_roots / num_gc_threads;
      // for each gc thread, evenly subdivide blocks and roots.
      for (size_t i = 0; i < m_gc_threads.size() - 1; ++i) {
        m_gc_threads[i]->set_allocator_blocks(blocks.data() + block_per_thread * i, blocks.data() + block_per_thread * (i + 1));
        m_gc_threads[i]->set_root_iterators(m_roots.data() + roots_per_thread * i, m_roots.data() + roots_per_thread * (i + 1));
      }
      // last gc thread may not have even number of blocks or roots so give it what remains.
      m_gc_threads.back()->set_allocator_blocks(blocks.data() + block_per_thread * (num_gc_threads - 1),
                                                blocks.data() + num_blocks);
      m_gc_threads.back()->set_root_iterators(m_roots.data() + roots_per_thread * (num_gc_threads - 1),
                                              m_roots.data() + num_roots);
    }
    void global_kernel_state_t::wait_for_finalization(bool do_local_finalization)
    {
      wait_for_collection2();
      MCPPALLOC_CONCURRENCY_LOCK_GUARD_TAKE(m_thread_mutex);
      m_mutex.unlock();
      for (auto &gc_thread : m_gc_threads) {
        gc_thread->wait_until_finalization_finished();
      }
      if (do_local_finalization)
        local_thread_finalization();
    }
    void global_kernel_state_t::local_thread_finalization()
    {
      _local_thread_finalization_sparse();
    }
    void global_kernel_state_t::_local_thread_finalization_sparse()
    {
      cgc_internal_vector_t<gc_sparse_object_state_t *> vec;
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        vec = _u_get_local_finalization_vector_sparse();
      }
      _do_local_finalization_sparse(vec);
    }
    auto global_kernel_state_t::_u_get_local_finalization_vector_sparse() -> cgc_internal_vector_t<gc_sparse_object_state_t *>
    {
      auto ret = ::std::move(m_need_special_finalizing_collection_sparse);
      // explicitly put in known good state.
      m_need_special_finalizing_collection_sparse.clear();
      return ret;
    }
    void global_kernel_state_t::_do_local_finalization_sparse(cgc_internal_vector_t<gc_sparse_object_state_t *> &vec)
    {
      cgc_internal_vector_t<uintptr_t> to_be_freed;
      for (auto &&os : vec) {
        gc_user_data_t *ud = static_cast<gc_user_data_t *>(os->user_data());
        if (mcppalloc_likely(ud)) {
          if (mcppalloc_unlikely(ud->is_default())) {
          }
          else {
            // if it has a finalizer that can run in this thread, finalize.
            if (ud->m_finalizer) {
              ud->m_finalizer(os->object_start());
            }
            unique_ptr_allocated<gc_user_data_t, cgc_internal_allocator_t<void>> up(ud);
          }
        }
        mcppalloc::secure_zero(os->object_start(), os->object_size());
        to_be_freed.emplace_back(::mcppalloc::hide_pointer(os->object_start()));
      }
      _add_freed_in_last_collection(to_be_freed);
      gc_allocator().bulk_destroy_memory(vec);
    }

    void global_kernel_state_t::collect()
    {
      // make sure all threads are done finalizing already.
      // if not no reason to try to gc.
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_thread_mutex);
        for (auto &gc_thread : m_gc_threads) {
          if (!gc_thread->finalization_finished())
            return;
        }
      }
      // force a collection.
      force_collect();
    }
    void global_kernel_state_t::force_collect(bool do_local_finalization)
    {
      if (mcppalloc_unlikely(!get_tlks())) {
        ::std::cerr << "Attempted to gc with no thread state" << ::std::endl;
        ::std::terminate();
      }
      if (!enabled())
        return;
      bool expected = false;
      m_collect.compare_exchange_strong(expected, true);
      if (expected) {
        // if another thread is trying to collect, let them do it instead.
        // this is memory order acquire because it should be a barrier like locking a mutex.
        while (m_collect.load(::std::memory_order_acquire))
          ::std::this_thread::yield();
        return;
      }
      // wait until safe to collect.
      wait_for_finalization();
      // we need to maintain global allocator at some point so do it here.
      m_gc_allocator.collect();
      // note that the order of allocator locks and unlocks are all important here to prevent deadlocks!
      // grab allocator locks so that they are in a consistent state for garbage collection.
      lock(m_mutex, m_bitmap_allocator._mutex(), m_gc_allocator._mutex(), m_cgc_allocator._mutex(), m_slab_allocator._mutex(),
           m_thread_mutex, m_start_world_condition_mutex);
      // make sure we aren't already collecting
      while (mcppalloc_likely(m_num_collections) &&
             m_num_paused_threads.load(::std::memory_order_acquire) != m_num_resumed_threads.load(::std::memory_order_acquire)) {
        // we need to unlock these because gc_thread could be using them.
        // and gc_thread is not paused.
        unlock(m_cgc_allocator._mutex(), m_slab_allocator._mutex(), m_start_world_condition_mutex);
        //        unlock(m_thread_mutex, m_bitmap_allocator._mutex(), m_gc_allocator._mutex(), m_cgc_allocator._mutex(),
        //               m_slab_allocator._mutex(), m_mutex);
        //	m_collect = false;
        //        return;
        // this is not good enough
        // we need an array so we can try to repause threads.
        ::std::this_thread::yield();
        lock(m_cgc_allocator._mutex(), m_slab_allocator._mutex(), m_start_world_condition_mutex);
      }
      m_start_world_condition_mutex.unlock();
      ::std::atomic_thread_fence(::std::memory_order_acq_rel);
      //      m_collect = true;
      m_num_freed_in_last_collection = 0;
      m_freed_in_last_collection.clear();
      m_collect_finished.store(0, ::std::memory_order_release);
      m_num_paused_threads.store(0, ::std::memory_order_release);
      m_num_resumed_threads.store(0, ::std::memory_order_release);
      ::std::atomic_thread_fence(::std::memory_order_acq_rel);
      m_allocators_unavailable_mutex.lock();
      _u_suspend_threads();
      m_thread_mutex.unlock();
      get_tlks()->set_stack_ptr(mcppalloc_builtin_current_stack());
      m_bitmap_allocator._u_set_force_maintenance();
      m_gc_allocator._u_set_force_free_empty_blocks();
      m_cgc_allocator._u_set_force_free_empty_blocks();
      // release allocator locks so they can be used.
      m_bitmap_allocator._mutex().unlock();
      m_gc_allocator._mutex().unlock();
      m_cgc_allocator._mutex().unlock();
      m_slab_allocator._mutex().unlock();
      m_allocators_unavailable_mutex.unlock();
      // do collection
      {
        // Thread data can not be modified during collection.
        MCPPALLOC_CONCURRENCY_LOCK_ASSUME(m_thread_mutex);
        _u_setup_gc_threads();
      }
      ::std::chrono::high_resolution_clock::time_point t1, t2, tstart;
      tstart = t1 = ::std::chrono::high_resolution_clock::now();
      // clear all marks.
      for (auto &gc_thread : m_gc_threads) {
        gc_thread->start_clear();
      }
      _bitmap_allocator()._for_all_state([](auto &&state) { state->clear_mark_bits(); });
      ::std::atomic_thread_fence(::std::memory_order_release);
      // wait for clear to finish.
      for (auto &gc_thread : m_gc_threads) {
        gc_thread->wait_until_clear_finished();
      }
      t2 = ::std::chrono::high_resolution_clock::now();
      m_clear_mark_time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
      t1 = t2;
      // start marking.
      for (auto &gc_thread : m_gc_threads) {
        gc_thread->start_mark();
      }
      // wait for marking to finish.
      for (auto &gc_thread : m_gc_threads) {
        gc_thread->wait_until_mark_finished();
      }
      t2 = ::std::chrono::high_resolution_clock::now();
      m_mark_time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
      t1 = t2;
      // start sweeping.
      for (auto &gc_thread : m_gc_threads) {
        gc_thread->start_sweep();
      }
      _bitmap_allocator()._for_all_state([](auto &&state) { state->free_unmarked(); });
      // wait for sweeping to finish.
      for (auto &gc_thread : m_gc_threads) {
        gc_thread->wait_until_sweep_finished();
      }
      t2 = ::std::chrono::high_resolution_clock::now();
      m_sweep_time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
      t1 = t2;
      // notify safe to resume threads.
      for (auto &gc_thread : m_gc_threads) {
        gc_thread->notify_all_threads_resumed();
      }
      t2 = ::std::chrono::high_resolution_clock::now();
      m_notify_time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - t1);
      m_total_collect_time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(t2 - tstart);
      m_num_collections++;
      m_thread_mutex.lock();
      // tell threads they make wake up.
      m_start_world_condition_mutex.lock();
      m_collect_finished = true;
      // make sure everything is committed
      ::std::atomic_thread_fence(::std::memory_order_seq_cst);
      _u_resume_threads();
      m_start_world_condition_mutex.unlock();
      m_collect = false;
      m_thread_mutex.unlock();
      m_mutex.unlock();
      if (do_local_finalization) {
        wait_for_finalization();
      }
    }
    void global_kernel_state_t::_add_num_freed_in_last_collection(size_t num_freed) noexcept
    {
      m_num_freed_in_last_collection += num_freed;
    }
    size_t global_kernel_state_t::num_freed_in_last_collection() const noexcept
    {
      return m_num_freed_in_last_collection;
    }
    cgc_internal_vector_t<uintptr_t> global_kernel_state_t::_d_freed_in_last_collection() const
    {
      MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
      return m_freed_in_last_collection;
    }
    void global_kernel_state_t::wait_for_collection()
    {
      // this just needs mutex.
      ::std::unique_lock<decltype(m_mutex)> lock(m_mutex);
#ifndef _WIN32
      m_start_world_condition.wait(lock, [this]() { return m_collect_finished.load(::std::memory_order_acquire); });
#else
      while (m_collect) {
        lock.unlock();
        ::std::this_thread::yield();
        lock.lock();
      }
#endif
      lock.release();
    }
    void global_kernel_state_t::wait_for_collection2()
    {
      // this needs both thread mutex and mutex.t
      ::mcppalloc::double_lock_t<decltype(m_mutex), decltype(m_thread_mutex)> lock(m_mutex, m_thread_mutex);

      while (!m_collect_finished && m_collect) {
        lock.unlock();
        ::std::this_thread::yield();
        lock.lock();
      }
      lock.release();
    }
    void global_kernel_state_t::initialize_current_thread(void *top_of_stack)
    {
      // this is not a race condition because only this thread could initialize.
      auto tlks = details::get_tlks();
      // do not start creating threads during an ongoing collection.
      wait_for_collection2();
      // assume we have mutex and thread mutex from wait for collection 2 specs.
      MCPPALLOC_CONCURRENCY_LOCK_GUARD_TAKE(m_mutex);
      MCPPALLOC_CONCURRENCY_LOCK_GUARD_TAKE(m_thread_mutex);
      // if no tlks, create one.
      if (tlks == nullptr) {
        tlks = ::mcppalloc::make_unique_allocator<details::thread_local_kernel_state_t, cgc_internal_malloc_allocator_t<void>>()
                   .release();
        set_tlks(tlks);
        // make sure gc kernel is initialized.
        _u_initialize();
      }
      else
        ::std::terminate();
      // set very top of stack.
      tlks->set_top_of_stack(top_of_stack);
      m_threads.push_back(tlks);
      // initialize thread allocators for this thread.
      m_cgc_allocator.initialize_thread();
      m_gc_allocator.initialize_thread();
    }
    void global_kernel_state_t::destroy_current_thread()
    {
      auto tlks = details::get_tlks();
      // do not start destroying threads during an ongoing collection.
      wait_for_collection2();
      // assume we have mutex and thread mutex from wait for collection 2 specs.
      MCPPALLOC_CONCURRENCY_LOCK_GUARD_TAKE(m_mutex);
      MCPPALLOC_CONCURRENCY_LOCK_GUARD_TAKE(m_thread_mutex);
      // find thread and erase it from global state
      const auto it = find(m_threads.begin(), m_threads.end(), tlks);
      if (mcppalloc_unlikely(it == m_threads.end())) {
        // this is a pretty big logic error, so catch in debug mode.
        ::std::cerr << "can not find thread with id " << tlks->thread_id() << " 614164d3-1ab6-4079-b978-7880aa74b566"
                    << ::std::endl;
        ::std::terminate();
      }
      // destroy thread allocators for this thread.
      m_gc_allocator.destroy_thread();
      m_cgc_allocator.destroy_thread();
      m_bitmap_allocator.destroy_thread();
      // remove thread from gks.
      m_threads.erase(it);
      // this will delete our tks.
      ::std::unique_ptr<details::thread_local_kernel_state_t, cgc_internal_malloc_deleter_t> tlks_deleter(tlks);
      // do this to make any changes to state globally visible.
      ::std::atomic_thread_fence(::std::memory_order_release);
    }
    void global_kernel_state_t::_u_initialize()
    {
      // if already initialized, this is a no-op
      if (m_initialized)
        return;
#ifndef _WIN32
      details::initialize_thread_suspension();
#endif
      m_gc_allocator.initialize(::mcppalloc::pow2(33), ::mcppalloc::pow2(36));
      const size_t num_gc_threads = ::std::thread::hardware_concurrency();
      // sanity check bad stl implementations.
      if (mcppalloc_unlikely(!num_gc_threads)) {
        ::std::cerr << "std::thread::hardware_concurrency not well defined\n";
        ::std::terminate();
      }
      // add gc threads
      for (size_t i = 0; i < num_gc_threads; ++i)
        m_gc_threads.emplace_back(make_unique_malloc<gc_thread_t>());
      m_initialized = true;
    }
#ifndef _WIN32
    void global_kernel_state_t::_u_suspend_threads()
    {
      // adopt the lock since we need to be able to lock/unlock it.
      ::std::unique_lock<decltype(m_mutex)> lock(m_mutex, ::std::adopt_lock);
      // for each thread
      for (auto state : m_threads) {
        // don't want to suspend this thread.
        if (state->thread_id() == ::std::this_thread::get_id())
          continue;
        // send signal to stop it.
        if (mcppalloc_unlikely(cgc1::pthread_kill(state->thread_handle(), SIGUSR1))) {
          ::std::cerr << "Thread went away during suspension 3c846d50-475f-488c-82b5-15ba2c5fa508\n";
          // there is no way to recover from this error.
          ::std::terminate();
        }
      }
      // wait for all threads to stop
      // note threads.size() can not change out from under us here by logic.
      // in particular we can't add threads during collection cycle.
      lock.unlock();
      ::std::chrono::high_resolution_clock::time_point start_time = ::std::chrono::high_resolution_clock::now();
      while (m_num_paused_threads.load(::std::memory_order_acquire) != m_threads.size() - 1) {
        // os friendly spin a bit.
        ::std::this_thread::yield();
        ::std::chrono::high_resolution_clock::time_point cur_time = ::std::chrono::high_resolution_clock::now();
        auto time_span = ::std::chrono::duration_cast<::std::chrono::duration<double>>(cur_time - start_time);
        if (mcppalloc_unlikely(time_span > ::std::chrono::seconds(1)))
          ::std::cerr << "Waiting on thread to pause\n";
      }
      lock.lock();
      // we shouldn't unlock at end of this.
      lock.release();
    }
    void global_kernel_state_t::_u_resume_threads()
    {
      m_start_world_condition.notify_all();
    }
    void global_kernel_state_t::_collect_current_thread()
    {
      // if no thread llks we really can't collect.
      auto tlks = details::get_tlks();
      if (!tlks) {
        // this should be considered a fatal logic error.
        ::std::terminate();
      }
      // set stack pointer.
      tlks->set_stack_ptr(__builtin_frame_address(0));
      // Make sure all data is committed.
      ::std::atomic_thread_fence(std::memory_order_release);
      m_num_paused_threads++;
      {
        // Make sure that allocators are available by now since we need them for our conditional.
        // We don't actually need to hold the lock, just wait if it is locked.
        // this is an os lock so there isn't a better way to do this.
        m_allocators_unavailable_mutex.lock();
        m_allocators_unavailable_mutex.unlock();
      }
      ::mcppalloc::unique_lock_t<decltype(m_start_world_condition_mutex)> lock(m_start_world_condition_mutex);
      // deadlock potential here if conditional var sets mutexes in kernel
      // so we use our own conditional variable implementation.
      m_start_world_condition.wait(lock, [this]() { return m_collect_finished.load(::std::memory_order_acquire); });
      // do this inside lock just to prevent race conditions.
      // This is defensive programming only, it shouldn't be required.
      tlks->set_in_signal_handler(false);
      // this thread is resumed.
      m_num_resumed_threads++;
      lock.unlock();
    }
#else
    void global_kernel_state_t::_u_suspend_threads()
    {
      // for each thread.
      for (auto it = m_threads.begin() + 1; it != m_threads.end(); ++it) {
        auto state = *it;
        // we may have to retry mutliple times.
        for (size_t i = 0; i < 5000; ++i) {
          // bug in Windows function spec!
          int ret = static_cast<int>(::SuspendThread(state->thread_handle()));
          if (ret == 0) {
            // success suspending thread.
            state->set_in_signal_handler(true);
            break;
          }
          else if (ret < 0) {
            // give everything else a chance to go.
            m_mutex.unlock();
            m_gc_allocator._mutex().unlock();
            m_cgc_allocator._mutex().unlock();
            ::std::lock(m_mutex, m_gc_allocator._mutex(), m_cgc_allocator._mutex());
          }
          else {
            ::std::cerr << "Suspend thread failed\n";
            ::std::terminate();
          }
        }
        // if not in signal handler abort.
        if (!state->in_signal_handler()) {
          ::std::terminate();
        }
        // get thread context.
        CONTEXT context = {0};
        context.ContextFlags = CONTEXT_FULL;
        auto ret = ::GetThreadContext(state->thread_handle(), &context);
        if (mcppalloc_unlikely(!ret)) {
          ::std::cerr << "Get thread context failed\n";
          ::std::terminate();
        }
        // get stack pointer.
        uint8_t *stack_ptr = nullptr;
#ifdef _WIN64
        stack_ptr = reinterpret_cast<uint8_t *>(context.Rsp);
#else
        stack_ptr = reinterpret_cast<uint8_t *>(context.Esp);
#endif
        // if stack pointer is not recoverable, unrecoverable.
        if (!stack_ptr)
          ::std::terminate();
        state->set_stack_ptr(stack_ptr);
        // grab registers.
        void **reg_begin = reinterpret_cast<void **>(&context.Rax);
        void **reg_end = reinterpret_cast<void **>(&context.VectorControl);
        for (auto context_it = reg_begin; context_it != reg_end; ++context_it) {
          // we skip stack register since we already handle that specially.
          if (context_it == reinterpret_cast<const void **>(&context.Rsp))
            continue;
          state->add_potential_root(*context_it);
        }
      }
      ::std::atomic_thread_fence(std::memory_order_release);
    }
    void global_kernel_state_t::_u_resume_threads()
    {
      for (auto state : m_threads) {
        // don't touch this thread.
        if (state->thread_id() == ::std::this_thread::get_id())
          continue;
        // reset tlks
        state->set_stack_ptr(nullptr);
        state->set_in_signal_handler(false);
        // resume threads.
        if (static_cast<int>(::ResumeThread(state->thread_handle())) < 0)
          ::std::terminate();
      }
      // force commit.
      ::std::atomic_thread_fence(std::memory_order_release);
    }
    void global_kernel_state_t::_collect_current_thread()
    {
    }
#endif
    auto gc_allocator_thread_policy_t::on_allocation_failure(const ::mcppalloc::details::allocation_failure_t &failure)
        -> ::mcppalloc::details::allocation_failure_action_t
    {
      g_gks->gc_allocator().initialize_thread()._do_maintenance();
      g_gks->force_collect();
      return ::mcppalloc::details::allocation_failure_action_t{false, failure.m_failures < 5};
    }
  }
}
