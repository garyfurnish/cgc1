#pragma once
#include "gc_user_data.hpp"
namespace cgc1
{
  inline bool is_bitmap_allocator(void *addr) noexcept
  {
    if (addr >= details::g_gks->fast_slab_begin() && addr < details::g_gks->fast_slab_end())
      return true;
    return false;
  }
  inline bool is_sparse_allocator(void *addr) noexcept
  {
    return !is_bitmap_allocator(addr);
  }
  namespace details
  {

    /**
   * \brief User data that can be associated with an allocation.
   **/
    class alignas(::mcppalloc::details::user_data_alignment_t) bitmap_gc_user_data_t
    {
    public:
      auto gc_user_data_ref() noexcept -> gc_user_data_t &
      {
        return m_gc_user_data;
      }
      auto gc_user_data_ref() const noexcept -> const gc_user_data_t &
      {
        return m_gc_user_data;
      }

      auto atomic() const noexcept -> bool
      {
        return m_atomic;
      }
      void set_atomic(bool atomic) noexcept
      {
        m_atomic = atomic;
      }

    private:
      gc_user_data_t m_gc_user_data;
      bool m_atomic{false};
    };

    inline details::bitmap_gc_user_data_t *bitmap_allocator_user_data(void *addr)
    {
      if (!is_bitmap_allocator(addr))
        return nullptr;
      const auto state = ::mcppalloc::bitmap_allocator::details::get_state(addr);
      if (mcppalloc_unlikely(!state->has_valid_magic_numbers())) {
        assert(0);
        return nullptr;
      }
      if (mcppalloc_unlikely(state->type_id() != 2)) {
        assert(0);
        return nullptr;
      }
      const auto bin_sz = state->real_entry_size();
      uint8_t *const start_of_gc_user_data =
          ::mcppalloc::unsafe_cast<uint8_t>(addr) + bin_sz - sizeof(details::bitmap_gc_user_data_t);
      return ::mcppalloc::unsafe_cast<details::bitmap_gc_user_data_t>(start_of_gc_user_data);
    }
  }
}
