#pragma once
namespace cgc1
{
  namespace details
  {
    template <typename Allocator_Policy>
    packed_object_thread_allocator_t<Allocator_Policy>::packed_object_thread_allocator_t(
        packed_object_thread_allocator_t &&ta) noexcept : m_locals(::std::move(ta.m_locals)),
                                                          m_force_maintenance(ta.force_maintenance.load()),
                                                          m_free_list(::std::move(ta.m_free_list)),
                                                          m_allocator(ta.m_allocator),
                                                          m_popcount_max(::std::move(ta.m_popcount_max)),
                                                          m_max_in_use(::std::move(ta.m_max_in_use)),
                                                          m_max_free(::std::move(ta.m_max_free))
    {
    }
    template <typename Allocator_Policy>
    packed_object_thread_allocator_t<Allocator_Policy>::packed_object_thread_allocator_t(
        packed_object_allocator_t<allocator_policy_type> &allocator)
        : m_allocator(allocator)
    {
      for (size_t i = 0; i < m_popcount_max.size(); ++i) {
        packed_object_state_t state;
        state.m_info = packed_object_package_t::_get_info(i);
        m_popcount_max[i] = state.size() / 2;
      }
    }
    template <typename Allocator_Policy>
    packed_object_thread_allocator_t<Allocator_Policy>::~packed_object_thread_allocator_t()
    {
      // set max to zero to move all states.
      ::std::fill(m_popcount_max.begin(), m_popcount_max.end(), 0);
      set_max_in_use(0);
      set_max_free(0);
      do_maintenance();
    }
    template <typename Allocator_Policy>
    void packed_object_thread_allocator_t<Allocator_Policy>::set_max_in_use(size_t max_in_use)
    {
      m_max_in_use = max_in_use;
    }
    template <typename Allocator_Policy>
    void packed_object_thread_allocator_t<Allocator_Policy>::set_max_free(size_t max_free)
    {
      m_max_free = max_free;
    }
    template <typename Allocator_Policy>
    auto packed_object_thread_allocator_t<Allocator_Policy>::max_in_use() const noexcept -> size_t
    {
      return m_max_in_use;
    }
    template <typename Allocator_Policy>
    auto packed_object_thread_allocator_t<Allocator_Policy>::max_free() const noexcept -> size_t
    {
      return m_max_free;
    }
    template <typename Allocator_Policy>
    void packed_object_thread_allocator_t<Allocator_Policy>::do_maintenance()
    {
      m_force_maintenance = false;

      m_locals.do_maintenance(m_free_list);

      size_t id = 0;
      for (auto &&vec : m_locals.m_vectors) {
        size_t num_potential_moves = 0;
        // move extra in use to global.
        for (auto &&state : vec) {
          if (state->free_popcount() > state->size() / 2)
            // approximately half free.
            num_potential_moves++;
        }
        if (num_potential_moves > m_max_in_use) {
          auto num_to_be_moved = num_potential_moves - m_max_in_use;
          auto end = vec.end();
          {
            size_t threshold = static_cast<size_t>(num_potential_moves - num_to_be_moved);
            size_t num_found = 0;
            auto it = vec.begin();
            while (it != end) {
              auto &state = **it;
              if (state.free_popcount() > state.size() / 2) {
                num_found++;
                if (num_found > threshold) {
                  --end;
                  continue;
                }
              }
              ++it;
            }
          }
          assert(static_cast<size_t>(vec.end() - end) == num_to_be_moved);
          {
            CGC1_CONCURRENCY_LOCK_GUARD(m_allocator._mutex());
            for (auto it = end; it != vec.end(); ++it) {
              m_allocator._u_to_global(id, *it);
            }
          }
          vec.resize(vec.size() - num_to_be_moved);
        }
        // move extra free to global.

        if (m_free_list.size() > m_max_free) {
          CGC1_CONCURRENCY_LOCK_GUARD(m_allocator._mutex());
          while (m_free_list.size() != m_max_free) {
            m_allocator._u_to_free(m_free_list.back());
            m_free_list.pop_back();
          }
        }
        ++id;
      }
    }
    template <typename Allocator_Policy>
    auto packed_object_thread_allocator_t<Allocator_Policy>::allocate(size_t sz) noexcept -> void *
    {

      size_t attempts = 1;
      bool expand = false;
      (void)expand;
    RESTART:
      _check_maintenance();
      const auto id = get_packed_object_size_id(sz);
      void *v;
      size_t ret_size;
      ::std::tie(v, ret_size) = m_locals.allocate(id);
      if (!v) {
        if (!m_free_list.empty()) {
          packed_object_state_t *state = unsafe_cast<packed_object_state_t>(m_free_list.back());
          assert(state);
          state->m_info = packed_object_package_t::_get_info(id);
          state->initialize();
          m_free_list.pop_back();
          v = state->allocate();

          m_locals.insert(id, state);
          m_allocator.allocator_policy().on_allocate(v, state->real_entry_size());
          return v;
        } else {
          packed_object_state_t *state = m_allocator._get_memory();
          if (!state) {
            packed_allocation_failure_t failure{attempts};
            auto action = m_allocator.allocator_policy().on_allocation_failure(failure);
            expand = action.m_attempt_expand;
            if (action.m_repeat)
              goto RESTART;
            else
              abort();
          }
          state->m_info = packed_object_package_t::_get_info(id);
          state->initialize();
          assert(state->first_free() == 0);

          v = state->allocate();
          assert(v);
          m_locals.insert(id, state);
          m_allocator.allocator_policy().on_allocate(v, state->real_entry_size());
          return v;
        }
      }
      m_allocator.allocator_policy().on_allocate(v, ret_size);
      return v;
    }
    template <typename Allocator_Policy>
    auto packed_object_thread_allocator_t<Allocator_Policy>::deallocate(void *v) noexcept -> bool
    {
      packed_object_state_t *state = get_state(v);
      state->deallocate(v);
      if (state->all_free()) {
        auto id = get_packed_object_size_id(state->declared_entry_size());
        bool success = m_locals.remove(id, state);
        // if it fails it could be anywhere.
        if (!success)
          return false;
        m_free_list.push_back(state);
      }
      return true;
    }
    template <typename Allocator_Policy>
    void packed_object_thread_allocator_t<Allocator_Policy>::set_force_maintenance()
    {
      m_force_maintenance = true;
    }
    template <typename Allocator_Policy>
    void packed_object_thread_allocator_t<Allocator_Policy>::_check_maintenance()
    {
      if (unlikely(m_force_maintenance.load(::std::memory_order_relaxed)))
        do_maintenance();
    }
  }
}
