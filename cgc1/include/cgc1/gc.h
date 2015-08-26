#pragma once
extern "C"
{
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
   **/
  extern void GC_init();
}
#define GC_INIT() GC_init();
#define GC_MALLOC(sz) GC_malloc(sz)
#define GC_MALLOC_ATOMIC(sz) GC_malloc_atomic(sz)
#define GC_MALLOC_UNCOLLECTABLE(sz) GC_malloc_uncollectable(sz)
#define GC_FREE(p) GC_free(p)
#define GC_REALLOC(old, sz) GC_realloc(old, sz)
#define GC_NEW(t)               ((t*)GC_MALLOC(sizeof(t)))
#define GC_NEW_ATOMIC(t)        ((t*)GC_MALLOC_ATOMIC(sizeof(t)))
#define GC_NEW_STUBBORN(t)      ((t*)GC_MALLOC_STUBBORN(sizeof(t)))
#define GC_NEW_UNCOLLECTABLE(t) ((t*)GC_MALLOC_UNCOLLECTABLE(sizeof(t)))
