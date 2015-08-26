#pragma once
#include "allocator_block_set.hpp"
#include <assert.h>
#include <iostream>
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
      auto abrvr_compare = [](const sized_block_ref_t &r, typename allocator_block_reference_vector_t::const_reference it) {
        return r.first < it.first;
      };
      m_available_blocks.clear();
      for (auto &block : m_blocks) {
        if (&block == &m_blocks.back())
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
      // collect all blocks.
      for (auto &block : m_blocks) {
        block.collect();
      }
      // some blocks may have become available so regenerate available blocks.
      regenerate_available_blocks();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::_verify() const
    {
#if _CGC1_DEBUG_LEVEL > 1
      auto abrvr_compare = [](const sized_block_ref_t &r, typename allocator_block_reference_vector_t::const_reference it) {
        return r.first < it.first;
      };

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
      auto abrvr_compare = [](const sized_block_ref_t &r, typename allocator_block_reference_vector_t::const_reference it) {
        return r.first < it.first;
      };
      _verify();
      auto lower_bound =
          ::std::lower_bound(m_available_blocks.begin(), m_available_blocks.end(), sized_block_ref_t(sz, nullptr), abrvr_compare);
      if (lower_bound == m_available_blocks.end()) {
        if (!m_blocks.empty()) {
          void *ret = m_blocks.back().allocate(sz);
          if (ret) {
            return ret;
          }
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
          if (it != (m_blocks.end() - 1) && !it->full()) {
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
          // increment destroyed count.
          m_num_destroyed_since_free += 1;
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
        auto &back = m_blocks.back();
        m_blocks.emplace_back(::std::move(block));
        // if moved on emplacement.
        if (&m_blocks.front() != bbegin) {
          // this should not happen, if it does the world is inconsistent and everything can only end with memory corruption.
          abort();
        }
        size_t avail = back.max_alloc_available();
        if (avail)
          m_available_blocks.emplace_back(::std::make_pair(avail, &back));
        _verify();
      } else
        m_blocks.emplace_back(::std::move(block));
      _verify();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline auto allocator_block_set_t<Allocator, Allocator_Block_User_Data>::add_block() -> allocator_block_type &
    {
      add_block(allocator_block_type());
      return m_blocks.back();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline void
    allocator_block_set_t<Allocator, Allocator_Block_User_Data>::remove_block(typename allocator_block_vector_t::iterator it)
    {
      m_blocks.erase(it);
      auto ait =
          ::std::find_if(m_available_blocks.begin(), m_available_blocks.end(), [&it](auto &&abp) { return abp.second == &*it; });
      if (ait != m_available_blocks.end())
        m_available_blocks.erase(ait);
      _verify();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline bool allocator_block_set_t<Allocator, Allocator_Block_User_Data>::add_block_is_safe() const
    {
      // If capacity is equal to size then adding a block will trigger a reallocation of the internal vector.
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
      for (auto &pair : m_available_blocks) {
        unsafe_reference_cast<uint8_t *>(pair.second) += offset;
      }
      return static_cast<size_t>(offset);
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::_do_maintenance()
    {
      // collect all blocks.
      for (auto &block : m_blocks) {
        block.collect();
      }
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    auto allocator_block_set_t<Allocator, Allocator_Block_User_Data>::last_block() -> allocator_block_type &
    {
      return m_blocks.back();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    auto allocator_block_set_t<Allocator, Allocator_Block_User_Data>::last_block() const -> const allocator_block_type &
    {
      return m_blocks.back();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    template <typename Lambda>
    inline void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::free_empty_blocks(Lambda &&l, size_t min_to_leave)
    {
      // this looks really complicated but it is actually quite light weight since blocks are tiny and everything is contiguous.
      size_t num_empty = 0;
      // first we collect and see how many total empty blocks there are.
      for (auto &block : m_blocks) {
        block.collect();
        if (block.empty()) {
          num_empty++;
        } else {
        }
      }
      for (auto it = m_blocks.rbegin(); it != m_blocks.rend(); ++it) {
        auto &block = *it;
        // now go through empty blocks
        if (block.empty()) {
          // if num empty is now at minimum, we are done moving blocks out.
          if (num_empty <= min_to_leave) {
            break;
          }
          // one less empty block
          num_empty--;
          // remove them until we hit our min to leave.
          l(::std::move(block));
          // we have moved the block out, now remove it.
          remove_block(it.base() - 1);
          // adjust pointer (erase does not invalidate).
          it--;
        }
      }
      // Regenerate available blocks since we have altered m_blocks.
      regenerate_available_blocks();
      // reset destroyed counter.
      m_num_destroyed_since_free = 0;
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    auto allocator_block_set_t<Allocator, Allocator_Block_User_Data>::num_destroyed_since_last_free() const noexcept -> size_t
    {
      return m_num_destroyed_since_free;
    }
  }
}
