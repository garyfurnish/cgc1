#pragma once
#include "allocator_thread_policy.hpp"
#include <mcpputil/mcpputil/declarations.hpp>

namespace mcppalloc
{
  namespace details
  {
    class user_data_base_t;
  }
  /**
     * \brief Default allocator policy.
     *
     * Does nothing on any event.
     **/
  struct default_allocator_thread_policy_t : public details::allocator_thread_policy_tag_t {
    mcpputil::do_nothing_t on_allocation;
    mcpputil::do_nothing_t on_create_allocator_block;
    mcpputil::do_nothing_t on_destroy_allocator_block;
    mcpputil::do_nothing_t on_creation;
    details::allocation_failure_action_t on_allocation_failure(const details::allocation_failure_t &)
    {
      return details::allocation_failure_action_t{false, false};
    }
    using allocator_block_user_data_type = details::user_data_base_t;
  };
}
