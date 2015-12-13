#pragma once
#include <mcppalloc/mcppalloc_utils/alignment.hpp>
namespace mcppalloc
{
  namespace details
  {
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE bool is_aligned_properly(const object_state_t<Allocator_Policy> *os) noexcept
    {
      return os == align(os, object_state_t<Allocator_Policy>::cs_alignment);
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE object_state_t<Allocator_Policy> *
    object_state_t<Allocator_Policy>::from_object_start(void *v, size_type alignment) noexcept
    {
      return reinterpret_cast<object_state_t<Allocator_Policy> *>(reinterpret_cast<uint8_t *>(v) -
                                                                  align(sizeof(object_state_t<Allocator_Policy>), alignment));
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE void object_state_t<Allocator_Policy>::set_all(object_state_t<Allocator_Policy> *next,
                                                                           bool in_use,
                                                                           bool next_valid,
                                                                           bool quasi_freed) noexcept
    {
      m_pre_magic = cs_pre_magic;
      m_next = reinterpret_cast<size_type>(next) | static_cast<size_type>(in_use) | (static_cast<size_type>(next_valid) << 1) |
               (static_cast<size_type>(quasi_freed) << 2);
      m_post_magic = cs_post_magic;
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE void object_state_t<Allocator_Policy>::verify_magic() noexcept
    {
      if (mcppalloc_unlikely(m_pre_magic != cs_pre_magic)) {
        ::std::terminate();
      }
      else if (mcppalloc_unlikely(m_post_magic != cs_post_magic)) {
        ::std::terminate();
      }
    }

    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE void object_state_t<Allocator_Policy>::set_in_use(bool v) noexcept
    {
      size_type ptr = static_cast<size_type>(m_next);
      size_type iv = static_cast<size_type>(v);
      m_next = (ptr & size_inverse(1)) | (iv & 1);
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE bool object_state_t<Allocator_Policy>::not_available() const noexcept
    {
      return 0 < (m_next & 5);
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE bool object_state_t<Allocator_Policy>::in_use() const noexcept
    {
      return m_next & 1;
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE bool object_state_t<Allocator_Policy>::quasi_freed() const noexcept
    {
      return (m_next & 4) > 0;
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE void object_state_t<Allocator_Policy>::set_quasi_freed() noexcept
    {
      set_all(next(), false, next_valid(), true);
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE void object_state_t<Allocator_Policy>::set_quasi_freed(bool val) noexcept
    {
      assert(!(val && in_use()));
      set_all(next(), in_use(), next_valid(), val);
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE void object_state_t<Allocator_Policy>::set_next_valid(bool v) noexcept
    {
      size_type ptr = static_cast<size_type>(m_next);
      size_type iv = static_cast<size_type>(v) << 1;
      m_next = (ptr & size_inverse(2)) | (iv & 2);
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE bool object_state_t<Allocator_Policy>::next_valid() const noexcept
    {
      return (static_cast<size_type>(m_next) & 2) > 0;
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE object_state_t<Allocator_Policy> *object_state_t<Allocator_Policy>::next() const noexcept
    {
      return reinterpret_cast<object_state_t<Allocator_Policy> *>(m_next & size_inverse(7));
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE void object_state_t<Allocator_Policy>::set_next(object_state_t<Allocator_Policy> *state) noexcept
    {
      size_type ptr = reinterpret_cast<size_type>(state);
      //      size_type iv = static_cast<size_type>(not_available());
      m_next = (ptr & static_cast<size_type>(-4)) | (m_next & 1); //(iv & 1);
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE uint8_t *object_state_t<Allocator_Policy>::object_start(size_type alignment) const noexcept
    {
      return const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(this) +
                                   align(sizeof(object_state_t<Allocator_Policy>), alignment));
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE auto object_state_t<Allocator_Policy>::object_size(size_type alignment) const noexcept -> size_type
    {
      // It is invariant that object_start() > next for all valid objects.
      return static_cast<size_type>(reinterpret_cast<uint8_t *>(next()) - object_start(alignment));
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE uint8_t *object_state_t<Allocator_Policy>::object_end(size_type alignment) const noexcept
    {
      return const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(this) +
                                   align(sizeof(object_state_t<Allocator_Policy>), cs_alignment) + object_size(alignment));
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE user_data_base_t *object_state_t<Allocator_Policy>::user_data() const noexcept
    {
      return reinterpret_cast<user_data_base_t *>(m_user_data & (~static_cast<size_type>(7)));
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE void object_state_t<Allocator_Policy>::set_user_data(void *user_data) noexcept
    {
      assert(user_data == align_pow2(user_data, 3));
      m_user_data = reinterpret_cast<size_type>(user_data) | user_flags();
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE auto object_state_t<Allocator_Policy>::user_flags() const noexcept -> size_type
    {
      return m_user_data & 7;
    }
    template <typename Allocator_Policy>
    MCPPALLOC_ALWAYS_INLINE void object_state_t<Allocator_Policy>::set_user_flags(size_type flags) noexcept
    {
      assert(flags < 8);
      m_user_data = reinterpret_cast<size_type>(user_data()) | flags;
    }
  }
}
