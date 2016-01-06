#pragma once
#include <mcppalloc/mcppalloc_utils/boost/iterator/zip_iterator.hpp>
#include <mcppalloc/mcppalloc_utils/boost/property_tree/ptree.hpp>
#include <mcppalloc/mcppalloc_utils/boost/range/iterator_range.hpp>
#include <mcppalloc/mcppalloc_utils/literals.hpp>
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {
      constexpr const size_t min_2 = 5;
      constexpr const size_t max_bins = 4;

      inline size_t fits_in_bins(size_t sz)
      {
        sz -= 1;
        sz = sz >> min_2;
        sz = sz >> max_bins;
        return sz == 0;
      }
      inline size_t get_bitmap_size_id(size_t sz)
      {
        sz -= 1;
        sz = sz >> min_2;
        if (!sz)
          return 0;
        size_t ret = 64 - static_cast<size_t>(__builtin_clzll(sz));
        if (ret > max_bins)
          ret = ::std::numeric_limits<size_t>::max();
        return ret;
      }
      template <typename Allocator_Policy>
      bitmap_package_t<Allocator_Policy>::bitmap_package_t(type_id_t type_id) : m_type_id(type_id)
      {
      }
      template <typename Allocator_Policy>
      auto bitmap_package_t<Allocator_Policy>::type_id() const noexcept -> type_id_t
      {
        return m_type_id;
      }
      template <typename Allocator_Policy>
      auto bitmap_package_t<Allocator_Policy>::allocate(size_t id) noexcept -> block_type
      {
        block_type ret{nullptr, 0};
        auto &entry = m_vectors[id];
        auto &vec = entry.m_vector;
        while (!vec.empty()) {
          auto &packed = *vec.back();
          packed.verify_magic();
          ret.m_ptr = packed.allocate();
          if (ret.m_ptr) {
            ret.m_size = packed.real_entry_size();
            return ret;
          } else {
            entry.m_full_vector.push_back(&packed);
            vec.pop_back();
          }
        }
        entry.m_times_since_full_search++;
        if (entry.m_times_since_full_search == 50) {
          vec.insert(vec.end(), entry.m_full_vector.begin(), entry.m_full_vector.end());
          entry.m_full_vector.clear();
          entry.m_times_since_full_search = 0;
        }
        return ret;
      }
      template <typename Allocator_Policy>
      void bitmap_package_t<Allocator_Policy>::to_ptree(::boost::property_tree::ptree &ptree, int level) const
      {
        (void)level;
        ptree.put("num_vectors", ::std::to_string(cs_num_vectors));
        {
          boost::property_tree::ptree vectors;
          for (size_t i = 0; i < m_vectors.size(); ++i) {
            boost::property_tree::ptree vec;
            vec.put("id", i);
            vec.put("size", m_vectors[i].m_vector.size());
            vectors.add_child("vector", vec);
          }
          ptree.add_child("vectors", vectors);
        }
      }
      template <typename Allocator_Policy>
      bitmap_state_info_t bitmap_package_t<Allocator_Policy>::_get_info(size_t id, type_id_t type_id)
      {
        using namespace mcppalloc::literals;
        bitmap_state_t state;
        state.m_internal.m_info = bitmap_state_info_t{static_cast<uint32_t>(cs_total_size / ((1 + id) << 5) / 512),
                                                      (1u << (5 + id)),
                                                      0,
                                                      0,
                                                      0,
                                                      nullptr,
                                                      type_id,
                                                      0,
                                                      {0, 0, 0}};
        state._compute_size();
        return state.m_internal.m_info;
      }
      template <typename Allocator_Policy>
      void bitmap_package_t<Allocator_Policy>::insert(bitmap_package_t &&state)
      {
        auto begin = ::boost::make_zip_iterator(::boost::make_tuple(m_vectors.begin(), state.m_vectors.begin()));
        auto end = ::boost::make_zip_iterator(::boost::make_tuple(m_vectors.end(), state.m_vectors.end()));
        for (auto entry : ::boost::make_iterator_range(begin, end)) {
          entry.template get<0>().m_vector.insert(entry.template get<0>().m_vector.end(),
                                                  entry.template get<1>().m_vector.begin(),
                                                  entry.template get<1>().m_vector.end());
          entry.template get<1>().m_vector.clear();
        }
      }
      template <typename Allocator_Policy>
      void bitmap_package_t<Allocator_Policy>::insert(size_t id, bitmap_state_t *state)
      {
        m_vectors[id].m_vector.emplace_back(state);
      }
      template <typename Allocator_Policy>
      auto bitmap_package_t<Allocator_Policy>::remove(size_t id, bitmap_state_t *state) -> bool
      {
        assert(id < m_vectors.size());
        auto &entry = m_vectors[id];
        auto &vec = entry.m_vector;
        auto it = ::std::find(vec.begin(), vec.end(), state);
        if (it == vec.end())
          return false;
        vec.erase(it);
        return true;
      }
      template <typename Allocator_Policy>
      void bitmap_package_t<Allocator_Policy>::do_maintenance(free_list_type &free_list) noexcept
      {
        for (auto &&entry : m_vectors) {
          auto &&vec = entry.m_vector;
          entry.m_times_since_full_search = 0;
          vec.insert(vec.end(), entry.m_full_vector.begin(), entry.m_full_vector.end());
          entry.m_full_vector.clear();
          // move out any free vectors.
          auto it = ::std::partition(vec.begin(), vec.end(), [](auto &&obj) { return !obj->all_free(); });
          size_t num_to_move = static_cast<size_t>(vec.end() - it);
          if (num_to_move) {
            free_list.insert(free_list.end(), ::std::make_move_iterator(it), ::std::make_move_iterator(vec.end()));
            vec.resize(vec.size() - num_to_move);
          }
        }
      }
      template <typename Allocator_Policy>
      void bitmap_package_t<Allocator_Policy>::shutdown()
      {
        for (auto &&entry : m_vectors) {
          auto &&vec = entry.m_vector;
          vec.clear();
          auto a1 = ::std::move(vec);
          (void)a1;
        }
      }
      template <typename Allocator_Policy>
      template <typename Predicate>
      void bitmap_package_t<Allocator_Policy>::for_all(Predicate &&predicate)
      {
        for (auto &&entry : m_vectors) {
          auto &&vec = entry.m_vector;
          for (auto &&state : vec) {
            predicate(state);
          }
          auto &&full_vec = entry.m_full_vector;
          for (auto &&state : full_vec) {
            predicate(state);
          }
        }
      }
    }
  }
}
