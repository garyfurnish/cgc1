#pragma once
#include "allocator_policy.hpp"
#include "default_allocator_thread_policy.hpp"
#include <cstdint>
namespace mcppalloc
{
  template <typename Internal_Allocator>
  struct default_allocator_policy_t : public allocator_policy_tag_t {
    using pointer_type = void *;
    using uintptr_type = uintptr_t;
    using size_type = size_t;
    using ptrdiff_type = ptrdiff_t;
    using internal_allocator_type = Internal_Allocator;
    using user_data_type = details::user_data_base_t;
    using thread_policy_type = default_allocator_thread_policy_t;
    static const constexpr size_type cs_minimum_alignment = 16;
    default_allocator_policy_t() = delete;
  };
}
