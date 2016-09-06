#pragma once
#include "cgc1_dll.hpp"
#include "declarations.hpp"
#include <array>
#include <functional>
#include <type_traits>
#include <vector>
namespace cgc1
{
  /**
   * \brief Standard library compatible gc allocator.
  **/
  template <class T>
  class cgc_allocator_t;
  /**
   * \brief Allocate sz bytes.
   *
   * Guaranteed to be 16 byte aligned.
  **/
  extern CGC1_DLL_PUBLIC void *cgc_malloc(size_t sz);
  /**
   * \brief Allocate sz bytes.
   *
   * Guaranteed to be 16 byte aligned.
  **/
  extern CGC1_DLL_PUBLIC uintptr_t cgc_hidden_malloc(size_t sz);
  extern void *cgc_malloc_atomic(::std::size_t size_in_bytes);
  extern void *cgc_malloc_uncollectable(::std::size_t size_in_bytes);
  /**
   * \brief Realloc sz bytes.
   *
   * Guaranteed to be 16 byte aligned.
  **/
  extern CGC1_DLL_PUBLIC void *cgc_realloc(void *v, size_t sz);
  /**
   * \brief Free a cgc pointer.
  **/
  extern CGC1_DLL_PUBLIC void cgc_free(void *v);
  /**
   * \brief Return true if the pointer is a gc object, false otherwise.
  **/
  extern CGC1_DLL_PUBLIC bool cgc_is_cgc(void *v);
  /**
   * \brief Return the size of a gc object.
   *
   * May be larger than the amount requested.
  **/
  extern CGC1_DLL_PUBLIC size_t cgc_size(void *v);
  /**
   * \brief Return the start of an allocated block of memory.
   *
   * @return Return nullptr on error.
  **/
  extern CGC1_DLL_PUBLIC void *cgc_start(void *v);
  /**
   * \brief Add a root to scan.
  **/
  extern CGC1_DLL_PUBLIC void cgc_add_root(void **v);
  /**
   * \brief Remove a root to scan.
  **/
  extern CGC1_DLL_PUBLIC void cgc_remove_root(void **v);
  /**
   * \brief Return true if pointer is root, false otherwise.
   **/
  extern CGC1_DLL_PUBLIC bool cgc_has_root(void **v);
  /**
   * \brief Add a root to scan.
  **/
  template <typename T>
  void cgc_add_root(T **v)
  {
    cgc_add_root(static_cast<void **>(v));
  }
  /**
   * \brief Remove a root to scan.
  **/
  template <typename T>
  void cgc_remove_root(T **v)
  {
    cgc_remove_root(static_cast<void **>(v));
  }
  /**
   * \brief Return the current heap size.
  **/
  extern CGC1_DLL_PUBLIC size_t cgc_heap_size();
  /**
   * \brief Return the heap free.
  **/
  extern CGC1_DLL_PUBLIC size_t cgc_heap_free();
  /**
   * \brief Enable garbage collection.
  **/
  extern CGC1_DLL_PUBLIC void cgc_enable();
  /**
   * \brief Disable garbage collection.
  **/
  extern CGC1_DLL_PUBLIC void cgc_disable();
  /**
   * \brief Return true if CGC is enabled, false otherwise.
  **/
  extern CGC1_DLL_PUBLIC bool cgc_is_enabled();
  /**
   * \brief Return a vector of num new T's.
  **/
  template <typename T>
  ::std::vector<T *, cgc_allocator_t<T>> make_cgc_many(size_t num);
  /**
   * \brief Register a thread
   *
   * @param top_of_stack Highest position to search in a gc.
  **/
  extern CGC1_DLL_PUBLIC void cgc_register_thread(void *top_of_stack);
  /**
   * \brief Register a finalizer.
   * @param addr Start of memory to register finalizer for.
   * @param finalizer Finalizer for memory.
   * @param allow_arbitrary_finalizer_thread Allow finalizer to run in an arbitrary background gc thread instead of a user created
   *thread.
  **/
  extern CGC1_DLL_PUBLIC void cgc_register_finalizer(void *addr,
                                                     ::std::function<void(void *)> finalizer,
                                                     bool allow_arbitrary_finalizer_thread = false,
                                                     bool throws = true);
  /**
   * \brief Set if a given address is uncollectable.
  **/
  extern CGC1_DLL_PUBLIC void cgc_set_uncollectable(void *addr, bool is_uncollectable);
  /**
   * \brief Set if a given address is atomic.
  **/
  extern CGC1_DLL_PUBLIC void cgc_set_atomic(void *addr, bool is_atomic);
  /**
   * \brief Shutdown the garbage collector.  Can not be undone.
  **/
  extern CGC1_DLL_PUBLIC void cgc_shutdown();
  /**
   * \brief Unregister a gc thread.
  **/
  extern CGC1_DLL_PUBLIC void cgc_unregister_thread();
  /**
   * \brief Trigger a collection.
  **/
  extern CGC1_DLL_PUBLIC void cgc_collect();
  /**
   * \brief Force a collection.
   * @param do_local_finalization Should local finalization be performed.
   **/
  extern CGC1_DLL_PUBLIC void cgc_force_collect(bool do_local_finalization = true);
  /**
   * \brief Wait for collection to finish.
  **/
  extern CGC1_DLL_PUBLIC void cgc_wait_collect();
  /**
   * \brief Wait for finalization to finsih.
   **/
  extern CGC1_DLL_PUBLIC void cgc_wait_finalization(bool do_local_finalization = true);
  /**
   * \brief Set if program should abort if this object is collected.
   **/
  extern CGC1_DLL_PUBLIC void cgc_set_abort_on_collect(void *v, bool abort_on_collect);
  namespace debug
  {
    /**
     * \brief Return the number of times the garbage collector has run.
     * May roll over.
    **/
    size_t num_gc_collections() noexcept;
    /**
     * \brief Return if a hidden pointer derived from a packed allocation is marked.
     * This is not considered a stable API.
     **/
    auto _cgc_hidden_packed_marked(uintptr_t loc) -> bool;
    /**
     * \brief Return if a hidden pointer derived from a packed allocation is free.
     * This is not considered a stable API.
     **/
    auto _cgc_hidden_packed_free(uintptr_t loc) -> bool;
  }
  namespace details
  {
    extern int _is_bitmap_addr_markable(void *addr, bool do_mark = false, bool force_mark = false);
    extern bool is_bitmap_allocator(void *addr) noexcept;
    extern bool is_sparse_allocator(void *addr) noexcept;
  }
}
#define CGC1_INITIALIZE_THREAD(...) cgc1::cgc_register_thread(mcpputil_builtin_current_stack())
#include "cgc_root.hpp"
#include "cgc_root_pointer.hpp"
#include "gc_allocator.hpp"
