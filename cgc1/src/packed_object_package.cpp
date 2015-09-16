#include "packed_object_package.hpp"
#include "packed_object_package_impl.hpp"

using namespace ::cgc1::literals;
namespace cgc1
{
  namespace details
  {
    packed_object_state_info_t packed_object_package_t::_get_info(size_t id)
    {
      packed_object_state_t state;
      state.m_info =
          packed_object_state_info_t{cs_total_size / ((1 + id) << 5) / 512,
                                     (1_sz << (5 + id)),
                                     0,
                                     0,
                                     {0, 0, packed_object_state_t::cs_magic_number_0, packed_object_state_t::cs_magic_number_1}};
      state._compute_size();
      return state.m_info;
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
