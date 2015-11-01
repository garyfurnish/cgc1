#pragma once
#include <cgc1/boost/property_tree/ptree.hpp>
#include <cgc1/boost/property_tree/json_parser.hpp>
namespace cgc1
{
  namespace details
  {
    template <typename Allocator_Policy>
    auto packed_object_allocator_t<Allocator_Policy>::begin() const noexcept -> uint8_t *
    {
      return m_slab.begin();
    }
    template <typename Allocator_Policy>
    auto packed_object_allocator_t<Allocator_Policy>::end() const noexcept -> uint8_t *
    {
      return m_slab.end();
    }
    template <typename Allocator_Policy>
    auto packed_object_allocator_t<Allocator_Policy>::_slab() const noexcept -> slab_allocator_t &
    {
      return m_slab;
    }
    template <typename Allocator_Policy>
    packed_object_allocator_t<Allocator_Policy>::packed_object_allocator_t(size_t size, size_t size_hint)
        : m_slab(size, size_hint)
    {
      m_slab.align_next(c_packed_object_block_size);
    }
    template <typename Allocator_Policy>
    packed_object_allocator_t<Allocator_Policy>::~packed_object_allocator_t() = default;
    template <typename Allocator_Policy>
    void packed_object_allocator_t<Allocator_Policy>::shutdown()
    {
      {
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      }
      CGC1_CONCURRENCY_LOCK_ASSUME(m_mutex);
      m_thread_allocators.clear();
      m_free_globals.clear();
      m_globals.shutdown();
      {
        auto a1 = ::std::move(m_thread_allocators);
        auto a2 = ::std::move(m_free_globals);
      }
    }
    template <typename Allocator_Policy>
    auto packed_object_allocator_t<Allocator_Policy>::get_ttla() noexcept -> thread_allocator_type *
    {
      return t_thread_allocator;
    }
    template <typename Allocator_Policy>
    void packed_object_allocator_t<Allocator_Policy>::set_ttla(thread_allocator_type *ta) noexcept
    {
      t_thread_allocator = ta;
    }
    template <typename Allocator_Policy>
    auto packed_object_allocator_t<Allocator_Policy>::_get_memory() noexcept -> packed_object_state_t *
    {
      {
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
        if (!m_free_globals.empty()) {
          auto ret = m_free_globals.back();
          m_free_globals.pop_back();
          return unsafe_cast<packed_object_state_t>(ret);
        }
      }
      auto new_memory =
          unsafe_cast<packed_object_state_t>(m_slab.allocate_raw(c_packed_object_block_size - slab_allocator_t::cs_alignment));
      return new_memory;
    }
    template <typename Allocator_Policy>
    auto packed_object_allocator_t<Allocator_Policy>::initialize_thread() -> thread_allocator_type &
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      auto ttla = get_ttla();
      if (ttla)
        return *ttla;
      // one doesn't already exist.
      // create a thread allocator.
      thread_allocator_unique_ptr_type ta = make_unique_allocator<thread_allocator_type, allocator>(*this);
      // get a reference to thread allocator.
      auto &ret = *ta.get();
      set_ttla(&ret);
      // put the thread allocator in the thread allocator list.
      m_thread_allocators.emplace(::std::this_thread::get_id(), ::std::move(ta));
      return ret;
    }
    template <typename Allocator_Policy>
    void packed_object_allocator_t<Allocator_Policy>::destroy_thread()
    {
      // this is outside of scope so that the lock is not held when it is destroyed.
      thread_allocator_unique_ptr_type ptr;
      {
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
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
    void packed_object_allocator_t<Allocator_Policy>::_u_to_global(size_t id, packed_object_state_t *state) noexcept
    {
      m_globals.insert(id, state);
    }
    template <typename Allocator_Policy>
    void packed_object_allocator_t<Allocator_Policy>::_u_to_free(void *v) noexcept
    {
      m_free_globals.push_back(v);
    }
    template <typename Allocator_Policy>
    auto packed_object_allocator_t<Allocator_Policy>::num_free_blocks() const noexcept -> size_t
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return m_free_globals.size();
    }
    template <typename Allocator_Policy>
    auto packed_object_allocator_t<Allocator_Policy>::num_globals(size_t id) const noexcept -> size_t
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return m_globals.m_vectors[id].size();
    }
    template <typename Allocator_Policy>
    auto packed_object_allocator_t<Allocator_Policy>::allocator_policy() noexcept -> allocator_policy_type &
    {
      return m_policy;
    }
    template <typename Allocator_Policy>
    auto packed_object_allocator_t<Allocator_Policy>::allocator_policy() const noexcept -> const allocator_policy_type &
    {
      return m_policy;
    }
    template <typename Allocator_Policy>
    void packed_object_allocator_t<Allocator_Policy>::_u_set_force_maintenance()
    {
      for (auto &&ta : m_thread_allocators) {
        ta.second->set_force_maintenance();
      }
    }
    template <typename Allocator_Policy>
    void packed_object_allocator_t<Allocator_Policy>::to_ptree(::boost::property_tree::ptree &ptree, int level) const
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      ptree.put("num_free_globals", ::std::to_string(m_free_globals.size()));
      {
        ::boost::property_tree::ptree globals;
        m_globals.to_ptree(globals, level);
        ptree.add_child("globals", globals);
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
  }
}
