#include "bitmap_gc_user_data.hpp"
#include <mcppalloc/mcppalloc_bitmap_allocator/bitmap_state.hpp>
namespace cgc1
{
  namespace details
  {
    void finalize(::mcppalloc::bitmap_allocator::details::bitmap_state_t *state)
    {
      const size_t alloca_size =
          state->block_size_in_bytes() + ::mcppalloc::bitmap::dynamic_bitmap_ref_t<false>::bits_type::cs_alignment;
      const auto to_be_freed_memory = alloca(alloca_size);
      auto to_be_freed =
          ::mcppalloc::bitmap::make_dynamic_bitmap_ref_from_alloca(to_be_freed_memory, state->num_blocks(), alloca_size);
      to_be_freed.clear();
      state->or_with_to_be_freed(to_be_freed);
      const auto free_with_finalizer_memory = alloca(alloca_size);
      auto free_with_finalizer =
          ::mcppalloc::bitmap::make_dynamic_bitmap_ref_from_alloca(free_with_finalizer_memory, state->num_blocks(), alloca_size);
      free_with_finalizer.deep_copy(to_be_freed);
      free_with_finalizer &= state->user_bits_ref(cs_bitmap_allocation_user_bit_finalizeable);
      for (size_t i = 0; i < state->size(); ++i) {
        if (!free_with_finalizer.get_bit(i)) {
          continue;
        }
        const auto object = state->get_object(i);
        if (state->user_bits_ref(cs_bitmap_allocation_user_bit_finalizeable).get_bit(i)) {
          const auto ud = bitmap_allocator_user_data(object);
          if (mcpputil_unlikely(!ud)) {
            continue;
          }
          if (ud->abort_on_collect()) {

            ::std::cerr << __FILE__ << " " << __LINE__ << " " << state->is_marked(i) << ::std::endl;
            ::std::terminate();
          }
          state->user_bits_ref(cs_bitmap_allocation_user_bit_finalizeable).set_bit(i, false);
          state->user_bits_ref(cs_bitmap_allocation_user_bit_finalizeable_arbitrary_thread).set_bit(i, false);
          auto finalizer = ::std::move(ud->gc_user_data_ref().m_finalizer);
          try {
            finalizer(object);
          } catch (::std::exception &e) {
            ::std::cerr << "CGC1: Finalizer exception: " << e.what();
          } catch (...) {
            ::std::cerr << "CGC1: Finalizer threw unknown exception: 872ed1cd-be5c-4e65-baed-9e44de0a1dc8";
            ::std::terminate();
          }
          ::mcpputil::secure_zero_stream(object, state->real_entry_size());
        }
      }

      to_be_freed &= free_with_finalizer.negate();
      to_be_freed.for_some_contiguous_bits_flip(state->size(), [state](size_t begin, size_t end) {
        ::mcpputil::secure_zero_stream(state->get_object(begin), state->real_entry_size() * (end - begin));
      });
      to_be_freed.for_set_bits(
          state->size(), [state](size_t i) { ::mcpputil::secure_zero_stream(state->get_object(i), state->real_entry_size()); });
      state->free_unmarked();
    }
  }
}
