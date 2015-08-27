#pragma once
#include "allocator_block_set.hpp"
#include <assert.h>
#include <iostream>
namespace cgc1
{
  namespace details
  {

    // size comparison for available blocks.
    static const auto abrvr_compare = [](auto &&r, auto &&it) { return r.first < it.first; };

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
      // clear available blocks.
      m_available_blocks.clear();
      for (auto &block : m_blocks) {
        // the back one doesn't go in available blocks,
        // we handle it explicitly.
        if (&block == &m_blocks.back())
          continue;
        if (!block.full()) {
          sized_block_ref_t pair = ::std::make_pair(block.max_alloc_available(), &block);
          m_available_blocks.emplace_back(::std::move(pair));
        }
      }
      // since available blocks is always sorted, sort it.
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
      // make sure that available blocks first is equal tomax available.
      for (const auto &ab : m_available_blocks) {
        assert(ab.first == ab.second->max_alloc_available());
      }
      // make sure available blocks is sorted.
      assert(::std::is_sorted(m_available_blocks.begin(), m_available_blocks.end(), abrvr_compare));
      // make sure there are no duplicates in available blocks (since sorted, not a problem).
      if (::std::adjacent_find(m_available_blocks.begin(), m_available_blocks.end()) != m_available_blocks.end())
        assert(0);
      // make sure back is not in adjacent blocks.
      auto ait = ::std::find_if(m_available_blocks.begin(), m_available_blocks.end(),
                                [&block](auto &&abp) { return abp.second == &m_blocks.back(); });
      assert(ait == m_available_blocks.end());

#endif
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline void *allocator_block_set_t<Allocator, Allocator_Block_User_Data>::allocate(size_t sz)
    {
      _verify();
      // lb is >= sz.
      auto lower_bound =
          ::std::lower_bound(m_available_blocks.begin(), m_available_blocks.end(), sized_block_ref_t(sz, nullptr), abrvr_compare);
      // no place to put it in available blocks, put it in last block.
      if (lower_bound == m_available_blocks.end()) {
        if (!m_blocks.empty()) {
          void *ret = m_blocks.back().allocate(sz);
          if (ret) {
            return ret;
          }
        }
        return nullptr;
      }
      // if here, there is a block in available blocks to use.
      // so try to allocate in there.
      auto ret = lower_bound->second->allocate(sz);
      if (!ret) {
        // this shouldn't happen
        // so memory corruption, abort.
        ::std::cerr << __FILE__ << " " << __LINE__ << "ABS failed to allocate, logic error/memory corruption." << ::std::endl;
        abort();
        return nullptr;
      }
      // ok, so we have allocated the memory.
      auto it = lower_bound;
      it->first = it->second->max_alloc_available();
      // see if there is allocation left in block.
      if (it->first == 0) {
        m_available_blocks.erase(it);
        _verify();
        return ret;
      }
      // find new insertion point.
      // want UB because we need > size.
      auto new_ub = ::std::upper_bound(m_available_blocks.begin(), it, *it, abrvr_compare);
      // if its in same place, we are done.
      if (new_ub == it)
        return ret;
      // otherwise rotate the list to move it to the new_ub position.
      ::std::rotate(new_ub, it, it + 1);
      _verify();
      return ret;
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline bool allocator_block_set_t<Allocator, Allocator_Block_User_Data>::destroy(void *v)
    {
      _verify();
      for (typename allocator_block_vector_t::iterator it = m_blocks.begin(); it != m_blocks.end(); ++it) {
        if (it->destroy(v)) {
          if (it != (m_blocks.end() - 1) && !it->full()) {
            // this is goofy
            sized_block_ref_t pair = ::std::make_pair(it->max_alloc_available(), &*it);
            auto ub = ::std::upper_bound(m_available_blocks.begin(), m_available_blocks.end(), pair, abrvr_compare);
            auto ab_it = ::std::find_if(m_available_blocks.begin(), m_available_blocks.end(),
                                        [&it](auto &ab) { return ab.second == &*it; });

            if (ab_it != m_available_blocks.end()) {
              assert(ab_it < ub);
              if (ub - 1 == ab_it) {
                // don't move at all, life made easy.
              } else {
                // ab_it and ub-1 swap places while maintaing ordering of stuff inbetween them.
                // rotate is optimal over erase/insert.
                ::std::rotate(ab_it, ab_it + 1, ub);
                *(ub - 1) = pair;
              }
            } else {
              m_available_blocks.emplace(ub, ::std::move(pair));
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
          ::std::cerr << __FILE__ << " " << __LINE__ << "ABS blocks moved on emplacement" << ::std::endl;
          abort();
        }
        size_t avail = back.max_alloc_available();
        if (avail) {
          auto pair = ::std::make_pair(avail, &back);
          // make sure not already in available blocks.
          // if this happens, the previous back was in the available blocks.
          // but back of m_blocks is guarented never to be in available blocks by constraint.
          // so this should not happen.
          assert(::std::find_if(m_available_blocks.begin(), m_available_blocks.end(),
                                [&pair](auto a) { return a.second == pair.second; }) == m_available_blocks.end());
          // remember, available blocks is sorted, so must emplace in appropriate spot.
          auto insertion_point = ::std::upper_bound(m_available_blocks.begin(), m_available_blocks.end(), pair, abrvr_compare);
          m_available_blocks.emplace(insertion_point, ::std::move(pair));
        }
        _verify();
      } else {
        m_blocks.emplace_back(::std::move(block));
      }
      _verify();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    inline auto allocator_block_set_t<Allocator, Allocator_Block_User_Data>::add_block() -> allocator_block_type &
    {
      add_block(allocator_block_type());
      return m_blocks.back();
    }
    template <typename Allocator, typename Allocator_Block_User_Data>
    template <typename Lock_Functional, typename Unlock_Functional, typename Move_Functional>
    inline void
    allocator_block_set_t<Allocator, Allocator_Block_User_Data>::remove_block(typename allocator_block_vector_t::iterator it,
                                                                              Lock_Functional &&lock_func,
                                                                              Unlock_Functional &&unlock_func,
                                                                              Move_Functional &&move_func)
    {

      // adjust available blocks.
      auto ait =
          ::std::find_if(m_available_blocks.begin(), m_available_blocks.end(), [&it](auto &&abp) { return abp.second == &*it; });
      if (ait != m_available_blocks.end())
        m_available_blocks.erase(ait);
      for (auto &&ab : m_available_blocks) {
        if (ab.second > &*it)
          ab.second--;
      }
      // use lock functional.
      lock_func();
      // move blocks (potentially)
      auto moved_begin = m_blocks.erase(it);
      // notification of moving blocks.
      move_func(moved_begin, m_blocks.end(), -static_cast<ptrdiff_t>(sizeof(typename allocator_block_vector_t::value_type)));
      // unlock functional
      unlock_func();

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
      // save old location of blocks.
      typename allocator_block_vector_t::value_type *bbegin = &m_blocks.front();
      if (sz == 0 || sz < m_blocks.size())
        m_blocks.reserve(m_blocks.size() * 2);
      else
        m_blocks.reserve(sz);
      // get offset from old location
      auto offset = reinterpret_cast<uint8_t *>(&m_blocks.front()) - reinterpret_cast<uint8_t *>(bbegin);
      // adjust location of available blocks by adding offset to each.
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
    template <typename L, typename Lock_Functional, typename Unlock_Functional, typename Move_Functional>
    inline void allocator_block_set_t<Allocator, Allocator_Block_User_Data>::free_empty_blocks(
        L &&l, Lock_Functional &&lock_func, Unlock_Functional &&unlock_func, Move_Functional &&move_func, size_t min_to_leave)
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
          remove_block(it.base() - 1, ::std::forward<Lock_Functional>(lock_func), ::std::forward<Unlock_Functional>(unlock_func),
                       ::std::forward<Move_Functional>(move_func));
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
