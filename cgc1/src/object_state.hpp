#pragma once
#include <cgc1/declarations.hpp>
#include <iostream>
namespace cgc1
{
  namespace details
  {
    class object_state_t;
    class user_data_base_t;
    /**
    All object_state_t must be at least c_align_pow2 aligned, so that.
    **/
    inline bool is_aligned_properly(const object_state_t *os)
    {
      return os == align_pow2(os, c_align_pow2);
    }
    class object_state_t
    {
    public:
      /**
      Return the total size needed for an allocation of object state of sz with header_sz.
      **/
      static constexpr size_t needed_size(size_t header_sz, size_t sz)
      {
        return align(header_sz) + align(sz);
      }
      /**
      Return the address of the object_state from the allocated object memory.
      **/
      static object_state_t *from_object_start(void *v)
      {
        return reinterpret_cast<object_state_t *>(reinterpret_cast<uint8_t *>(v) - align(sizeof(object_state_t)));
      }
      /**
      Set next, in_use, and next_valid at once.
      **/
      void set_all(object_state_t *next, bool in_use, bool next_valid, bool quasi_freed = false)
      {
        m_next = ((size_t)next) | ((size_t)in_use) | (((size_t)next_valid) << 1 | (((size_t)quasi_freed) << 2));
      }
      /**
      Set if the memory associated with this state is in use.
      **/
      void set_in_use(bool v)
      {
        size_t ptr = (size_t)m_next;
        size_t iv = (size_t)v;
        m_next = (ptr & size_inverse(1)) | (iv & 1);
      }
      /**
      Set if the memory associated with this state is in use in a way that destroys next pointer.
      **/
      void set_in_use_destructive(bool v)
      {
        m_next = (size_t)v;
      }
      /**
      Return false if the memory associated with this state is available for use, true otherwise.
      **/
      bool not_available() const
      {
        return 0 < (m_next & 5);
      }
      /**
      Return true if the memory associated with this state is in use, false otherwise.
      **/
      bool in_use() const
      {
        return m_next & 1;
      }
      /**
      Return true if the memory associated with this state has been freed by another thread, but not processed yet.
      **/
      bool quasi_freed() const
      {
        return (m_next & 4) > 0;
      }
      /**
      Set that the memory associated with this state has been freed by another thread but not processed yet.
      **/
      void set_quasi_freed()
      {
        set_all(next(), false, next_valid(), true);
      }
      void set_quasi_freed(bool val)
      {
        assert(!(val && in_use()));
        set_all(next(), in_use(), next_valid(), val);
      }
      /**
      Set if next state is valid.
      **/
      void set_next_valid(bool v)
      {
        size_t ptr = (size_t)m_next;
        size_t iv = ((size_t)v) << 1;
        m_next = (ptr & size_inverse(2)) | (iv & 2);
      }
      /**
      Returns true if next state is valid, false otherwise.
      **/
      bool next_valid() const
      {
        return (((size_t)m_next) & 2) > 0;
      }
      /**
      Returns next state.
      **/
      object_state_t *next() const
      {
        return reinterpret_cast<object_state_t *>(m_next & size_inverse(7));
      }
      /**
      Sets next state.
      Clears next_valid state.
      **/
      void set_next(object_state_t *state)
      {
        size_t ptr = (size_t)state;
        size_t iv = (size_t)not_available();
        m_next = (ptr & ((size_t)-4)) | (iv & 1);
      }
      /**
      Returns start of object.
      **/
      uint8_t *object_start() const
      {
        return const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(this) + align(sizeof(object_state_t)));
      }
      /**
      Returns size of memory associated with this state.
      **/
      size_t object_size() const
      {
        return reinterpret_cast<uint8_t *>(next()) - object_start();
      }
      /**
      Returns end of object.
      **/
      uint8_t *object_end() const
      {
        return const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(this) + align(sizeof(object_state_t)) + object_size());
      }
      /**
      Returns user data pointer.
      **/
      user_data_base_t *user_data() const
      {
        return reinterpret_cast<user_data_base_t *>(m_user_data & (~((size_t)7)));
      }
      /**
      Sets user data pointer.
      User data must be 8 byte aligned.
      **/
      void set_user_data(void *user_data)
      {
        assert(user_data == align_pow2(user_data, 3));
        m_user_data = reinterpret_cast<size_t>(user_data) | user_flags();
      }
      /**
      Return 3 user flags bitwise.
      **/
      size_t user_flags() const
      {
        return m_user_data & 7;
      }
      /**
      Set user flags.
      **/
      void set_user_flags(size_t flags)
      {
        assert(flags < 8);
        m_user_data = reinterpret_cast<size_t>(user_data()) | flags;
      }
      /**
      Using pointer hiding, store next pointer, next_valid, in_use.
      **/
      size_t m_next;
      /**
      Using pointer hiding, store user data pointer and 3 user flags.
      **/
      size_t m_user_data;
    };
    static_assert(sizeof(object_state_t) == c_alignment, "object_state_t too large");
    static_assert(::std::is_pod<object_state_t>::value, "object_state_t is not POD");
    template <typename charT, typename Traits>
    ::std::basic_ostream<charT, Traits> &operator<<(::std::basic_ostream<charT, Traits> &os, const object_state_t &state)
    {
      os << "Object_State_t = (" << &state << "," << state.next() << "," << state.not_available() << "," << state.m_user_data
         << ")";
      return os;
    }
  }
}
