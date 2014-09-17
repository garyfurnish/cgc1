namespace cgc1
{
  namespace details
  {
    CGC1_OPT_INLINE slab_allocator_t::slab_allocator_t(size_t size, size_t size_hint)
    {
      if (!m_slab.allocate(size, slab_t::find_hole(size_hint)))
        throw ::std::runtime_error("Unable to allocate slab");
      m_end = reinterpret_cast<slab_allocator_object_t *>(m_slab.begin());
      m_end->set_all(reinterpret_cast<slab_allocator_object_t *>(m_slab.end()), false, false);
    }
    CGC1_OPT_INLINE uint8_t *slab_allocator_t::begin() const
    {
      return m_slab.begin();
    }
    CGC1_OPT_INLINE uint8_t *slab_allocator_t::end() const
    {
      return m_slab.end();
    }
    CGC1_OPT_INLINE next_iterator<slab_allocator_object_t> slab_allocator_t::_u_object_begin()
    {
      return make_next_iterator(reinterpret_cast<slab_allocator_object_t *>(begin()));
    }
    CGC1_OPT_INLINE next_iterator<slab_allocator_object_t> slab_allocator_t::_u_object_end()
    {
      return make_next_iterator(reinterpret_cast<slab_allocator_object_t *>(end()));
    }
    CGC1_OPT_INLINE next_iterator<slab_allocator_object_t> slab_allocator_t::_u_object_current_end()
    {
      return make_next_iterator(reinterpret_cast<slab_allocator_object_t *>(m_end));
    }
    CGC1_OPT_INLINE bool slab_allocator_t::_u_empty()
    {
      return _u_object_current_end() == _u_object_begin();
    }
    CGC1_OPT_INLINE void slab_allocator_t::lock()
    {
      m_mutex.lock();
    }
    CGC1_OPT_INLINE void slab_allocator_t::unlock()
    {
      m_mutex.unlock();
    }
    CGC1_OPT_INLINE bool slab_allocator_t::try_lock()
    {
      return m_mutex.try_lock();
    }

    CGC1_OPT_INLINE void *slab_allocator_t::_u_split_allocate(slab_allocator_object_t *object, size_t sz)
    {
      if (sz + c_alignment * 2 > object->object_size()) {
        object->set_in_use(true);
        return object->object_start();
      } else {
        auto new_next = reinterpret_cast<slab_allocator_object_t *>(object->object_start() + sz);
        new_next->set_all(object->next(), false, object->next_valid());
        object->set_all(new_next, true, true);
        return object->object_start();
      }
    }
    CGC1_OPT_INLINE void *slab_allocator_t::_u_allocate_raw_at_end(size_t sz)
    {
      size_t total_size = slab_allocator_object_t::needed_size(sizeof(slab_allocator_object_t), sz);
      auto object = m_end;
      m_end = reinterpret_cast<slab_allocator_object_t *>(reinterpret_cast<uint8_t *>(m_end) + total_size);
      while (m_end > _u_object_end() && m_slab.expand(m_slab.size() * 2)) {
      }
      if (m_end > _u_object_end())
        abort();
      object->set_in_use(true);
      object->set_next(m_end);
      if (m_end == &*_u_object_end()) {
        m_end = _u_object_end();
        object->set_next_valid(false);
      } else {
        object->set_next_valid(true);
        m_end->set_all(&*_u_object_end(), false, false);
      }
      return object->object_start();
    }
    CGC1_OPT_INLINE void *slab_allocator_t::allocate_raw(size_t sz)
    {
      sz = align(sz);
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      if (_u_empty()) {
        return _u_allocate_raw_at_end(sz);
      }
      auto lb = _u_object_end();
      auto ub = _u_object_end();
      for (auto it = _u_object_begin(); it != _u_object_current_end(); ++it) {
        if (it->not_available())
          continue;
        while (it->next_valid() && !it->next()->not_available()) {
          if (!it->next()->next_valid())
            break;
          it->set_all(it->next()->next(), false, it->next()->next_valid());
        }
        // This looks horrible but is just finding approximate lower bound and exact upper bound at the same time.
        if (lb == _u_object_end() && it->object_size() >= sz && it->object_size() <= sz + c_alignment)
          lb = it;
        if (ub != _u_object_end() && it->object_size() >= ub->object_size())
          ub = it;
        else if (ub == _u_object_end() && it->object_size() >= sz)
          ub = it;
      }
      if (lb == _u_object_end()) {
        if (ub == _u_object_end()) {
          return _u_allocate_raw_at_end(sz);
        } else {
          return _u_split_allocate(&*ub, sz);
        }
      } else {
        lb->set_in_use(true);
        return lb->object_start();
      }
    }
    CGC1_OPT_INLINE void slab_allocator_t::deallocate_raw(void *v)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      auto object = slab_allocator_object_t::from_object_start(v);
      object->set_in_use(false);
      while (object->next_valid() && !object->next()->not_available()) {
        auto next = object->next();
        object->set_all(next->next(), false, next->next_valid());
      }
      if (!object->next_valid() ||
          (object->next_valid() && !object->next()->not_available() && object->next()->next() == &*_u_object_end())) {
        object->set_next(&*_u_object_end());
        m_end = object;
      }
    }
  }
}
