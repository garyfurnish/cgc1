#pragma once
#include <atomic>
#include <mcppalloc/mcppalloc_bitmap/dynamic_bitmap.hpp>
#include <mcppalloc/mcppalloc_slab_allocator/slab_allocator.hpp>
namespace mcppalloc
{
  namespace bitmap_allocator
  {
    namespace details
    {
      inline bitmap_state_t *get_state(void *v)
      {
        uintptr_t vi = reinterpret_cast<uintptr_t>(v);
        size_t mask = ::std::numeric_limits<size_t>::max() << (__builtin_ffsll(c_bitmap_block_size) - 1);

        vi &= mask;
        vi += ::mcppalloc::slab_allocator::details::slab_allocator_t::cs_header_sz;
        bitmap_state_t *state = reinterpret_cast<bitmap_state_t *>(vi);
        return state;
      }
      MCPPALLOC_OPT_ALWAYS_INLINE auto bitmap_state_t::declared_entry_size() const noexcept -> size_t
      {
        return m_internal.m_info.m_data_entry_sz;
      }
      MCPPALLOC_OPT_ALWAYS_INLINE auto bitmap_state_t::real_entry_size() const noexcept -> size_t
      {
        return cs_object_alignment > declared_entry_size() ? cs_object_alignment : declared_entry_size();
      }
      MCPPALLOC_OPT_ALWAYS_INLINE auto bitmap_state_t::header_size() const noexcept -> size_t
      {
        return m_internal.m_info.m_header_size;
      }
      inline void bitmap_state_t::initialize_consts() noexcept
      {
        assert(this);
        m_internal.m_pre_magic_number = cs_magic_number_pre;
        m_internal.m_post_magic_number = cs_magic_number_0;
      }
      inline void bitmap_state_t::initialize(type_id_t type_id) noexcept
      {
        initialize_consts();
        m_internal.m_info.m_type_id = type_id;
        for (size_t i = 0; i < num_blocks(); ++i)
          free_bits()[i].fill(::std::numeric_limits<uint64_t>::max());
      }
      inline void bitmap_state_t::clear_mark_bits() noexcept
      {
        bitmap::dynamic_bitmap_ref_t<false>(mark_bits(), num_blocks()).clear();
      }
      inline void bitmap_state_t::clear_user_bits(size_t index) noexcept
      {
        bitmap::dynamic_bitmap_ref_t<false>(user_bits(index), num_blocks()).clear();
      }
      inline auto bitmap_state_t::all_free() const noexcept -> bool
      {
        return bitmap::dynamic_bitmap_ref_t<true>(free_bits(), num_blocks()).all_set();
      }
      inline auto bitmap_state_t::num_bit_arrays() const noexcept -> size_t
      {
        return cs_bits_array_multiple + m_internal.m_info.m_num_user_bit_fields;
      }

      inline auto bitmap_state_t::any_free() const noexcept -> bool
      {
        for (size_t i = 0; i < num_blocks(); ++i) {
          auto &&it = free_bits()[i];
          if (it.any_set())
            return true;
        }
        return false;
      }
      inline auto bitmap_state_t::none_free() const noexcept -> bool
      {
        for (size_t i = 0; i < num_blocks(); ++i) {
          auto &&it = free_bits()[i];
          if (it.any_set())
            return false;
        }
        return true;
      }
      inline auto bitmap_state_t::first_free() const noexcept -> size_t
      {
        for (size_t i = 0; i < num_blocks(); ++i) {
          auto ret = free_bits()[i].first_set();
          if (ret != ::std::numeric_limits<size_t>::max())
            return i * bits_array_type::size_in_bits() + ret;
        }
        return ::std::numeric_limits<size_t>::max();
      }
      inline auto bitmap_state_t::any_marked() const noexcept -> bool
      {
        for (size_t i = 0; i < num_blocks(); ++i) {
          auto &&it = mark_bits()[i];
          if (it.any_set())
            return true;
        }
        return false;
      }
      inline auto bitmap_state_t::none_marked() const noexcept -> bool
      {
        for (size_t i = 0; i < num_blocks(); ++i) {
          auto &&it = mark_bits()[i];
          if (it.any_set())
            return false;
        }
        return true;
      }
      inline auto bitmap_state_t::free_popcount() const noexcept -> size_t
      {
        return ::std::accumulate(free_bits(), free_bits() + num_blocks(), static_cast<size_t>(0),
                                 [](size_t b, auto &&x) { return x.popcount() + b; });
      }
      inline void bitmap_state_t::set_free(size_t i, bool val) noexcept
      {
        if (mcppalloc_unlikely(i >= size())) {
          throw ::std::runtime_error("mcppalloc");
        }

        auto pos = i / bits_array_type::size_in_bits();
        auto sub_pos = i - (pos * bits_array_type::size_in_bits());
        free_bits()[pos].set_bit(sub_pos, val);
      }
      inline void bitmap_state_t::set_marked(size_t i) noexcept
      {
        if (mcppalloc_unlikely(i >= size())) {
          throw ::std::runtime_error("mcppalloc");
        }

        auto pos = i / bits_array_type::size_in_bits();
        auto sub_pos = i - (pos * bits_array_type::size_in_bits());
        mark_bits()[pos].set_bit_atomic(sub_pos, true, ::std::memory_order_relaxed);
      }
      inline auto bitmap_state_t::is_free(size_t i) const noexcept -> bool
      {
        if (mcppalloc_unlikely(i >= size())) {
          throw ::std::runtime_error("mcppalloc");
        }

        auto pos = i / bits_array_type::size_in_bits();
        auto sub_pos = i - (pos * bits_array_type::size_in_bits());
        return free_bits()[pos].get_bit(sub_pos);
      }
      inline auto bitmap_state_t::is_marked(size_t i) const noexcept -> bool
      {
        if (mcppalloc_unlikely(i >= size())) {
          throw ::std::runtime_error("mcppalloc");
        }

        auto pos = i / bits_array_type::size_in_bits();
        auto sub_pos = i - (pos * bits_array_type::size_in_bits());
        return mark_bits()[pos].get_bit(sub_pos);
      }
      inline auto bitmap_state_t::type_id() const noexcept -> type_id_t
      {
        return m_internal.m_info.m_type_id;
      }
      inline auto bitmap_state_t::size() const noexcept -> size_t
      {
        return m_internal.m_info.m_size;
      }
      inline void bitmap_state_t::_compute_size() noexcept
      {
        auto blocks = m_internal.m_info.m_num_blocks;
        auto unaligned = sizeof(*this) + sizeof(bits_array_type) * blocks * num_bit_arrays();
        m_internal.m_info.m_header_size = align(unaligned, cs_header_alignment);

        auto hdr_sz = header_size();
        auto data_sz = c_bitmap_block_size - slab_allocator::details::slab_allocator_t::cs_header_sz - hdr_sz - 400;
        // this needs to be min of stuff
        auto num_data = data_sz / (real_entry_size());
        num_data = ::std::min(num_data, m_internal.m_info.m_num_blocks * bits_array_type::size_in_bits());
        m_internal.m_info.m_size = num_data;
      }
      inline auto bitmap_state_t::size_bytes() const noexcept -> size_t
      {
        return size() * real_entry_size();
      }
      inline auto bitmap_state_t::total_size_bytes() const noexcept -> size_t
      {
        return size_bytes() + header_size();
      }
      inline auto bitmap_state_t::begin() noexcept -> uint8_t *
      {
        return reinterpret_cast<uint8_t *>(this) + header_size();
      }
      inline auto bitmap_state_t::end() noexcept -> uint8_t *
      {
        return begin() + size_bytes();
      }
      inline auto bitmap_state_t::begin() const noexcept -> const uint8_t *
      {
        return reinterpret_cast<const uint8_t *>(this) + header_size();
      }
      inline auto bitmap_state_t::end() const noexcept -> const uint8_t *
      {
        return begin() + size_bytes();
      }

      inline void *bitmap_state_t::allocate() noexcept
      {
        size_t retries = 0;
      RESTART:
        auto i = first_free();
        if (i >= size())
          return nullptr;
        // guarentee the memory address exists somewhere that is visible to gc
        volatile auto memory_address = begin() + real_entry_size() * i;
        set_free(i, false);
        // this awful code is because for a conservative gc
        // we could set free before memory_address is live.
        // this can go wrong because we could mark while it is still free.
        if (mcppalloc_unlikely(is_free(i))) {
          if (mcppalloc_likely(retries < 15)) {
            goto RESTART;
          } else
            ::std::terminate();
        }
        assert(memory_address);
        verify_magic();
        return memory_address;
      }
      inline bool bitmap_state_t::deallocate(void *vv) noexcept
      {
        auto v = reinterpret_cast<uint8_t *>(vv);
        if (v < begin() || v > end())
          return false;
        size_t byte_diff = static_cast<size_t>(v - begin());
        if (mcppalloc_unlikely(byte_diff % real_entry_size()))
          return false;
        auto i = byte_diff / real_entry_size();
        set_free(i, true);
        return true;
      }

      inline void bitmap_state_t::free_unmarked() noexcept
      {
        for (size_t i = 0; i < num_blocks(); ++i) {
          free_bits()[i] |= ~mark_bits()[i];
        }
      }
      inline auto bitmap_state_t::num_blocks() const noexcept -> size_t
      {
        return m_internal.m_info.m_num_blocks;
      }
      inline auto bitmap_state_t::free_bits() noexcept -> bits_array_type *
      {
        return unsafe_cast<bits_array_type>(this + 1);
      }
      inline auto bitmap_state_t::free_bits() const noexcept -> const bits_array_type *
      {
        return unsafe_cast<bits_array_type>(this + 1);
      }

      inline auto bitmap_state_t::mark_bits() noexcept -> bits_array_type *
      {
        return free_bits() + num_blocks();
      }
      inline auto bitmap_state_t::mark_bits() const noexcept -> const bits_array_type *
      {
        return free_bits() + num_blocks();
      }
      inline auto bitmap_state_t::user_bits(size_t i) noexcept -> bits_array_type *
      {
        return mark_bits() + num_blocks() * (i + 1);
      }
      inline auto bitmap_state_t::user_bits(size_t i) const noexcept -> const bits_array_type *
      {
        return mark_bits() + num_blocks() * (i + 1);
      }
      inline auto bitmap_state_t::user_bits_checked(size_t i) noexcept -> bits_array_type *
      {
        if (mcppalloc_unlikely(i >= m_internal.m_info.m_num_user_bit_fields))
          throw ::std::out_of_range("mcppalloc: User bits out of range: 224f26b3-d2e6-47f3-b6de-6a4194750242");
        return user_bits(i);
      }
      inline auto bitmap_state_t::user_bits_checked(size_t i) const noexcept -> const bits_array_type *
      {
        if (mcppalloc_unlikely(i >= m_internal.m_info.m_num_user_bit_fields))
          throw ::std::out_of_range("mcppalloc: User bits out of range: 24a934d1-160f-4bfc-b765-e0e21ee69605");
        return user_bits(i);
      }

      inline auto bitmap_state_t::get_index(void *v) const noexcept -> size_t
      {
        auto diff = reinterpret_cast<const uint8_t *>(v) - begin();
        if (mcppalloc_unlikely(diff < 0)) {
          assert(0);
          return ::std::numeric_limits<size_t>::max();
        }
        const auto index = static_cast<size_t>(diff) / real_entry_size();
        if (mcppalloc_unlikely(index >= size())) {
          assert(0);
          return ::std::numeric_limits<size_t>::max();
        }
        return index;
      }
      inline auto bitmap_state_t::get_object(size_t i) noexcept -> void *
      {
        if (mcppalloc_unlikely(i >= size())) {
          throw ::std::runtime_error("mcppalloc: bitmap_state get object failed");
        }
        return reinterpret_cast<void *>(begin() + i * real_entry_size());
      }
      inline auto bitmap_state_t::has_valid_magic_numbers() const noexcept -> bool
      {
        return m_internal.m_pre_magic_number == cs_magic_number_pre && m_internal.m_post_magic_number == cs_magic_number_0;
      }
      inline void bitmap_state_t::verify_magic() const
      {
        if (mcppalloc_unlikely(!has_valid_magic_numbers())) {
          ::std::terminate();
        }
      }
      inline auto bitmap_state_t::addr_in_header(void *v) const noexcept -> bool
      {
        return begin() > v;
      }
    }
  }
}
