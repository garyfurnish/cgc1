#pragma once
#include <atomic>
#include <mcppalloc/mcppalloc_slab_allocator/slab_allocator.hpp>
#include <mcppalloc/mcppalloc_utils/security.hpp>
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
      template <typename Allocator_Policy>
      void bitmap_state_t::set_bitmap_package(bitmap_package_t<Allocator_Policy> *package) noexcept
      {
        m_internal.m_info.m_package = package;
      }

      inline auto bitmap_state_t::bitmap_package() const noexcept -> void *
      {
        return m_internal.m_info.m_package;
      }
      inline void bitmap_state_t::initialize_consts() noexcept
      {
        assert(this);
        m_internal.m_pre_magic_number = cs_magic_number_pre;
        m_internal.m_post_magic_number = cs_magic_number_0;
      }
      template <typename Allocator_Policy>
      inline void
      bitmap_state_t::initialize(type_id_t type_id, uint8_t user_bit_fields, bitmap_package_t<Allocator_Policy> *package) noexcept
      {
        initialize_consts();
        m_internal.m_info.m_type_id = type_id;
        m_internal.m_info.m_num_user_bit_fields = user_bit_fields;
        _compute_size();
        free_bits_ref().fill(::std::numeric_limits<uint64_t>::max());
        m_internal.m_info.m_cached_first_free = 0;
        m_internal.m_info.m_package = package;
        for (size_t i = 0; i < num_user_bit_fields(); ++i)
          clear_user_bits(i);
      }
      inline void bitmap_state_t::clear_mark_bits() noexcept
      {
        mark_bits_ref().clear();
      }
      inline void bitmap_state_t::clear_user_bits(size_t index) noexcept
      {
        user_bits_ref(index).clear();
      }
      inline auto bitmap_state_t::all_free() const noexcept -> bool
      {
        return free_bits_ref().all_set();
      }
      inline auto bitmap_state_t::num_bit_arrays() const noexcept -> size_t
      {
        return cs_bits_array_multiple + m_internal.m_info.m_num_user_bit_fields;
      }

      inline auto bitmap_state_t::any_free() const noexcept -> bool
      {
        return free_bits_ref().any_set();
      }
      inline auto bitmap_state_t::none_free() const noexcept -> bool
      {
        return free_bits_ref().none_set();
      }
      inline auto bitmap_state_t::first_free() const noexcept -> size_t
      {
        return m_internal.m_info.m_cached_first_free;
      }
      inline auto bitmap_state_t::_compute_first_free() noexcept -> size_t
      {
        auto ret = free_bits_ref().first_set();
        m_internal.m_info.m_cached_first_free = ret;
        return ret;
      }
      inline auto bitmap_state_t::any_marked() const noexcept -> bool
      {
        return mark_bits_ref().any_set();
      }
      inline auto bitmap_state_t::none_marked() const noexcept -> bool
      {
        return mark_bits_ref().none_set();
      }
      inline auto bitmap_state_t::free_popcount() const noexcept -> size_t
      {
        return free_bits_ref().popcount();
      }
      inline void bitmap_state_t::set_free(size_t i, bool val) noexcept
      {
        if (val)
          m_internal.m_info.m_cached_first_free = ::std::min(i, m_internal.m_info.m_cached_first_free);
        else if (i == m_internal.m_info.m_cached_first_free) {
          // not free
          if (i + 1 >= size())
            m_internal.m_info.m_cached_first_free = ::std::numeric_limits<size_t>::max();
          else if (is_free(i + 1))
            m_internal.m_info.m_cached_first_free++;
          else {
            // could be anywhere
            _compute_first_free();
          }
        }
        free_bits_ref().set_bit(i, val);
      }
      inline void bitmap_state_t::set_marked(size_t i) noexcept
      {
        mark_bits_ref().set_bit_atomic(i, true, ::std::memory_order_relaxed);
      }
      inline auto bitmap_state_t::is_free(size_t i) const noexcept -> bool
      {
        return free_bits_ref().get_bit(i);
      }
      inline auto bitmap_state_t::is_marked(size_t i) const noexcept -> bool
      {
        return mark_bits_ref().get_bit(i);
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
        auto data_sz = c_bitmap_block_size - slab_allocator::details::slab_allocator_t::cs_header_sz - hdr_sz;
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
        // guarentee the memory address exists somewhere that is visible to gc
        volatile auto memory_address = begin() + real_entry_size() * i;
        if (i >= size())
          return nullptr;
        set_free(i, false);
        // this awful code is because for a conservative gc
        // we could set free before memory_address is live.
        // this can go wrong because we could mark while it is still free.
        if (mcppalloc_unlikely(is_free(i))) {
          if (mcppalloc_likely(retries < 15)) {
            goto RESTART;
          } else {
            ::std::cerr << "mcppalloc: bitmap_state terminating due to allocation failure 0b3fb7a6-7270-45e3-a1cf-da341de0ccfb\n";
            ::std::abort();
          }
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
        secure_zero_stream(v, real_entry_size());
        auto i = byte_diff / real_entry_size();
        set_free(i, true);
        for (size_t j = 0; j < num_user_bit_fields(); ++j)
          user_bits_ref(j).set_bit(i, false);
        return true;
      }
      inline void bitmap_state_t::or_with_to_be_freed(bitmap::dynamic_bitmap_ref_t<false> to_be_freed)
      {
        const size_t alloca_size = block_size_in_bytes() + bitmap::dynamic_bitmap_ref_t<false>::bits_type::cs_alignment;
        const auto mark_memory = alloca(alloca_size);
        auto mark = bitmap::make_dynamic_bitmap_ref_from_alloca(mark_memory, num_blocks(), alloca_size);
        mark.deep_copy(mark_bits_ref());
        to_be_freed |= mark.negate();
      }
      inline void bitmap_state_t::free_unmarked()
      {
        or_with_to_be_freed(free_bits_ref());
        _compute_first_free();
      }
      inline auto bitmap_state_t::num_blocks() const noexcept -> size_t
      {
        return m_internal.m_info.m_num_blocks;
      }
      inline auto bitmap_state_t::num_user_bit_fields() const noexcept -> size_t
      {
        return m_internal.m_info.m_num_user_bit_fields;
      }
      inline auto bitmap_state_t::block_size_in_bytes() const noexcept -> size_t
      {
        return num_blocks() * sizeof(bits_array_type);
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

      inline auto bitmap_state_t::free_bits_ref() noexcept -> bitmap::dynamic_bitmap_ref_t<false>
      {
        return bitmap::make_dynamic_bitmap_ref(free_bits(), num_blocks());
      }
      inline auto bitmap_state_t::free_bits_ref() const noexcept -> bitmap::dynamic_bitmap_ref_t<true>
      {
        return bitmap::make_dynamic_bitmap_ref(free_bits(), num_blocks());
      }
      inline auto bitmap_state_t::mark_bits_ref() noexcept -> bitmap::dynamic_bitmap_ref_t<false>
      {
        return bitmap::make_dynamic_bitmap_ref(mark_bits(), num_blocks());
      }
      inline auto bitmap_state_t::mark_bits_ref() const noexcept -> bitmap::dynamic_bitmap_ref_t<true>
      {
        return bitmap::make_dynamic_bitmap_ref(mark_bits(), num_blocks());
      }
      inline auto bitmap_state_t::user_bits_ref(size_t index) noexcept -> bitmap::dynamic_bitmap_ref_t<false>
      {
        return bitmap::make_dynamic_bitmap_ref(user_bits(index), num_blocks());
      }
      inline auto bitmap_state_t::user_bits_ref(size_t index) const noexcept -> bitmap::dynamic_bitmap_ref_t<true>
      {
        return bitmap::make_dynamic_bitmap_ref(user_bits(index), num_blocks());
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
#ifdef _DEBUG
        if (mcppalloc_unlikely(!has_valid_magic_numbers())) {
          ::std::cerr << "mcppalloc: bitmap_state: invalid magic numbers 027e8d50-8555-4e7f-93a7-4d048b506436\n";
          ::std::abort();
        }
#endif
      }
      inline auto bitmap_state_t::addr_in_header(void *v) const noexcept -> bool
      {
        return begin() > v;
      }
    }
  }
}
