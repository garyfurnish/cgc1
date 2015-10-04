#include "internal_declarations.hpp"
#include "global_kernel_state.hpp"
#include "thread_local_kernel_state.hpp"
#include <cgc1/cgc1.hpp>
#include <cstring>
#include <cgc1/cgc1_dll.hpp>
namespace cgc1
{
  namespace details
  {
    unique_ptr_malloc_t<global_kernel_state_t> g_gks;
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
  }
  namespace debug
  {
    size_t num_gc_collections() noexcept
    {
      return details::g_gks->num_collections();
    }

    auto _cgc_hidden_packed_marked(uintptr_t loc) -> bool
    {
      auto state = cgc1::details::get_state(cgc1::unhide_pointer(loc));
      auto index = state->get_index(cgc1::unhide_pointer(loc));
      return state->is_marked(index);
    }
    auto _cgc_hidden_packed_free(uintptr_t loc) -> bool
    {
      auto state = cgc1::details::get_state(cgc1::unhide_pointer(loc));
      auto index = state->get_index(cgc1::unhide_pointer(loc));
      return state->is_free(index);
    }
  }
  bool in_signal_handler() noexcept
  {
    auto tlks = details::get_tlks();
    if (tlks)
      return tlks->in_signal_handler();
    return false;
  }

  void *cgc_malloc(size_t sz)
  {
    auto &ta = details::g_gks->gc_allocator().initialize_thread();
    return ta.allocate(sz);
  }
  uintptr_t cgc_hidden_malloc(size_t sz)
  {
    void *addr = cgc_malloc(sz);
    secure_zero(addr, sz);
    return hide_pointer(addr);
  }
  void *cgc_realloc(void *v, size_t sz)
  {
    void *ret = cgc_malloc(sz);
    if (v)
      ::memcpy(ret, v, sz);
    return ret;
  }
  void cgc_free(void *v)
  {
    auto &ta = details::g_gks->gc_allocator().initialize_thread();
    ta.destroy(v);
  }
  bool cgc_is_cgc(void *v)
  {
    return cgc_size(v) > 0;
  }
  void *cgc_start(void *addr)
  {
    if (!addr)
      return nullptr;
    if (addr >= details::g_gks->fast_slab_begin() && addr < details::g_gks->fast_slab_end()) {
      auto state = details::get_state(addr);
      if (state->has_valid_magic_numbers())
        return state->begin() + state->get_index(addr) * state->real_entry_size();
      return nullptr;
    }
    details::object_state_t *os = details::object_state_t::from_object_start(addr);
    if (!details::g_gks->is_valid_object_state(os)) {
      os = details::g_gks->find_valid_object_state(addr);
      if (!os)
        return nullptr;
    }
    return os->object_start();
  }
  size_t cgc_size(void *addr)
  {
    if (!addr)
      return 0;
    void *start = cgc_start(addr);
    if (!start)
      return 0;
    if (start >= details::g_gks->fast_slab_begin() && start < details::g_gks->fast_slab_end()) {
      auto state = details::get_state(addr);
      if (state->has_valid_magic_numbers())
        return state->declared_entry_size();
      return 0;
    }
    details::object_state_t *os = details::object_state_t::from_object_start(start);
    if (os)
      return os->object_size();
    else
      return 0;
  }
  void cgc_add_root(void **v)
  {
    details::g_gks->add_root(v);
  }
  void cgc_remove_root(void **v)
  {
    details::g_gks->remove_root(v);
  }
  size_t cgc_heap_size()
  {
    // this cast is safe because end > begin is an invariant.
    return static_cast<size_t>(details::g_gks->gc_allocator().end() - details::g_gks->gc_allocator().begin());
  }
  size_t cgc_heap_free()
  {
    // this cast is safe because end > current_end is an invariant.
    return static_cast<size_t>(details::g_gks->gc_allocator().end() - details::g_gks->gc_allocator().current_end());
  }
  void cgc_enable()
  {
    details::g_gks->enable();
  }
  void cgc_disable()
  {
    details::g_gks->disable();
  }
  bool cgc_is_enabled()
  {
    return details::g_gks->enabled();
  }
  void cgc_register_thread(void *top_of_stack)
  {
    if (!details::g_gks)
      details::g_gks = make_unique_malloc<details::global_kernel_state_t>();
    details::g_gks->initialize_current_thread(top_of_stack);
  }
  void cgc_collect()
  {
    details::g_gks->collect();
  }
  void cgc_force_collect()
  {
    details::g_gks->force_collect();
  }
  void cgc_unregister_thread()
  {
    details::g_gks->destroy_current_thread();
  }
  void cgc_shutdown()
  {
    details::g_gks->shutdown();
  }
  void cgc_register_finalizer(void *addr, ::std::function<void(void *)> finalizer)
  {
    if (!addr)
      return;
    void *start = cgc_start(addr);
    if (!start)
      return;
    details::object_state_t *os = details::object_state_t::from_object_start(start);
    details::gc_user_data_t *ud = static_cast<details::gc_user_data_t *>(os->user_data());
    if (ud->m_is_default) {
      ud = make_unique_allocator<details::gc_user_data_t, cgc_internal_allocator_t<void>>(*ud).release();
      ud->m_is_default = false;
      os->set_user_data(ud);
    }
    ud->m_finalizer = finalizer;
  }
  void cgc_set_uncollectable(void *addr, bool is_uncollectable)
  {
    if (!addr)
      return;
    void *start = cgc_start(addr);
    if (!start)
      return;
    details::object_state_t *os = details::object_state_t::from_object_start(start);
    details::gc_user_data_t *ud = static_cast<details::gc_user_data_t *>(os->user_data());
    if (ud->m_is_default) {
      ud = make_unique_allocator<details::gc_user_data_t, cgc_internal_allocator_t<void>>(*ud).release();
      ud->m_is_default = false;
      os->set_user_data(ud);
    }
    ud->m_uncollectable = is_uncollectable;
    set_complex(os, true);
  }
  void cgc_set_atomic(void *addr, bool is_atomic)
  {
    if (!addr)
      return;
    void *start = cgc_start(addr);
    if (!start)
      return;
    details::object_state_t *os = details::object_state_t::from_object_start(start);
    set_atomic(os, is_atomic);
  }
}
extern "C" {
CGC1_DLL_PUBLIC void *GC_realloc(void *old_object, ::std::size_t new_size)
{
  return cgc1::cgc_realloc(old_object, new_size);
}
CGC1_DLL_PUBLIC void *GC_malloc(::std::size_t size_in_bytes)
{
  return cgc1::cgc_malloc(size_in_bytes);
}
CGC1_DLL_PUBLIC void *GC_malloc_atomic(::std::size_t size_in_bytes)
{
  auto ret = cgc1::cgc_malloc(size_in_bytes);
  cgc1::cgc_set_atomic(ret, true);
  return ret;
}
CGC1_DLL_PUBLIC void *GC_malloc_uncollectable(::std::size_t size_in_bytes)
{
  auto ret = cgc1::cgc_malloc(size_in_bytes);
  cgc1::cgc_set_uncollectable(ret, true);
  return ret;
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
  if (b || c) {
    ::std::cerr << "arguments to register finalizer must be null\n";
    abort();
  }
  auto real_finalizer = [finalizer, user_data](void *ptr) { finalizer(ptr, user_data); };
  cgc1::cgc_register_finalizer(addr, real_finalizer);
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
  return true;
}
CGC1_DLL_PUBLIC int GC_get_finalize_on_demand()
{
  return false;
}
CGC1_DLL_PUBLIC int GC_get_java_finalization()
{
  return false;
}
CGC1_DLL_PUBLIC int GC_get_dont_expand()
{
  return false;
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
}
