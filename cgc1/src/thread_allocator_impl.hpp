#pragma once
namespace cgc1
{
  namespace details
  {
    ::std::ptrdiff_t size(const ::std::pair<uint8_t *, uint8_t *> &pair)
    {
      return pair.second - pair.first;
    }
    size_t size_pos(const ::std::pair<uint8_t *, uint8_t *> &pair)
    {
      auto ret = pair.second - pair.first;
      assert(ret >= 0);
      return static_cast<size_t>(ret);
    }
    template <typename T>
    void print_memory_pair(T &os, const ::std::pair<uint8_t *, uint8_t *> &pair)
    {
      os << "(" << (void *)pair.first << " " << (void *)pair.second << ")";
    }
    ::std::string to_string(const ::std::pair<uint8_t *, uint8_t *> &pair)
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
      for (auto &abs : m_allocators)
        abs._verify();
      m_allocator._d_verify();
      free_empty_blocks();
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
    void thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::free_empty_blocks()
    {
      m_allocator._d_verify();
      rebind_vector_t<allocator_block_t<Allocator> *, Allocator> empty_blocks;
      for (auto &abs : m_allocators) {
        for (auto &&block : abs.m_blocks) {
          m_allocator._d_verify();
          block.collect();
          m_allocator._d_verify();
          if (block.empty())
            m_allocator.destroy_allocator_block(*this, std::move(block));
          m_allocator._d_verify();
        }
      }
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    size_t thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::find_block_set_id(size_t sz)
    {
      if (sz <= 16)
        return 0;
      sz = sz >> 4;
      size_t id = 64 - cgc1_builtin_clz1(sz);
      if (id > c_bins - 1)
        id = c_bins - 1;
      return id;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    void thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::fill_multiples_with_default_values()
    {
      size_t min_size = slab_t::page_size();
      if (min_size <= 4096 * 4)
        min_size *= 4;
      if (min_size > 4096 * 128)
        min_size = 4096 * 128;
      for (size_t i = 0; i < c_bins; ++i) {
        m_allocator_multiples[i] = ::std::max((size_t)1, min_size / (2 << (i + 4)));
        size_t min = ((size_t)1) << (i + 3);
        size_t max = (((size_t)1) << (i + 4)) - 1;
        m_allocators[i]._set_allocator_sizes(min, max);
      }
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    bool thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::set_allocator_multiple(size_t id, size_t multiple)
    {
      if (id > c_bins)
        return false;
      m_allocator_multiples[id] = multiple;
      return true;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    size_t thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::get_allocator_multiple(size_t id)
    {
      if (id > c_bins)
        return 0;
      return m_allocator_multiples[id];
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    size_t thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::get_allocator_block_size(size_t id) const
    {
      return m_allocator_multiples[id] * (2 << (id + 4));
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    auto thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::allocator_by_size(size_t sz)
        -> this_allocator_block_set_t &
    {
      return m_allocators[find_block_set_id(sz)];
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    auto thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::allocator_block_sizes() const
        -> ::std::array<size_t, c_bins>
    {
      ::std::array<size_t, c_bins> array;
      for (size_t id = 0; id < c_bins; ++id)
        array[id] = get_allocator_block_size(id);
      return array;
    }
    template <typename Global_Allocator, typename Allocator, typename Allocator_Traits>
    bool thread_allocator_t<Global_Allocator, Allocator, Allocator_Traits>::destroy(void *v)
    {
      object_state_t *os = object_state_t::from_object_start(v);
      return m_allocators[find_block_set_id(os->object_size())].destroy(v);
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
      size_t id = find_block_set_id(sz);
      void *ret = m_allocators[id].allocate(sz);
      if (ret)
        return ret;
      size_t memory_request = m_allocator_multiples[id] * (2 << (id + 4));
      try {
        auto block = m_allocator.create_allocator_block(*this, memory_request, m_allocators[id].allocator_min_size());
        auto &abs = m_allocators[id];
        if (!abs.add_block_is_safe()) {
          m_allocator.lock();
          m_allocator._ud_verify();
          size_t offset = abs.grow_blocks();
          m_allocator._u_move_registered_blocks(abs.m_blocks, offset);
          m_allocator.unlock();
        }
        abs.add_block(::std::move(block));
        m_allocator.register_allocator_block(*this, abs.last_block());
      } catch (out_of_memory_exception_t) {
        abort();
        // we aren't going to try to handle out of memory at this point.
      }
      ret = m_allocators[id].allocate(sz);
      if (!ret) // should be impossible.
        abort();
      return ret;
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
