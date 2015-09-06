#include "packed_object_package.hpp"
#include "packed_object_package_impl.hpp"
#include <boost/iterator/zip_iterator.hpp>
#include <boost/range/iterator_range.hpp>

using namespace ::cgc1::literals;
namespace cgc1
{
  namespace details
  {
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
    auto packed_object_package_t::remove(size_t id, packed_object_state_t *state) -> bool
    {
      assert(id < m_vectors.size());
      auto &vec = m_vectors[id];
      auto it = ::std::find(vec.begin(), vec.end(), state);
      if (it == vec.end())
        return false;
      vec.erase(it);
      return true;
    }
    void packed_object_package_t::do_maintenance(cgc_internal_vector_t<void *> &free_list) noexcept
    {
      for (auto &&vec : m_vectors) {
        // move out any free vectors.
        auto it = ::std::remove_if(vec.begin(), vec.end(), [](auto &&obj) { return obj->all_free(); });
        size_t num_to_move = static_cast<size_t>(vec.end() - it);
        if (num_to_move) {
          free_list.resize(free_list.size() + num_to_move);
          ::std::move(it, vec.end(), free_list.end());
          vec.resize(vec.size() - num_to_move);
        }
      }
    }
  }
}
