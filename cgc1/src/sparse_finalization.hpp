#include "gc_allocator.hpp"
#include "global_kernel_state.hpp"
#include <cgc1/hide_pointer.hpp>
namespace cgc1::details
{
  template <typename Allocator>
  auto do_local_sparse_finalization(Allocator &allocator, cgc_internal_vector_t<gc_sparse_object_state_t *> &vec)
  {
    cgc_internal_vector_t<uintptr_t> to_be_freed;
    for (auto &&os : vec) {
      gc_user_data_t *ud = static_cast<gc_user_data_t *>(os->user_data());
      if (mcpputil_likely(ud)) {
        if (mcpputil_unlikely(ud->is_default())) {
        } else {
          // if it has a finalizer that can run in this thread, finalize.
          if (ud->m_finalizer) {
            ud->m_finalizer(os->object_start());
          }
          unique_ptr_allocated<gc_user_data_t, cgc_internal_allocator_t<void>> up(ud);
        }
      }
      mcpputil::secure_zero(os->object_start(), os->object_size());
      to_be_freed.emplace_back(::mcpputil::hide_pointer(os->object_start()));
    }
    allocator.bulk_destroy_memory(vec);
    return to_be_freed;
  }
}
