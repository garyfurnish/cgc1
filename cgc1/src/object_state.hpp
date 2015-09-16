#pragma once
#include <cgc1/declarations.hpp>
namespace cgc1
{
  namespace details
  {
    class object_state_t;
    class user_data_base_t;
    /**
     * \brief All object_state_t must be at least c_align_pow2 aligned, so test that.
    **/
    bool is_aligned_properly(const object_state_t *os) noexcept;
    /**
     * \brief Object state for an object in allocator block.
     **/
    class alignas(16) object_state_t
    {
    public:
      /**
       * \brief Return the total size needed for an allocation of object state of sz with header_sz.
      **/
      static constexpr size_t needed_size(size_t header_sz, size_t sz, size_t alignment = c_alignment)
      {
        return align(header_sz, alignment) + align(sz, alignment);
      }
      /**
       * \brief Return the address of the object_state from the allocated object memory.
      **/
      static object_state_t *from_object_start(void *v, size_t alignment = c_alignment) noexcept;
      /**
       * \brief Set next, in_use, and next_valid at once.
      **/
      void set_all(object_state_t *next, bool in_use, bool next_valid, bool quasi_freed = false) noexcept;
      /**
       * \brief Set if the memory associated with this state is in use.
      **/
      void set_in_use(bool v) noexcept;
      /**
       * \brief Return false if the memory associated with this state is available for use, true otherwise.
      **/
      bool not_available() const noexcept;
      /**
       * \brief Return true if the memory associated with this state is in use, false otherwise.
      **/
      bool in_use() const noexcept;
      /**
       * \brief Return true if the memory associated with this state has been freed by another thread, but not processed yet.
      **/
      bool quasi_freed() const noexcept;
      /**
       * \brief Set that the memory associated with this state has been freed by another thread but not processed yet.
      **/
      void set_quasi_freed() noexcept;
      /**
       * \brief Set that the memory associated with this state has been freed by another thread but not processed yet.
      **/
      void set_quasi_freed(bool val) noexcept;
      /**
       * \brief Set if next state is valid.
      **/
      void set_next_valid(bool v) noexcept;
      /**
       * \brief Returns true if next state is valid, false otherwise.
      **/
      bool next_valid() const noexcept;
      /**
       * \brief Returns next state.
      **/
      object_state_t *next() const noexcept;
      /**
       * \brief Sets next state.
       *
       * Sets next state.
       * Clears next_valid state.
      **/
      void set_next(object_state_t *state) noexcept;
      /**
       * \brief Returns start of object.
      **/
      uint8_t *object_start(size_t alignment = c_alignment) const noexcept;
      /**
       * \brief Returns size of memory associated with this state.
      **/
      size_t object_size(size_t alignment = c_alignment) const noexcept;
      /**
       * \brief Returns end of object.
      **/
      uint8_t *object_end(size_t alignment = c_alignment) const noexcept;
      /**
       * \brief Returns user data pointer.
      **/
      user_data_base_t *user_data() const noexcept;
      /**
       * \brief Sets user data pointer.
       *
       * Sets user data pointer.
       * User data must be 8 byte aligned.
      **/
      void set_user_data(void *user_data) noexcept;
      /**
       * \brief Return 3 user flags bitwise.
      **/
      size_t user_flags() const noexcept;
      /**
       * \brief Set user flags.
      **/
      void set_user_flags(size_t flags) noexcept;
      /**
       * \brief Using pointer hiding, store next pointer, next_valid, in_use.
       *
       * Description is in little endian.
       * 0th bit is in_use
       * 1st bit is next_valid
       * 2nd bit is quasi-freed (allocator intends for free to happen during collect).
      **/
      size_t m_next;
      /**
       * \brief Using pointer hiding, store user data pointer and 3 user flags.
       *
       * Description is in little endian.  Bottom 3 bits are user flags.
      **/
      size_t m_user_data;
    };
    static_assert(sizeof(object_state_t) == c_alignment, "object_state_t too large");
    static_assert(::std::is_pod<object_state_t>::value, "object_state_t is not POD");
  }
}
#ifdef CGC1_INLINES
#include "object_state_impl.hpp"
#endif
