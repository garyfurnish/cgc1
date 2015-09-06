#include "packed_object_allocator.hpp"
#include "packed_object_thread_allocator.hpp"
namespace cgc1
{
  namespace details
  {
    thread_local packed_object_thread_allocator_t *packed_object_allocator_t::t_thread_allocator = nullptr;
    packed_object_allocator_t::packed_object_allocator_t(size_t size, size_t size_hint) : m_slab(size, size_hint)
    {
      m_slab.align_next(c_packed_object_block_size);
    }
    packed_object_allocator_t::~packed_object_allocator_t() = default;
    auto packed_object_allocator_t::get_ttla() noexcept -> thread_allocator_type *
    {
      return t_thread_allocator;
    }
    void packed_object_allocator_t::set_ttla(thread_allocator_type *ta) noexcept
    {
      t_thread_allocator = ta;
    }

    auto packed_object_allocator_t::_get_memory() noexcept -> packed_object_state_t *
    {
      {
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
        if (!m_free_globals.empty()) {
          auto ret = m_free_globals.back();
          m_free_globals.pop_back();
          return unsafe_cast<packed_object_state_t>(ret);
        }
      }
      return unsafe_cast<packed_object_state_t>(m_slab.allocate_raw(c_packed_object_block_size));
    }
    auto packed_object_allocator_t::initialize_thread() -> thread_allocator_type &
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
    void packed_object_allocator_t::destroy_thread()
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

    void packed_object_allocator_t::_u_to_global(size_t id, packed_object_state_t *state) noexcept
    {
      m_globals.insert(id, state);
    }
    void packed_object_allocator_t::_u_to_free(void *v) noexcept
    {
      m_free_globals.push_back(v);
    }

    auto packed_object_allocator_t::num_free_blocks() const noexcept -> size_t
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return m_free_globals.size();
    }
    auto packed_object_allocator_t::num_globals(size_t id) const noexcept -> size_t
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return m_globals.m_vectors[id].size();
    }
  }
}
