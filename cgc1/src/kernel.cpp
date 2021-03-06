#include "bitmap_gc_user_data.hpp"
#include "gc_allocator.hpp"
#include "global_kernel_state.hpp"
#include "internal_declarations.hpp"
#include "thread_local_kernel_state.hpp"
#include <cgc1/cgc1.hpp>
#include <cgc1/cgc1_dll.hpp>
#include <cgc1/hide_pointer.hpp>
#include <cstring>
namespace cgc1
{
  namespace details
  {
    global_kernel_state_t *g_gks{nullptr};
    static auto &get_gks()
    {
      static unique_ptr_malloc_t<global_kernel_state_t> gks;
      return gks;
    }
    void check_initialized()
    {
      if (nullptr == details::g_gks) {
        global_kernel_state_param_t param;
        get_gks() = make_unique_malloc<details::global_kernel_state_t>(param);
        g_gks = get_gks().get();
        get_gks()->initialize();
      }
      if (nullptr == details::g_gks) {
        {
          g_gks = get_gks().get();
        }
      }
    }
    void thread_gc_handler(int)
    {
      g_gks->_collect_current_thread();
    }
#ifdef __APPLE__
    pthread_key_t g_pkey;
#else
#ifndef _WIN32
    thread_local thread_local_kernel_state_t *t_tlks;
#else
    __declspec(thread) thread_local_kernel_state_t *t_tlks;
#endif
#endif
    bool is_bitmap_allocator(void *addr) noexcept
    {
      return details::g_gks->_bitmap_allocator().underlying_memory().memory_range().contains(addr);
    }
    bool is_sparse_allocator(void *addr) noexcept
    {
      return details::g_gks->gc_allocator().underlying_memory().memory_range().contains(addr);
    }
  }
  namespace debug
  {
    size_t num_gc_collections() noexcept
    {
      return details::g_gks->num_collections();
    }

    auto _cgc_hidden_packed_marked(uintptr_t loc) -> bool
    {
      auto state = ::mcppalloc::bitmap_allocator::details::get_state(::mcpputil::unhide_pointer(loc));
      assert(state);
      auto index = state->get_index(::mcpputil::unhide_pointer(loc));
      assert(index != ::std::numeric_limits<size_t>::max());
      return state->is_marked(index);
    }
    auto _cgc_hidden_packed_free(uintptr_t loc) -> bool
    {
      auto state = ::mcppalloc::bitmap_allocator::details::get_state(::mcpputil::unhide_pointer(loc));
      assert(state);
      auto index = state->get_index(::mcpputil::unhide_pointer(loc));
      assert(index != ::std::numeric_limits<size_t>::max());
      return state->is_free(index);
    }
  }
  bool in_signal_handler() noexcept
  {
    auto tlks = details::get_tlks();
    if (nullptr != tlks) {
      {
        return tlks->in_signal_handler();
      }
    }
    return false;
  }
  CGC1_DLL_PUBLIC void *cgc_malloc(size_t sz)
  {
    return ::cgc1::details::g_gks->allocate(sz).m_ptr;
  }
  CGC1_DLL_PUBLIC auto cgc_allocate(size_t sz) -> details::gc_allocator_t::block_type
  {
    return ::cgc1::details::g_gks->allocate(sz);
  }
  CGC1_DLL_PUBLIC uintptr_t cgc_hidden_malloc(size_t sz)
  {
    void *addr = cgc_malloc(sz);
    return ::mcpputil::hide_pointer(addr);
  }
  CGC1_DLL_PUBLIC void *cgc_realloc(void *v, size_t sz)
  {
    void *ret = cgc_malloc(sz);
    if (nullptr != v) {
      {
        ::memcpy(ret, v, sz);
      }
    }
    return ret;
  }
  CGC1_DLL_PUBLIC void cgc_free(void *v)
  {
    ::cgc1::details::g_gks->deallocate(v);
  }
  CGC1_DLL_PUBLIC bool cgc_is_cgc(void *v)
  {
    return cgc_size(v) > 0;
  }
  CGC1_DLL_PUBLIC void *cgc_start(void *addr)
  {
    if (nullptr == addr) {
      {
        return nullptr;
      }
    }
    if (details::is_bitmap_allocator(addr)) {
      auto state = ::mcppalloc::bitmap_allocator::details::get_state(addr);
      if (state->has_valid_magic_numbers()) {
        return state->begin() + state->get_index(addr) * state->real_entry_size();
      }
      return nullptr;
    }
    if (details::is_sparse_allocator(addr)) {
      details::gc_sparse_object_state_t *os =
          details::gc_sparse_object_state_t::template from_object_start<details::gc_sparse_object_state_t>(addr);
      if (!details::g_gks->is_valid_object_state(os)) {
        os = details::g_gks->find_valid_object_state(addr);
        if (nullptr == os) {
          return nullptr;
        }
      }
      return os->object_start();
    }
    return nullptr;
  }
  CGC1_DLL_PUBLIC size_t cgc_size(void *addr)
  {
    if (nullptr == addr) {
      return 0;
    }
    void *start = cgc_start(addr);
    if (nullptr == start) {
      return 0;
    }
    if (details::g_gks->_bitmap_allocator().underlying_memory().memory_range().contains(start)) {
      auto state = ::mcppalloc::bitmap_allocator::details::get_state(addr);
      if (state->has_valid_magic_numbers()) {
        return state->declared_entry_size();
      }
      return 0;
    }
    details::gc_sparse_object_state_t *os =
        details::gc_sparse_object_state_t::template from_object_start<details::gc_sparse_object_state_t>(start);
    if (nullptr != os) {
      return os->object_size();
    }
    return 0;
  }
  CGC1_DLL_PUBLIC void cgc_add_root(void **v)
  {
    details::check_initialized();
    details::g_gks->root_collection().add_root(v);
  }
  CGC1_DLL_PUBLIC void cgc_remove_root(void **v)
  {
    if (nullptr == details::g_gks) {
      {
        return;
      }
    }
    details::g_gks->root_collection().remove_root(v);
  }
  CGC1_DLL_PUBLIC bool cgc_has_root(void **v)
  {
    return details::g_gks->root_collection().has_root(v);
  }
  CGC1_DLL_PUBLIC void cgc_add_range(mcpputil::system_memory_range_t range)
  {
    details::check_initialized();
    details::g_gks->root_collection().add_range(range);
  }
  CGC1_DLL_PUBLIC void cgc_remove_range(mcpputil::system_memory_range_t range)
  {
    if (nullptr == details::g_gks) {
      {
        return;
      }
    }
    details::g_gks->root_collection().remove_range(range);
  }
  CGC1_DLL_PUBLIC bool cgc_has_range(mcpputil::system_memory_range_t range)
  {
    return details::g_gks->root_collection().has_range(range);
  }
  CGC1_DLL_PUBLIC size_t cgc_heap_size()
  {
    // this cast is safe because end > begin is an invariant.
    return details::g_gks->gc_allocator().underlying_memory().size();
  }
  CGC1_DLL_PUBLIC size_t cgc_heap_free()
  {
    // this cast is safe because end > current_end is an invariant.
    return static_cast<size_t>(details::g_gks->gc_allocator().underlying_memory().end() -
                               details::g_gks->gc_allocator().current_end());
  }
  CGC1_DLL_PUBLIC void cgc_enable()
  {
    details::g_gks->enable();
  }
  CGC1_DLL_PUBLIC void cgc_disable()
  {
    details::g_gks->disable();
  }
  CGC1_DLL_PUBLIC bool cgc_is_enabled()
  {
    return details::g_gks->enabled();
  }
  CGC1_DLL_PUBLIC void cgc_register_thread(void *top_of_stack)
  {
    details::check_initialized();
    details::g_gks->initialize_current_thread(top_of_stack);
    auto tlks = details::get_tlks();
    tlks->set_thread_allocator(&::cgc1::details::g_gks->gc_allocator().initialize_thread());
    tlks->set_bitmap_thread_allocator(&::cgc1::details::g_gks->_bitmap_allocator().initialize_thread());
  }
  CGC1_DLL_PUBLIC void cgc_collect()
  {
    details::g_gks->collect();
  }
  CGC1_DLL_PUBLIC void cgc_force_collect(bool do_local_finalization)
  {
    details::g_gks->force_collect(do_local_finalization);
  }
  CGC1_DLL_PUBLIC void cgc_wait_collect()
  {
    details::g_gks->wait_for_collection();
    details::g_gks->_mutex().unlock();
  }
  CGC1_DLL_PUBLIC void cgc_wait_finalization(bool do_local_finalization)
  {
    details::g_gks->wait_for_finalization(do_local_finalization);
  }
  CGC1_DLL_PUBLIC void cgc_unregister_thread()
  {
    details::g_gks->destroy_current_thread();
  }
  CGC1_DLL_PUBLIC void cgc_shutdown()
  {
    details::g_gks->shutdown();
    details::g_gks = nullptr;
  }
  CGC1_DLL_PUBLIC void
  cgc_register_finalizer(void *addr, ::std::function<void(void *)> finalizer, bool allow_arbitrary_finalizer_thread, bool throws)
  {
    if (nullptr == addr) {
      if (throws) {
        throw ::std::runtime_error("cgc1: nullptr 2731790e-a3be-4824-9cce-cee54cd06606");
      } else {
        {
          return;
        }
      }
    }
    const auto ba_user_data = details::bitmap_allocator_user_data(addr);
    if (ba_user_data != nullptr) {
      const auto ba_state = ::mcppalloc::bitmap_allocator::details::get_state(addr);
      if (mcpputil_unlikely(!ba_state)) {
        {
          throw ::std::runtime_error("cgc1: could not find state to register finalizer: 47af381e-214d-4d4d-85e3-2f7ac93fc20d");
        }
      }
      if (mcpputil_unlikely(ba_state->type_id() != 2)) {
        {
          throw ::std::runtime_error(
              "cgc1: tried to finalize wrong type of bitmap allocation: 1bfb19d0-014a-4811-b97e-07fb2720b72a");
        }
      }
      const size_t index = ba_state->get_index(addr);
      if (mcpputil_unlikely(index == ::std::numeric_limits<size_t>::max())) {
        {
          throw ::std::runtime_error("cgc1: could not find index to register finalizer: c002eaba-8b43-4710-a6c0-46336101d45f");
        }
      }
      if (mcpputil_unlikely(ba_state->num_user_bit_fields() != 2)) {
        {
          throw ::std::runtime_error("cgc1: unable to register finalizer: c0d42cd8-23fc-43d3-be6b-daee36fe17be");
        }
      }
      ba_state->user_bits_ref(details::cs_bitmap_allocation_user_bit_finalizeable).set_bit(index, true);
      ba_state->user_bits_ref(details::cs_bitmap_allocation_user_bit_finalizeable_arbitrary_thread)
          .set_bit(index, allow_arbitrary_finalizer_thread);
      ba_user_data->gc_user_data_ref().set_is_default(false);
      ba_user_data->gc_user_data_ref().set_allow_arbitrary_finalizer_thread(allow_arbitrary_finalizer_thread);
      ba_user_data->gc_user_data_ref().m_finalizer = ::std::move(finalizer);
    } else if (details::is_sparse_allocator(addr)) {
      void *const start = cgc_start(addr);
      if (mcpputil_unlikely(!start)) {
        if (throws) {
          {
            throw ::std::runtime_error("cgc1: bad sparse address to register finalizer: 44084b93-08b2-4511-9432-0ea986c6f1e5");
          }
        } else {
          {
            return;
          }
        }
      }
      details::gc_sparse_object_state_t *const os =
          details::gc_sparse_object_state_t::template from_object_start<details::gc_sparse_object_state_t>(start);
      details::gc_user_data_t *ud = static_cast<details::gc_user_data_t *>(os->user_data());
      if (ud->is_default()) {
        ud = ::mcpputil::make_unique_allocator<details::gc_user_data_t, cgc_internal_allocator_t<void>>(*ud).release();
        ud->set_is_default(false);
        ud->set_allow_arbitrary_finalizer_thread(allow_arbitrary_finalizer_thread);
        os->set_user_data(ud);
      }
      ud->m_finalizer = ::std::move(finalizer);
    } else {
      if (throws) {
        {
          throw ::std::runtime_error("cgc1: Register finalizer called on bad address: 9f02d33c-dbd1-4f19-92a7-e7d273809590");
        }
      } else {
        {
          return;
        }
      }
    }
  }
  CGC1_DLL_PUBLIC void cgc_set_abort_on_collect(void *addr, bool abort_on_collect)
  {
    const auto ba_user_data = details::bitmap_allocator_user_data(addr);
    if (nullptr == ba_user_data) {
      if (details::is_sparse_allocator(addr)) {
        throw ::std::runtime_error("cgc1: abort on collect not implemented for sparse allocator");
      } else if (details::is_bitmap_allocator(addr)) {
        throw ::std::runtime_error("cgc1: abort on collect not implemented for bitmap allocator");
      } else {
        throw ::std::runtime_error("cgc1: abort on collect not implemented for unknown allocator");
      }
    } else {
      ba_user_data->set_abort_on_collect(abort_on_collect);
    }
  }
  CGC1_DLL_PUBLIC void cgc_set_uncollectable(void *const addr, const bool is_uncollectable)
  {
    if (nullptr == addr) {
      return;
    }
    const auto ba_user_data = details::bitmap_allocator_user_data(addr);
    if (nullptr != ba_user_data) {
      throw ::std::runtime_error("cgc1: set uncollectable: NOT IMPLEMENTED");
    } else if (details::is_sparse_allocator(addr)) {
      if (nullptr == addr) {
        return;
      }
      void *start = cgc_start(addr);
      if (nullptr == start) {
        return;
      }
      details::gc_sparse_object_state_t *os =
          details::gc_sparse_object_state_t::template from_object_start<details::gc_sparse_object_state_t>(start);
      details::gc_user_data_t *ud = static_cast<details::gc_user_data_t *>(os->user_data());
      if (ud->is_default()) {
        ud = ::mcpputil::make_unique_allocator<details::gc_user_data_t, cgc_internal_allocator_t<void>>(*ud).release();
        ud->set_is_default(false);
        os->set_user_data(ud);
      }
      ud->set_uncollectable(is_uncollectable);
      set_complex(os, true);
    } else {
      throw ::std::runtime_error("cgc1: set uncollectable called on bad address. 3d503975-2d36-4c20-b426-a7c9727508f3");
    }
  }
  CGC1_DLL_PUBLIC void cgc_set_atomic(void *addr, bool is_atomic)
  {
    if (nullptr == addr) {
      return;
    }
    void *start = cgc_start(addr);
    if (nullptr == start) {
      return;
    }
    const auto ba_user_data = details::bitmap_allocator_user_data(addr);
    if (nullptr != ba_user_data) {
      throw ::std::runtime_error("NOT IMPLEMENTED");
    }
    if (!mcpputil_unlikely(details::is_sparse_allocator(addr))) {
      ::std::abort();
    }
    details::gc_sparse_object_state_t *os =
        details::gc_sparse_object_state_t::template from_object_start<details::gc_sparse_object_state_t>(start);
    set_atomic(os, is_atomic);
  }
  void *cgc_malloc_atomic(::std::size_t size_in_bytes)
  {
    return ::cgc1::details::g_gks->allocate_atomic(size_in_bytes).m_ptr;
  }
  void *cgc_malloc_uncollectable(::std::size_t size_in_bytes)
  {
    auto ret = ::cgc1::details::g_gks->allocate_sparse(size_in_bytes).m_ptr;
    cgc1::cgc_set_uncollectable(ret, true);
    return ret;
  }
}
extern "C" {
#include "../include/gc/gc_version.h"
#include <cgc1/gc.h>
CGC1_DLL_PUBLIC void *GC_realloc(void *old_object, ::std::size_t new_size)
{
  return ::cgc1::cgc_realloc(old_object, new_size);
}
CGC1_DLL_PUBLIC void *GC_malloc(::std::size_t size_in_bytes)
{
  return ::cgc1::cgc_malloc(size_in_bytes);
}
CGC1_DLL_PUBLIC void *GC_malloc_atomic(::std::size_t size_in_bytes)
{
  return ::cgc1::cgc_malloc_atomic(size_in_bytes);
}
CGC1_DLL_PUBLIC void *GC_malloc_uncollectable(::std::size_t size_in_bytes)
{
  return ::cgc1::cgc_malloc_uncollectable(size_in_bytes);
}
CGC1_DLL_PUBLIC void GC_free(void *object_addr)
{
  cgc1::cgc_free(object_addr);
}
CGC1_DLL_PUBLIC void GC_init(void *stack_addr)
{
  cgc1::cgc_register_thread(stack_addr);
}
CGC1_DLL_PUBLIC void GC_gcollect()
{
  cgc1::cgc_force_collect();
}
CGC1_DLL_PUBLIC void GC_register_finalizer(void *addr, void (*finalizer)(void *, void *), void *user_data, void *b, void *c)
{
  if (mcpputil_unlikely(b || c)) {
    ::std::cerr << "arguments to register finalizer must be null 06b40398-36e4-47fc-bc07-78817f612e2c\n";
    ::std::terminate();
  }
  auto real_finalizer = [finalizer, user_data](void *ptr) { finalizer(ptr, user_data); };
  cgc1::cgc_register_finalizer(addr, real_finalizer, false);
}
CGC1_DLL_PUBLIC int GC_get_heap_size()
{
  return ::std::numeric_limits<int>::max();
}
CGC1_DLL_PUBLIC int GC_get_gc_no()
{
  return static_cast<int>(::cgc1::debug::num_gc_collections());
}
CGC1_DLL_PUBLIC int GC_get_parallel()
{
  return 1;
}
CGC1_DLL_PUBLIC int GC_get_finalize_on_demand()
{
  return 0;
}
CGC1_DLL_PUBLIC int GC_get_java_finalization()
{
  return 0;
}
CGC1_DLL_PUBLIC int GC_get_dont_expand()
{
  return 0;
}
CGC1_DLL_PUBLIC int GC_get_full_freq()
{
  return 0;
}
CGC1_DLL_PUBLIC int GC_get_max_retries()
{
  return 0;
}
CGC1_DLL_PUBLIC unsigned long GC_get_time_limit()
{
  return ::std::numeric_limits<unsigned long>::max();
}
CGC1_DLL_PUBLIC long GC_get_free_space_divisor()
{
  return 1;
}
CGC1_DLL_PUBLIC long GC_get_all_interior_pointers()
{
  return 0;
}
CGC1_DLL_PUBLIC void *GC_base(void *addr)
{
  return cgc1::cgc_start(addr);
}
CGC1_DLL_PUBLIC int GC_is_visible(void *)
{
  return 1;
}
CGC1_DLL_PUBLIC void *GC_check_annotated_obj(void *)
{
  return nullptr;
}
CGC1_DLL_PUBLIC unsigned GC_get_version()
{
  return static_cast<unsigned>((GC_VERSION_MAJOR << 16) | (GC_VERSION_MINOR << 8));
}
}
