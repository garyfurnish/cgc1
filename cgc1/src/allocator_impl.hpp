#pragma once
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
#error "IDL must be zero"
#endif
#include <iostream>
namespace cgc1
{
  namespace details
  {
    inline constexpr size_t pow2(int n)
    {
      return static_cast<size_t>(2) << (n - 1);
    }
    template <typename Allocator, typename Traits>
    inline allocator_t<Allocator, Traits>::allocator_t()
        : m_shutdown(false)
    {
      m_traits.on_creation(*this);
    }
    template <typename Allocator, typename Traits>
    allocator_t<Allocator, Traits>::~allocator_t()
    {
      m_shutdown = true;
      _ud_verify();
      m_thread_allocators.clear();
      m_blocks.clear();
      m_global_blocks.clear();
    }
    template <typename Allocator, typename Traits>
    inline bool allocator_t<Allocator, Traits>::is_shutdown() const
    {
      return m_shutdown;
    }
    template <typename Allocator, typename Traits>
    inline bool allocator_t<Allocator, Traits>::initialize(size_t initial_gc_heap_size, size_t suggested_max_heap_size)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      if (m_initial_gc_heap_size)
        return false;
      m_initial_gc_heap_size = initial_gc_heap_size;
      m_minimum_expansion_size = m_initial_gc_heap_size;
      if (!m_slab.allocate(m_initial_gc_heap_size, slab_t::find_hole(suggested_max_heap_size)))
        return false;
      m_current_end = m_slab.begin();
      _ud_verify();
      return true;
    }
    template <typename Allocator, typename Traits>
    inline auto allocator_t<Allocator, Traits>::get_memory(size_t sz) -> memory_pair_t
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      _ud_verify();
      // do worst fit memory vector lookup
      typename memory_pair_vector_t::iterator worst = m_free_list.end();
      for (auto it = m_free_list.begin(); it != m_free_list.end(); ++it) {
        if (size_pos(*it) >= sz) {
          if (worst == m_free_list.end())
            worst = it;
          else if (size(*worst) <= size(*it))
            worst = it;
        }
      }
      if (worst != m_free_list.end()) {
        memory_pair_t ret = *worst;
        memory_pair_t free_pair = ::std::make_pair(worst->first + sz, worst->second);
        ret.second = free_pair.first;
        if (size(free_pair))
          ::std::swap(*worst, free_pair);
        else
          m_free_list.erase(worst);
        return ret;
      }
      auto sz_available = m_slab.end() - m_current_end - 1;
      if (sz_available < 0) // shouldn't happen
        abort();
      if (static_cast<size_t>(sz_available) >= sz) {
        uint8_t *new_end = m_current_end + sz;
        auto ret = ::std::make_pair(m_current_end, new_end);
        assert(new_end < m_slab.end());
        m_current_end = new_end;
        return ret;
      }
      assert(m_current_end <= m_slab.end());
      size_t expansion_size = ::std::max(m_slab.size() + m_minimum_expansion_size, m_slab.size() + sz);
      if (!m_slab.expand(expansion_size))
        return ::std::make_pair(nullptr, nullptr);
      uint8_t *new_end = m_current_end + sz;
      assert(new_end <= m_slab.end());
      auto ret = ::std::make_pair(m_current_end, new_end);
      m_current_end = new_end;
      _ud_verify();
      return ret;
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::get_allocator_block(this_thread_allocator_t &ta,
                                                             size_t create_sz,
                                                             size_t minimum_alloc_length,
                                                             size_t maximum_alloc_length,
                                                             size_t allocate_sz,
                                                             block_type &block)
    {
      {
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
        auto found_block = _u_find_global_allocator_block(allocate_sz, minimum_alloc_length, maximum_alloc_length);
        if (found_block != m_global_blocks.end()) {
          auto old_block_addr = &*found_block;
          block = ::std::move(*found_block);
          _u_move_registered_block(old_block_addr, &block);
          // figure out location in vector that shifted.
          size_t location = static_cast<size_t>(found_block - m_global_blocks.begin());
          m_global_blocks.erase(found_block);
          // move everything back one.
          for (size_t i = location; i < m_global_blocks.size(); ++i)
            _u_move_registered_block(&m_global_blocks[i + 1], &m_global_blocks[i]);
          return;
        }
      }
      block = _create_allocator_block(ta, create_sz, minimum_alloc_length, maximum_alloc_length);
      register_allocator_block(ta, block);
    }
    template <typename Allocator, typename Traits>
    inline auto allocator_t<Allocator, Traits>::_create_allocator_block(this_thread_allocator_t &ta,
                                                                        size_t sz,
                                                                        size_t minimum_alloc_length,
                                                                        size_t maximum_alloc_length) -> block_type
    {
      auto memory = get_memory(sz);
      if (!memory.first)
        throw out_of_memory_exception_t();
      auto memory_size = size(memory);
      assert(memory_size > 0);
      auto block = block_type(memory.first, static_cast<size_t>(memory_size), minimum_alloc_length, maximum_alloc_length);
      m_traits.on_create_allocator_block(ta, block);
      return block;
    }
    template <typename Allocator, typename Traits>
    inline void allocator_t<Allocator, Traits>::destroy_allocator_block(this_thread_allocator_t &ta, block_type &&in_block)
    {
      m_traits.on_destroy_allocator_block(ta, in_block);
      auto block = std::move(in_block);
      unregister_allocator_block(ta, block);
      release_memory(std::make_pair(block.begin(), block.end()));
    }
    struct block_handle_begin_compare_t {
      template <typename T1>
      bool operator()(const T1 &h1, const T1 &h2)
      {
        return h1.m_begin < h2.m_begin;
      }
    };
    template <typename Allocator, typename Traits>
    inline void allocator_t<Allocator, Traits>::register_allocator_block(this_thread_allocator_t &ta, block_type &block)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      _ud_verify();
      for (auto &&it : m_blocks) {
        if (it.m_block == &block)
          abort();
        if (it.m_begin == block.begin())
          abort();
      }
      this_allocator_block_handle_t handle(&ta, &block, block.begin());
      auto ub = ::std::upper_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
      m_blocks.emplace(ub, &ta, &block, block.begin());
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    inline void allocator_t<Allocator, Traits>::unregister_allocator_block(this_thread_allocator_t &ta, block_type &block)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      _u_unregister_allocator_block(ta, block);
    }

    template <typename Allocator, typename Traits>
    inline void allocator_t<Allocator, Traits>::_u_unregister_allocator_block(this_thread_allocator_t &ta, block_type &block)
    {
      _ud_verify();
      this_allocator_block_handle_t handle(&ta, &block, block.begin());
      auto lb = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
      if (lb != m_blocks.end() && lb->m_begin == block.begin())
        m_blocks.erase(lb);
      else {
        // This should never happen, so memory corruption issue if it has, so kill the program.
        abort();
      }
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_ud_verify()
    {
#if _CGC1_DEBUG_LEVEL > 1
      assert(m_blocks.empty() || ::std::is_sorted(m_blocks.begin(), m_blocks.end(), block_handle_begin_compare_t{}));
#endif
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_d_verify()
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_u_move_registered_block(block_type *old_block, block_type *new_block)
    {
      this_allocator_block_handle_t handle(nullptr, old_block, new_block->begin());
      auto lb = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
      if (lb != m_blocks.end()) {
        if (lb->m_block == old_block) {
          this_allocator_block_handle_t new_handle(lb->m_thread_allocator, new_block, new_block->begin());
          auto ub = ::std::upper_bound(m_blocks.begin(), m_blocks.end(), new_handle, block_handle_begin_compare_t{});
          if (ub > lb) {
            ::std::rotate(lb, lb + 1, ub);
            (ub - 1)->m_block = new_block;
          } else {
            ::std::rotate(ub, ub + 1, lb);
            ub->m_block = new_block;
          }
          _ud_verify();
        } else {
          ::std::cerr << "CGC1: Unable to find block to move\n";
          ::std::cerr << old_block << " " << new_block << ::std::endl;
          // This should never happen, so memory corruption issue if it has, so kill the program.
          abort();
        }
      } else {
        ::std::cerr << "CGC1: Unable to find block to move, lb is end\n";
        // This should never happen, so memory corruption issue if it has, so kill the program.
        abort();
      }
    }
    template <typename Allocator, typename Traits>
    template <typename Container>
    void allocator_t<Allocator, Traits>::_u_move_registered_blocks(const Container &blocks, size_t offset)
    {
      for (block_type &block : const_cast<Container &>(blocks)) {
        block_type *old_block = reinterpret_cast<block_type *>(reinterpret_cast<uint8_t *>(&block) - offset);
        _u_move_registered_block(old_block, &block);
      }
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    inline auto allocator_t<Allocator, Traits>::_u_find_block(void *in_addr) -> const this_allocator_block_handle_t *
    {
      uint8_t *addr = reinterpret_cast<uint8_t *>(in_addr);
      _ud_verify();
      this_allocator_block_handle_t handle(nullptr, nullptr, addr);
      auto lb = ::std::upper_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
      if (lb == m_blocks.begin())
        return nullptr;
      lb--;
      assert(lb->m_block->begin() <= addr);
      if (lb->m_block->end() <= addr)
        return nullptr;
      _ud_verify();
      return &*lb;
    }
    template <typename Allocator, typename Traits>
    inline void allocator_t<Allocator, Traits>::to_global_allocator_block(block_type &&block)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      if (!m_global_blocks.capacity())
        m_global_blocks.reserve(20000);
      auto old_block_addr = &block;
      m_global_blocks.emplace_back(std::move(block));
      _u_move_registered_block(old_block_addr, &m_global_blocks.back());
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    inline void allocator_t<Allocator, Traits>::release_memory(const memory_pair_t &pair)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      _ud_verify();
      if (pair.second == m_current_end) {
        m_current_end = pair.first;
        assert(m_current_end <= m_slab.end());
        return;
      } else {
        m_free_list.push_back(pair);
      }
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    inline uint8_t *allocator_t<Allocator, Traits>::begin() const
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return m_slab.begin();
    }
    template <typename Allocator, typename Traits>
    inline uint8_t *allocator_t<Allocator, Traits>::_u_begin() const
    {
      return m_slab.begin();
    }
    template <typename Allocator, typename Traits>
    inline uint8_t *allocator_t<Allocator, Traits>::_u_current_end() const
    {
      return m_current_end;
    }
    template <typename Allocator, typename Traits>
    inline uint8_t *allocator_t<Allocator, Traits>::end() const
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return m_slab.end();
    }
    template <typename Allocator, typename Traits>
    inline uint8_t *allocator_t<Allocator, Traits>::current_end() const
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return m_current_end;
    }
    template <typename Allocator, typename Traits>
    inline void allocator_t<Allocator, Traits>::collapse()
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      _ud_verify();
      auto end = m_free_list.end();
      for (auto it = m_free_list.begin(); it < end; ++it) {
        for (auto it2 = it; it2 < end; ++it2) {
          if (it->first == it2->second) {
            // it->second stays same
            it->first = it2->first;
            ::std::swap(*--end, *it2);
          }
          if (it->second == it2->first) {
            // it->first stays same
            it->second = it2->second;
            ::std::swap(*--end, *it2);
          }
          if (it2->second == m_current_end) {
            m_current_end = it2->first;
            ::std::swap(*--end, *it2);
          }
        }
      }
      // static cast is fine as end() is guarenteed to be greater then "real" end.
      m_free_list.resize(m_free_list.size() - static_cast<size_t>(m_free_list.end() - end));
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    inline auto allocator_t<Allocator, Traits>::_d_free_list() const -> memory_pair_vector_t
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return _ud_free_list();
    }
    template <typename Allocator, typename Traits>
    inline auto allocator_t<Allocator, Traits>::_ud_free_list() const -> const memory_pair_vector_t &
    {
      return m_free_list;
    }
    template <typename Allocator, typename Traits>
    inline auto allocator_t<Allocator, Traits>::initialize_thread() -> this_thread_allocator_t &
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      _ud_verify();
      auto it = m_thread_allocators.find(::std::this_thread::get_id());
      if (it != m_thread_allocators.end())
        return *it->second;
      _ud_verify();
      thread_allocator_unique_ptr_t ta = make_unique_allocator<this_thread_allocator_t, allocator>(*this);
      _ud_verify();
      auto &ret = *ta.get();
      m_thread_allocators.emplace(::std::this_thread::get_id(), ::std::move(ta));
      _ud_verify();
      return ret;
    }
    template <typename Allocator, typename Traits>
    inline void allocator_t<Allocator, Traits>::destroy_thread()
    {
      thread_allocator_unique_ptr_t ptr;
      {
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
        _ud_verify();
        auto it = m_thread_allocators.find(::std::this_thread::get_id());
        ptr = std::move(it->second);
        if (it != m_thread_allocators.end())
          m_thread_allocators.erase(it);
        _ud_verify();
      }
    }
    template <typename Allocator, typename Traits>
    auto allocator_t<Allocator, Traits>::_u_blocks() -> decltype(m_blocks) &
    {
      return m_blocks;
    }
    template <typename Allocator, typename Traits>
    template <typename Container>
    void allocator_t<Allocator, Traits>::bulk_destroy_memory(Container &container)
    {
      _d_verify();
      for (object_state_t *os : container) {
        if (os)
          os->set_quasi_freed();
      }
      container.clear();
    }
    template <typename Allocator, typename Traits>
    size_t allocator_t<Allocator, Traits>::num_global_blocks()
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return _u_num_global_blocks();
    }
    template <typename Allocator, typename Traits>
    size_t allocator_t<Allocator, Traits>::_u_num_global_blocks()
    {
      return m_global_blocks.size();
    }
    template <typename Allocator, typename Traits>
    auto allocator_t<Allocator, Traits>::_u_find_global_allocator_block(size_t sz,
                                                                        size_t minimum_alloc_length,
                                                                        size_t maximum_alloc_length) ->
        typename global_block_vector_type::iterator
    {
      minimum_alloc_length = object_state_t::needed_size(sizeof(object_state_t), minimum_alloc_length);
      maximum_alloc_length = object_state_t::needed_size(sizeof(object_state_t), maximum_alloc_length);
      auto it = ::std::find_if(m_global_blocks.begin(), m_global_blocks.end(), [this, sz, minimum_alloc_length,
                                                                                maximum_alloc_length](const block_type &block) {
        return block.minimum_allocation_length() == minimum_alloc_length &&
               block.maximum_allocation_length() == maximum_alloc_length && block.max_alloc_available() >= sz;
      });
      return it;
    }
  }
}
