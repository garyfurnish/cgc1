#pragma once
#include <iostream>
#include <mcpputil/mcpputil/unsafe_cast.hpp>
#include <stdexcept>
namespace mcppalloc
{
  namespace slab_allocator
  {
    namespace details
    {
      slab_allocator_t::slab_allocator_t(size_t size, size_t size_hint)

      {
        m_free_map = free_map_type(m_free_map_back.data(), sizeof(m_free_map_back));
        if (!m_slab.allocate(size, mcpputil::slab_t::find_hole(size_hint)))
          throw ::std::runtime_error("Unable to allocate slab");
        m_end = reinterpret_cast<slab_allocator_object_t *>(m_slab.begin());
        m_end->set_all(reinterpret_cast<slab_allocator_object_t *>(m_slab.end()), false, false);
      }
      slab_allocator_t::~slab_allocator_t()
      {
        _verify();
      }
      void slab_allocator_t::_verify()
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        for (auto it = _u_object_begin(); it != _u_object_current_end(); ++it) {
          it->verify_magic();
        }
      }
      void slab_allocator_t::align_next(size_t sz)
      {
        uint8_t *new_end = mcpputil::unsafe_cast<uint8_t>(mcpputil::align(m_end, sz));
        auto offset = static_cast<size_t>(new_end - mcpputil::unsafe_cast<uint8_t>(m_end)) - cs_header_sz;
        allocate_raw(offset);
      }
      void slab_allocator_t::_u_add_free(slab_allocator_object_t *v)
      {
        auto ret = m_free_map.insert(::std::make_pair(v->object_size(), v));
        if (!ret.second)
          m_free_map_needs_regeneration = true;
      }
      void slab_allocator_t::_u_remove_free(slab_allocator_object_t *v)
      {
        for (auto it = m_free_map.begin(); it != m_free_map.end(); ++it) {
          if (it->second == v) {
            m_free_map.erase(it);
            return;
          }
        }
      }
      void slab_allocator_t::_u_move_free(slab_allocator_object_t *orig, slab_allocator_object_t *new_obj)
      {
        auto orig_it = m_free_map.lower_bound(orig->object_size());
        while (orig_it != m_free_map.end()) {
          if (orig_it->second != orig)
            orig_it++;
          else
            break;
        }
        if (orig_it == m_free_map.end()) {
          _u_add_free(new_obj);
          return;
        }
        m_free_map.move(orig_it, ::std::make_pair(new_obj->object_size(), new_obj));
      }
      void slab_allocator_t::_u_generate_free_list()
      {
        m_free_map_needs_regeneration = false;
        for (auto it = _u_object_begin(); it != _u_object_current_end(); ++it) {
          it->verify_magic();
          if (mcpputil_unlikely(&*it >= _u_object_end())) {
            ::std::cerr << "mcppalloc slab allocator consistency error a35664e3-21f9-4ea7-a921-844b8a2dc598" << ::std::endl;
            ::std::terminate();
          }
          // if in use, go to next.
          if (it->not_available())
            continue;
          // if next valid, and both this and next object state are not in use, coalesce.
          while (it->next_valid() && !it->next()->not_available()) {
            if (!it->next()->next_valid())
              break;
            it->set_all(it->next()->next(), false, it->next()->next_valid());
          }
          _u_add_free(it);
        }
      }
      void *slab_allocator_t::_u_split_allocate(slab_allocator_object_t *object, size_t sz)
      {
        object->verify_magic();
        if (sz + cs_header_sz * 2 > object->object_size(cs_alignment)) {
          _u_remove_free(object);
          // if not enough space to split, just take it all.
          object->set_in_use(true);
          return object->object_start(cs_alignment);
        } else {
          // else create a new object state at the right place.
          auto new_next = reinterpret_cast<slab_allocator_object_t *>(object->object_start(cs_alignment) + sz);
          // set new object state.
          new_next->set_all(object->next(), false, object->next_valid());
          _u_move_free(object, new_next);
          // set current object  state.
          object->set_all(new_next, true, true);
          // return location of start of object.
          return object->object_start(cs_alignment);
        }
      }
      void *slab_allocator_t::_u_allocate_raw_at_end(size_t sz)
      {
        // get total needed size.
        size_t total_size = slab_allocator_object_t::needed_size(sizeof(slab_allocator_object_t), sz, cs_alignment);
        auto object = m_end;
        // tack on needed size to current end.
        auto new_end = reinterpret_cast<slab_allocator_object_t *>(reinterpret_cast<uint8_t *>(m_end) + total_size);
        // expand until current end is in the slab.
        while (new_end > _u_object_end() && m_slab.expand(m_slab.size() * 2)) {
        }
        // if we couldn't do that, then we are out of memory and hard fail.
        if (new_end > _u_object_end())
          return nullptr;
        m_end = new_end;
        // ok, setup object state for new allocation.
        object->set_in_use(true);
        object->set_next(m_end);

        if (m_end == &*_u_object_end()) {
          // have used all space so no object state afterwards.
          m_end = _u_object_end();
          object->set_next_valid(false);
        } else {
          // ok, we haven't used all the space, so lets put a object state afterwards.
          object->set_next_valid(true);
          m_end->set_all(&*_u_object_end(), false, false);
        }
        auto ret = object->object_start(cs_alignment);
        return ret;
      }
      void *slab_allocator_t::allocate_raw(size_t sz)
      {
        sz = mcpputil::align(sz, cs_alignment);
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        // if empty, create at end.
        if (_u_empty()) {
          return _u_allocate_raw_at_end(sz);
        }
        auto it = m_free_map.lower_bound(sz);
        if (it == m_free_map.end()) {
          if (m_free_map.size() == m_free_map.capacity())
            return _u_allocate_raw_at_end(sz);
          else {
            if (m_free_map_needs_regeneration) {
              _u_generate_free_list();
              it = m_free_map.find(sz);
            }
            if (it == m_free_map.end())
              return _u_allocate_raw_at_end(sz);
            else
              return _u_split_allocate(it->second, sz);
          }
        } else {
          return _u_split_allocate(it->second, sz);
        }
      }
      void slab_allocator_t::deallocate_raw(void *v)
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        auto object = slab_allocator_object_t::from_object_start(v, cs_alignment);
        object->verify_magic();
        // set not in use.
        object->set_in_use(false);
        // coalesce if possible.
        while (object->next_valid() && !object->next()->not_available()) {
          auto next = object->next();
          object->set_all(next->next(), false, next->next_valid());
          if (next != _u_object_current_end()) {
            _u_remove_free(next);
          }
        }
        // if we can coallesce into end pointer, do that.
        if (!object->next_valid() ||
            (object->next_valid() && !object->next()->not_available() && object->next()->next() == &*_u_object_end())) {
          object->set_next(&*_u_object_end());
          m_end = object;
        } else {
          _u_add_free(object);
        }
      }
      ptrdiff_t slab_allocator_t::offset(void *v) const noexcept
      {
        return static_cast<ptrdiff_t>(reinterpret_cast<uint8_t *>(v) - begin());
      }
      auto slab_allocator_t::current_size() const noexcept -> size_t
      {
        return static_cast<size_t>(reinterpret_cast<uint8_t *>(m_end) - m_slab.begin());
      }
      void slab_allocator_t::to_ptree(::boost::property_tree::ptree &ptree, int level) const
      {
        (void)level;
        ptree.put("size", ::std::to_string(m_slab.size()));
        ptree.put("current_size", ::std::to_string(current_size()));
      }
    }
  }
}
