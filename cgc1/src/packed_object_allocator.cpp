#include "packed_object_allocator.hpp"
#include "packed_object_thread_allocator.hpp"
namespace cgc1
{
  namespace details
  {
    packed_object_allocator_t::packed_object_allocator_t(size_t size, size_t size_hint) : m_slab(size, size_hint)
    {
      m_slab.align_next(c_packed_object_block_size);
    }
    packed_object_allocator_t::~packed_object_allocator_t() = default;
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
    void packed_object_allocator_t::_u_to_global(size_t id, packed_object_state_t *state) noexcept
    {
      m_globals.insert(id, state);
    }
    void packed_object_allocator_t::_u_to_free(void *v) noexcept
    {
      m_free_globals.push_back(v);
    }
  }
}
