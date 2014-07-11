#pragma once
#include "internal_allocator.hpp"
#include "allocator.hpp"
namespace cgc1
{
  namespace details
  {
    struct gc_allocator_traits_t
    {
      do_nothing_t on_create_allocator_block;
      do_nothing_t on_destroy_allocator_block;
      do_nothing_t on_creation;
      using allocator_block_user_data_type = gc_user_data_t;
    };

    using gc_allocator_allocator_t = cgc_internal_allocator_t<void>;
    using gc_allocator_t = allocator_t<gc_allocator_allocator_t, gc_allocator_traits_t>;
    /**
    Return true if the object state is marked, false otherwise.
    **/
    inline bool is_marked(const object_state_t *os)
    {
      return os->user_flags() & 1;
    }
    /**
    Return true if the memory pointed to by the object state is atomic, false otherwise.
    **/
    inline bool is_atomic(const object_state_t *os)
    {
      return 0 != (os->user_flags() & 2);
    }
    /**
    Return true if the memory pointed to by the object state is complicated and needs special handeling.
    **/
    inline bool is_complex(const object_state_t *os)
    {
      return 0 != (os->user_flags() & 4);
    }
    /**
    Clear the gc mark for a given object state.
    **/
    inline void clear_mark(object_state_t *os)
    {
      os->set_user_flags(os->user_flags() & 6);
    }
    /**
    Set the gc mark for a given object state.
    **/
    inline void set_mark(object_state_t *os, bool status = true)
    {
      os->set_user_flags((os->user_flags() & 6) | ((size_t)status));
    }
    /**
    Set the atomic flag for a given object state.
    **/
    inline void set_atomic(object_state_t *os, bool status)
    {
      os->set_user_flags((os->user_flags() & 5) | (((size_t)status) << 1));
    }
    /**
    Set the complex memory flag for a given object state.
    **/
    inline void set_complex(object_state_t *os, bool status)
    {
      os->set_user_flags((os->user_flags() & 3) | (((size_t)status) << 2));
    }
  }
}
