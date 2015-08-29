#pragma once
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
#error "IDL must be zero"
#endif
#include <iostream>
namespace cgc1
{
  namespace details
  {
    /**
     * \brief Return 2^n.
    **/
    inline constexpr size_t pow2(int n)
    {
      return static_cast<size_t>(2) << (n - 1);
    }
    template <typename Allocator, typename Traits>
    inline allocator_t<Allocator, Traits>::allocator_t()
        : m_shutdown(false)
    {
      // notify traits that the allocator was created.
      m_traits.on_creation(*this);
    }
    template <typename Allocator, typename Traits>
    allocator_t<Allocator, Traits>::~allocator_t()
    {
      // tell the world the destructor has been called.
      m_shutdown = true;
      _ud_verify();
      // first shutdown all thread allocators.
      m_thread_allocators.clear();
      // then get rid of all block handles.
      m_blocks.clear();
      // then get rid of any lingering blocks.
      m_global_blocks.clear();
    }
    template <typename Allocator, typename Traits>
    inline bool allocator_t<Allocator, Traits>::is_shutdown() const
    {
      return m_shutdown;
    }
    template <typename Allocator, typename Traits>
    bool allocator_t<Allocator, Traits>::initialize(size_t initial_gc_heap_size, size_t suggested_max_heap_size)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      // sanity check heap size.
      if (m_initial_gc_heap_size)
        return false;
      m_initial_gc_heap_size = initial_gc_heap_size;
      m_minimum_expansion_size = m_initial_gc_heap_size;
      // try to allocate at a location that has room for expansion.
      if (!m_slab.allocate(m_initial_gc_heap_size, slab_t::find_hole(suggested_max_heap_size)))
        return false;
      // setup current end point (nothing used yet).
      m_current_end = m_slab.begin();
      _ud_verify();
      return true;
    }
    template <typename Allocator, typename Traits>
    auto allocator_t<Allocator, Traits>::get_memory(size_t sz) -> memory_pair_t
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return _u_get_memory(sz);
    }

    template <typename Allocator, typename Traits>
    auto allocator_t<Allocator, Traits>::_u_get_memory(size_t sz) -> memory_pair_t
    {
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
        // subdivide
        memory_pair_t ret = *worst;
        // calculate part not used.
        memory_pair_t free_pair = ::std::make_pair(worst->first + sz, worst->second);
        ret.second = free_pair.first;
        if (size(free_pair)) {
          // positive size of free pair, so just swap it into the list in the same place.
          ::std::swap(*worst, free_pair);
        } else {
          // request for all memory, just erase the interval.
          m_free_list.erase(worst);
        }
        return ret;
      }
      // no space available in free list.
      auto sz_available = m_slab.end() - m_current_end ;
      // heap out of memory.
      if (sz_available < 0) // shouldn't happen
        abort();
      if (static_cast<size_t>(sz_available) >= sz) {
        // recalculate used end.
        uint8_t *new_end = m_current_end + sz;
        // create the memory interval pair.
        auto ret = ::std::make_pair(m_current_end, new_end);
        assert(new_end < m_slab.end());
        m_current_end = new_end;
        return ret;
      }
      // we need to expand the heap.
      assert(m_current_end <= m_slab.end());
      size_t expansion_size = ::std::max(m_slab.size() + m_minimum_expansion_size, m_slab.size() + sz);
      if (!m_slab.expand(expansion_size)) {
	::std::cerr << "Unable to expand slab to " << expansion_size << ::std::endl;
        // unable to expand heap so return error condition.
        return ::std::make_pair(nullptr, nullptr);
      }
      // recalculate used end.
      uint8_t *new_end = m_current_end + sz;
      assert(new_end <= m_slab.end());
      // create the memory interval pair.
      auto ret = ::std::make_pair(m_current_end, new_end);
      m_current_end = new_end;
      _ud_verify();
      return ret;
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_u_get_unregistered_allocator_block(this_thread_allocator_t &ta,
                                                             size_t create_sz,
                                                             size_t minimum_alloc_length,
                                                             size_t maximum_alloc_length,
                                                             size_t allocate_sz,
                                                             block_type &block)
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
          return;
        }
	// otherwise just create a new block
	_u_create_allocator_block(ta, create_sz, minimum_alloc_length, maximum_alloc_length, block);
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_u_create_allocator_block(this_thread_allocator_t &ta,
                                                                        size_t sz,
                                                                        size_t minimum_alloc_length,
                                                                        size_t maximum_alloc_length,
									block_type& block)
    {
      // try to allocate memory.
      auto memory = _u_get_memory(sz);
      if (!memory.first)
	{
	  ::std::cerr << "out of memory\n";
        throw out_of_memory_exception_t();
	}
      // get actual size of memory.
      auto memory_size = size(memory);
      assert(memory_size > 0);
      // create block.
      //      block = block_type(memory.first, static_cast<size_t>(memory_size), minimum_alloc_length, maximum_alloc_length);
      block.~block_type();
      new(&block) block_type(memory.first, static_cast<size_t>(memory_size), minimum_alloc_length, maximum_alloc_length);
      // call traits function that gets called when block is created.
      m_traits.on_create_allocator_block(ta, block);
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::destroy_allocator_block(this_thread_allocator_t &ta, block_type &&block)
    {
      // notify traits that a memory block is being destroyed.
      m_traits.on_destroy_allocator_block(ta, block);
      // unregister block.
      unregister_allocator_block(block);
      // release memory now that block is unregistered.
      release_memory(std::make_pair(block.begin(), block.end()));
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_u_destroy_global_allocator_block(block_type &&block)
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
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_u_register_allocator_block(this_thread_allocator_t &ta, block_type &block)
    {
#if _CGC1_DEBUG_LEVEL>0
      _ud_verify();
      for (auto &&it : m_blocks) {
        // it is a fatal error to try to double add and something is inconsistent.  Terminate before memory corruption spreads.
        if (it.m_block == &block)
          abort();
        // it is a fatal error to try to double add and something is inconsistent.  Terminate before memory corruption spreads.
        if (it.m_begin == block.begin())
	  {
	    ::std::cerr << __FILE__ << " " << __LINE__ << " Attempt to double register block.\n";
	    ::std::cerr << __FILE__ << " " << __LINE__ << " " << &block << " " << reinterpret_cast<void*>(block.begin()) << ::std::endl;
	    abort();
	  }
      }
#endif
      // create a fake handle to search for.
      this_allocator_block_handle_t handle; handle.initialize(&ta, &block, block.begin());
      // find the place to insert the block.
      auto ub = ::std::upper_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
      // emplace a block handle.
      auto it = m_blocks.emplace(ub);
      it->initialize(&ta, &block, block.begin());
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::unregister_allocator_block(block_type &block)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      // forward unregistration.
      _u_unregister_allocator_block(block);
    }

    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_u_unregister_allocator_block(block_type &block)
    {
      _ud_verify();
      // create a fake handle to search for.
      this_allocator_block_handle_t handle; handle.initialize(nullptr, &block, block.begin());
      // uniqueness guarentees lb is handle if found.
      auto lb = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
      if (lb != m_blocks.end() && lb->m_begin == block.begin()) {
        // erase if found.
        m_blocks.erase(lb);
      } else {
        // This should never happen, so memory corruption issue if it has, so kill the program.
        ::std::cerr << __FILE__ << " " << __LINE__ << "Unable to find allocator block to unregister\n" << ::std::endl;
        abort();
      }
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_ud_verify()
    {
#if _CGC1_DEBUG_LEVEL > 1
      // this is really expensive, but verify that blocks are sorted.
      assert(m_blocks.empty() || ::std::is_sorted(m_blocks.begin(), m_blocks.end(), block_handle_begin_compare_t{}));
#endif
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_d_verify()
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      // forward verify request.
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    template <typename Iterator>
    void allocator_t<Allocator, Traits>::_u_move_registered_blocks(const Iterator &begin, const Iterator & end, const ptrdiff_t offset)
    {

      if(begin!=end)
	{
	  block_type*  new_block = &*begin;
	  block_type * old_block = reinterpret_cast<block_type *>(reinterpret_cast<uint8_t *>(new_block) - offset);
      
	  // create fake handle to search for.
	  this_allocator_block_handle_t handle;
	  handle.initialize(nullptr, old_block, new_block->begin());
	  auto lb = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
	  assert(lb!=m_blocks.end());
	  assert(lb->m_begin==new_block->begin());
	  size_t i = 0;
	  size_t sz = static_cast<size_t>(end-begin);
	  size_t contig_start = 0;
	  size_t contiguous = 0;
	  //first we must locate a section of contiuous blocks.
	    do
	    {
	      auto& block_handle =* (lb+static_cast<difference_type>(i));
	      auto next_old_block = reinterpret_cast<block_type*>(reinterpret_cast<uint8_t*>(old_block)+i*sizeof(block_type));
	      if(block_handle.m_block==next_old_block)
		{
		  //another contiguous block found.
		  contiguous++;
		}
	      else
		{
		  //if here, next block is not contiguous so move first.
		  //move first
		  _u_move_registered_blocks_contiguous(contiguous, begin+static_cast<ptrdiff_t>(contig_start), lb);
		  //then update search.
		  new_block = &*(begin+static_cast<ptrdiff_t>(i));
		  old_block = reinterpret_cast<block_type *>(reinterpret_cast<uint8_t *>(new_block) - offset);
		  handle.initialize(nullptr, old_block, new_block->begin());
		  lb = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
		  contiguous = 1;
		  contig_start = i;
		}
	      ++i;
	    } while(i < sz);
	    //move last set of contiguous blocks.
	    _u_move_registered_blocks_contiguous(contiguous, begin+static_cast<ptrdiff_t>(contig_start), lb);
	}
      _ud_verify();
    }


    template <typename Allocator, typename Traits>
    template <typename Iterator, typename LB>
    void allocator_t<Allocator, Traits>::_u_move_registered_blocks_contiguous(size_t  contiguous, const Iterator& new_location, const LB& lb)
    {


      
      this_allocator_block_handle_t new_handle;
      auto adj_location = new_location+static_cast<ptrdiff_t>(contiguous-1);
      new_handle.initialize(lb->m_thread_allocator, &*adj_location, adj_location->begin());


      
      // uniqueness guarentees this is correct insertion point.
      auto ub = ::std::upper_bound(m_blocks.begin(), m_blocks.end(), new_handle, block_handle_begin_compare_t{});
      // while block handle is not default constructable, we can move away from it.
      // thus we can use rotate to create an empty location to modify.
      // this is the optimal solution for moving in this fashion.
      if (likely(ub > lb)) {
	::std::rotate(lb, lb + static_cast<ptrdiff_t>(contiguous), ub);
	for(size_t i = contiguous; i >0; --i)
	{
	  auto ub_offset = static_cast<ptrdiff_t>(i);
	  auto new_location_offset = static_cast<ptrdiff_t>(contiguous-i);
	  (ub -  ub_offset)->m_block = &*(new_location+new_location_offset);
	}
      } else {
	::std::cerr << "During move, UB <=lb";
	abort();
      }
      _ud_verify();

    }

    
    template <typename Allocator, typename Traits>
    template <typename Container>
    void allocator_t<Allocator, Traits>::_u_move_registered_blocks(Container &blocks, ptrdiff_t offset)
    {      
      _u_move_registered_blocks(blocks.begin(), blocks.end(), offset);
    }
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_u_move_registered_block(block_type *old_block, block_type *new_block)
    {
      // create fake handle to search for.
      this_allocator_block_handle_t handle;
      handle.initialize(nullptr, old_block, new_block->begin());
      auto lb = ::std::lower_bound(m_blocks.begin(), m_blocks.end(), handle, block_handle_begin_compare_t{});
      // uniqueness guarentees lb is old block if it exists.
      if (lb != m_blocks.end()) {
        // sanity check uniqueness.
        if (lb->m_block == old_block) {
	  _u_move_registered_blocks_contiguous(1,new_block,lb);
        } else {
          // Uniqueness of block failed.
          ::std::cerr << "CGC1: Unable to find block to move\n";
          ::std::cerr << old_block << " " << new_block << ::std::endl;
          // This should never happen, so memory corruption issue if it has, so kill the program.
          abort();
        }
      } else {
        // couldn't find old block
        ::std::cerr << "CGC1: Unable to find block to move, lb is end\n";
        // This should never happen, so memory corruption issue if it has, so kill the program.
        abort();
      }
    }
    template <typename Allocator, typename Traits>
    auto allocator_t<Allocator, Traits>::_u_find_block(void *in_addr) -> const this_allocator_block_handle_t *
    {
      uint8_t *addr = reinterpret_cast<uint8_t *>(in_addr);
      _ud_verify();
      // create fake block handle to search for.
      this_allocator_block_handle_t handle; handle.initialize(nullptr, nullptr, addr);
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
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::to_global_allocator_block(block_type &&block)
    {
      assert(block.valid());
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
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
    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::release_memory(const memory_pair_t &pair)
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      _u_release_memory(pair);
    }

    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_u_release_memory(const memory_pair_t &pair)
    {
      _ud_verify();
      // if the interval is at the end of the currently used part of slab, just move slab pointer.
      if (pair.second == m_current_end) {
        m_current_end = pair.first;
        assert(m_current_end <= m_slab.end());
        return;
      } else {
        // otherwise add it to free list.
        m_free_list.push_back(pair);
      }
      _ud_verify();
    }
    template <typename Allocator, typename Traits>
    auto allocator_t<Allocator, Traits>::in_free_list(const memory_pair_t &pair) const noexcept -> bool
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      // first check to see if it is past end of used slab.
      if (_u_current_end() <= pair.first && pair.second <= _u_end())
        return true;
      // otherwise check to see if it is in some interval in the free list.
      for (auto &&fpair : m_free_list) {
        if (fpair.first <= pair.first && pair.second <= fpair.second)
          return true;
      }
      return false;
    }
    template <typename Allocator, typename Traits>
    auto allocator_t<Allocator, Traits>::free_list_length() const noexcept -> size_t
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return m_free_list.size();
    }
    template <typename Allocator, typename Traits>
    inline uint8_t *allocator_t<Allocator, Traits>::begin() const
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      return m_slab.begin();
    }
    template <typename Allocator, typename Traits>
    inline auto allocator_t<Allocator, Traits>::_u_begin() const noexcept -> uint8_t *
    {
      return m_slab.begin();
    }
    template <typename Allocator, typename Traits>
    inline auto allocator_t<Allocator, Traits>::_u_end() const noexcept -> uint8_t *
    {
      return m_slab.end();
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
      // this is just a simple O(n^2) operation to coallese all adjacent free segments.
      // should fit in cache, so should be fast.
      for (auto it = m_free_list.begin(); it < end; ++it) {
        for (auto it2 = it; it2 < end; ++it2) {
          if (it->first == it2->second) {
            // it->second stays same
            it->first = it2->first;
            // put it2(invalid) to end, so we can remove it at end.
            ::std::swap(*--end, *it2);
          }
          if (it->second == it2->first) {
            // it->first stays same
            it->second = it2->second;
            // put it2 to end, so we can remove it at end.
            ::std::swap(*--end, *it2);
          }
          if (it2->second == m_current_end) {
            // case where we can just move the slab pointer.
            m_current_end = it2->first;
            // put it2 to end, so we can remove it at end.
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
      // first check to see if there is already a thread allocator for this thread.
      auto it = m_thread_allocators.find(::std::this_thread::get_id());
      if (it != m_thread_allocators.end())
        return *it->second;
      _ud_verify();
      // one doesn't already exist.
      // create a thread allocator.
      thread_allocator_unique_ptr_t ta = make_unique_allocator<this_thread_allocator_t, allocator>(*this);
      _ud_verify();
      // get a reference to thread allocator.
      auto &ret = *ta.get();
      // put the thread allocator in the thread allocator list.
      m_thread_allocators.emplace(::std::this_thread::get_id(), ::std::move(ta));
      _ud_verify();
      return ret;
    }
    template <typename Allocator, typename Traits>
    inline void allocator_t<Allocator, Traits>::destroy_thread()
    {
      // this is outside of scope so that the lock is not held when it is destroyed.
      thread_allocator_unique_ptr_t ptr;
      {
        CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
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
    template <typename Allocator, typename Traits>
    auto allocator_t<Allocator, Traits>::_u_blocks() -> decltype(m_blocks) &
    {
      return m_blocks;
    }
    template <typename Allocator, typename Traits>
    auto allocator_t<Allocator, Traits>::_ud_global_blocks() -> global_block_vector_type &
    {
      return m_global_blocks;
    }
    template <typename Allocator, typename Traits>
    template <typename Container>
    void allocator_t<Allocator, Traits>::bulk_destroy_memory(Container &container)
    {
      _d_verify();
      // this doesn't really free these objects.
      // instead it just marks the state that it should be freed in the future.
      for (object_state_t *os : container) {
        if (os)
          os->set_quasi_freed();
      }
      // clear the container per function spec.
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
    void allocator_t<Allocator, Traits>::collect()
    {
      CGC1_CONCURRENCY_LOCK_GUARD(m_mutex);
      _u_collect();
    }

    template <typename Allocator, typename Traits>
    void allocator_t<Allocator, Traits>::_u_collect()
    {
      // go through each block globally owned
      for (auto block_it = m_global_blocks.rbegin(); block_it != m_global_blocks.rend(); ++block_it) {
        auto &&block = *block_it;
        // collect it
        block.collect();
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

    template <typename Allocator, typename Traits>
    auto allocator_t<Allocator, Traits>::_u_find_global_allocator_block(size_t sz,
                                                                        size_t minimum_alloc_length,
                                                                        size_t maximum_alloc_length) ->
        typename global_block_vector_type::iterator
    {
      // sanity check min and max lengths.
      minimum_alloc_length = object_state_t::needed_size(sizeof(object_state_t), minimum_alloc_length);
      maximum_alloc_length = object_state_t::needed_size(sizeof(object_state_t), maximum_alloc_length);
      // run find.
      auto it = ::std::find_if(m_global_blocks.begin(), m_global_blocks.end(), [this, sz, minimum_alloc_length,
                                                                                maximum_alloc_length](block_type &block) {
        return block.minimum_allocation_length() == minimum_alloc_length &&
               block.maximum_allocation_length() == maximum_alloc_length && block.max_alloc_available() >= sz;
      });
      return it;
    }
  }
}
