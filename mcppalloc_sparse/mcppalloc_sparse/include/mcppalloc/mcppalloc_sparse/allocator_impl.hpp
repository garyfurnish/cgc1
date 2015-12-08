#pragma once
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
#error "IDL must be zero"
#endif
#include <iostream>
#include <mcppalloc/mcppalloc_utils/container_functions.hpp>
namespace mcppalloc
{
  namespace sparse
  {
    namespace details
    {
      template <typename Allocator_Policy>
      inline allocator_t<Allocator_Policy>::allocator_t()
      {
        // notify traits that the allocator was created.
        m_thread_policy.on_creation(*this);
      }
      template <typename Allocator_Policy>
      allocator_t<Allocator_Policy>::~allocator_t()
      {
        if (!m_shutdown)
          shutdown();
        assert(m_shutdown);
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::shutdown()
      {
        {
          MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        }
        MCPPALLOC_CONCURRENCY_LOCK_ASSUME(m_mutex);
        _ud_verify();
        // first shutdown all thread allocators.
        m_thread_allocators.clear();
        // then get rid of all block handles.
        m_blocks.clear();
        // then get rid of any lingering blocks.
        m_global_blocks.clear();
        m_free_list.clear();
        {
          auto a1 = ::std::move(m_thread_allocators);
          auto a2 = ::std::move(m_blocks);
          auto a3 = ::std::move(m_global_blocks);
          auto a4 = ::std::move(m_free_list);
        }
        // tell the world the destructor has been called.
        m_shutdown = true;
      }
      template <typename Allocator_Policy>
      bool allocator_t<Allocator_Policy>::is_shutdown() const
      {
        return m_shutdown;
      }
      template <typename Allocator_Policy>
      bool allocator_t<Allocator_Policy>::initialize(size_t initial_gc_heap_size, size_t max_heap_size)
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        // sanity check heap size.
        if (m_initial_gc_heap_size)
          return false;
        m_initial_gc_heap_size = initial_gc_heap_size;
        m_minimum_expansion_size = m_initial_gc_heap_size;
        // try to allocate at a location that has room for expansion.
        if (!m_slab.allocate(m_initial_gc_heap_size, slab_t::find_hole(max_heap_size)))
          return false;
        // setup current end point (nothing used yet).
        m_current_end = m_slab.begin();
        _ud_verify();
        return true;
      }
      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::get_memory(size_t sz, bool try_expand) -> system_memory_range_t
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        return _u_get_memory(sz, try_expand);
      }

      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::_u_get_memory(size_t sz, bool try_expand) -> system_memory_range_t
      {
        _ud_verify();
        sz = align(sz, c_alignment);
        // do worst fit memory vector lookup
        typename memory_pair_vector_t::iterator worst = m_free_list.end();
        auto find_pair = memory_pair_t(nullptr, reinterpret_cast<uint8_t *>(sz));
        worst =
            last_greater_equal_than(m_free_list.begin(), m_free_list.end(), find_pair, system_memory_range_t::size_comparator());
        if (worst != m_free_list.end()) {
          // subdivide
          system_memory_range_t ret = *worst;
          m_free_list.erase(worst);
          // calculate part not used.
          system_memory_range_t free_pair(ret.begin() + sz, ret.end());
          ret.set_end(free_pair.begin());
          if (!free_pair.empty()) {
            auto ub =
                ::std::upper_bound(m_free_list.begin(), m_free_list.end(), free_pair, system_memory_range_t::size_comparator());
            if (m_current_end == free_pair.end())
              m_current_end = free_pair.begin();
            else {
              m_free_list.emplace(ub, free_pair);
            }
          }
          assert(reinterpret_cast<uintptr_t>(ret.begin()) % c_alignment == 0);
          assert(reinterpret_cast<uintptr_t>(ret.end()) % c_alignment == 0);
          return memory_pair_t(ret.begin(), ret.end());
        }
        // no space available in free list.
        auto sz_available = m_slab.end() - m_current_end;
        // heap out of memory.
        if (sz_available < 0) // shouldn't happen
          ::std::terminate();
        if (static_cast<size_t>(sz_available) >= sz) {
          // recalculate used end.
          uint8_t *new_end = m_current_end + sz;
          // create the memory interval pair.
          auto ret = ::std::make_pair(m_current_end, new_end);
          assert(new_end <= m_slab.end());
          m_current_end = new_end;
          assert(reinterpret_cast<uintptr_t>(ret.first) % c_alignment == 0);
          assert(reinterpret_cast<uintptr_t>(ret.second) % c_alignment == 0);
          return ret;
        }
        // we need to expand the heap.
        assert(m_current_end <= m_slab.end());
        size_t expansion_size = ::std::max(m_slab.size() + m_minimum_expansion_size, m_slab.size() + sz);
        if (expansion_size > max_heap_size())
          return system_memory_range_t::make_nullptr();
        if (!try_expand)
          return system_memory_range_t::make_nullptr();
        if (!m_slab.expand(expansion_size)) {
          ::std::cerr << "Unable to expand slab to " << expansion_size << ::std::endl;
          // unable to expand heap so return error condition.
          return system_memory_range_t::make_nullptr();
        }
        // recalculate used end.
        uint8_t *new_end = m_current_end + sz;
        assert(new_end <= m_slab.end());
        // create the memory interval pair.
        auto ret = ::std::make_pair(m_current_end, new_end);
        m_current_end = new_end;
        _ud_verify();
        assert(reinterpret_cast<uintptr_t>(ret.first) % c_alignment == 0);
        assert(reinterpret_cast<uintptr_t>(ret.second) % c_alignment == 0);
        return ret;
      }
      template <typename Allocator_Policy>
      bool allocator_t<Allocator_Policy>::_u_get_unregistered_allocator_block(this_thread_allocator_t &ta,
                                                                              size_t create_sz,
                                                                              size_t minimum_alloc_length,
                                                                              size_t maximum_alloc_length,
                                                                              size_t allocate_sz,
                                                                              allocator_block_type &block,
                                                                              bool try_expand)
      {
        // first check to see if we can find a partially used block that fits parameters.
        auto found_block = _u_find_global_allocator_block(allocate_sz, minimum_alloc_length, maximum_alloc_length);
        if (found_block != m_global_blocks.end()) {
          // reuse old block.
          auto old_block_addr = &*found_block;
          _u_unregister_allocator_block(*old_block_addr);
          // move old block into new address.
          block = ::std::move(*found_block);
          // move block registration.
          //          _u_move_registered_block(old_block_addr, &block);
          // figure out location in vector that shifted.
          size_t location = static_cast<size_t>(found_block - m_global_blocks.begin());
          // remove block from vector.
          m_global_blocks.erase(found_block);
          // move all registrations after that global block back one position.
          for (size_t i = location; i < m_global_blocks.size(); ++i) {
            _u_move_registered_block(&m_global_blocks[i + 1], &m_global_blocks[i]);
          }
          return true;
        }
        // otherwise just create a new block
        return _u_create_allocator_block(ta, create_sz, minimum_alloc_length, maximum_alloc_length, block, try_expand);
      }
      template <typename Allocator_Policy>
      bool allocator_t<Allocator_Policy>::_u_create_allocator_block(this_thread_allocator_t &ta,
                                                                    size_t sz,
                                                                    size_t minimum_alloc_length,
                                                                    size_t maximum_alloc_length,
                                                                    allocator_block_type &block,
                                                                    bool try_expand)
      {
        (void)try_expand;
        // try to allocate memory.
        auto memory = _u_get_memory(sz, true);
        if (!memory.begin()) {
          return false;
        }
        // get actual size of memory.
        auto memory_size = memory.size();
        assert(memory_size > 0);
        // create block.
        block.~allocator_block_type();
        new (&block)
            allocator_block_type(memory.begin(), static_cast<size_t>(memory_size), minimum_alloc_length, maximum_alloc_length);
        // call traits function that gets called when block is created.
        m_thread_policy.on_create_allocator_block(ta, block);
        return true;
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::destroy_allocator_block(this_thread_allocator_t &ta, allocator_block_type &&block)
      {
        // notify traits that a memory block is being destroyed.
        m_thread_policy.on_destroy_allocator_block(ta, block);
        // unregister block.
        unregister_allocator_block(block);
        // release memory now that block is unregistered.
        release_memory(std::make_pair(block.begin(), block.end()));
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::_u_destroy_global_allocator_block(allocator_block_type &&block)
      {
        // unregister block.
        _u_unregister_allocator_block(block);
        // release memory now that block is unregistered.
        _u_release_memory(std::make_pair(block.begin(), block.end()));
      }

      /**
       * \brief Functor to compare block handles by beginning locations.
       **/
      struct block_handle_begin_compare_t {
        template <typename T1>
        bool operator()(const T1 &h1, const T1 &h2)
        {
          return h1.m_begin < h2.m_begin;
        }
      };
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::_u_register_allocator_block(this_thread_allocator_t &ta, allocator_block_type &block)
      {
#if MCPPALLOC_DEBUG_LEVEL > 0
        _ud_verify();
        for (auto &&it : m_blocks) {
          // it is a fatal error to try to double add and something is inconsistent.  Terminate before memory corruption spreads.
          if (it.m_block == &block)
            ::std::terminate();
          // it is a fatal error to try to double add and something is inconsistent.  Terminate before memory corruption spreads.
          if (it.m_begin == block.begin()) {
            ::std::cerr << " Attempt to double register block. 77dbea01-7e0f-49da-81f1-9ad7f4616eea\n";
            ::std::cerr << "77dbea01-7e0f-49da-81f1-9ad7f4616eea " << &block << " " << reinterpret_cast<void *>(block.begin())
                        << ::std::endl;
            ::std::terminate();
          }
        }
#endif
        // create a fake handle to search for.
        this_allocator_block_handle_t handle;
        handle.initialize(&ta, &block, block.begin());
        // find the place to insert the block.
        auto ub = ::std::upper_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
        // emplace a block handle.
        auto it = m_blocks.emplace(ub);
        it->initialize(&ta, &block, block.begin());
        _ud_verify();
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::unregister_allocator_block(allocator_block_type &block)
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        // forward unregistration.
        _u_unregister_allocator_block(block);
      }

      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::_u_unregister_allocator_block(allocator_block_type &block)
      {
        _ud_verify();
        // create a fake handle to search for.
        this_allocator_block_handle_t handle;
        handle.initialize(nullptr, &block, block.begin());
        // uniqueness guarentees lb is handle if found.
        auto lb = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
        if (lb != m_blocks.end() && lb->m_begin == block.begin()) {
          // erase if found.
          m_blocks.erase(lb);
        }
        else {
          // This should never happen, so memory corruption issue if it has, so kill the program.
          ::std::cerr << "Unable to find allocator block to unregister e5471709-3eae-43bf-bdd9-86ba9064f103\n" << ::std::endl;
          ::std::terminate();
        }
        _ud_verify();
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::_ud_verify()
      {
#if MCPPALLOC_DEBUG_LEVEL > 1
        // this is really expensive, but verify that blocks are sorted.
        assert(m_blocks.empty() || ::std::is_sorted(m_blocks.begin(), m_blocks.end(), block_handle_begin_compare_t{}));
#endif
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::_d_verify()
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        // forward verify request.
        _ud_verify();
      }
      template <typename Allocator_Policy>
      template <typename Iterator>
      void
      allocator_t<Allocator_Policy>::_u_move_registered_blocks(const Iterator &begin, const Iterator &end, const ptrdiff_t offset)
      {

        if (begin != end) {
          allocator_block_type *new_block = &*begin;
          allocator_block_type *old_block =
              reinterpret_cast<allocator_block_type *>(reinterpret_cast<uint8_t *>(new_block) - offset);

          // create fake handle to search for.
          this_allocator_block_handle_t handle;
          handle.initialize(nullptr, old_block, new_block->begin());
          auto lb = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
          assert(lb != m_blocks.end());
          assert(lb->m_begin == new_block->begin());
          size_t i = 0;
          size_t sz = static_cast<size_t>(end - begin);
          size_t contig_start = 0;
          size_t contiguous = 0;
          // first we must locate a section of contiuous blocks.
          do {
            auto &block_handle = *(lb + static_cast<difference_type>(i));
            auto next_old_block = reinterpret_cast<allocator_block_type *>(reinterpret_cast<uint8_t *>(old_block) +
                                                                           i * sizeof(allocator_block_type));
            if (block_handle.m_block == next_old_block) {
              // another contiguous block found.
              contiguous++;
            }
            else {
              // if here, next block is not contiguous so move first.
              // move first
              _u_move_registered_blocks_contiguous(contiguous, begin + static_cast<ptrdiff_t>(contig_start), lb);
              // then update search.
              new_block = &*(begin + static_cast<ptrdiff_t>(i));
              old_block = reinterpret_cast<allocator_block_type *>(reinterpret_cast<uint8_t *>(new_block) - offset);
              handle.initialize(nullptr, old_block, new_block->begin());
              lb = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
              contiguous = 1;
              contig_start = i;
            }
            ++i;
          } while (i < sz);
          // move last set of contiguous blocks.
          _u_move_registered_blocks_contiguous(contiguous, begin + static_cast<ptrdiff_t>(contig_start), lb);
        }
        _ud_verify();
      }

      template <typename Allocator_Policy>
      template <typename Iterator, typename LB>
      void allocator_t<Allocator_Policy>::_u_move_registered_blocks_contiguous(size_t contiguous,
                                                                               const Iterator &new_location,
                                                                               const LB &lb)
      {

        this_allocator_block_handle_t new_handle;
        auto adj_location = new_location + static_cast<ptrdiff_t>(contiguous - 1);
        new_handle.initialize(lb->m_thread_allocator, &*adj_location, adj_location->begin());

        // uniqueness guarentees this is correct insertion point.
        auto ub = ::std::upper_bound(m_blocks.begin(), m_blocks.end(), new_handle, block_handle_begin_compare_t{});
        // while block handle is not default constructable, we can move away from it.
        // thus we can use rotate to create an empty location to modify.
        // this is the optimal solution for moving in this fashion.
        if (mcppalloc_likely(ub > lb)) {
          ::std::rotate(lb, lb + static_cast<ptrdiff_t>(contiguous), ub);
          for (size_t i = contiguous; i > 0; --i) {
            auto ub_offset = static_cast<ptrdiff_t>(i);
            auto new_location_offset = static_cast<ptrdiff_t>(contiguous - i);
            (ub - ub_offset)->m_block = &*(new_location + new_location_offset);
          }
        }
        else {
          ::std::cerr << "During move, UB <=lb";
          ::std::terminate();
        }
        _ud_verify();
      }

      template <typename Allocator_Policy>
      template <typename Container>
      void allocator_t<Allocator_Policy>::_u_move_registered_blocks(Container &blocks, ptrdiff_t offset)
      {
        _u_move_registered_blocks(blocks.begin(), blocks.end(), offset);
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::_u_move_registered_block(allocator_block_type *old_block,
                                                                   allocator_block_type *new_block)
      {
        // create fake handle to search for.
        this_allocator_block_handle_t handle;
        handle.initialize(nullptr, old_block, new_block->begin());
        auto lb = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
        // uniqueness guarentees lb is old block if it exists.
        if (lb != m_blocks.end()) {
          // sanity check uniqueness.
          if (lb->m_block == old_block) {
            _u_move_registered_blocks_contiguous(1, new_block, lb);
          }
          else {
            // Uniqueness of block failed.
            ::std::cerr << "MCPPALLOC: Unable to find block to move. 1b455b54-e6b2-4f5e-9f1c-957012dfddc5\n";
            ::std::cerr << old_block << " " << new_block << ::std::endl;
            // This should never happen, so memory corruption issue if it has, so kill the program.
            ::std::terminate();
          }
        }
        else {
          // couldn't find old block
          ::std::cerr << "MCPPALLOC: Unable to find block to move, lb is end. e2c0011f-52fa-4f86-886e-b9b932cc0cb3\n";
          // This should never happen, so memory corruption issue if it has, so kill the program.
          ::std::terminate();
        }
      }
      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::_u_find_block(void *in_addr) -> const this_allocator_block_handle_t *
      {
        uint8_t *addr = reinterpret_cast<uint8_t *>(in_addr);
        _ud_verify();
        // create fake block handle to search for.
        this_allocator_block_handle_t handle;
        handle.initialize(nullptr, nullptr, addr);
        // note we find the ub, then move backwards one.
        // guarenteed that moving backwards one works because uniqueness.
        auto ub = ::std::upper_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
        // if we are already at beginning though, we can't do that.
        if (ub == m_blocks.begin())
          return nullptr;
        ub--;
        assert(ub->m_block->begin() <= addr);
        // check that the previous block does not end before the address.
        // this could happen if the block the address belonged to was destroyed.
        if (ub->m_block->end() <= addr)
          return nullptr;
        _ud_verify();
        // get address stored in iterator.
        return &*ub;
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::to_global_allocator_block(allocator_block_type &&block)
      {
        assert(block.valid());
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        // if there is not already memory for global blocks, reserve a bunch.
        // this is done because moving them is non-trivial.
        if (!m_global_blocks.capacity())
          m_global_blocks.reserve(20000);
        // grab the old block address.
        auto old_block_addr = &block;
        // move the block into global blocks.
        m_global_blocks.emplace_back(std::move(block));
        // move the registration for the block.
        _u_move_registered_block(old_block_addr, &m_global_blocks.back());
        _ud_verify();
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::release_memory(const memory_pair_t &pair)
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        _u_release_memory(pair);
      }

      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::_u_release_memory(const memory_pair_t &pair)
      {
        _ud_verify();
        // if the interval is at the end of the currently used part of slab, just move slab pointer.
        if (pair.second == m_current_end) {
          m_current_end = pair.first;
          assert(m_current_end <= m_slab.end());
          return;
        }
        else {
          auto ub = ::std::upper_bound(m_free_list.begin(), m_free_list.end(), pair, system_memory_range_t::size_comparator());
          m_free_list.insert(ub, pair);
        }
        _ud_verify();
      }
      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::in_free_list(const memory_pair_t &pair) const noexcept -> bool
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        // first check to see if it is past end of used slab.
        if (_u_current_end() <= pair.first && pair.second <= _u_end())
          return true;
        // otherwise check to see if it is in some interval in the free list.
        for (auto &&fpair : m_free_list) {
          if (fpair.contains(pair))
            return true;
        }
        return false;
      }
      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::free_list_length() const noexcept -> size_t
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        return m_free_list.size();
      }
      template <typename Allocator_Policy>
      inline uint8_t *allocator_t<Allocator_Policy>::begin() const
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        return m_slab.begin();
      }
      template <typename Allocator_Policy>
      inline auto allocator_t<Allocator_Policy>::_u_begin() const noexcept -> uint8_t *
      {
        return m_slab.begin();
      }
      template <typename Allocator_Policy>
      inline auto allocator_t<Allocator_Policy>::_u_end() const noexcept -> uint8_t *
      {
        return m_slab.end();
      }
      template <typename Allocator_Policy>
      inline uint8_t *allocator_t<Allocator_Policy>::_u_current_end() const
      {
        return m_current_end;
      }
      template <typename Allocator_Policy>
      inline uint8_t *allocator_t<Allocator_Policy>::end() const
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        return m_slab.end();
      }
      template <typename Allocator_Policy>
      inline uint8_t *allocator_t<Allocator_Policy>::current_end() const
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        return m_current_end;
      }
      template <typename Allocator_Policy>
      inline auto allocator_t<Allocator_Policy>::size() const noexcept -> size_t
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        return _u_size();
      }
      template <typename Allocator_Policy>
      inline auto allocator_t<Allocator_Policy>::current_size() const noexcept -> size_t
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        return _u_current_size();
      }
      template <typename Allocator_Policy>
      inline auto allocator_t<Allocator_Policy>::_u_size() const noexcept -> size_t
      {
        return m_slab.size();
      }
      template <typename Allocator_Policy>
      inline auto allocator_t<Allocator_Policy>::_u_current_size() const noexcept -> size_t
      {
        return static_cast<size_t>(_u_current_end() - _u_begin());
      }

      template <typename Allocator_Policy>
      inline void allocator_t<Allocator_Policy>::collapse()
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        _ud_verify();
        ::std::sort(m_free_list.begin(), m_free_list.end());
        for (auto it = m_free_list.rbegin(), end = m_free_list.rend(); it != end; ++it) {
          auto prev = it + 1;
          if (prev != end) {
            if (prev->end() == it->begin()) {
              prev->set_end(it->end());
              m_free_list.erase(it.base() - 1);
            }
          }
        }
        if (!m_free_list.empty()) {
          if (m_free_list.back().end() == m_current_end) {
            m_current_end = m_free_list.back().begin();
            m_free_list.pop_back();
          }
        }
        ::std::sort(m_free_list.begin(), m_free_list.end(), system_memory_range_t::size_comparator());
        _ud_verify();
      }
      template <typename Allocator_Policy>
      inline auto allocator_t<Allocator_Policy>::_d_free_list() const -> memory_pair_vector_t
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        return _ud_free_list();
      }
      template <typename Allocator_Policy>
      inline auto allocator_t<Allocator_Policy>::_ud_free_list() const -> const memory_pair_vector_t &
      {
        return m_free_list;
      }
      template <typename Allocator_Policy>
      inline auto allocator_t<Allocator_Policy>::initialize_thread() -> this_thread_allocator_t &
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        _ud_verify();
        // first check to see if there is already a thread allocator for this thread.
        auto it = m_thread_allocators.find(::std::this_thread::get_id());
        if (it != m_thread_allocators.end())
          return *it->second;
        // one doesn't already exist.
        // create a thread allocator.
        thread_allocator_unique_ptr_t ta = make_unique_allocator<this_thread_allocator_t, allocator>(*this);
        // get a reference to thread allocator.
        auto &ret = *ta.get();
        // put the thread allocator in the thread allocator list.
        m_thread_allocators.emplace(::std::this_thread::get_id(), ::std::move(ta));
        return ret;
      }
      template <typename Allocator_Policy>
      inline void allocator_t<Allocator_Policy>::destroy_thread()
      {
        // this is outside of scope so that the lock is not held when it is destroyed.
        thread_allocator_unique_ptr_t ptr;
        {
          MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
          _ud_verify();
          // find ta entry.
          auto it = m_thread_allocators.find(::std::this_thread::get_id());
          // check to make sure it was initialized at some point.
          if (it == m_thread_allocators.end())
            return;
          // just move the owning pointer into ptr.
          // this guarentees it will be erased when this function returns.
          ptr = std::move(it->second);
          // erase the entry in the ta list.
          m_thread_allocators.erase(it);
          _ud_verify();
        }
      }
      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::_u_blocks() -> decltype(m_blocks) &
      {
        return m_blocks;
      }
      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::_ud_global_blocks() -> global_block_vector_type &
      {
        return m_global_blocks;
      }
      template <typename Allocator_Policy>
      template <typename Container>
      void allocator_t<Allocator_Policy>::bulk_destroy_memory(Container &container)
      {
        _d_verify();
        // this doesn't really free these objects.
        // instead it just marks the state that it should be freed in the future.
        for (auto &&os : container) {
          if (os)
            os->set_quasi_freed();
        }
        // clear the container per function spec.
        container.clear();
      }
      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::thread_policy() noexcept -> allocator_thread_policy_type &
      {
        return m_thread_policy;
      }
      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::thread_policy() const noexcept -> const allocator_thread_policy_type &
      {
        return m_thread_policy;
      }

      template <typename Allocator_Policy>
      size_t allocator_t<Allocator_Policy>::num_global_blocks()
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        return _u_num_global_blocks();
      }
      template <typename Allocator_Policy>
      size_t allocator_t<Allocator_Policy>::_u_num_global_blocks()
      {
        return m_global_blocks.size();
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::collect()
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        _u_collect();
      }

      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::_u_collect()
      {
        // go through each block globally owned
        for (auto block_it = m_global_blocks.rbegin(); block_it != m_global_blocks.rend(); ++block_it) {
          auto &&block = *block_it;
          // collect it
          size_t num_quasifreed = 0;
          block.collect(num_quasifreed);
          // if after collection it is empty, destroy it.
          if (block.empty()) {
            _u_destroy_global_allocator_block(::std::move(block));
            auto forward_it = (block_it + 1).base();
            assert(&*forward_it == &*block_it);
            auto moved_begin = m_global_blocks.erase(forward_it);
            _u_move_registered_blocks(moved_begin, m_global_blocks.end(),
                                      -static_cast<::std::ptrdiff_t>(sizeof(typename global_block_vector_type::value_type)));
          }
        }
        for (auto block_it = m_global_blocks.rbegin(); block_it != m_global_blocks.rend(); ++block_it) {
          auto &&block = *block_it;
          assert(!block.empty());
          (void)block;
        }
      }

      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::_u_find_global_allocator_block(size_t sz,
                                                                         size_t minimum_alloc_length,
                                                                         size_t maximum_alloc_length) ->
          typename global_block_vector_type::iterator
      {
        // sanity check min and max lengths.
        minimum_alloc_length = object_state_type::needed_size(sizeof(object_state_type), minimum_alloc_length);
        maximum_alloc_length = object_state_type::needed_size(sizeof(object_state_type), maximum_alloc_length);
        // run find.
        auto it = ::std::find_if(m_global_blocks.begin(), m_global_blocks.end(),
                                 [this, sz, minimum_alloc_length, maximum_alloc_length](allocator_block_type &block) {
                                   return block.minimum_allocation_length() == minimum_alloc_length &&
                                          block.maximum_allocation_length() == maximum_alloc_length &&
                                          block.max_alloc_available() >= sz;
                                 });
        return it;
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::to_ptree(::boost::property_tree::ptree &ptree, int level) const
      {
        MCPPALLOC_CONCURRENCY_LOCK_GUARD(m_mutex);
        ptree.put("name", typeid(*this).name());
        {
          ::boost::property_tree::ptree slab;
          slab.put("size", ::std::to_string(m_slab.size()));
          ptree.put_child("slab", slab);
        }
        ptree.put("size", ::std::to_string(_u_size()));
        ptree.put("current_size", ::std::to_string(_u_current_size()));
        ptree.put("num_blocks", ::std::to_string(m_blocks.size()));
        ptree.put("num_global_blocks", ::std::to_string(m_global_blocks.size()));
        ptree.put("num_thread_allocators", ::std::to_string(m_thread_allocators.size()));
        ptree.put("free_list_size", ::std::to_string(m_free_list.size()));
        ptree.put("initial_heap_size", ::std::to_string(m_initial_gc_heap_size));
        ptree.put("minimum_expansion_size", ::std::to_string(m_minimum_expansion_size));
        ptree.put("maximum_heap_size", ::std::to_string(max_heap_size()));
        ::boost::property_tree::ptree thread_ptree;
        for (auto &&thread_pair : m_thread_allocators) {
          ::boost::property_tree::ptree local_ptree;
          auto &&thread = thread_pair.second;
          thread->to_ptree(local_ptree, level);
          thread_ptree.add_child("thread_allocator", local_ptree);
        }
        ptree.put_child("thread_allocators", thread_ptree);
      }
      template <typename Allocator_Policy>
      void allocator_t<Allocator_Policy>::_u_set_force_free_empty_blocks()
      {
        for (auto &&pair : m_thread_allocators)
          pair.second->set_force_free_empty_blocks();
      }
      template <typename Allocator_Policy>
      auto allocator_t<Allocator_Policy>::max_heap_size() const noexcept -> size_type
      {
        return m_maximum_heap_size;
      }
    }
  }
}
