#pragma once
#include <type_traits>
#include <memory>
namespace mcppalloc
{
  template <typename Allocator_Policy>
  struct block_t {
    using allocator_policy_type = Allocator_Policy;
   // using pointer_type = typename ::std::pointer_traits<typename allocator_policy_type::pointer_type>::template rebind<void>;
	using pointer_type = void*;
    using size_type = typename allocator_policy_type::size_type;
    pointer_type m_ptr;
    size_type m_size;
    template <typename Return_Allocator_Policy>
    operator block_t<Return_Allocator_Policy>()
    {
      return block_t<Return_Allocator_Policy>{m_ptr, m_size};
    }
  };
}
#include "default_allocator_policy.hpp"
static_assert(::std::is_pod<::mcppalloc::block_t<::mcppalloc::default_allocator_policy_t<::std::allocator<void>>>>::value, "");
