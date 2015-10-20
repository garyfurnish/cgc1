#pragma once
#include <cgc1/warning_wrapper_push.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/range/iterator_range.hpp>
#include <cgc1/warning_wrapper_pop.hpp>
#include <boost/property_tree/ptree.hpp>

namespace cgc1
{
  namespace details
  {
    CGC1_OPT_INLINE size_t get_packed_object_size_id(size_t sz)
    {
      const size_t min_2 = 5;
      const size_t max_bins = 4;
      sz -= 1;
      sz = sz >> min_2;
      if (!sz)
        return 0;
      size_t ret = 64 - static_cast<size_t>(__builtin_clzll(sz));
      if (ret > max_bins)
        return ::std::numeric_limits<size_t>::max();
      return ret;
    }
    CGC1_OPT_INLINE auto packed_object_package_t::allocate(size_t id) noexcept -> ::std::pair<void *, size_t>
    {
      auto &vec = m_vectors[id];
      auto range = ::boost::make_iterator_range(vec.rbegin(), vec.rend());
      for (auto &&it = range.begin(); it != range.end(); ++it) {
        auto prev = it + 1;
        if (vec.rend() != prev) {
          auto &prev_packed = **prev;
          cgc1_builtin_prefetch(&prev_packed);
        }
        auto &packed = **it;
        auto ret = packed.allocate();
        if (ret) {
          return ::std::make_pair(ret, packed.real_entry_size());
        }
      }
      return ::std::make_pair(nullptr, 0);
    }
    CGC1_OPT_INLINE void packed_object_package_t::to_ptree(::boost::property_tree::ptree &ptree, int level) const
    {
      (void)level;
      ptree.put("num_vectors", ::std::to_string(cs_num_vectors));
      {
        boost::property_tree::ptree vectors;
        for (size_t i = 0; i < m_vectors.size(); ++i) {
          boost::property_tree::ptree vec;
          vec.put("id", i);
          vec.put("size", m_vectors[i].size());
          vectors.add_child("vector", vec);
        }
        ptree.add_child("vectors", vectors);
      }
    }
  }
}
