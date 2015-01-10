#pragma once
#include "allocator_block_set.hpp"
#include <assert.h>
namespace cgc1
{
  namespace details
  {
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline allocator_block_set_t<Allocator, Allocator_Block_User_Data>::allocator_block_set_t(size_t allocator_min_size,
                                                                                              size_t allocator_max_size)
        : m_allocator_min_size(allocator_min_size), m_allocator_max_size(allocator_max_size)
    {
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::_set_allocator_sizes(size_t min, size_t max)
    {
      m_allocator_min_size = min;
      m_allocator_max_size = max;
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline size_t allocator_block_set_t<Allocator, Allocator_Block_User_Data>::allocator_min_size() const
    {
      return m_allocator_min_size;
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline size_t allocator_block_set_t<Allocator, Allocator_Block_User_Data>::allocator_max_size() const
    {
      return m_allocator_max_size;
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline size_t allocator_block_set_t<Allocator, Allocator_Block_User_Data>::size() const
    {
      return m_blocks.size();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::regenerate_available_blocks()
    {
      auto abrvr_compare = [](const sized_block_ref_t &r,
                              typename allocator_block_reference_vector_t::const_reference it) { return r.first < it.first; };
      m_available_blocks.clear();
      for (auto &block : m_blocks) {
        if (&block == m_back)
          continue;
        if (!block.full()) {
          sized_block_ref_t pair = ::std::make_pair(block.max_alloc_available(), &block);
          m_available_blocks.push_back(::std::move(pair));
        }
      }
      ::std::sort(m_available_blocks.begin(), m_available_blocks.end(), abrvr_compare);
      _verify();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::collect()
    {
      for (auto &block : m_blocks) {
        block.collect();
      }
      regenerate_available_blocks();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::_verify() const
    {
#if _CGC1_DEBUG_LEVEL > 1
      auto abrvr_compare = [](const sized_block_ref_t &r,
                              typename allocator_block_reference_vector_t::const_reference it) { return r.first < it.first; };

      for (const auto &ab : m_available_blocks) {
        assert(ab.first == ab.second->max_alloc_available());
      }
      assert(::std::is_sorted(m_available_blocks.begin(), m_available_blocks.end(), abrvr_compare));
      if (::std::adjacent_find(m_available_blocks.begin(), m_available_blocks.end()) != m_available_blocks.end())
        assert(0);
#endif
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline void *allocator_block_set_t<Allocator, Allocator_Block_User_Data>::allocate(size_t sz)
    {
      auto abrvr_compare = [](const sized_block_ref_t &r,
                              typename allocator_block_reference_vector_t::const_reference it) { return r.first < it.first; };
      _verify();
      auto lower_bound =
          ::std::lower_bound(m_available_blocks.begin(), m_available_blocks.end(), sized_block_ref_t(sz, nullptr), abrvr_compare);
      if (lower_bound == m_available_blocks.end()) {
        if (m_back) {
          void *ret = m_back->allocate(sz);
          if (ret)
            return ret;
        }
        return nullptr;
      }
      auto ret = lower_bound->second->allocate(sz);
      if (!ret) {
        ret = lower_bound->second->allocate(sz);
        assert(0);
        return nullptr;
      }
      auto it = lower_bound;
      it->first = it->second->max_alloc_available();
      if (it->first == 0) {
        m_available_blocks.erase(it);
        _verify();
        return ret;
      }
      auto new_ub = ::std::upper_bound(m_available_blocks.begin(), it, *it, abrvr_compare);
      if (new_ub == it)
        return ret;
      ::std::rotate(new_ub, it, it + 1);
      _verify();
      return ret;
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline bool allocator_block_set_t<Allocator, Allocator_Block_User_Data>::destroy(void *v)
    {
      _verify();
      auto abrvr_compare = [](const allocator_block_set_t<Allocator, Allocator_Block_User_Data>::sized_block_ref_t &r,
                              typename allocator_block_reference_vector_t::const_reference it) { return r.first < it.first; };
      for (typename allocator_block_vector_t::iterator it = m_blocks.begin(); it != m_blocks.end(); ++it) {
        size_t prev_available = it->max_alloc_available();
        if (it->destroy(v)) {
          if (&*it != m_back && !it->full()) {
            sized_block_ref_t pair = ::std::make_pair(it->max_alloc_available(), &*it);
            auto ub = ::std::upper_bound(m_available_blocks.begin(), m_available_blocks.end(), pair, abrvr_compare);
            auto ab_it =
                ::std::find(m_available_blocks.begin(), m_available_blocks.end(), sized_block_ref_t(prev_available, &*it));
            if (ab_it == m_available_blocks.end()) {
              m_available_blocks.insert(ub, ::std::move(pair));
            } else {
              assert(ab_it <= ub);
              if (ub == m_available_blocks.end()) {
                m_available_blocks.erase(ab_it);
                m_available_blocks.emplace_back(::std::move(pair));
                _verify();
              } else {
                insert_replace(ab_it, ub - 1, ::std::move(pair));
                _verify();
              }
              _verify();
            }
            _verify();
          } else {
          }
          _verify();
          return true;
        }
      }
      _verify();
      return false;
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::add_block(
        allocator_block_t<Allocator, Allocator_Block_User_Data> &&block)
    {
      _verify();
      if (!m_blocks.empty()) {
        typename allocator_block_vector_t::value_type *bbegin = &m_blocks.front();
        m_blocks.emplace_back(::std::move(block));
        // if moved on emplacement.
        if (&m_blocks.front() != bbegin) {
          // this should not happen, if it does the world is inconsistent and everything can only end with memory corruption.
          abort();
        }
        if (m_back) {
          size_t avail = m_back->max_alloc_available();
          if (avail)
            m_available_blocks.emplace_back(::std::make_pair(avail, m_back));
          _verify();
        }
      } else
        m_blocks.emplace_back(::std::move(block));
      m_back = &m_blocks.back();
      _verify();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline void
    allocator_block_set_t<Allocator, Allocator_Block_User_Data>::remove_block(typename allocator_block_vector_t::iterator it)
    {
      m_blocks.erase(it);
      auto ait = ::std::find(m_available_blocks.begin(), m_available_blocks.end(), &*it);
      if (ait != m_available_blocks.end())
        m_available_blocks.erase(ait);
      _verify();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline bool allocator_block_set_t<Allocator, Allocator_Block_User_Data>::add_block_is_safe() const
    {
      return m_blocks.capacity() != m_blocks.size();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline size_t allocator_block_set_t<Allocator, Allocator_Block_User_Data>::grow_blocks(size_t sz)
    {
      typename allocator_block_vector_t::value_type *bbegin = &m_blocks.front();
      if (sz == 0 || sz < m_blocks.size())
        m_blocks.reserve(m_blocks.size() * 2);
      else
        m_blocks.reserve(sz);
      auto offset = reinterpret_cast<uint8_t *>(&m_blocks.front()) - reinterpret_cast<uint8_t *>(bbegin);
      if (m_back)
        reinterpret_cast<uint8_t *&>(m_back) += offset;
      for (auto &pair : m_available_blocks) {
        reinterpret_cast<uint8_t *&>(pair.second) += offset;
      }
      return offset;
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::_do_maintenance()
    {
      for (auto& block : m_blocks)
      {
        block.collect();
      }
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    auto allocator_block_set_t<Allocator, Allocator_Block_User_Data>::last_block() -> allocator_block_type &
    {
      return m_blocks.back();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    template <typename T>
    inline void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::free_empty_blocks(T &t)
    {
      for (auto &block : m_blocks) {
        block.collect();
        if (block.empty())
          t.emplace_back(std::move(block));
      }
      auto it = ::std::remove_if(m_blocks.begin(),
                                 m_blocks.end(),
                                 [](allocator_block_t<Allocator, Allocator_Block_User_Data> &block) { return block.empty(); });
      auto num_to_remove = m_blocks.end() - it;
      // this is needed because we can't resize because allocator_block is not trivially constructable.
      for (decltype(num_to_remove) i = 0; i < num_to_remove; ++i)
        m_blocks.pop_back();
      regenerate_available_blocks();
    }
  }
}
