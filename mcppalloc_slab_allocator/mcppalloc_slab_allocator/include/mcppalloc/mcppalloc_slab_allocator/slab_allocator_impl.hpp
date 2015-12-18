#include <stdexcept>
#include <mcppalloc/mcppalloc_utils/unsafe_cast.hpp>
namespace mcppalloc
{
  namespace slab_allocator
  {
    namespace details
    {
      inline slab_allocator_t::slab_allocator_t(size_t size, size_t size_hint)
      {
        if (!m_slab.allocate(size, slab_t::find_hole(size_hint)))
          throw ::std::runtime_error("Unable to allocate slab");
        m_end = reinterpret_cast<slab_allocator_object_t *>(m_slab.begin());
        m_end->set_all(reinterpret_cast<slab_allocator_object_t *>(m_slab.end()), false, false);
      }
      inline slab_allocator_t::~slab_allocator_t()
      {
        _verify();
      }
      inline void slab_allocator_t::_verify()
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        for (auto it = _u_object_begin(); it != _u_object_current_end(); ++it) {
          it->verify_magic();
        }
      }
      inline void slab_allocator_t::align_next(size_t sz)
      {
        uint8_t *new_end = unsafe_cast<uint8_t>(align(m_end, sz));
        auto offset = static_cast<size_t>(new_end - unsafe_cast<uint8_t>(m_end)) - cs_header_sz;
        allocate_raw(offset);
      }
      inline uint8_t *slab_allocator_t::begin() const
      {
        return m_slab.begin();
      }
      inline uint8_t *slab_allocator_t::end() const
      {
        return m_slab.end();
      }
      inline next_iterator<slab_allocator_object_t> slab_allocator_t::_u_object_begin()
      {
        return make_next_iterator(reinterpret_cast<slab_allocator_object_t *>(begin()));
      }
      inline next_iterator<slab_allocator_object_t> slab_allocator_t::_u_object_end()
      {
        return make_next_iterator(reinterpret_cast<slab_allocator_object_t *>(end()));
      }
      inline next_iterator<slab_allocator_object_t> slab_allocator_t::_u_object_current_end()
      {
        return make_next_iterator(reinterpret_cast<slab_allocator_object_t *>(m_end));
      }
      inline bool slab_allocator_t::_u_empty()
      {
        return _u_object_current_end() == _u_object_begin();
      }

      inline void *slab_allocator_t::_u_split_allocate(slab_allocator_object_t *object, size_t sz)
      {
        object->verify_magic();
        if (sz + cs_header_sz * 2 > object->object_size(cs_alignment)) {
          // if not enough space to split, just take it all.
          object->set_in_use(true);
          return object->object_start(cs_alignment);
        }
        else {
          // else create a new object state at the right place.
          auto new_next = reinterpret_cast<slab_allocator_object_t *>(object->object_start(cs_alignment) + sz);
          // set new object state.
          new_next->set_all(object->next(), false, object->next_valid());
          // set current object  state.
          object->set_all(new_next, true, true);
          // return location of start of object.
          return object->object_start(cs_alignment);
        }
      }
      inline void *slab_allocator_t::_u_allocate_raw_at_end(size_t sz)
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
        }
        else {
          // ok, we haven't used all the space, so lets put a object state afterwards.
          object->set_next_valid(true);
          m_end->set_all(&*_u_object_end(), false, false);
        }
        auto ret = object->object_start(cs_alignment);
        return ret;
      }
      inline void *slab_allocator_t::allocate_raw(size_t sz)
      {
        // TODO: THIS IS AWFUL as we have to look through everything to see if anything is free.
        // align request.
        sz = align(sz, cs_alignment);
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        // if empty, create at end.
        if (_u_empty()) {
          return _u_allocate_raw_at_end(sz);
        }

        // lb is (right now) for trying precise fit.
        auto lb = _u_object_end();
        // up is basically for doing worst fit by dividing biggest object.
        auto ub = _u_object_end();
        for (auto it = _u_object_begin(); it != _u_object_current_end(); ++it) {
          it->verify_magic();
          if (mcppalloc_unlikely(&*it == _u_object_end())) {
            ::std::cerr << "mcppalloc slab allocator consistency error 8b0fdce5-4991-4ba8-af46-e18cbdaf9dbd" << ::std::endl;
            ::std::terminate();
          }
          if (mcppalloc_unlikely(&*it > _u_object_end())) {
            ::std::cerr << "mcppalloc slab allocator consistency error a35664e3-21f9-4ea7-a921-844b8a2dc598" << ::std::endl;
            ::std::terminate();
          }
          if (mcppalloc_unlikely(reinterpret_cast<uint8_t *>(&*it) < begin())) {
            ::std::cerr << &*it << " " << reinterpret_cast<void *>(begin()) << "\n";
            ::std::cerr << "mcppalloc slab allocator consistency error d4f8215d-98c3-4d1f-9e1b-00f09ae42e5c" << ::std::endl;
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
          // This looks horrible but is just finding approximate lower bound and exact upper bound at the same time.
          // TODO: Can these bounds be made more precise.
          // Try to find an exact LB.
          if (lb == _u_object_end() && it->object_size(cs_alignment) >= sz && it->object_size(cs_alignment) <= sz + cs_alignment)
            lb = it;
          // if new ub would be bigger then current ub, take it.
          if (ub != _u_object_end() && it->object_size(cs_alignment) >= ub->object_size(cs_alignment))
            ub = it;
          // if ub is undefined, take anything valid.
          else if (ub == _u_object_end() && it->object_size(cs_alignment) >= sz)
            ub = it;
          if (mcppalloc_unlikely(!it->next_valid() && it->next() != _u_object_current_end())) {
            ::std::cerr << "mcppalloc slab allocator: consistency error ef2626f0-2073-4932-b3f6-466d4da1bfe1\n";
            ::std::cerr << it->next() << " " << _u_object_current_end() << ::std::endl;
            ::std::terminate();
          }
        }
        if (lb == _u_object_end()) {
          // no precise fit, either split or allocate more memory.
          if (ub == _u_object_end()) {
            return _u_allocate_raw_at_end(sz);
          }
          else {
            return _u_split_allocate(&*ub, sz);
          }
        }
        else {
          lb->verify_magic();
          // precise fit, use it.
          lb->set_in_use(true);
          return lb->object_start(cs_alignment);
        }
      }
      inline void slab_allocator_t::deallocate_raw(void *v)
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
        }
        // if we can coallesce into end pointer, do that.
        if (!object->next_valid() ||
            (object->next_valid() && !object->next()->not_available() && object->next()->next() == &*_u_object_end())) {
          object->set_next(&*_u_object_end());
          m_end = object;
        }
      }
      inline ptrdiff_t slab_allocator_t::offset(void *v) const noexcept
      {
        return reinterpret_cast<ptrdiff_t>(reinterpret_cast<uint8_t *>(v) - begin());
      }
      inline auto slab_allocator_t::current_size() const noexcept -> size_t
      {
        return static_cast<size_t>(reinterpret_cast<uint8_t *>(m_end) - m_slab.begin());
      }
      inline void slab_allocator_t::to_ptree(::boost::property_tree::ptree &ptree, int level) const
      {
        (void)level;
        ptree.put("size", ::std::to_string(m_slab.size()));
        ptree.put("current_size", ::std::to_string(current_size()));
      }
    }
  }
}
