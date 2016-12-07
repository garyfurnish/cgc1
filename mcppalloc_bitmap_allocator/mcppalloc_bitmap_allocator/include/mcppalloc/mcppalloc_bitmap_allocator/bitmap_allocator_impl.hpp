#pragma once
#include <mcpputil/mcpputil/boost/property_tree/json_parser.hpp>
#include <mcpputil/mcpputil/boost/property_tree/ptree.hpp>
#include <mcpputil/mcpputil/thread_id_manager.hpp>
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::begin() const noexcept -> uint8_t *
      {
        return m_slab.begin();
      }
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::end() const noexcept -> uint8_t *
      {
        return m_slab.end();
      }
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::_slab() const noexcept -> slab_allocator_type &
      {
        return m_slab;
      }
      template <typename Allocator_Policy>
      bitmap_allocator_t<Allocator_Policy>::bitmap_allocator_t(size_t size, size_t size_hint) : m_slab(size, size_hint)
      {
        m_thread_allocator_by_manager_id.resize(gsl::narrow<size_t>(mcpputil::thread_id_manager_t::gs().current_thread_id()));
        for (auto &&ptr : m_thread_allocator_by_manager_id)
          ptr = nullptr;
        m_slab.align_next(c_bitmap_block_size);
      }
      template <typename Allocator_Policy>
      bitmap_allocator_t<Allocator_Policy>::~bitmap_allocator_t() = default;
      template <typename Allocator_Policy>
      void bitmap_allocator_t<Allocator_Policy>::shutdown()
      {
        {
          MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        }
        MCPPALLOC_CONCURRENCY_LOCK_ASSUME(m_mutex);
        m_types.clear();
        m_thread_allocators.clear();
        m_free_globals.clear();
        for (auto &&package_it : m_globals)
          package_it.second.shutdown();
        {
          auto a1 = ::std::move(m_thread_allocators);
          auto a4 = ::std::move(m_types);
          auto a2 = ::std::move(m_free_globals);
          auto a3 = ::std::move(m_globals);
        }
      }
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::get_ttla() noexcept -> thread_allocator_type *
      {
        return m_thread_allocator_by_manager_id[::gsl::narrow_cast<size_t>(
            mcpputil::thread_id_manager_t::gs().current_thread_id())];
      }
      template <typename Allocator_Policy>
      void bitmap_allocator_t<Allocator_Policy>::set_ttla(thread_allocator_type *ta) noexcept
      {
        m_thread_allocator_by_manager_id[::gsl::narrow<size_t>(mcpputil::thread_id_manager_t::gs().current_thread_id())] = ta;
      }
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::_get_memory() -> bitmap_state_t *
      {
        bitmap_state_t *ret;
        {
          MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
          if (!m_free_globals.empty()) {
            ret = mcpputil::unsafe_cast<bitmap_state_t>(m_free_globals.back());
            m_free_globals.pop_back();
            return ret;
          }
        }
        ret = mcpputil::unsafe_cast<bitmap_state_t>(m_slab.allocate_raw(c_bitmap_block_size - slab_allocator_type::cs_header_sz));
        if (mcpputil_unlikely(!ret)) {
          return nullptr;
        }
        ret->initialize_consts();
        return ret;
      }
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::initialize_thread() -> thread_allocator_type &
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        auto ttla = get_ttla();
        if (ttla)
          return *ttla;
        // static initialization could have caused this to spuriously fail, so check that.
        {
          const auto existing = m_thread_allocators.find(::std::this_thread::get_id());
          if (existing != m_thread_allocators.end()) {
            ttla = existing->second.get();
            return *ttla;
          }
        }
        // one doesn't already exist.
        // create a thread allocator.
        thread_allocator_unique_ptr_type ta =
            mcpputil::make_unique_allocator<thread_allocator_type, internal_allocator_type>(*this);
        // get a reference to thread allocator.
        auto &ret = *ta.get();
        set_ttla(&ret);
        // put the thread allocator in the thread allocator list.
        m_thread_allocators.emplace(::std::this_thread::get_id(), ::std::move(ta));
        return ret;
      }
      template <typename Allocator_Policy>
      void bitmap_allocator_t<Allocator_Policy>::destroy_thread()
      {
        // this is outside of scope so that the lock is not held when it is destroyed.
        thread_allocator_unique_ptr_type ptr;
        {
          MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
          set_ttla(nullptr);
          auto it = m_thread_allocators.find(::std::this_thread::get_id());
          // check to make sure it was initialized at some point.
          if (it == m_thread_allocators.end())
            return;
          // just move the owning pointer into ptr.
          // this guarentees it will be erased when this function returns.
          ptr = std::move(it->second);
          // erase the entry in the ta list.
          m_thread_allocators.erase(it);
        }
      }
      template <typename Allocator_Policy>
      void bitmap_allocator_t<Allocator_Policy>::_u_to_global(size_t id, type_id_t type, bitmap_state_t *state) noexcept
      {
        auto package = m_globals.find(type);
        if (package == m_globals.end()) {
          package = m_globals.insert(::std::make_pair(type, package_type(type))).first;
        }
        package->second.insert(id, state);
        state->set_bitmap_package(&package->second);
      }
      template <typename Allocator_Policy>
      void bitmap_allocator_t<Allocator_Policy>::_u_to_free(void *v) noexcept
      {
        m_free_globals.push_back(v);
      }
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::num_free_blocks() const noexcept -> size_t
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        return m_free_globals.size();
      }
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::num_globals(size_t id, type_id_t type) const noexcept -> size_t
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        auto it = m_globals.find(type);
        if (mcpputil_unlikely(it == m_globals.end()))
          return 0;
        return it->second.m_vectors[id].m_vector.size();
      }
      template <typename Allocator_Policy>
      void bitmap_allocator_t<Allocator_Policy>::add_type(bitmap_type_info_t &&type_info)
      {
        m_types[type_info.type_id()] = ::std::move(type_info);
      }
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::get_type(type_id_t type_id) -> const bitmap_type_info_t &
      {
        auto it = m_types.find(type_id);
        if (it == m_types.end())
          throw ::std::runtime_error("mcppalloc: bitmap_allocator::get_type 8c622768-3fe5-4338-b9a3-1db6b9d2ba98");
        return it->second;
      }

      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::allocator_policy() noexcept -> allocator_thread_policy_type &
      {
        return m_policy;
      }
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::allocator_policy() const noexcept -> const allocator_thread_policy_type &
      {
        return m_policy;
      }
      template <typename Allocator_Policy>
      void bitmap_allocator_t<Allocator_Policy>::_u_set_force_maintenance()
      {
        for (auto &&ta : m_thread_allocators) {
          ta.second->set_force_maintenance();
        }
      }
      template <typename Allocator_Policy>
      void bitmap_allocator_t<Allocator_Policy>::to_ptree(::boost::property_tree::ptree &ptree, int level) const
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        ptree.put("num_free_globals", ::std::to_string(m_free_globals.size()));
        {
          ::boost::property_tree::ptree types;
          for (auto &&package_it : m_globals) {
            ::boost::property_tree::ptree globals;
            package_it.second.to_ptree(globals, level);
            types.add_child("globals", globals);
          }
          ptree.add_child("types", types);
        }
        {
          ::boost::property_tree::ptree thread_allocators;
          for (auto &&pair : m_thread_allocators) {
            ::boost::property_tree::ptree thread_allocator;
            thread_allocator.put("id", pair.first);
            pair.second->to_ptree(thread_allocator, level);
            thread_allocators.add_child("thread_allocator", thread_allocator);
          }
          ptree.add_child("thread_allocators", thread_allocators);
        }
      }
      template <typename Allocator_Policy>
      auto bitmap_allocator_t<Allocator_Policy>::_mutex() const noexcept -> mutex_type &
      {
        return m_mutex;
      }
      template <typename Allocator_Policy>
      template <typename Predicate>
      void bitmap_allocator_t<Allocator_Policy>::_for_all_state(Predicate &&predicate)
      {
        MCPPALLOC_CONCURRENCY_LOCK_ASSUME(m_mutex);
        for (auto &&thread : m_thread_allocators)
          thread.second->for_all_state(predicate);
        for (auto &&package_it : m_globals)
          package_it.second.for_all(::std::forward<Predicate>(predicate));
      }
    }
  }
}
