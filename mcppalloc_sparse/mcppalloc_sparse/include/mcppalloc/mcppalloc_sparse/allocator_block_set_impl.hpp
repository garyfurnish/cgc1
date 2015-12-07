#pragma once
#include "allocator_block_set.hpp"
#include <cassert>
#include <iostream>
#include <mcppalloc/mcppalloc_utils/boost/property_tree/ptree.hpp>
#include <mcppalloc/mcppalloc_utils/to_json.hpp>
namespace mcppalloc
{
  namespace sparse
  {
    namespace details
    {

      // size comparison for available blocks.
      // begin compare for block ordering.
      static const auto begin_compare = [](auto &&r, auto &&it) { return r.begin() < it.begin(); };
      static const auto begin_val_compare = [](auto &&r, auto &&val) { return r.begin() < val; };
      static const auto end_val_compare = [](auto &&r, auto &&val) { return r.end() < val; };
      static const auto begin_val_ub_compare = [](auto &&val, auto &&r) { return val < r.begin(); };

      template <typename Allocator_Policy>
      allocator_block_set_t<Allocator_Policy>::allocator_block_set_t(size_t allocator_min_size, size_t allocator_max_size)
          : m_allocator_min_size(allocator_min_size), m_allocator_max_size(allocator_max_size)
      {
      }
      template <typename Allocator_Policy>
      void allocator_block_set_t<Allocator_Policy>::_set_allocator_sizes(size_t min, size_t max)
      {
        m_allocator_min_size = min;
        m_allocator_max_size = max;
      }
      template <typename Allocator_Policy>
      size_t allocator_block_set_t<Allocator_Policy>::allocator_min_size() const
      {
        return m_allocator_min_size;
      }
      template <typename Allocator_Policy>
      size_t allocator_block_set_t<Allocator_Policy>::allocator_max_size() const
      {
        return m_allocator_max_size;
      }
      template <typename Allocator_Policy>
      size_t allocator_block_set_t<Allocator_Policy>::size() const
      {
        return m_blocks.size();
      }
      template <typename Allocator_Policy>
      void allocator_block_set_t<Allocator_Policy>::regenerate_available_blocks()
      {
        allocator_block_vector_t blocks;
        // clear available blocks.
        m_available_blocks.clear();
        for (auto &block : m_blocks) {
          // the back one doesn't go in available blocks,
          // we handle it explicitly.
          if (&block == &last_block()) {
            continue;
          }
          if (!block.full()) {
            sized_block_ref_t pair = ::std::make_pair(block.max_alloc_available(), &block);
            blocks.emplace_back(::std::move(pair));
          }
        }
        m_available_blocks.insert(blocks.begin(), blocks.end());
        _verify();
      }
      template <typename Allocator_Policy>
      void allocator_block_set_t<Allocator_Policy>::collect()
      {
        // collect all blocks.
        for (auto &block : m_blocks) {
          block.collect(m_num_destroyed_since_free);
        }
        // some blocks may have become available so regenerate available blocks.
        regenerate_available_blocks();
        _verify();
      }
      template <typename Allocator_Policy>
      void allocator_block_set_t<Allocator_Policy>::_verify() const
      {
        if (cgc1_unlikely(m_magic_prefix != cs_magic_prefix)) {
          ::std::cerr << __FILE__ << " " << __LINE__ << " ABS MEMORY CORRUPTION\n";
          abort();
          return;
        }
        for (auto &&ab : m_available_blocks) {
          auto &block = *ab.second;
          if (cgc1_unlikely(block.max_alloc_available() != ab.first)) {
            ::std::cerr << __FILE__ << " " << __LINE__ << " ABS CONSISTENCY ERROR\n";
            ::std::cerr << "min/max allocation sizes: (" << allocator_min_size() << ", " << allocator_max_size() << ")\n";
            ::std::cerr << "available:  " << ab.first << ::std::endl;
            ::std::cerr << &block << " " << block.valid() << " " << block.last_max_alloc_available() << ::std::endl;
            ::std::cerr << block.secondary_memory_used() << " " << block.memory_size() << " " << block.full() << ::std::endl;
            ::std::cerr << "recomp max alloc " << block.max_alloc_available() << ::std::endl;
            ::std::cerr << "free list size " << block.m_free_list.size() << ::std::endl;

            ::std::cerr << "available blocks\n";
            for (auto pair : m_available_blocks) {
              ::std::cerr << pair.first << " " << pair.second << "\n";
            }

            abort();
            return;
          }

          if (cgc1_unlikely(&block == &last_block())) {
            ::std::cerr << __FILE__ << " " << __LINE__ << " ABS CONSISTENCY ERROR\n";
            ::std::cerr << "Last block in available blocks\n";
            abort();
            return;
          }
          if (cgc1_unlikely(block.full())) {
            ::std::cerr << __FILE__ << " " << __LINE__ << " ABS CONSISTENCY ERROR\n";
            ::std::cerr << "Block full\n";
            abort();
            return;
          }
        }
        if (cgc1_unlikely(!::std::is_sorted(m_available_blocks.begin(), m_available_blocks.end(), abrvr_compare))) {
          ::std::cerr << __FILE__ << " " << __LINE__ << " ABS CONSISTENCY ERROR\n";
          ::std::cerr << "available blocks not sorted\n";
          ::std::cerr << "bad: \n";
          for (auto it = m_available_blocks.begin(); it != m_available_blocks.end() - 1; ++it) {
            if (!abrvr_compare(*it, *(it + 1)))
              ::std::cerr << static_cast<size_t>(it - m_available_blocks.begin()) << " " << it->first << " " << it->second << " "
                          << (it + 1)->first << " " << (it + 1)->second;
          }
          ::std::cerr << "all: \n";
          for (auto &&ab : m_available_blocks)
            ::std::cerr << ab.first << " " << &ab.second << " ";
          abort();
          return;
        }
        // make sure there are no duplicates in available blocks (since sorted, not a problem).
        if (cgc1_unlikely(::std::adjacent_find(m_available_blocks.begin(), m_available_blocks.end()) !=
                          m_available_blocks.end())) {
          ::std::cerr << __FILE__ << " " << __LINE__ << " ABS CONSISTENCY ERROR\n";
          ::std::cerr << "available blocks contains duplicates\n";
        }
      }
      template <typename Allocator_Policy>
      auto allocator_block_set_t<Allocator_Policy>::allocate(size_t sz) -> block_type
      {
        block_type ret{nullptr, 0};
        _verify();
        if (m_available_blocks.empty()) {
          if (!m_blocks.empty()) {
            assert(&last_block());
            ret = last_block().allocate(sz);
            _verify();
            return ret;
          }
        }
        // lb is >= sz.
        // THIS IS WRONG COMPARE!!!!!

        auto lower_bound = ::std::lower_bound(m_available_blocks.begin(), m_available_blocks.end(),
                                              sized_block_ref_t(sz, nullptr), abrvr_size_compare);
        // no place to put it in available blocks, put it in last block.
        if (lower_bound == m_available_blocks.end()) {
          if (!m_blocks.empty()) {
            assert(&last_block());
            ret = last_block().allocate(sz);
            _verify();
            return ret;
          }
          _verify();
          return ret;
        }
        // if here, there is a block in available blocks to use.
        // so try to allocate in there.
        ret = lower_bound->second->allocate(sz);
        if (cgc1_unlikely(!ret.m_ptr)) {
          // this shouldn't happen
          // so memory corruption, abort.
          ::std::cerr << __FILE__ << " " << __LINE__ << " ABS failed to allocate, logic error/memory corruption."
                      << "\n";
          ::std::cerr << "was trying to allocate bytes: " << sz << "\n";
          ::std::cerr << "min/max allocation sizes: (" << allocator_min_size() << ", " << allocator_max_size() << ")\n";
          //        ::std::cerr << to_json(*lower_bound->second, 2) << "\n";
          auto &block = *lower_bound->second;
          ::std::cerr << "available:  " << lower_bound->first << "\n";
          ::std::cerr << &block << " " << block.valid() << " " << block.last_max_alloc_available() << "\n";
          ::std::cerr << block.secondary_memory_used() << " " << block.memory_size() << " " << block.full() << "\n";
          ::std::cerr << "recomp max alloc " << block.max_alloc_available() << "\n";
          ::std::cerr << "free list size " << block.m_free_list.size() << "\n";
          abort();
          return ret;
        }
        // ok, so we have allocated the memory.
        auto it = lower_bound;
        auto new_max_alloc = it->second->max_alloc_available();
        new_max_alloc = it->second->max_alloc_available();
        // see if there is allocation left in block.
        if (new_max_alloc == 0) {
          m_available_blocks.erase(it);
          _verify();

          return ret;
        }
        // TODO: debug
        if (cgc1_unlikely(new_max_alloc > it->second->memory_size())) {
          ::std::cerr << __FILE__ << " " << __LINE__ << "Consistency error\n";
          abort();
        }
        if (cgc1_unlikely(new_max_alloc < allocator_min_size())) {
          ::std::cerr << __FILE__ << " " << __LINE__ << "Consistency error\n";
          abort();
        }
        // find new insertion point.
        // want UB because we need > size.
        if (it == m_available_blocks.end())
          abort();
        sized_block_ref_t new_pair(new_max_alloc, it->second);
        // this must be +1 because it could stay in the same place.
        //        auto new_ub = ::std::upper_bound(m_available_blocks.begin(), it + 1, new_pair, abrvr_compare);
        it->first = new_max_alloc;
        if (cgc1_unlikely(it->first != it->second->last_max_alloc_available())) {
          ::std::cerr << __FILE__ << " " << __LINE__ << "Consistency error\n";
          abort();
        }
        auto block = it->second;
        // this case needs optimizing.
        it = m_available_blocks.erase(it);
        m_available_blocks.emplace(new_max_alloc, block);
        _verify();
        return ret;
      }
      template <typename Allocator_Policy>
      bool allocator_block_set_t<Allocator_Policy>::destroy(void *v)
      {
        _verify();
        auto it = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), v, end_val_compare);
        size_t last_collapsed_size = 0;
        size_t prev_last_max_alloc_available = 0;
        if (it == m_blocks.end())
          return false;
        /*	auto debug_last_max_alloc_available = it->last_max_alloc_available();
        if(debug_last_max_alloc_available!=it->max_alloc_available())
          {
            ::std::cerr << __FILE__ << " " << __LINE__ << "\n";
            abort();
            }*/
        if (it->destroy(v, last_collapsed_size, prev_last_max_alloc_available)) {
          if (&*it != &last_block() && !it->full()) {
            // find the block.
            sized_block_ref_t pair2 = ::std::make_pair(prev_last_max_alloc_available, &*it);
            auto ab_it2 = m_available_blocks.lower_bound(pair2);
            if (ab_it2 != m_available_blocks.end()) {
              if (ab_it2->second != &*it) {
                goto NOT_FOUND;
              }
              sized_block_ref_t pair = ::std::make_pair(it->max_alloc_available(), &*it);
              auto ub = m_available_blocks.upper_bound(pair);
              // DEBUG
              if (cgc1_unlikely(ub != m_available_blocks.end() && ub->first < pair.first)) {
                ::std::cerr << abrvr_compare(pair, *ub) << ::std::endl;
                ::std::cerr << ub->first << " " << ub->second << " " << pair.first << " " << pair.second << "\n";
                ::std::cerr << __FILE__ << " " << __LINE__ << "\n";
                abort();
              }
              if (ub - 1 == ab_it2) {
                // DEBUG
                if (cgc1_unlikely((ub - 1)->second != &*it)) {
                  ::std::cerr << __FILE__ << " " << __LINE__ << "\n";
                  abort();
                }
                *(ub - 1) = pair;
                // don't move at all, life made easy.
              }
              else if (ab_it2 < ub) {
                // ab_it2 and ub-1 swap places while maintaing ordering of stuff inbetween them.
                // rotate is optimal over erase/insert.

                ::std::rotate(ab_it2, ab_it2 + 1, ub);
                // DEBUG
                (ub - 1)->first = pair.first;
                _verify();
              }
              else {
                // ub < ab_it2
                ::std::rotate(ub, ab_it2, ab_it2 + 1);
                (ub)->first = pair.first;
                ::std::cout << "Possible consistency error " << __FILE__ << " " << __LINE__ << "\n";
                _verify();
              }
              //		  *(ub - 1) = pair;
              _verify();
            }
            else {
            NOT_FOUND:
              _verify();
              sized_block_ref_t pair = ::std::make_pair(it->max_alloc_available(), &*it);
              m_available_blocks.insert(pair);
              _verify();
            }
          }
          // increment destroyed count.
          m_num_destroyed_since_free += 1;
          _verify();
          return true;
        }
        _verify();
        return false;
      }
      template <typename Allocator_Policy>
      template <typename Lock_Functional, typename Unlock_Functional, typename Move_Functional>
      auto allocator_block_set_t<Allocator_Policy>::add_block(allocator_block_t<Allocator_Policy> &&block,
                                                              Lock_Functional &&lock_func,
                                                              Unlock_Functional &&unlock_func,
                                                              Move_Functional &&move_func) -> allocator_block_type &
      {

        _verify();
        if (!m_blocks.empty()) {
          assert(m_last_block < &*m_blocks.end());
          typename allocator_block_vector_t::value_type *bbegin = &m_blocks.front();
          auto blocks_insertion_point = ::std::upper_bound(m_blocks.begin(), m_blocks.end(), block, begin_compare);
          lock_func();
          const auto moved_begin = m_blocks.emplace(blocks_insertion_point, ::std::move(block));
          move_func(moved_begin + 1, m_blocks.end(),
                    static_cast<ptrdiff_t>(sizeof(typename allocator_block_vector_t::value_type)));
          unlock_func();
          // if moved on emplacement.
          if (cgc1_unlikely(&m_blocks.front() != bbegin)) {
            // this should not happen, if it does the world is inconsistent and everything can only end with memory corruption.
            ::std::cerr << __FILE__ << " " << __LINE__ << "ABS blocks moved on emplacement"
                        << "\n";
            abort();
          }
          for (auto &&pair : m_available_blocks) {
            assert(!pair.second->full());
            //          assert(pair.second->last_max_alloc_available() == pair.first);
            //          assert(pair.second->last_max_alloc_available() == pair.second->max_alloc_available());
            if (pair.second >= &*moved_begin) {
              pair.second += 1;
            }
            //	  pair.first = pair.second->max_alloc_available();
            assert(pair.second->last_max_alloc_available() == pair.first);
          }
          if (m_last_block >= &*moved_begin) {
            m_last_block++;
          }
          assert(m_last_block < &*m_blocks.end());
          assert(m_last_block);
          auto pair = ::std::make_pair(m_last_block->max_alloc_available(), m_last_block);
          if (pair.first) {
            m_available_blocks.insert(pair);
          }
          assert(::std::is_sorted(m_available_blocks.begin(), m_available_blocks.end(), abrvr_compare));
          m_last_block = &*moved_begin;
          _verify();
        }
        else {
          m_blocks.emplace_back(::std::move(block));
          m_last_block = &m_blocks.back();
        }
        _verify();
        assert(!m_blocks.empty());
        return *m_last_block;
      }
      template <typename Allocator_Policy>
      auto allocator_block_set_t<Allocator_Policy>::add_block(allocator_block_type &&block) -> allocator_block_type &
      {
        return add_block(::std::move(block), []() {}, []() {}, [](auto &&, auto &&, auto &&) {});
      }
      template <typename Allocator_Policy>
      template <typename Lock_Functional, typename Unlock_Functional, typename Move_Functional>
      void allocator_block_set_t<Allocator_Policy>::remove_block(typename allocator_block_vector_t::iterator it,
                                                                 Lock_Functional &&lock_func,
                                                                 Unlock_Functional &&unlock_func,
                                                                 Move_Functional &&move_func)
      {
        _verify();
        // adjust available blocks.
        auto ait = ::std::find_if(m_available_blocks.begin(), m_available_blocks.end(),
                                  [&it](auto &&abp) { return abp.second == &*it; });
        if (ait != m_available_blocks.end())
          m_available_blocks.erase(ait);
        // is this  a bug? THIS WAS >, now >=
        for (auto &&ab : m_available_blocks) {
          if (ab.second >= &*it)
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
        if (&last_block() == &*it) {
          if (!m_available_blocks.empty()) {
            auto back_it = m_available_blocks.end() - 1;
            m_last_block = back_it->second;
            m_available_blocks.erase(back_it);
          }
          else
            m_last_block = nullptr;
        }
        else if (&last_block() > &*it) {
          m_last_block--;
        }
        _verify();
      }
      template <typename Allocator_Policy>
      bool allocator_block_set_t<Allocator_Policy>::add_block_is_safe() const
      {
        // If capacity is equal to size then adding a block will trigger a reallocation of the internal vector.
        return m_blocks.capacity() != m_blocks.size();
      }
      template <typename Allocator_Policy>
      size_t allocator_block_set_t<Allocator_Policy>::grow_blocks(size_t sz)
      {
        _verify();
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
          pair.second = reinterpret_cast<allocator_block_type *>(reinterpret_cast<uint8_t *>(pair.second) + offset);
        }
        m_last_block = reinterpret_cast<allocator_block_type *>(reinterpret_cast<uint8_t *>(m_last_block) + offset);
        _verify();
        return static_cast<size_t>(offset);
      }
      template <typename Allocator_Policy>
      void allocator_block_set_t<Allocator_Policy>::_do_maintenance()
      {
        collect();
      }
      template <typename Allocator_Policy>
      auto allocator_block_set_t<Allocator_Policy>::primary_memory_used() const noexcept -> size_t
      {
        size_t sz = 0;
        for (auto &&block : m_blocks)
          sz += block.memory_size();
        return sz;
      }
      template <typename Allocator_Policy>
      auto allocator_block_set_t<Allocator_Policy>::secondary_memory_used() const noexcept -> size_t
      {
        size_t sz = 0;
        for (auto &&block : m_blocks)
          sz += block.secondary_memory_used();
        sz += secondary_memory_used_self();
        return sz;
      }
      template <typename Allocator_Policy>
      auto allocator_block_set_t<Allocator_Policy>::secondary_memory_used_self() const noexcept -> size_t
      {
        size_t sz = 0;
        sz += m_available_blocks.capacity() * sizeof(typename allocator_block_reference_vector_t::value_type);
        sz += m_blocks.capacity() * sizeof(typename allocator_block_vector_t::value_type);
        return sz;
      }
      template <typename Allocator_Policy>
      void allocator_block_set_t<Allocator_Policy>::shrink_secondary_memory_usage_to_fit()
      {
        shrink_secondary_memory_usage_to_fit_self();
        for (auto &&block : m_blocks)
          block.shrink_secondary_memory_usage_to_fit();
      }
      template <typename Allocator_Policy>
      void allocator_block_set_t<Allocator_Policy>::shrink_secondary_memory_usage_to_fit_self()
      {
        m_available_blocks.shrink_to_fit();
      }

      template <typename Allocator_Policy>
      CGC1_ALWAYS_INLINE auto allocator_block_set_t<Allocator_Policy>::last_block() noexcept -> allocator_block_type &
      {
        return *m_last_block;
      }
      template <typename Allocator_Policy>
      CGC1_ALWAYS_INLINE auto allocator_block_set_t<Allocator_Policy>::last_block() const noexcept -> const allocator_block_type &
      {
        return *m_last_block;
      }
      template <typename Allocator_Policy>
      template <typename L, typename Lock_Functional, typename Unlock_Functional, typename Move_Functional>
      void allocator_block_set_t<Allocator_Policy>::free_empty_blocks(
          L &&l, Lock_Functional &&lock_func, Unlock_Functional &&unlock_func, Move_Functional &&move_func, size_t min_to_leave)
      {
        _verify();
        // this looks really complicated but it is actually quite light weight since blocks are tiny and everything is contiguous.
        size_t num_empty = 0;
        // first we collect and see how many total empty blocks there are.
        for (auto &block : m_blocks) {
          block.collect(m_num_destroyed_since_free);
          if (block.empty()) {
            num_empty++;
          }
          else {
          }
        }
        // update available blocks after update.
        for (auto &&ab : m_available_blocks) {
          ab.first = ab.second->last_max_alloc_available();
        }
        ::std::sort(m_available_blocks.begin(), m_available_blocks.end(), abrvr_compare);
        _verify();
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
            remove_block((it + 1).base(), ::std::forward<Lock_Functional>(lock_func),
                         ::std::forward<Unlock_Functional>(unlock_func), ::std::forward<Move_Functional>(move_func));
            // adjust pointer (erase does not invalidate).
          }
        }
        // reset destroyed counter.
        m_num_destroyed_since_free = 0;
        _verify();
      }
      template <typename Allocator_Policy>
      auto allocator_block_set_t<Allocator_Policy>::num_destroyed_since_last_free() const noexcept -> size_t
      {
        return m_num_destroyed_since_free;
      }
      template <typename Allocator_Policy>
      void allocator_block_set_t<Allocator_Policy>::to_ptree(::boost::property_tree::ptree &ptree, int level) const
      {
        (void)level;
        ptree.put("primary_memory_used", ::std::to_string(primary_memory_used()));
        ptree.put("secondary_memory_used_self", ::std::to_string(secondary_memory_used_self()));
        ptree.put("secondary_memory_used", ::std::to_string(secondary_memory_used()));
        ptree.put("allocator_min_size", ::std::to_string(allocator_min_size()));
        ptree.put("allocator_max_size", ::std::to_string(allocator_max_size()));
        ptree.put("num_destroyed_since_free", ::std::to_string(num_destroyed_since_last_free()));
        ptree.put("num_blocks", ::std::to_string(m_blocks.size()));
        ptree.put("num_available_blocks", ::std::to_string(m_available_blocks.size()));
        ptree.put("last_block_null", ::std::to_string(m_last_block == nullptr));
      }
    }
  }
}
