#pragma once
namespace cgc1
{
  namespace details
  {
    inline ::std::ptrdiff_t size(const ::std::pair<uint8_t *, uint8_t *> &pair)
    {
      return pair.second - pair.first;
    }
    inline size_t size_pos(const ::std::pair<uint8_t *, uint8_t *> &pair)
    {
      auto ret = pair.second - pair.first;
      assert(ret >= 0);
      return static_cast<size_t>(ret);
    }
    template <typename T>
    void print_memory_pair(T &os, const ::std::pair<uint8_t *, uint8_t *> &pair)
    {
      os << "(" << static_cast<void *>(pair.first) << " " << static_cast<void *>(pair.second) << ")";
    }
    inline ::std::string to_string(const ::std::pair<uint8_t *, uint8_t *> &pair)
    {
      ::std::stringstream ss;
      print_memory_pair(ss, pair);
      return ss.str();
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::thread_allocator_t(global_allocator &allocator)
        : m_allocator(allocator)
    {
      fill_multiples_with_default_values();
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::~thread_allocator_t()
    {
      // set minimum local blocks to 0 so all free blooks go to global.
      set_minimum_local_blocks(0);
      // debug mode verify.
      for (auto &abs : m_allocators)
        abs._verify();
      m_allocator._d_verify();
      // free empty blocks.
      free_empty_blocks(0, true);
      // blocks left need to be moved to global.
      for (auto &abs : m_allocators) {
        for (auto &&block : abs.m_blocks) {
          if (block.valid()) {
            assert(!block.empty());
            m_allocator.to_global_allocator_block(std::move(block));
          }
        }
      }
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    void thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::free_empty_blocks(size_t min_to_leave, bool force)
    {
      m_allocator._d_verify();
      for (auto &abs : m_allocators) {
        // if num destroyed > threshold, try to free blocks.
        if (force || abs.num_destroyed_since_last_free() > destroy_threshold()) {
          abs.free_empty_blocks(
              [this](typename this_allocator_block_set_t::allocator_block_type &&block) {
                m_allocator.destroy_allocator_block(*this, ::std::move(block));
              },
              [this]() {
                m_allocator._mutex().lock();
                assume_unlock(m_allocator._mutex());
              },
              [this]() {
                CGC1_CONCURRENCY_LOCK_ASSUME(m_allocator._mutex());
                m_allocator._mutex().unlock();
              },
              [this](auto begin, auto end, auto offset) {
                CGC1_CONCURRENCY_LOCK_ASSUME(m_allocator._mutex());
                m_allocator._u_move_registered_blocks(begin, end, offset);
              },
              min_to_leave);
        }
      }
    }

    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    size_t thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::find_block_set_id(size_t sz)
    {
      if (sz <= 16)
        return 0;
      sz = sz >> 4;
      // This is guarenteed to be positive.
      size_t id = static_cast<size_t>(64 - cgc1_builtin_clz1(sz));
      // cap num bins.
      id = ::std::min(id, c_bins - 1);
      return id;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    void thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::fill_multiples_with_default_values()
    {
      // we want minimums to be page size compatible.
      size_t min_size = slab_t::page_size();
      if (min_size <= 4096 * 4)
        min_size = 4096 * 4;
      if (min_size > 4096 * 128)
        min_size = 4096 * 128;
      for (size_t i = 0; i < c_bins; ++i) {
        auto &abs_data = m_allocator_multiples[i];
        // guarentee multiple is positive integer.
        size_t allocator_multiple = ::std::max(static_cast<size_t>(1), min_size / static_cast<unsigned>(2 << (i + 3)));
        if (allocator_multiple > ::std::numeric_limits<uint32_t>().max()) {
          ::std::cerr << "Allocator multiple too large\n";
          abort();
        }
	//magic numbers (default multiples seem small emperically)
	allocator_multiple*=2;
        abs_data.set_allocator_multiple(static_cast<uint32_t>(allocator_multiple));
        // use a nice default number of blocks before recycling.
        abs_data.set_max_blocks_before_recycle(5);
        // based on the numbers we use for multiple, generate min and max allocation sizes.
        size_t min = static_cast<size_t>(1) << (i + 3);
        size_t max = (static_cast<size_t>(1) << (i + 4)) - 1;
        if (i == c_bins - 1)
          max = c_infinite_length;
        m_allocators[i]._set_allocator_sizes(min, max);
      }
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    bool thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::set_allocator_multiple(size_t id, size_t multiple)
    {
      // sanity check id.
      if (id > c_bins)
        return false;
      // sanity check multiple.
      if (multiple > ::std::numeric_limits<uint32_t>().max())
        return false;
      m_allocator_multiples[id].set_allocator_multiple(static_cast<uint32_t>(multiple));
      return true;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    size_t thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::get_allocator_multiple(size_t id) noexcept
    {
      // sanity check id.
      if (id > c_bins)
        return 0;
      // return multiple.
      return m_allocator_multiples[id].allocator_multiple();
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    size_t thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::get_allocator_block_size(size_t id) const noexcept
    {
      return m_allocator_multiples[id].allocator_multiple() * static_cast<unsigned>(2 << (id + 4));
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    auto thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::allocator_by_size(size_t sz) noexcept
        -> this_allocator_block_set_t &
    {
      return m_allocators[find_block_set_id(sz)];
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    auto thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::allocator_block_sizes() const noexcept
        -> ::std::array<size_t, c_bins>
    {
      // build an array in place, this should get optimized away.
      ::std::array<size_t, c_bins> array;
      for (size_t id = 0; id < c_bins; ++id)
        array[id] = get_allocator_block_size(id);
      return array;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    bool thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::destroy(void *v)
    {
      // get object state
      object_state_t *os = object_state_t::from_object_start(v);
      // find block set id for object.
      auto block_id = find_block_set_id(os->object_size());
      // get a reference to the allocator.
      auto allocator = &m_allocators[block_id];
      // destroy object.
      auto ret = allocator->destroy(v);
      // handle allocator rounded size up.
      if (!ret && block_id > 0) {
        allocator = &m_allocators[block_id - 1];
        ret = allocator->destroy(v);
      }
      // do book keeping for returning memory to global.
      // do this if we exceed the destroy threshold or externally forced.
      if (m_force_free_empty_blocks.load(::std::memory_order_relaxed) ||
          allocator->num_destroyed_since_last_free() > destroy_threshold()) {
        // ok, this needs to be tweaked.
        free_empty_blocks(m_minimum_local_blocks, false);
      }
      return ret;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    auto thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::allocator_multiples() const
        -> const ::std::array<size_t, c_bins> &
    {
      return m_allocator_multiples;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    auto thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::allocators() const
        -> const ::std::array<this_allocator_block_set_t, c_bins> &
    {
      return m_allocators;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    void *thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::allocate(size_t sz)
    {
      // find allocation set for allocation size.
      size_t id = find_block_set_id(sz);
      // try allocation.
      void *ret = m_allocators[id].allocate(sz);
      // if successful returned.
      if (ret)
        return ret;
      // if not succesful, that allocator needs more memory.
      // figre out how much memory to request.
      size_t memory_request = get_allocator_block_size(id);
      if (memory_request < sz * 3)
        memory_request = sz * 3;
      try {
        // Get the allocator for the size requested.
        auto &abs = m_allocators[id];
	CGC1_CONCURRENCY_LOCK_GUARD(m_allocator._mutex());
        // see if safe to add a block
        if (!abs.add_block_is_safe()) {
          // if not safe to move a block, expand that allocator block set.
	  //          m_allocator._mutex().lock();
          m_allocator._ud_verify();
          abs._verify();
          ptrdiff_t offset = static_cast<ptrdiff_t>(abs.grow_blocks());
          abs._verify();
          m_allocator._u_move_registered_blocks(abs.m_blocks, offset);
	  //          m_allocator._mutex().unlock();
        }
        typename global_allocator::block_type block;
	// fill the empty block.
	m_allocator._u_get_unregistered_allocator_block(*this, memory_request, m_allocators[id].allocator_min_size(),
							m_allocators[id].allocator_max_size(), sz, block);

	// gcreate and grab the empty block.
	auto &inserted_block_ref = abs.add_block(::std::move(block), [this]() {}, [this]() {},
						 [this](auto begin, auto end, auto offset) {
						   CGC1_CONCURRENCY_LOCK_ASSUME(m_allocator._mutex());
						   m_allocator._u_move_registered_blocks(begin, end, offset);
						 });
	m_allocator._u_register_allocator_block(*this, inserted_block_ref);
      } catch (out_of_memory_exception_t) {
        abort();
        // we aren't going to try to handle out of memory at this point.
      }
      ret = m_allocators[id].allocate(sz);
      if (unlikely(!ret)) // should be impossible.
        abort();
      return ret;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    auto thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::destroy_threshold() const noexcept
        -> destroy_threshold_type
    {
      return static_cast<destroy_threshold_type>(m_destroy_threshold << 3);
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    auto thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::minimum_local_blocks() const noexcept -> uint16_t
    {
      return m_minimum_local_blocks;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    void
    thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::set_destroy_threshold(destroy_threshold_type threshold)
    {
      m_destroy_threshold = static_cast<uint16_t>(threshold >> 3);
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    void thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::set_minimum_local_blocks(uint16_t minimum)
    {
      m_minimum_local_blocks = minimum;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    void thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::set_force_free_empty_blocks() noexcept
    {
      m_force_free_empty_blocks = true;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    void thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::_do_maintenance()
    {
      for (auto &allocator : m_allocators) {
        allocator._do_maintenance();
      }
    }
    template <typename charT, typename Traits, typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    ::std::basic_ostream<charT, Traits> &operator<<(::std::basic_ostream<charT, Traits> &os,
                                                    const thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits> &ta)
    {
      os << "thread_allocator_t = (" << &ta << "\n";
      os << "multiples = { ";
      for (auto multiple : ta.allocator_multiples())
        os << multiple << ",";
      os << " }\n";
      os << "block sz  = { ";
      for (auto sz : ta.allocator_block_sizes())
        os << sz << ",";
      os << " }\n";
      os << ")";
      return os;
    }
  }
}
