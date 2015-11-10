#pragma once
#include "cgc1_dll.hpp"
#include <stdint.h>
#define CGC1_FAKE_BOEHM
#define USE_CGC1
#ifndef _WIN32
#ifndef cgc1_builtin_current_stack
#define cgc1_builtin_current_stack() __builtin_frame_address(0)
#endif
#else
#ifndef cgc1_builtin_current_stack
#define cgc1_builtin_current_stack() _AddressOfReturnAddress()
#endif
#endif
#ifdef __cplusplus
extern "C"
{
#endif
  typedef void (*GC_finalization_proc)(void * /* obj */,
						    void * /* client_data */);
  /**
   * \brief Reallocate memory.
   *
   * May copy if can not realloc at same location
   * Not required to shrink.
   * @param old_object Old object.
   * @param new_size New size of object.
   **/
  extern void* GC_realloc(void* old_object, size_t new_size);
  /**
   * \brief Allocate memory.
   **/
  extern void* GC_malloc(size_t size_in_bytes);
  /**
   * \brief Allocate atomic memory.
   *
   * Atomic memory is memory that is not searched for pointers.
   * @param size_in_bytes Size of memory.
   **/
  extern void* GC_malloc_atomic(size_t size_in_bytes);
  /**
   * \brief Allocate uncollectable memory.
   *
   * @param size_in_bytes Size of memory.
   **/
  extern void* GC_malloc_uncollectable(size_t size_in_bytes);
  /**
   * \brief Explicitly free garbage collected memory.
   *
   * @param object_addr Adddress to free.
   **/
  extern void GC_free(void* object_addr);
  /**
   * \brief Initialize garbage collector.
   * 
   * @param current_stack Current top of stack.
   **/
  extern void GC_init(void* current_stack);
  /**
   * \brief Force garbage collection.
   **/
  extern void GC_gcollect();
  /**
   * \brief Register a finalizer.
   **/
  extern void GC_register_finalizer(void *addr, GC_finalization_proc finalizer, void* user_data, void* b, void* c);
  extern void* GC_base(void* addr);
  extern int GC_get_heap_size();
  extern int GC_get_gc_no();
  extern int GC_get_parallel();
  extern int GC_get_finalize_on_demand();
  extern int GC_get_java_finalization();
  extern int GC_get_dont_expand();
  extern int GC_get_full_freq();
  extern int GC_get_max_retries();
  extern unsigned long GC_get_time_limit();
  extern long GC_get_free_space_divisor();
  extern long GC_get_all_interior_pointers();
  extern int GC_is_visible(void* addr);
  extern void *GC_check_annotated_obj(void *);
#ifdef __cplusplus
}
#endif
#define GC_INIT() GC_init(cgc1_builtin_current_stack())
#define GC_MALLOC(sz) GC_malloc(sz)
#define GC_MALLOC_ATOMIC(sz) GC_malloc_atomic(sz)
#define GC_MALLOC_UNCOLLECTABLE(sz) GC_malloc_uncollectable(sz)
#define GC_FREE(p) GC_free(p)
#define GC_REALLOC(old, sz) GC_realloc(old, sz)
#define GC_NEW(t)               ((t*)GC_MALLOC(sizeof(t)))
#define GC_NEW_ATOMIC(t)        ((t*)GC_MALLOC_ATOMIC(sizeof(t)))
#define GC_NEW_STUBBORN(t)      ((t*)GC_MALLOC_STUBBORN(sizeof(t)))
#define GC_NEW_UNCOLLECTABLE(t) ((t*)GC_MALLOC_UNCOLLECTABLE(sizeof(t)))
# define GC_REGISTER_FINALIZER(p, f, d, of, od) \
      GC_register_finalizer(p, f, d, of, od)
# define GC_REGISTER_FINALIZER_IGNORE_SELF(p, f, d, of, od) \
      GC_register_finalizer_ignore_self(p, f, d, of, od)

#define GC_register_finalizer_ignore_self GC_register_finalizer
#define GC_CALLBACK
