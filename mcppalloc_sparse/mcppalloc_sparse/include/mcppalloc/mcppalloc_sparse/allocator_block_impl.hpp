#pragma once
#include "allocator_block.hpp"
#include <assert.h>
#include <mcppalloc/mcppalloc_utils/boost/property_tree/ptree.hpp>
#include <mcppalloc/block.hpp>
#include <mcppalloc/user_data_base.hpp>
#include <mcppalloc/mcppalloc_utils/function_iterator.hpp>
#include <iostream>
namespace mcppalloc
{
  namespace sparse
  {
    namespace details
    {

      template <typename Allocator_Policy>
      CGC1_ALWAYS_INLINE allocator_block_t<Allocator_Policy>::allocator_block_t(void *start,
                                                                                size_t length,
                                                                                size_t minimum_alloc_length,
                                                                                size_t maximum_alloc_length) noexcept
          : m_next_alloc_ptr(reinterpret_cast<object_state_type *>(start)),
            m_end(reinterpret_cast<uint8_t *>(start) + length),
            m_minimum_alloc_length(object_state_type::needed_size(sizeof(object_state_type), minimum_alloc_length)),
            m_start(reinterpret_cast<uint8_t *>(start))
      {
#if CGC1_DEBUG_LEVEL > 0
        // sanity check alignment of start.
        if (unlikely(reinterpret_cast<size_t>(m_start) % minimum_header_alignment() != 0))
          abort();
        if (unlikely(reinterpret_cast<size_t>(m_end) % minimum_header_alignment() != 0))
          abort();
#endif
        if (maximum_alloc_length == c_infinite_length) {
          m_maximum_alloc_length = maximum_alloc_length;
        }
        else {
          m_maximum_alloc_length = object_state_type::needed_size(sizeof(object_state_type), maximum_alloc_length);
        }
        assert(m_minimum_alloc_length <= m_maximum_alloc_length);
        // setup first object state
        m_next_alloc_ptr->set_all(reinterpret_cast<object_state_type *>(reinterpret_cast<uint8_t *>(start) + length), false,
                                  false, false);

        // setup default user data.
        m_default_user_data = allocator_unique_ptr_t<user_data_type, allocator>(&s_default_user_data);
        m_default_user_data->set_is_default(true);
      }
      template <typename Allocator_Policy>
      CGC1_ALWAYS_INLINE allocator_block_t<Allocator_Policy>::allocator_block_t(allocator_block_t &&block) noexcept
          : m_free_list(::std::move(block.m_free_list)),
            m_next_alloc_ptr(::std::move(block.m_next_alloc_ptr)),
            m_end(::std::move(block.m_end)),
            m_minimum_alloc_length(::std::move(block.m_minimum_alloc_length)),
            m_start(::std::move(block.m_start)),
            m_default_user_data(::std::move(block.m_default_user_data)),
            m_last_max_alloc_available(::std::move(block.m_last_max_alloc_available)),
            m_maximum_alloc_length(::std::move(block.m_maximum_alloc_length))
      {
      }
      template <typename Allocator_Policy>
      CGC1_ALWAYS_INLINE allocator_block_t<Allocator_Policy> &allocator_block_t<Allocator_Policy>::
      operator=(allocator_block_t<Allocator_Policy> &&block) noexcept
      {
        if (m_default_user_data.get() == &s_default_user_data)
          m_default_user_data.release();
        m_default_user_data = ::std::move(block.m_default_user_data);
        m_free_list = std::move(block.m_free_list);
        m_next_alloc_ptr = block.m_next_alloc_ptr;
        m_end = block.m_end;
        m_minimum_alloc_length = block.m_minimum_alloc_length;
        m_start = block.m_start;
        m_last_max_alloc_available = block.m_last_max_alloc_available;
        m_maximum_alloc_length = block.m_maximum_alloc_length;
        // invalidate moved from block.
        // block.clear();
        return *this;
      }
      template <typename Allocator_Policy>
      allocator_block_t<Allocator_Policy>::~allocator_block_t()
      {
        if (m_default_user_data.get() == &s_default_user_data)
          m_default_user_data.release();
      }

      template <typename Allocator_Policy>
      void allocator_block_t<Allocator_Policy>::clear()
      {
        m_free_list.clear();
        m_next_alloc_ptr = nullptr;
        m_end = nullptr;
        m_start = nullptr;
      }
      template <typename Allocator_Policy>
      inline bool allocator_block_t<Allocator_Policy>::valid() const noexcept
      {
        return m_start != nullptr;
      }
      template <typename Allocator_Policy>
      bool allocator_block_t<Allocator_Policy>::empty() const noexcept
      {
        if (!m_next_alloc_ptr)
          return false;
        return reinterpret_cast<uint8_t *>(m_next_alloc_ptr) == begin() && !m_next_alloc_ptr->next_valid();
      }
      template <typename Allocator_Policy>
      inline bool allocator_block_t<Allocator_Policy>::full() const noexcept
      {
        return m_next_alloc_ptr == nullptr && m_free_list.empty();
      }
      template <typename Allocator_Policy>
      CGC1_ALWAYS_INLINE uint8_t *allocator_block_t<Allocator_Policy>::begin() const noexcept
      {
        return m_start;
      }
      template <typename Allocator_Policy>
      CGC1_ALWAYS_INLINE uint8_t *allocator_block_t<Allocator_Policy>::end() const noexcept
      {
        return m_end;
      }
      template <typename Allocator_Policy>
      auto allocator_block_t<Allocator_Policy>::memory_size() const noexcept -> size_type
      {
        return static_cast<size_type>(end() - begin());
      }
      template <typename Allocator_Policy>
      auto allocator_block_t<Allocator_Policy>::current_end() const noexcept -> object_state_type *
      {
        if (!m_next_alloc_ptr)
          return reinterpret_cast<object_state_type *>(end());
        else
          return m_next_alloc_ptr;
      }
      template <typename Allocator_Policy>
      CGC1_ALWAYS_INLINE auto allocator_block_t<Allocator_Policy>::_object_state_begin() const noexcept -> object_state_type *
      {
        return reinterpret_cast<object_state_type *>(begin());
      }
      template <typename Allocator_Policy>
      auto allocator_block_t<Allocator_Policy>::find_address(void *addr) const noexcept -> object_state_type *
      {
        for (auto it = make_next_iterator(_object_state_begin()); it != make_next_iterator(current_end()); ++it) {
          if (it->object_end() > addr) {
            return it;
          }
        }
        return nullptr;
      }
#if CGC1_DEBUG_LEVEL > 1
      template <typename Allocator_Policy>
      void allocator_block_t<Allocator_Policy>::_verify(const object_state_type *state)
      {
        if (state && state->next_valid()) {
          assert(state->object_size() < static_cast<size_t>(end() - begin()));
          assert(state->next());
        }
        auto begin = make_next_iterator(reinterpret_cast<object_state_type *>(this->begin()));
        auto end = make_next_iterator(current_end());
        assert(begin.m_t <= end.m_t);
        for (auto os_it = begin; os_it != end; ++os_it) {
          assert(os_it->next_valid() || os_it->next() == end);
          assert(os_it->next() != nullptr);
        }
      }
#else
      template <typename Allocator_Policy>
      void allocator_block_t<Allocator_Policy>::_verify(const object_state_type *)
      {
      }
#endif
      template <typename Allocator_Policy>
      auto allocator_block_t<Allocator_Policy>::allocate(size_t size, bool debug) -> block_type
      {
        assert(minimum_allocation_length() <= maximum_allocation_length());
        _verify(nullptr);
        // these help, especially when prefetch crosses cache or page boundry.
        cgc1_builtin_prefetch(this);
        cgc1_builtin_prefetch(reinterpret_cast<uint8_t *>(this) + 16);
        cgc1_builtin_prefetch(reinterpret_cast<uint8_t *>(this) + 32);
        const size_t original_size = size;
        size = object_state_type::needed_size(sizeof(object_state_type), size);
        assert(size >= minimum_allocation_length());
        assert(size <= maximum_allocation_length());
        assert(reinterpret_cast<uint8_t *>(m_next_alloc_ptr) == reinterpret_cast<uint8_t *>(&m_next_alloc_ptr->m_next));
        assert(reinterpret_cast<uint8_t *>(m_next_alloc_ptr) + 8 == reinterpret_cast<uint8_t *>(&m_next_alloc_ptr->m_user_data));
        // compute this now.
        object_state_type *later_next =
            reinterpret_cast<object_state_type *>(reinterpret_cast<uint8_t *>(m_next_alloc_ptr) + size);
        // if the free list isn't trivial, check it first.
        if (!m_free_list.empty()) {
          // do a reverse search from back of free list for somewhere to put the data.
          // we don't have time to do an exhaustive search.
          // also deleting an item from the free list could involve lots of copying if at beginning of list.
          // finally free list is a vector so this should be cache friendly.
          for (auto it = m_free_list.rbegin(); it != m_free_list.rend(); ++it) {
            object_state_type *state = *it;
            // if it doesn't fit, move on to next one.
            if (state->object_size() < original_size)
              continue;
            auto forward_it = it.base() - 1;
            if (cgc1_unlikely(forward_it >= m_free_list.end())) {
              ::std::cerr << "Consistency error 8e4970f8-9691-42c0-b018-2a6920e40f39\n";
              abort();
            }
            if (cgc1_unlikely(forward_it < m_free_list.begin())) {
              ::std::cerr << "Consistency error 9521a31e-9d29-48c1-b971-4f2f8e75ea44\n";
              abort();
            }
            // DEBUG
            if (cgc1_unlikely(state != *forward_it)) {
              ::std::cerr << "Consistency error 659de4b2-0370-4a45-acc0-4423169e33f6\n";
              abort();
            }
            if (cgc1_unlikely((forward_it + 1) != m_free_list.end() &&
                              (*forward_it)->object_size() > (*(forward_it + 1))->object_size())) {
              ::std::cerr << "Consistency error 14d852ad-a67f-4b80-9eab-d0e666a7b38f\n";
              abort();
            }
            if (cgc1_unlikely(debug)) {
              if (!::std::is_sorted(m_free_list.begin(), m_free_list.end(), ::mcppalloc::details::os_size_compare{})) {
                ::std::cerr << __FILE__ << " " << __LINE__ << " 2a23cbf4-37a0-49a1-983b-bb7797e99f67\n";
                ::std::cerr << "WARNING FREE LIST NOT SORTED\n";
                abort();
              }
            }
            const auto prev_max_size = (*m_free_list.rbegin())->object_size();
            // erase found from free list.
            m_free_list.erase(forward_it);
            if (!m_free_list.empty()) {
              const auto new_max_size = (*m_free_list.rbegin())->object_size();
              if (cgc1_unlikely(prev_max_size < new_max_size)) {
                ::std::cerr << "Consistency error 20d77f37-80da-43d5-9eab-1e701d2218ff\n";
                ::std::cerr << prev_max_size << " " << new_max_size << "\n";
                abort();
              }
            }

            // figure out theoretical next pointer.
            object_state_type *next = reinterpret_cast<object_state_type *>(reinterpret_cast<uint8_t *>(state) + size);
            // see if we can split the memory.
            if (reinterpret_cast<uint8_t *>(next) + m_minimum_alloc_length <= reinterpret_cast<uint8_t *>(state->next())) {
              // if we are here, the memory left over is bigger then minimum alloc size, so split.
              next->set_all(state->next(), false, state->next_valid());
              if (cgc1_unlikely(debug)) {
                if (!m_free_list.empty()) {
                  const auto new_max_size = (*m_free_list.rbegin())->object_size();
                  if (cgc1_unlikely(prev_max_size < new_max_size)) {
                    ::std::cerr << "Consistency error 762bac31-5126-4ebc-a993-148747345e58\n";
                    ::std::cerr << prev_max_size << " " << new_max_size << "\n";
                    abort();
                  }
                }
              }

              assert(next->object_size() >=
                     m_minimum_alloc_length - align(sizeof(object_state_type), minimum_header_alignment()));
              state->set_next(next);
              state->set_next_valid(true);
              _verify(next);
              _verify(state);
              m_free_list.insert(next);
            }
            // take all of the memory.
            state->set_in_use(true);
            state->set_user_data(m_default_user_data.get());
            assert(state->object_size() >= original_size);
            //          }
            _verify(state);
            return block_type{state->object_start(), state->object_size()};
          }
        }
        object_state_type *next = later_next;
        // check to see if we have memory left over at tail.
        if (!m_next_alloc_ptr)
          return block_type{nullptr, 0};
        // check to see we haven't requested an excessively large allocation.
        if (m_next_alloc_ptr->next_valid()) {
          if (m_next_alloc_ptr->object_size() < original_size)
            return block_type{nullptr, 0};
        }
        else if (static_cast<size_t>(end() - m_next_alloc_ptr->object_start()) < original_size)
          return block_type{nullptr, 0};
        m_next_alloc_ptr->m_user_data = 0;
        // see if we should split memory left over after this allocation.
        bool do_split = reinterpret_cast<uint8_t *>(next) + m_minimum_alloc_length <= end();
        if (do_split) {
          // enough memory is left over after allocation to have a minimum allocation.
          // so perform split.
          next->set_all(m_next_alloc_ptr->next(), false, m_next_alloc_ptr->next_valid());
          m_next_alloc_ptr->set_all(next, true, true);
          m_next_alloc_ptr->set_user_data(m_default_user_data.get());
          auto ret = m_next_alloc_ptr->object_start();
          auto sz = m_next_alloc_ptr->object_size();
          assert(m_next_alloc_ptr->object_size() >= original_size);
          _verify(m_next_alloc_ptr);
          m_next_alloc_ptr = next;
          _verify(next);
          return block_type{ret, sz};
        }
        else {
          // memory left over would be smaller then minimum allocation.
          // take all the memory.
          m_next_alloc_ptr->set_all(reinterpret_cast<object_state_type *>(end()), true, false);
          assert(m_next_alloc_ptr->next() == reinterpret_cast<object_state_type *>(end()));
          m_next_alloc_ptr->set_user_data(m_default_user_data.get());
          auto ret = m_next_alloc_ptr->object_start();
          auto sz = m_next_alloc_ptr->object_size();
          assert(m_next_alloc_ptr->object_size() >= original_size);
          assert(m_next_alloc_ptr->next() == reinterpret_cast<object_state_type *>(end()));
          _verify(m_next_alloc_ptr);
          m_next_alloc_ptr = nullptr;
          _verify(m_next_alloc_ptr);
          return block_type{ret, sz};
        }
      }
      template <typename Allocator_Policy>
      bool allocator_block_t<Allocator_Policy>::destroy(void *v)
      {
        size_t tmp = 0;
        size_t last_max_alloc_available = 0;
        return destroy(v, tmp, last_max_alloc_available);
      }

      template <typename Allocator_Policy>
      bool allocator_block_t<Allocator_Policy>::destroy(void *v, size_t &last_collapsed_size, size_t &last_max_alloc_available)
      {
        // get object state.
        object_state_type *state = object_state_type::from_object_start(v);
        // sanity check that addr belongs to this block.
        if (v < begin() || v >= end())
          return false;
        // if has user data, destroy it.
        if (state->user_data() && state->user_data() != m_default_user_data.get()) {
          typename allocator::template rebind<user_data_type>::other a;
          a.destroy(static_cast<user_data_type *>(state->user_data()));
          a.deallocate(static_cast<user_data_type *>(state->user_data()), 1);
        }
        // no longer in use.
        state->set_in_use(false);
        object_state_type *next = state->next();
        // collapse states.
        while (state->next_valid() && !next->not_available()) {
          state->set_all(next->next(), false, next->next_valid());
          auto it = ::std::find(m_free_list.begin(), m_free_list.end(), next);
          if (it != m_free_list.end()) {
            m_free_list.erase(it);
          }
          next = next->next();
        }
        if (state->next_valid()) {
          // if the next state is valid, then there are states after
          // so add it to free list.
          m_free_list.insert(state);
          last_collapsed_size = state->object_size();
        }
        else {
          // if here the next state is invalid, so this is at tail
          // so just adjust pointer.
          m_next_alloc_ptr = state;
          last_collapsed_size = static_cast<size_t>(end() - reinterpret_cast<uint8_t *>(m_next_alloc_ptr)) -
                                align(sizeof(object_state_type), minimum_header_alignment());
        }
        last_max_alloc_available = m_last_max_alloc_available;
        m_last_max_alloc_available = ::std::max(m_last_max_alloc_available, last_collapsed_size);
        _verify(state);
        return true;
      }
      template <typename Allocator_Policy>
      size_t allocator_block_t<Allocator_Policy>::max_alloc_available(bool)
      {
        size_t max_alloc = 0;
        // if we can alloc at tail, first check that size.
        if (m_next_alloc_ptr)
          max_alloc = static_cast<size_t>(end() - reinterpret_cast<uint8_t *>(m_next_alloc_ptr)) -
                      align(sizeof(object_state_type), minimum_header_alignment());
        // then check size of all objects in free list.
        if (!m_free_list.empty()) {
          const auto top_os = (*m_free_list.rbegin())->object_size();
          max_alloc = ::std::max(max_alloc, top_os);
        }
        m_last_max_alloc_available = max_alloc;
        return max_alloc;
      }
      template <typename Allocator_Policy>
      void allocator_block_t<Allocator_Policy>::collect(size_t &num_quasifreed)
      {
        // reset max alloc available.
        m_last_max_alloc_available = 0;
        // start at beginning of list
        object_state_type *state = reinterpret_cast<object_state_type *>(begin());
        _verify(state);
        m_free_list.clear();
        // while there are more elements in list.
        auto last_insert_point = m_free_list.end();
        bool did_merge = false;
        bool needs_insert = true;
        while (state->next_valid()) {
          cgc1_builtin_prefetch(state->next()->next());
          if (!did_merge && !state->in_use()) {
            needs_insert = true;
          }
          // if it has been marked to be freed
          if (!did_merge && state->quasi_freed()) {
            // we are in while loop, so not at end, so add it to free list.
            state->set_quasi_freed(false);
            needs_insert = true;
            num_quasifreed++;
            assert(!state->quasi_freed());
          }
          // note only check in use for next, it may be quasifree.
          if (!state->not_available() && !state->next()->in_use()) {
            if (state->next()->quasi_freed()) {
              num_quasifreed++;
            }
            // ok, both this state and next one available, so merge them.
            object_state_type *const next = state->next();
            // perform merge.
            state->set_next(next->next());
            state->set_next_valid(next->next_valid());
            if (state->next_valid()) {
              _verify(state);
            }
            did_merge = true;
          }
          else {
            // move onto next state since can't merge.
            _verify(state);
            _verify(state->next());
            if (needs_insert) {
              // put in free list.
              last_insert_point = m_free_list.insert(state).first;
            }
            state = state->next();
            did_merge = false;
          }
        }
        if (!state->not_available()) {
          if (state->quasi_freed()) {
            num_quasifreed++;
          }
          // ok at end of list, if its available.
          // try to find it in free list.
          // WARNING THIS IS ALMOST CERTAINLY BROKEN
          // DEBUG FIX THIS NOW!!! BUG BUG BUG
          if (last_insert_point != m_free_list.end()) {
            m_free_list.erase(last_insert_point);
          }
          // adjust pointer
          m_next_alloc_ptr = state;
          _verify(state);
        }
        if (!::std::is_sorted(m_free_list.begin(), m_free_list.end(), ::mcppalloc::details::os_size_compare{})) {
          ::std::cerr << __FILE__ << " " << __LINE__ << " 90fa93f0-b161-419d-b8ea-0f53752cac6f\n";
          ::std::cerr << "WARNING FREE LIST NOT SORTED\n";
          abort();
        }

        max_alloc_available();
      }
      template <typename Allocator_Policy>
      size_t allocator_block_t<Allocator_Policy>::minimum_allocation_length() const
      {
        return m_minimum_alloc_length;
      }
      template <typename Allocator_Policy>
      size_t allocator_block_t<Allocator_Policy>::maximum_allocation_length() const
      {
        return m_maximum_alloc_length;
      }
      template <typename Allocator_Policy>
      size_t allocator_block_t<Allocator_Policy>::last_max_alloc_available() const noexcept
      {
        return m_last_max_alloc_available;
      }
      template <typename Allocator_Policy>
      size_t allocator_block_t<Allocator_Policy>::secondary_memory_used() const noexcept
      {
        using free_list_type = decltype(m_free_list);
        return sizeof(typename free_list_type::value_type) * m_free_list.capacity();
      }
      template <typename Allocator_Policy>
      void allocator_block_t<Allocator_Policy>::shrink_secondary_memory_usage_to_fit()
      {
        m_free_list.shrink_to_fit();
      }
      template <typename Allocator_Policy>
      void allocator_block_t<Allocator_Policy>::to_ptree(::boost::property_tree::ptree &ptree, int) const
      {
        ptree.put("this", ::std::to_string(reinterpret_cast<uintptr_t>(this)));
        ptree.put("valid", ::std::to_string(valid()));
        ptree.put("last_max_alloc_available", ::std::to_string(last_max_alloc_available()));
        //      ptree.put("max_alloc_available", ::std::to_string(max_alloc_available()));
        ptree.put("secondary_memory_used", ::std::to_string(secondary_memory_used()));
        ptree.put("memory_size", ::std::to_string(memory_size()));
        ptree.put("full", ::std::to_string(full()));
        ptree.put("minimum_allocation_length", ::std::to_string(minimum_allocation_length()));
        ptree.put("maximum_allocation_length", ::std::to_string(maximum_allocation_length()));
      }
      template <typename Allocator_Policy>
      bool is_valid_object_state(const ::mcppalloc::details::object_state_t<Allocator_Policy> *state,
                                 const uint8_t *user_data_range_begin,
                                 const uint8_t *user_data_range_end)
      {
        if (!state->in_use())
          return false;
        auto user_data = state->user_data();
        // check if in valid range.
        if (reinterpret_cast<const uint8_t *>(user_data) < user_data_range_begin ||
            reinterpret_cast<const uint8_t *>(user_data) >= user_data_range_end)
          return false;
        // check for magic constant validity.
        return user_data->is_magic_constant_valid();
      }
    }
  }
}
