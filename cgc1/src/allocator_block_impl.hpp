#pragma once
#include "allocator_block.hpp"
#include "internal_allocator.hpp"
#include <assert.h>
namespace cgc1
{
  namespace details
  {
    template <typename Allocator, typename User_Data>
    user_data_base_t::user_data_base_t(allocator_block_t<Allocator, User_Data> *block)
        : m_allocator_block_begin(block->begin())
    {
    }
    template <typename Allocator, typename User_Data>
    gc_user_data_t::gc_user_data_t(allocator_block_t<Allocator, User_Data> *block)
        : user_data_base_t(block)
    {
    }
    template <typename Allocator, typename User_Data>
    inline allocator_block_t<Allocator, User_Data>::allocator_block_t(void *start, size_t length, size_t minimum_alloc_length)
        : m_next_alloc_ptr(reinterpret_cast<object_state_t *>(start)), m_end(reinterpret_cast<uint8_t *>(start) + length),
          m_minimum_alloc_length(object_state_t::needed_size(sizeof(object_state_t), minimum_alloc_length)),
          m_start(reinterpret_cast<uint8_t *>(start))
    {
      if (((size_t)m_start) % c_alignment != 0)
        abort();
      if (((size_t)m_end) % c_alignment != 0)
        abort();
      m_next_alloc_ptr->set_next_valid(false);
      m_next_alloc_ptr->set_in_use(false);
      m_next_alloc_ptr->set_next(reinterpret_cast<object_state_t *>(reinterpret_cast<uint8_t *>(start) + length));
      m_default_user_data = make_unique_allocator<user_data_type, Allocator>(this);
      m_default_user_data->m_is_default = true;
    }
    template <typename Allocator, typename User_Data>
    inline allocator_block_t<Allocator, User_Data>::allocator_block_t(allocator_block_t &&block)
        : m_free_list(std::move(block.m_free_list)), m_next_alloc_ptr(block.m_next_alloc_ptr), m_end(block.m_end),
          m_minimum_alloc_length(block.m_minimum_alloc_length), m_start(block.m_start),
          m_default_user_data(std::move(block.m_default_user_data))
    {
      block.clear();
    }
    template <typename Allocator, typename User_Data>
    inline allocator_block_t<Allocator, User_Data> &allocator_block_t<Allocator, User_Data>::
    operator=(allocator_block_t<Allocator, User_Data> &&block)
    {
      m_free_list = std::move(block.m_free_list);
      m_next_alloc_ptr = block.m_next_alloc_ptr;
      m_end = block.m_end;
      m_minimum_alloc_length = block.m_minimum_alloc_length;
      m_start = block.m_start;
      block.clear();
      return *this;
    }
    template <typename Allocator, typename User_Data>
    inline void allocator_block_t<Allocator, User_Data>::clear()
    {
      m_free_list.clear();
      m_next_alloc_ptr = nullptr;
      m_end = nullptr;
      m_start = nullptr;
    }
    template <typename Allocator, typename User_Data>
    inline bool allocator_block_t<Allocator, User_Data>::valid() const
    {
      return m_start != nullptr;
    }
    template <typename Allocator, typename User_Data>
    inline bool allocator_block_t<Allocator, User_Data>::empty() const
    {
      if (!m_next_alloc_ptr)
        return false;
      return reinterpret_cast<uint8_t *>(m_next_alloc_ptr) == begin() && !m_next_alloc_ptr->next_valid();
    }
    template <typename Allocator, typename User_Data>
    inline bool allocator_block_t<Allocator, User_Data>::full() const
    {
      return m_next_alloc_ptr == nullptr && m_free_list.empty();
    }
    template <typename Allocator, typename User_Data>
    inline uint8_t *allocator_block_t<Allocator, User_Data>::begin() const
    {
      return m_start;
    }
    template <typename Allocator, typename User_Data>
    inline uint8_t *allocator_block_t<Allocator, User_Data>::end() const
    {
      return m_end;
    }
    template <typename Allocator, typename User_Data>
    inline object_state_t *allocator_block_t<Allocator, User_Data>::current_end() const
    {
      if (!m_next_alloc_ptr)
        return reinterpret_cast<object_state_t *>(end());
      else
        return m_next_alloc_ptr;
    }
    template <typename Allocator, typename User_Data>
    inline object_state_t *allocator_block_t<Allocator, User_Data>::_object_state_begin() const
    {
      return reinterpret_cast<object_state_t *>(begin());
    }
    template <typename Allocator, typename User_Data>
    inline object_state_t *allocator_block_t<Allocator, User_Data>::find_address(void *addr) const
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
    inline void *allocator_block_t<Allocator, User_Data>::allocate(size_t size)
    {
      _verify(nullptr);
      cgc1_builtin_prefetch(this);
      cgc1_builtin_prefetch(reinterpret_cast<uint8_t *>(this) + 16);
      cgc1_builtin_prefetch(reinterpret_cast<uint8_t *>(this) + 32);
      const size_t original_size = size;
      size = object_state_t::needed_size(sizeof(object_state_t), size);
      assert(reinterpret_cast<uint8_t *>(m_next_alloc_ptr) == reinterpret_cast<uint8_t *>(&m_next_alloc_ptr->m_next));
      assert(reinterpret_cast<uint8_t *>(m_next_alloc_ptr) + 8 == reinterpret_cast<uint8_t *>(&m_next_alloc_ptr->m_user_data));
      // if the free list isn't trivial, check it first.
      if (!m_free_list.empty()) {
        for (auto it = m_free_list.rbegin(); it != m_free_list.rend(); ++it) {
          object_state_t *state = *it;
          if (state->object_size() < original_size)
            continue;
          m_free_list.erase(it.base() - 1);
          object_state_t *next = reinterpret_cast<object_state_t *>(reinterpret_cast<uint8_t *>(state) + size);
          if (reinterpret_cast<uint8_t *>(next) + m_minimum_alloc_length <= reinterpret_cast<uint8_t *>(state->next())) {
            next->set_all(state->next(), false, state->next_valid());
            assert(next->object_size() >= m_minimum_alloc_length - align(sizeof(object_state_t)));
            state->set_next(next);
            state->set_next_valid(true);
            _verify(next);
            _verify(state);
            m_free_list.push_back(next);
          }
          state->set_in_use(true);
          state->set_user_data(m_default_user_data.get());
          assert(state->object_size() >= original_size);
          _verify(state);
          return state->object_start();
        }
      }
      object_state_t *next = reinterpret_cast<object_state_t *>(reinterpret_cast<uint8_t *>(m_next_alloc_ptr) + size);
      cgc1_builtin_prefetch(next);
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
      bool do_split = reinterpret_cast<uint8_t *>(next) + m_minimum_alloc_length <= end();
      if (do_split) {
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
    inline bool allocator_block_t<Allocator, User_Data>::destroy(void *v)
    {
      if (!v)
        return true;
      if (v < begin() || v >= end())
        return false;
      object_state_t *state = object_state_t::from_object_start(v);
      if (state->user_data() && state->user_data() != m_default_user_data.get()) {
        typename allocator::template rebind<User_Data>::other a;
        a.destroy(static_cast<User_Data *>(state->user_data()));
        a.deallocate(static_cast<User_Data *>(state->user_data()), 1);
      }
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
      if (state->next_valid())
        m_free_list.push_back(state);
      else
        m_next_alloc_ptr = state;
      _verify(state);
      return true;
    }
    template <typename Allocator, typename User_Data>
    inline size_t allocator_block_t<Allocator, User_Data>::max_alloc_available() const
    {
      size_t max_alloc = 0;
      if (m_next_alloc_ptr)
        max_alloc = static_cast<size_t>(end() - reinterpret_cast<uint8_t *>(m_next_alloc_ptr)) - align(sizeof(object_state_t));
      for (object_state_t *state : m_free_list) {
        max_alloc = ::std::max(max_alloc, state->object_size());
      }
      return max_alloc;
    }
    template <typename Allocator, typename User_Data>
    inline void allocator_block_t<Allocator, User_Data>::collect()
    {
      object_state_t *state = reinterpret_cast<object_state_t *>(begin());
      _verify(state);
      while (state->next_valid()) {
        cgc1_builtin_prefetch(state->next()->next());
        if (state->quasi_freed()) {
          state->set_quasi_freed(false);
          m_free_list.emplace_back(state);
        }
        if (!state->not_available() && !state->next()->not_available()) {
          object_state_t *next = state->next();
          auto it = ::std::find(m_free_list.begin(), m_free_list.end(), next);
          if (it != m_free_list.end()) {
            m_free_list.erase(it);
          }
          state->set_next(next->next());
          state->set_next_valid(next->next_valid());
          if (state->next_valid())
            _verify(state);
        } else {

          _verify(state);
          _verify(state->next());
          state = state->next();
        }
      }
      if (!state->not_available()) {
        auto it = ::std::find(m_free_list.begin(), m_free_list.end(), state);
        if (it != m_free_list.end()) {
          m_free_list.erase(it);
        }
        m_next_alloc_ptr = state;
        _verify(state);
      }
    }
    inline bool
    is_valid_object_state(const object_state_t *state, const uint8_t *user_data_range_begin, const uint8_t *user_data_range_end)
    {
      if (!state->in_use())
        return false;
      auto user_data = state->user_data();
      if (reinterpret_cast<const uint8_t *>(user_data) < user_data_range_begin ||
          reinterpret_cast<const uint8_t *>(user_data) >= user_data_range_end)
        return false;
      return user_data->is_magic_constant_valid();
    }
  }
}
