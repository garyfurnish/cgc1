#pragma once
extern "C"
{
  extern void* GC_realloc(void* old_object, size_t new_size);
  extern void* GC_malloc(size_t size_in_bytes);
  extern void* GC_malloc_atomic(size_t size_in_bytes);
  extern void* GC_malloc_uncollectable(size_t size_in_bytes);
  extern void GC_free(void* object_addr);
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