#include "gc_thread.hpp"
#include "thread_local_kernel_state.hpp"
#include "global_kernel_state.hpp"
#ifdef _WIN32
#include <Windows.h>
#else
#include <string.h>
#endif
namespace cgc1
{
  namespace details
  {
    gc_thread_t::gc_thread_t()
    {
      m_run = true;
      reset();
      m_finalization_done = true;
      m_thread = ::std::thread([this]() {
        _run();
        g_gks._internal_allocator().destroy_thread();
      });
    }
    gc_thread_t::~gc_thread_t()
    {
      m_run = false;
      wake_up();
      m_thread.join();
    }
    void gc_thread_t::reset()
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      m_clear_done = false;
      m_mark_done = false;
      m_sweep_done = false;
      m_finalization_done = false;
      m_do_clear = false;
      m_do_mark = false;
      m_do_sweep = false;
      m_do_all_threads_resumed = false;
      m_block_begin = m_block_end = nullptr;
      m_root_begin = m_root_end = nullptr;
      m_addresses_to_mark.clear();
      m_stack_roots.clear();
      m_watched_threads.clear();
    }
    void gc_thread_t::wake_up()
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      m_wake_up.notify_all();
    }
    void gc_thread_t::add_thread(::std::thread::id id)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      insert_unique_sorted(m_watched_threads, id, ::std::less<::std::thread::id>());
    }
    void gc_thread_t::remove_thread(::std::thread::id id)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      auto it = ::std::find(m_watched_threads.begin(), m_watched_threads.end(), id);
      if (it != m_watched_threads.end())
        m_watched_threads.erase(it);
    }
    void gc_thread_t::set_allocator_blocks(gc_allocator_t::this_allocator_block_handle_t *begin,
                                           gc_allocator_t::this_allocator_block_handle_t *end)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      m_block_begin = begin;
      m_block_end = end;
    }
    void gc_thread_t::set_root_iterators(void ***begin, void ***end)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      m_root_begin = begin;
      m_root_end = end;
    }
    void gc_thread_t::_run()
    {
      while (m_run) {
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
        while (!m_do_clear) {
          m_wake_up.wait(m_mutex);
          if (!m_run)
            return;
        }
        m_do_clear = false;
        // Clear marks.
        _clear_marks();
        m_clear_done = true;
        m_done_clearing.notify_all();
        // wait to start marking
        m_start_mark.wait(m_mutex, [this]() -> bool { return m_do_mark; });
        m_do_mark = false;
        _mark();
        m_mark_done = true;
        m_done_mark.notify_all();
        // wait to start sweeping
        m_start_sweep.wait(m_mutex, [this]() -> bool { return m_do_sweep; });
        m_do_sweep = false;
        _sweep();
        m_sweep_done = true;
        m_done_sweep.notify_all();
        // wait for all threads to resume.
        m_all_threads_resumed.wait(m_mutex, [this]() -> bool { return m_do_all_threads_resumed; });
        m_do_all_threads_resumed = false;
        _finalize();
        m_finalization_done = true;
        m_done_finalization.notify_all();
      }
    }
    void gc_thread_t::start_clear()
    {
      m_do_clear = true;
      wake_up();
    }
    void gc_thread_t::wait_until_clear_finished()
    {
      ::std::unique_lock<decltype(m_mutex)> l(m_mutex);
      m_done_clearing.wait(l, [this]() -> bool { return m_clear_done; });
    }
    void gc_thread_t::start_mark()
    {
      m_do_mark = true;
      m_start_mark.notify_all();
    }
    void gc_thread_t::wait_until_mark_finished()
    {
      ::std::unique_lock<decltype(m_mutex)> l(m_mutex);
      m_done_mark.wait(l, [this]() -> bool { return m_mark_done; });
    }
    void gc_thread_t::start_sweep()
    {
      m_do_sweep = true;
      m_start_sweep.notify_all();
    }
    void gc_thread_t::wait_until_sweep_finished()
    {
      ::std::unique_lock<decltype(m_mutex)> l(m_mutex);
      ;
      m_done_sweep.wait(l, [this]() -> bool { return m_sweep_done; });
    }
    void gc_thread_t::notify_all_threads_resumed()
    {
      m_do_all_threads_resumed = true;
      m_all_threads_resumed.notify_all();
    }
    void gc_thread_t::wait_until_finalization_finished()
    {
      ::std::unique_lock<decltype(m_mutex)> l(m_mutex);
      m_done_finalization.wait(l, [this]() -> bool { return m_finalization_done; });
    }
    bool gc_thread_t::finalization_finished() const
    {
      return m_finalization_done;
    }
    bool gc_thread_t::handle_thread(::std::thread::id id)
    {
      thread_local_kernel_state_t *tlks = g_gks.tlks(id);
      if (!tlks) {
        return false;
      }
      // this is during GC so the slab will not be changed so no locks for gks needed.
      tlks->scan_stack(m_stack_roots, g_gks.gc_allocator()._u_begin(), g_gks.gc_allocator()._u_current_end());
      return true;
    }
    void gc_thread_t::_clear_marks()
    {
      assert(m_block_begin && m_block_end);
      for (auto it = m_block_begin; it != m_block_end; ++it) {
        auto &block_handle = *it;
        auto block = block_handle.m_block;
        auto begin = make_next_iterator(reinterpret_cast<object_state_t *>(block->begin()));
        block->_verify(begin);
        auto end = make_next_iterator(block->current_end());
        for (auto os_it = begin; os_it != end; ++os_it) {
          assert(os_it->next() != nullptr);
          if (os_it->in_use())
            clear_mark(&*os_it);
        }
      }
    }
    void gc_thread_t::_mark()
    {
      for (const auto &thread : m_watched_threads) {
        handle_thread(thread);
      }
      for (auto root : m_stack_roots)
        _mark_addrs(*reinterpret_cast<void **>(root), 0);
      for (auto it = m_root_begin; it != m_root_end; ++it)
        _mark_addrs(*it, 0);
      _mark_mark_vector();
    }
    void gc_thread_t::_mark_addrs(void *addr, size_t depth, bool ignore_skip_marked)
    {
      if (depth > 1000) {
        m_addresses_to_mark.push_back(addr);
        return;
      }
      void *heap_begin = g_gks.gc_allocator()._u_begin();
      void *heap_end = g_gks.gc_allocator()._u_current_end();
      object_state_t *os = object_state_t::from_object_start(addr);
      if (os < heap_begin || os >= heap_end)
        return;
      if (!g_gks.is_valid_object_state(os))
      {
        os = g_gks._u_find_valid_object_state(addr);
        if (!os)
          return;
      }
      assert(is_aligned_properly(os));
      // not valid.
      if (!os->not_available() || !os->next())
        return;
      if (!ignore_skip_marked && is_marked(os))
        return;
      set_mark(os);
      if (is_complex(os))
        abort();
      // if it is atomic we are done here.
      if (is_atomic(os))
        return;
      for (void **it = reinterpret_cast<void **>(os->object_start()); it != reinterpret_cast<void **>(os->object_end()); ++it) {
        if (*it > heap_begin && *it < heap_end) {
          _mark_addrs(*it, depth + 1, true);
        }
      }
    }
    void gc_thread_t::_mark_mark_vector()
    {
      while (!m_addresses_to_mark.empty()) {
        void *addr = m_addresses_to_mark.back();
        m_addresses_to_mark.pop_back();
        _mark_addrs(addr, 0);
      }
    }
    void gc_thread_t::_sweep()
    {
      assert(m_block_begin && m_block_end);
      for (auto it = m_block_begin; it != m_block_end; ++it) {
        auto &block_handle = *it;
        auto block = block_handle.m_block;
        auto begin = make_next_iterator(reinterpret_cast<object_state_t *>(block->begin()));
        auto end = make_next_iterator(block->current_end());
        for (auto os_it = begin; os_it != end; ++os_it) {
          assert(os_it->next() == end || os_it->next_valid());
          if (os_it->in_use() && !is_marked(os_it))
            m_to_be_freed.push_back(os_it);
        }
      }
    }
    void gc_thread_t::_finalize()
    {
      cgc_internal_vector_t<void *> to_be_freed;
      for (auto& os : m_to_be_freed) {
        gc_user_data_t *ud = static_cast<gc_user_data_t *>(os->user_data());
        if (ud) {
          if (ud->m_uncollectable)
          {
            os = nullptr;
            continue;
          }
          if (ud->m_finalizer)
            ud->m_finalizer(os->object_start());
          //delete user data if not owned by block.
          if (!ud->m_is_default)
          {
            unique_ptr_allocated<gc_user_data_t, cgc_internal_allocator_t<void>> up(ud);
          }
        }
        assert(os->object_end() < g_gks.gc_allocator().end());
#ifdef _WIN32
        SecureZeroMemory(os->object_start(), os->object_size());
#else
        ::memset(os->object_start(), 0, os->object_size());
#endif
        to_be_freed.push_back(os->object_start());
      }
      g_gks._add_freed_in_last_collection(to_be_freed);
      g_gks.gc_allocator().bulk_destroy_memory(m_to_be_freed);
    }
  }
}
