#pragma once
namespace mcppalloc
{
  namespace slab_allocator
  {
    namespace details
    {
      inline uint8_t *slab_allocator_t::begin() const
      {
        return m_slab.begin();
      }
      inline uint8_t *slab_allocator_t::end() const
      {
        return m_slab.end();
      }
      inline mcpputil::next_iterator<slab_allocator_object_t> slab_allocator_t::_u_object_begin()
      {
        return mcpputil::make_next_iterator(reinterpret_cast<slab_allocator_object_t *>(begin()));
      }
      inline mcpputil::next_iterator<slab_allocator_object_t> slab_allocator_t::_u_object_end()
      {
        return mcpputil::make_next_iterator(reinterpret_cast<slab_allocator_object_t *>(end()));
      }
      inline mcpputil::next_iterator<slab_allocator_object_t> slab_allocator_t::_u_object_current_end()
      {
        return mcpputil::make_next_iterator(reinterpret_cast<slab_allocator_object_t *>(m_end));
      }
      inline bool slab_allocator_t::_u_empty()
      {
        return _u_object_current_end() == _u_object_begin();
      }
    }
  }
}
