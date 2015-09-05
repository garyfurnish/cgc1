#include "packed_object_thread_allocator.hpp"
#include "packed_object_allocator.hpp"
#include <boost/iterator/zip_iterator.hpp>
#include <boost/range/iterator_range.hpp>
using namespace ::cgc1::literals;
namespace cgc1
{
  namespace details
  {
    packed_object_thread_allocator_t::packed_object_thread_allocator_t(packed_object_allocator_t *allocator)
        : m_allocator(allocator)
    {
    }

    void packed_object_package_t::insert(packed_object_package_t &&state)
    {
      auto begin = ::boost::make_zip_iterator(::boost::make_tuple(m_vectors.begin(), state.m_vectors.begin()));
      auto end = ::boost::make_zip_iterator(::boost::make_tuple(m_vectors.end(), state.m_vectors.end()));
      for (auto vec : ::boost::make_iterator_range(begin, end)) {
        vec.get<0>().insert(vec.get<0>().end(), vec.get<1>().begin(), vec.get<1>().end());
        vec.get<1>().clear();
      }
    }
    void packed_object_package_t::insert(size_t id, packed_object_state_t *state)
    {
      m_vectors[id].emplace_back(state);
    }
    auto packed_object_package_t::allocate(size_t id) noexcept -> void *
    {
      auto &vec = m_vectors[id];
      for (auto &it : ::boost::make_iterator_range(vec.rbegin(), vec.rend())) {
        auto &packed = *it;
        auto ret = packed.allocate();
        if (ret)
          return ret;
      }
      return nullptr;
    }

    auto packed_object_thread_allocator_t::allocate(size_t sz) noexcept -> void *
    {
      const auto id = get_packed_object_size_id(sz);
      void *v = m_locals.allocate(id);
      if (!v) {
        if (!m_free_list.empty()) {
          packed_object_state_t *state = unsafe_cast<packed_object_state_t>(m_free_list.back());
          state->m_info = packed_object_package_t::_get_info(id);
          state->initialize();
          m_free_list.pop_back();
          v = state->allocate();
          m_locals.insert(id, state);
          return v;
        } else {
          packed_object_state_t *state = m_allocator->_get_memory();
          state->m_info = packed_object_package_t::_get_info(id);
          state->initialize();
          v = state->allocate();
          m_locals.insert(id, state);
          return v;
        }
      }
      return v;
    }
    auto packed_object_thread_allocator_t::deallocate(void *v) noexcept -> bool
    {
      uintptr_t vi = reinterpret_cast<uintptr_t>(v);
      size_t mask = ::std::numeric_limits<size_t>::max() << (__builtin_ffsll(c_packed_object_block_size) - 1);

      vi &= mask;
      vi += slab_allocator_t::cs_alignment;
      packed_object_state_t *state = reinterpret_cast<packed_object_state_t *>(vi);
      state->deallocate(v);
      return true;
    }
  }
}
