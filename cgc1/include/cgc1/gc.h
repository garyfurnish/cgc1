#pragma once
#include "cgc1_dll.hpp"
#include <stdint.h>
#include <stddef.h>
#define CGC1_FAKE_BOEHM
#define USE_CGC1
#ifndef _WIN32
#ifndef mcppalloc_builtin_current_stack
#define mcppalloc_builtin_current_stack() __builtin_frame_address(0)
#endif
#else
#ifndef mcppalloc_builtin_current_stack
#define mcppalloc_builtin_current_stack() _AddressOfReturnAddress()
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
  CGC1_DLL_PUBLIC extern void* GC_realloc(void* old_object, size_t new_size);
  /**
   * \brief Allocate memory.
   **/
  CGC1_DLL_PUBLIC extern void* GC_malloc(size_t size_in_bytes);
  /**
   * \brief Allocate atomic memory.
   *
   * Atomic memory is memory that is not searched for pointers.
   * @param size_in_bytes Size of memory.
   **/
  CGC1_DLL_PUBLIC extern void* GC_malloc_atomic(size_t size_in_bytes);
  /**
   * \brief Allocate uncollectable memory.
   *
   * @param size_in_bytes Size of memory.
   **/
  CGC1_DLL_PUBLIC extern void* GC_malloc_uncollectable(size_t size_in_bytes);
  /**
   * \brief Explicitly free garbage collected memory.
   *
   * @param object_addr Adddress to free.
   **/
  CGC1_DLL_PUBLIC extern void GC_free(void* object_addr);
  /**
   * \brief Initialize garbage collector.
   * 
   * @param current_stack Current top of stack.
   **/
  CGC1_DLL_PUBLIC extern void GC_init(void* current_stack);
  /**
   * \brief Force garbage collection.
   **/
  CGC1_DLL_PUBLIC extern void GC_gcollect();
  /**
   * \brief Register a finalizer.
   **/
  CGC1_DLL_PUBLIC extern void GC_register_finalizer(void *addr, GC_finalization_proc finalizer, void* user_data, void* b, void* c);
  CGC1_DLL_PUBLIC extern void* GC_base(void* addr);
  CGC1_DLL_PUBLIC extern int GC_get_heap_size();
  CGC1_DLL_PUBLIC extern int GC_get_gc_no();
  CGC1_DLL_PUBLIC extern int GC_get_parallel();
  CGC1_DLL_PUBLIC extern int GC_get_finalize_on_demand();
  CGC1_DLL_PUBLIC extern int GC_get_java_finalization();
  CGC1_DLL_PUBLIC extern int GC_get_dont_expand();
  CGC1_DLL_PUBLIC extern int GC_get_full_freq();
  CGC1_DLL_PUBLIC  extern int GC_get_max_retries();
  CGC1_DLL_PUBLIC  extern unsigned long GC_get_time_limit();
  CGC1_DLL_PUBLIC  extern long GC_get_free_space_divisor();
  CGC1_DLL_PUBLIC  extern long GC_get_all_interior_pointers();
  CGC1_DLL_PUBLIC  extern int GC_is_visible(void* addr);
  CGC1_DLL_PUBLIC  extern void *GC_check_annotated_obj(void *);
#ifdef __cplusplus
}
#endif
#define GC_INIT() (GC_init(mcppalloc_builtin_current_stack()))
#define GC_MALLOC(sz) (GC_malloc(sz))
#define GC_MALLOC_ATOMIC(sz) (GC_malloc_atomic(sz))
#define GC_MALLOC_UNCOLLECTABLE(sz) (GC_malloc_uncollectable(sz))
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
