#pragma once
#include "allocator_block.hpp"
#include "internal_allocator.hpp"
#include <assert.h>
#include <iostream>
namespace cgc1
{
  namespace details
  {
    template <typename Allocator, typename User_Data>
    allocator_block_t<Allocator, User_Data>::allocator_block_t(void *start,
                                                               size_t length,
                                                               size_t minimum_alloc_length,
                                                               size_t maximum_alloc_length)
        : m_next_alloc_ptr(reinterpret_cast<object_state_t *>(start)), m_end(reinterpret_cast<uint8_t *>(start) + length),
          m_minimum_alloc_length(object_state_t::needed_size(sizeof(object_state_t), minimum_alloc_length)),
          m_start(reinterpret_cast<uint8_t *>(start))
    {
#if _CGC1_DEBUG_LEVEL > 0
      // sanity check alignment of start.
      if (unlikely(reinterpret_cast<size_t>(m_start) % c_alignment != 0))
        abort();
      // sanity check alignment of end.
      if (unlikely(reinterpret_cast<size_t>(m_end) % c_alignment != 0))
        abort();
#endif
      if (maximum_alloc_length == cgc1::details::c_infinite_length) {
        m_maximum_alloc_length = maximum_alloc_length;
      } else {
        m_maximum_alloc_length = object_state_t::needed_size(sizeof(object_state_t), maximum_alloc_length);
      }
      assert(m_minimum_alloc_length <= m_maximum_alloc_length);
      // setup first object state
      m_next_alloc_ptr->set_all(reinterpret_cast<object_state_t *>(reinterpret_cast<uint8_t *>(start) + length), false, false,
                                false);

      // setup default user data.
      m_default_user_data = unique_ptr_allocated<user_data_type, Allocator>(&s_default_user_data);
      m_default_user_data->m_is_default = true;
    }
    template <typename Allocator, typename User_Data>
    allocator_block_t<Allocator, User_Data>::allocator_block_t(allocator_block_t &&block) noexcept
        : m_free_list(std::move(block.m_free_list)),
          m_next_alloc_ptr(block.m_next_alloc_ptr),
          m_end(block.m_end),
          m_minimum_alloc_length(block.m_minimum_alloc_length),
          m_start(block.m_start),
          m_default_user_data(std::move(block.m_default_user_data)),
          m_last_max_alloc_available(block.m_last_max_alloc_available),
          m_maximum_alloc_length(block.m_maximum_alloc_length)
    {
      // invalidate moved from block.
      block.clear();
    }
    template <typename Allocator, typename User_Data>
    allocator_block_t<Allocator, User_Data> &allocator_block_t<Allocator, User_Data>::
    operator=(allocator_block_t<Allocator, User_Data> &&block) noexcept
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
      block.clear();
      return *this;
    }
    template <typename Allocator, typename User_Data>
    allocator_block_t<Allocator, User_Data>::~allocator_block_t()
    {
      if (m_default_user_data.get() == &s_default_user_data)
        m_default_user_data.release();
    }

    template <typename Allocator, typename User_Data>
    void allocator_block_t<Allocator, User_Data>::clear()
    {
      m_free_list.clear();
      m_next_alloc_ptr = nullptr;
      m_end = nullptr;
      m_start = nullptr;
    }
    template <typename Allocator, typename User_Data>
    inline bool allocator_block_t<Allocator, User_Data>::valid() const noexcept
    {
      return m_start != nullptr;
    }
    template <typename Allocator, typename User_Data>
    bool allocator_block_t<Allocator, User_Data>::empty() const noexcept
    {
      if (!m_next_alloc_ptr)
        return false;
      return reinterpret_cast<uint8_t *>(m_next_alloc_ptr) == begin() && !m_next_alloc_ptr->next_valid();
    }
    template <typename Allocator, typename User_Data>
    inline bool allocator_block_t<Allocator, User_Data>::full() const noexcept
    {
      return m_next_alloc_ptr == nullptr && m_free_list.empty();
    }
    template <typename Allocator, typename User_Data>
    ALWAYS_INLINE inline uint8_t *allocator_block_t<Allocator, User_Data>::begin() const noexcept
    {
      return m_start;
    }
    template <typename Allocator, typename User_Data>
    ALWAYS_INLINE inline uint8_t *allocator_block_t<Allocator, User_Data>::end() const noexcept
    {
      return m_end;
    }
    template <typename Allocator, typename User_Data>
    inline object_state_t *allocator_block_t<Allocator, User_Data>::current_end() const noexcept
    {
      if (!m_next_alloc_ptr)
        return reinterpret_cast<object_state_t *>(end());
      else
        return m_next_alloc_ptr;
    }
    template <typename Allocator, typename User_Data>
    ALWAYS_INLINE inline object_state_t *allocator_block_t<Allocator, User_Data>::_object_state_begin() const noexcept
    {
      return reinterpret_cast<object_state_t *>(begin());
    }
    template <typename Allocator, typename User_Data>
    object_state_t *allocator_block_t<Allocator, User_Data>::find_address(void *addr) const noexcept
    {
      for (auto it = make_next_iterator(_object_state_begin()); it != make_next_iterator(current_end()); ++it) {
        if (it->object_end() > addr) {
          return it;
        }
      }
      return nullptr;
    }
#if _CGC1_DEBUG_LEVEL > 1
    template <typename Allocator, typename User_Data>
    void allocator_block_t<Allocator, User_Data>::_verify(const object_state_t *state)
    {
      if (state) {
        assert(state->object_size() < static_cast<size_t>(end() - begin()));
        assert(state->next());
      }
      auto begin = make_next_iterator(reinterpret_cast<object_state_t *>(this->begin()));
      auto end = make_next_iterator(current_end());
      assert(begin.m_t <= end.m_t);
      for (auto os_it = begin; os_it != end; ++os_it) {
        assert(os_it->next_valid() || os_it->next() == end);
        assert(os_it->next() != nullptr);
      }
    }
#else
    template <typename Allocator, typename User_Data>
    void allocator_block_t<Allocator, User_Data>::_verify(const object_state_t *)
    {
    }
#endif
    template <typename Allocator, typename User_Data>
    void *allocator_block_t<Allocator, User_Data>::allocate(size_t size)
    {
      assert(minimum_allocation_length() <= maximum_allocation_length());
      _verify(nullptr);
      // these help, especially when prefetch crosses cache or page boundry.
      cgc1_builtin_prefetch(this);
      cgc1_builtin_prefetch(reinterpret_cast<uint8_t *>(this) + 16);
      cgc1_builtin_prefetch(reinterpret_cast<uint8_t *>(this) + 32);
      const size_t original_size = size;
      size = object_state_t::needed_size(sizeof(object_state_t), size);
      assert(size >= minimum_allocation_length());
      assert(size <= maximum_allocation_length());
      assert(reinterpret_cast<uint8_t *>(m_next_alloc_ptr) == reinterpret_cast<uint8_t *>(&m_next_alloc_ptr->m_next));
      assert(reinterpret_cast<uint8_t *>(m_next_alloc_ptr) + 8 == reinterpret_cast<uint8_t *>(&m_next_alloc_ptr->m_user_data));
      // compute this now.
      object_state_t *later_next = reinterpret_cast<object_state_t *>(reinterpret_cast<uint8_t *>(m_next_alloc_ptr) + size);
      // if the free list isn't trivial, check it first.
      if (!m_free_list.empty()) {
        // do a reverse search from back of free list for somewhere to put the data.
        // we don't have time to do an exhaustive search.
        // also deleting an item from the free list could involve lots of copying if at beginning of list.
        // finally free list is a vector so this should be cache friendly.
        for (auto it = m_free_list.rbegin(); it != m_free_list.rend(); ++it) {
          object_state_t *state = *it;
          // if it doesn't fit, move on to next one.
          if (state->object_size() < original_size)
            continue;
          // erase found from free list.
          m_free_list.erase(it.base() - 1);
          // figure out theoretical next pointer.
          object_state_t *next = reinterpret_cast<object_state_t *>(reinterpret_cast<uint8_t *>(state) + size);
          // see if we can split the memory.
          if (reinterpret_cast<uint8_t *>(next) + m_minimum_alloc_length <= reinterpret_cast<uint8_t *>(state->next())) {
            // if we are here, the memory left over is bigger then minimum alloc size, so split.
            next->set_all(state->next(), false, state->next_valid());
            assert(next->object_size() >= m_minimum_alloc_length - align(sizeof(object_state_t)));
            state->set_next(next);
            state->set_next_valid(true);
            _verify(next);
            _verify(state);
            m_free_list.push_back(next);
          }
          // take all of the memory.
          state->set_in_use(true);
          state->set_user_data(m_default_user_data.get());
          assert(state->object_size() >= original_size);
          _verify(state);
          return state->object_start();
        }
      }
      object_state_t *next = later_next;
      // check to see if we have memory left over at tail.
      if (!m_next_alloc_ptr)
        return nullptr;
      // check to see we haven't requested an excessively large allocation.
      if (m_next_alloc_ptr->next_valid()) {
        if (m_next_alloc_ptr->object_size() < original_size)
          return nullptr;
      } else if (static_cast<size_t>(end() - m_next_alloc_ptr->object_start()) < original_size)
        return nullptr;
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
        assert(m_next_alloc_ptr->object_size() >= original_size);
        _verify(m_next_alloc_ptr);
        m_next_alloc_ptr = next;
        _verify(next);
        return ret;
      } else {
        // memory left over would be smaller then minimum allocation.
        // take all the memory.
        m_next_alloc_ptr->set_all(reinterpret_cast<object_state_t *>(end()), true, false);
        m_next_alloc_ptr->set_user_data(m_default_user_data.get());
        auto ret = m_next_alloc_ptr->object_start();
        assert(m_next_alloc_ptr->object_size() >= original_size);
        _verify(m_next_alloc_ptr);
        m_next_alloc_ptr = nullptr;
        _verify(m_next_alloc_ptr);
        return ret;
      }
    }
    template <typename Allocator, typename User_Data>
    bool allocator_block_t<Allocator, User_Data>::destroy(void *v)
    {
      size_t tmp = 0;
      size_t last_max_alloc_available = 0;
      return destroy(v, tmp, last_max_alloc_available);
    }

    template <typename Allocator, typename User_Data>
    bool allocator_block_t<Allocator, User_Data>::destroy(void *v, size_t &last_collapsed_size, size_t &last_max_alloc_available)
    {
      // get object state.
      object_state_t *state = object_state_t::from_object_start(v);
      // sanity check that addr belongs to this block.
      if (v < begin() || v >= end())
        return false;
      // if has user data, destroy it.
      if (state->user_data() && state->user_data() != m_default_user_data.get()) {
        typename allocator::template rebind<User_Data>::other a;
        a.destroy(static_cast<User_Data *>(state->user_data()));
        a.deallocate(static_cast<User_Data *>(state->user_data()), 1);
      }
      // no longer in use.
      state->set_in_use(false);
      object_state_t *next = state->next();
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
        m_free_list.push_back(state);
        last_collapsed_size = state->object_size();
      } else {
        // if here the next state is invalid, so this is at tail
        // so just adjust pointer.
        m_next_alloc_ptr = state;
        last_collapsed_size =
            static_cast<size_t>(end() - reinterpret_cast<uint8_t *>(m_next_alloc_ptr)) - align(sizeof(object_state_t));
      }
      last_max_alloc_available = m_last_max_alloc_available;
      m_last_max_alloc_available = ::std::max(m_last_max_alloc_available, last_collapsed_size);
      _verify(state);
      return true;
    }
    template <typename Allocator, typename User_Data>
    size_t allocator_block_t<Allocator, User_Data>::max_alloc_available()
    {
      size_t max_alloc = 0;
      // if we can alloc at tail, first check that size.
      if (m_next_alloc_ptr)
        max_alloc = static_cast<size_t>(end() - reinterpret_cast<uint8_t *>(m_next_alloc_ptr)) - align(sizeof(object_state_t));
      // then check size of all objects in free list.
      for (object_state_t *state : m_free_list) {
        max_alloc = ::std::max(max_alloc, state->object_size());
      }
      m_last_max_alloc_available = max_alloc;
      return max_alloc;
    }
    template <typename Allocator, typename User_Data>
    void allocator_block_t<Allocator, User_Data>::collect()
    {
      // reset max alloc available.
      m_last_max_alloc_available = 0;
      // start at beginning of list
      object_state_t *state = reinterpret_cast<object_state_t *>(begin());
      _verify(state);
      m_free_list.clear();
      // while there are more elements in list.
      bool did_merge = false;
      while (state->next_valid()) {
        cgc1_builtin_prefetch(state->next()->next());
        if (!did_merge && !state->in_use()) {
          // update max alloca available.
          m_last_max_alloc_available = ::std::max(m_last_max_alloc_available, state->object_size());
          // put in free list.
          m_free_list.emplace_back(state);
        }
        // if it has been marked to be freed
        if (!did_merge && state->quasi_freed()) {
          // we are in while loop, so not at end, so add it to free list.
          state->set_quasi_freed(false);
          // update max alloca available.
          m_last_max_alloc_available = ::std::max(m_last_max_alloc_available, state->object_size());
          // put in free list.
          m_free_list.emplace_back(state);
          assert(!state->quasi_freed());
        }
        // note only check in use for next, it may be quasifree.
        if (!state->not_available() && !state->next()->in_use()) {
          // ok, both this state and next one available, so merge them.
          object_state_t *next = state->next();
          // perform merge.
          state->set_next(next->next());
          state->set_next_valid(next->next_valid());
          if (state->next_valid())
            _verify(state);
          m_last_max_alloc_available = ::std::max(m_last_max_alloc_available, state->object_size());
          did_merge = true;
        } else {
          // move onto next state since can't merge.
          _verify(state);
          _verify(state->next());
          state = state->next();
          did_merge = false;
        }
      }
      if (!state->not_available()) {
        // ok at end of list, if its available.
        // try to find it in free list.
        if (!m_free_list.empty()) {
          if (*m_free_list.rbegin() == state)
            m_free_list.pop_back();
        }
        // adjust pointer
        m_next_alloc_ptr = state;
        _verify(state);
      }
      // if we can alloc at tail, first check that size.
      if (m_next_alloc_ptr) {
        size_t max_alloc =
            static_cast<size_t>(end() - reinterpret_cast<uint8_t *>(m_next_alloc_ptr)) - align(sizeof(object_state_t));
        m_last_max_alloc_available = ::std::max(m_last_max_alloc_available, max_alloc);
      }
      assert(m_last_max_alloc_available == max_alloc_available());
    }
    template <typename Allocator, typename User_Data>
    size_t allocator_block_t<Allocator, User_Data>::minimum_allocation_length() const
    {
      return m_minimum_alloc_length;
    }
    template <typename Allocator, typename User_Data>
    size_t allocator_block_t<Allocator, User_Data>::maximum_allocation_length() const
    {
      return m_maximum_alloc_length;
    }
    template <typename Allocator, typename User_Data>
    size_t allocator_block_t<Allocator, User_Data>::last_max_alloc_available() const noexcept
    {
      return m_last_max_alloc_available;
    }
    bool
    is_valid_object_state(const object_state_t *state, const uint8_t *user_data_range_begin, const uint8_t *user_data_range_end)
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
