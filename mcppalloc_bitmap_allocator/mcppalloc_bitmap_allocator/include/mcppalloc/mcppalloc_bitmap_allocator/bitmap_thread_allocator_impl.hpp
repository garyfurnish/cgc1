#pragma once
#include <mcppalloc/allocator_thread_policy.hpp>
#include <mcppalloc/mcppalloc_utils/boost/property_tree/ptree.hpp>
#include <mcppalloc/mcppalloc_utils/concurrency.hpp>
#include <new>
#include <thread>
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {
      template <typename Allocator_Policy>
      bitmap_thread_allocator_t<Allocator_Policy>::bitmap_thread_allocator_t(bitmap_thread_allocator_t &&ta) noexcept
          : m_locals(::std::move(ta.m_locals)),
            m_force_maintenance(ta.force_maintenance.load()),
            m_free_list(::std::move(ta.m_free_list)),
            m_allocator(ta.m_allocator),
            m_popcount_max(::std::move(ta.m_popcount_max)),
            m_max_in_use(::std::move(ta.m_max_in_use)),
            m_max_free(::std::move(ta.m_max_free))
      {
      }
      template <typename Allocator_Policy>
      bitmap_thread_allocator_t<Allocator_Policy>::bitmap_thread_allocator_t(bitmap_allocator_t<allocator_policy_type> &allocator)
          : m_allocator(allocator)
      {
        for (size_t i = 0; i < m_popcount_max.size(); ++i) {
          bitmap_state_t state;
          state.m_internal.m_info = package_type::_get_info(i);
          m_popcount_max[i] = state.size() / 2;
        }
      }
      template <typename Allocator_Policy>
      bitmap_thread_allocator_t<Allocator_Policy>::~bitmap_thread_allocator_t()
      {
        m_in_destructor = true;
        // set max to zero to move all states.
        ::std::fill(m_popcount_max.begin(), m_popcount_max.end(), 0);
        set_max_in_use(0);
        set_max_free(0);
        do_maintenance();
        for (auto &&package : m_locals) {
          for (auto &&vec : package.second.m_vectors) {
            if (mcppalloc_unlikely(!vec.empty())) {
              ::std::cerr << ::std::this_thread::get_id() << " error: deallocating bitmap_thread_allocator_t with objects left\n";
              ::std::abort();
            }
          }
        }
        if (mcppalloc_unlikely(!m_free_list.empty())) {
          ::std::cerr << ::std::this_thread::get_id()
                      << " error deallocating bitmap_thread_allocator_t with free list nonempty\n";
        }
      }
      template <typename Allocator_Policy>
      void bitmap_thread_allocator_t<Allocator_Policy>::set_max_in_use(size_t max_in_use)
      {
        m_max_in_use = max_in_use;
      }
      template <typename Allocator_Policy>
      void bitmap_thread_allocator_t<Allocator_Policy>::set_max_free(size_t max_free)
      {
        m_max_free = max_free;
      }
      template <typename Allocator_Policy>
      auto bitmap_thread_allocator_t<Allocator_Policy>::max_in_use() const noexcept -> size_t
      {
        return m_max_in_use;
      }
      template <typename Allocator_Policy>
      auto bitmap_thread_allocator_t<Allocator_Policy>::max_free() const noexcept -> size_t
      {
        return m_max_free;
      }
      template <typename Allocator_Policy>
      void bitmap_thread_allocator_t<Allocator_Policy>::do_maintenance()
      {
        for (auto &&pair : m_locals)
          do_maintenance(pair.second);
      }
      template <typename Allocator_Policy>
      void bitmap_thread_allocator_t<Allocator_Policy>::do_maintenance(package_type &package)
      {
        m_force_maintenance = false;

        package.do_maintenance(m_free_list);

        for (auto &&vec : package.m_vectors) {
          for (auto &&it = vec.begin(); it != vec.end();) {
            auto &&state = *it;
            if (state->all_free()) {
              m_free_list.push_back(state);
              it = vec.erase(it);
            } else
              ++it;
          }
        }
        size_t id = 0;
        for (auto &&vec : package.m_vectors) {
          size_t num_potential_moves = 0;
          // move extra in use to global.
          for (auto &&state : vec) {
            const auto pop_count = state->free_popcount();
            if (pop_count > state->size() / 2 || mcppalloc_unlikely(m_in_destructor))
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
                if (state.free_popcount() > state.size() / 2 || mcppalloc_unlikely(m_in_destructor)) {
                  num_found++;
                  if (num_found > threshold) {
                    --end;
                    ::std::swap(*end, *it);
                    continue;
                  }
                }
                ++it;
              }
            }
            assert(static_cast<size_t>(vec.end() - end) == num_to_be_moved);
            {
              MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_allocator._mutex());
              for (auto it = end; it != vec.end(); ++it) {
                m_allocator._u_to_global(id, (*it)->type_id(), *it);
              }
            }
            vec.resize(vec.size() - num_to_be_moved);
          }
          // move extra free to global.

          if (m_free_list.size() > m_max_free) {
            MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_allocator._mutex());
            while (m_free_list.size() != m_max_free) {
              m_allocator._u_to_free(m_free_list.back());
              m_free_list.pop_back();
            }
          }
          ++id;
        }
      }
      template <typename Allocator_Policy>
      auto bitmap_thread_allocator_t<Allocator_Policy>::allocate(size_t sz, type_id_t type_id) -> block_type
      {
        auto package = m_locals.find(type_id);
        if (package == m_locals.end()) {
          package = m_locals.insert(::std::make_pair(type_id, package_type(type_id))).first;
        }
        return allocate(sz, package->second);
      }
      template <typename Allocator_Policy>
      auto bitmap_thread_allocator_t<Allocator_Policy>::allocate(size_t sz, package_type &package) -> block_type
      {
        size_t attempts = 1;
        bool expand = false;
        (void)expand;
      RESTART:
        _check_maintenance();
        const auto id = get_bitmap_size_id(sz);
        block_type ret = package.allocate(id);
        if (!ret.m_ptr) {
          if (!m_free_list.empty()) {
            auto &type_info = m_allocator.get_type(package.type_id());
            // free list not empty
            bitmap_state_t *state = unsafe_cast<bitmap_state_t>(m_free_list.back());
            state->verify_magic();
            assert(state);
            state->m_internal.m_info = package_type::_get_info(id);
            state->initialize(package.type_id(), type_info.num_user_bits());
            m_free_list.pop_back();
            ret.m_ptr = state->allocate();
	    ret.m_size = state->real_entry_size();

            package.insert(id, state);
            m_allocator.allocator_policy().on_allocation(ret.m_ptr,ret.m_size);
            state->verify_magic();
	    return ret;
          } else {
            // free list empty
            auto &type_info = m_allocator.get_type(package.type_id());
            bitmap_state_t *state = m_allocator._get_memory();
            if (!state) {
              ::mcppalloc::details::allocation_failure_t failure{attempts++};
              auto action = m_allocator.allocator_policy().on_allocation_failure(failure);
              expand = action.m_attempt_expand;
              if (action.m_repeat)
                goto RESTART;
              else
                throw ::std::bad_alloc();
            }
            state->verify_magic();
            state->m_internal.m_info = package_type::_get_info(id);
            state->initialize(package.type_id(), type_info.num_user_bits());
            assert(state->first_free() == 0);

            ret.m_ptr = state->allocate();
	    ret.m_size = state->real_entry_size();
            assert(v);
            package.insert(id, state);
            m_allocator.allocator_policy().on_allocation(ret.m_ptr,ret.m_size);
            state->verify_magic();
            return ret;
          }
        }
        assert(ret.m_size >= sz);
        m_allocator.allocator_policy().on_allocation(ret.m_ptr, ret.m_size);
        return ret;
      }
      template <typename Allocator_Policy>
      auto bitmap_thread_allocator_t<Allocator_Policy>::deallocate(void *v, package_type &package) noexcept -> bool
      {
        bitmap_state_t *state = get_state(v);
        state->verify_magic();
        if (mcppalloc_unlikely(!state->has_valid_magic_numbers())) {
          ::std::cerr
              << "mcppalloc bitmap_thread_allocator state has invalid magic numbers ad761a4e-656e-45a8-abe9-541414679c1f\n";
          ::std::abort();
        }
        state->deallocate(v);
        if (state->all_free()) {
          auto id = get_bitmap_size_id(state->declared_entry_size());
          if (mcppalloc_unlikely(id == ::std::numeric_limits<size_t>::max())) {
            ::std::cerr << "mcppalloc bitmap_thread_allocator consistency error aa16e07a-5f8c-4a97-b809-c944ff4c45b9\n";
            ::std::abort();
          }
          bool success = package.remove(id, state);
          // if it fails it could be anywhere.
          if (!success)
            return true;
          m_free_list.push_back(state);
        }
        return true;
      }
      template <typename Allocator_Policy>
      auto bitmap_thread_allocator_t<Allocator_Policy>::deallocate(void *v) noexcept -> bool
      {
        if (v < m_allocator.begin() || v >= m_allocator.end())
          return false;
        auto state = get_state(v);
        auto type_id = state->type_id();
        auto package = m_locals.find(type_id);
        if (package == m_locals.end()) {
          return false;
        }
        return deallocate(v, package->second);
      }
      template <typename Allocator_Policy>
      void bitmap_thread_allocator_t<Allocator_Policy>::set_force_maintenance()
      {
        m_force_maintenance = true;
      }
      template <typename Allocator_Policy>
      void bitmap_thread_allocator_t<Allocator_Policy>::_check_maintenance()
      {
        if (mcppalloc_unlikely(m_force_maintenance.load(::std::memory_order_relaxed)))
          do_maintenance();
      }
      template <typename Allocator_Policy>
      void bitmap_thread_allocator_t<Allocator_Policy>::to_ptree(::boost::property_tree::ptree &ptree, int level)
      {
        ptree.put("max_in_use", ::std::to_string(max_in_use()));
        ptree.put("max_free", ::std::to_string(max_free()));
        ptree.put("force_maintenance", ::std::to_string(m_force_maintenance));
        ptree.put("free_list_size", ::std::to_string(m_free_list.size()));
        {
          ::boost::property_tree::ptree types;
          for (auto &&package : m_locals) {
            ::boost::property_tree::ptree locals;
            package.second.to_ptree(locals, level);
            types.add_child("locals", locals);
          }
          ptree.add_child("types", types);
        }
      }
      template <typename Allocator_Policy>
      template <typename Predicate>
      void bitmap_thread_allocator_t<Allocator_Policy>::for_all_state(Predicate &predicate)
      {
        for (auto &&package : m_locals)
          for_all_state(predicate, package.second);
      }
      template <typename Allocator_Policy>
      template <typename Predicate>
      void bitmap_thread_allocator_t<Allocator_Policy>::for_all_state(Predicate &predicate, package_type &package)
      {
        package.for_all(predicate);
      }
    }
  }
}
