#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include "bandit.hpp"
#include <cgc1/posix_slab.hpp>
#include <cgc1/posix.hpp>
#include <cgc1/aligned_allocator.hpp>
#include "../cgc1/src/slab_allocator.hpp"
#include <thread>
#include <chrono>
#include <string.h>
#include <signal.h>
#include "../cgc1/src/allocator_block.hpp"
#include "../cgc1/src/allocator.hpp"
#include "../cgc1/src/internal_allocator.hpp"
#include "../cgc1/src/global_kernel_state.hpp"
#include "../cgc1/src/internal_stream.hpp"
static ::std::vector<void *> locations;
static ::std::mutex debug_mutex;
using namespace bandit;
namespace cgc1
{
  template <size_t bytes = 5000>
  static _NoInline_ void clean_stack()
  {
    int array[bytes];
    __asm__ __volatile__(
        "xorl %%eax, %%eax\n"
        "xorl %%ebx, %%ebx\n"
        "xorl %%ecx, %%ecx\n"
        "xorl %%edx, %%edx\n"
        "xorl %%esi, %%esi\n"
        "xorl %%edi, %%edi\n"
        "xorl %%r8d, %%r8d\n"
        "xorl %%r9d, %%r9d\n"
        "xorl %%r10d, %%r10d\n"
        "xorl %%r11d, %%r11d\n"
        "xorl %%r12d, %%r12d\n"
        "xorl %%r13d, %%r13d\n"
        "xorl %%r14d, %%r15d\n"
        "xorl %%r15d, %%r15d\n"
	"pxor %%xmm0, %%xmm0\n"
	"pxor %%xmm1, %%xmm1\n"
	"pxor %%xmm2, %%xmm2\n"
	"pxor %%xmm3, %%xmm3\n"
	"pxor %%xmm4, %%xmm4\n"
	"pxor %%xmm5, %%xmm5\n"
	"pxor %%xmm6, %%xmm6\n"
	"pxor %%xmm7, %%xmm7\n"
	"pxor %%xmm8, %%xmm8\n"
	"pxor %%xmm9, %%xmm9\n"
	"pxor %%xmm10, %%xmm10\n"
	"pxor %%xmm11, %%xmm11\n"
	"pxor %%xmm12, %%xmm12\n"
	"pxor %%xmm13, %%xmm13\n"
	"pxor %%xmm14, %%xmm14\n"
	"pxor %%xmm15, %%xmm15\n"
        :
        :
        : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7",
	  "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15");
    cgc1::secure_zero(array, bytes);
  }
}
static _NoInline_ void root_test__setup(void *&memory, size_t &old_memory)
{
  memory = cgc1::cgc_malloc(50);
  old_memory = cgc1::hide_pointer(memory);
  cgc1::cgc_add_root(&memory);
  AssertThat(cgc1::cgc_size(memory), Equals(static_cast<size_t>(64)));
  AssertThat(cgc1::cgc_is_cgc(memory), IsTrue());
  AssertThat(cgc1::cgc_is_cgc(nullptr), IsFalse());
}
static void root_test()
{
  void *memory;
  size_t old_memory;
  root_test__setup(memory, old_memory);
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  AssertThat(last_collect, HasLength(0));
  cgc1::cgc_remove_root(&memory);
  cgc1::secure_zero_pointer(memory);
  auto num_collections = cgc1::debug::num_gc_collections();
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(static_cast<size_t>(1)));
  AssertThat(last_collect[0] == cgc1::unhide_pointer(old_memory), IsTrue());
  AssertThat(cgc1::debug::num_gc_collections(), Equals(num_collections + 1));
}
static _NoInline_ void internal_pointer_test__setup(void *&memory, size_t &old_memory)
{
  memory = cgc1::cgc_malloc(50);
  uint8_t *&umemory = cgc1::unsafe_reference_cast<uint8_t *>(memory);
  old_memory = cgc1::hide_pointer(memory);
  umemory += 1;
  cgc1::cgc_add_root(&memory);
}
static void internal_pointer_test()
{
  void *memory;
  size_t old_memory;
  internal_pointer_test__setup(memory, old_memory);
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  AssertThat(last_collect, HasLength(0));
  cgc1::cgc_remove_root(&memory);
  cgc1::secure_zero_pointer(memory);
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  AssertThat(last_collect, HasLength(1));
  AssertThat(last_collect[0] == cgc1::unhide_pointer(old_memory), IsTrue());
}

static _NoInline_ void atomic_test__setup(void *&memory, size_t &old_memory)
{
  memory = cgc1::cgc_malloc(50);
  cgc1::cgc_add_root(&memory);
  void *memory2 = cgc1::cgc_malloc(50);
  *reinterpret_cast<void **>(memory) = memory2;
  old_memory = cgc1::hide_pointer(memory2);
  cgc1::secure_zero_pointer(memory2);
}

static void atomic_test()
{
  void *memory;
  size_t old_memory;
  atomic_test__setup(memory, old_memory);
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(static_cast<size_t>(0)));
  cgc1::cgc_set_atomic(memory, true);
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  AssertThat(last_collect, HasLength(1));
  AssertThat(last_collect[0] == cgc1::unhide_pointer(old_memory), IsTrue());
  cgc1::cgc_remove_root(&memory);
  cgc1::secure_zero_pointer(memory);
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  AssertThat(last_collect, HasLength(1));
  // test bad parameters
  cgc1::cgc_set_atomic(nullptr, true);
  cgc1::cgc_set_atomic(&old_memory, true);
}
static _NoInline_ void finalizer_test__setup(std::atomic<bool> &finalized, size_t &old_memory)
{
  void *memory = cgc1::cgc_malloc(50);
  old_memory = cgc1::hide_pointer(memory);
  finalized = false;
  cgc1::cgc_register_finalizer(memory, [&finalized](void *) { finalized = true; });
  cgc1::secure_zero_pointer(memory);
}
static void finalizer_test()
{
  size_t old_memory;
  std::atomic<bool> finalized;
  finalizer_test__setup(finalized, old_memory);
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(static_cast<size_t>(1)));
  AssertThat(last_collect[0] == cgc1::unhide_pointer(old_memory), IsTrue());
  AssertThat(static_cast<bool>(finalized), IsTrue());
  // test bad parameters
  cgc1::cgc_register_finalizer(nullptr, [&finalized](void *) { finalized = true; });
  AssertThat(cgc1::cgc_start(nullptr) == nullptr, IsTrue());
  AssertThat(cgc1::cgc_start(&old_memory) == nullptr, IsTrue());
  cgc1::cgc_register_finalizer(&old_memory, [&finalized](void *) { finalized = true; });
}
static _NoInline_ void uncollectable_test__setup(size_t &old_memory)
{
  void *memory = cgc1::cgc_malloc(50);
  old_memory = cgc1::hide_pointer(memory);
  cgc1::cgc_set_uncollectable(memory, true);
  cgc1::secure_zero_pointer(memory);
}
static _NoInline_ void uncollectable_test__cleanup(size_t &old_memory)
{
  cgc1::cgc_set_uncollectable(cgc1::unhide_pointer(old_memory), false);
}
static void uncollectable_test()
{
  size_t old_memory;
  uncollectable_test__setup(old_memory);
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(static_cast<size_t>(0)));
  uncollectable_test__cleanup(old_memory);
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  AssertThat(last_collect.size(), Equals(static_cast<size_t>(1)));
  AssertThat(last_collect[0] == cgc1::unhide_pointer(old_memory), IsTrue());
  // test bad parameters
  cgc1::cgc_set_uncollectable(nullptr, true);
  cgc1::cgc_set_uncollectable(&old_memory, true);
}
static void linked_list_test()
{
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  std::atomic<bool> keep_going{true};
  auto test_thread = [&keep_going]() {
    CGC1_INITIALIZE_THREAD();
    void **foo = reinterpret_cast<void **>(cgc1::cgc_malloc(100));
    {
      void **bar = foo;
      for (int i = 0; i < 3000; ++i) {
        {
          CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
          locations.push_back(bar);
        }
        cgc1::secure_zero(bar, 100);
        *bar = cgc1::cgc_malloc(100);
        bar = reinterpret_cast<void **>(*bar);
      }
      {
        CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
        locations.push_back(bar);
      }
    }
    while (keep_going) {
      ::std::stringstream ss;
      ss << foo << ::std::endl;
    }
    cgc1::cgc_unregister_thread();
  };
  ::std::thread t1(test_thread);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  for (int i = 0; i < 100; ++i) {
    cgc1::cgc_force_collect();
    cgc1::details::g_gks.wait_for_finalization();
    auto freed_last = cgc1::details::g_gks._d_freed_in_last_collection();
    assert(freed_last.empty());
    AssertThat(freed_last, HasLength(0));
  }
  keep_going = false;
  t1.join();
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  ::std::sort(locations.begin(), locations.end());
  ::std::sort(last_collect.begin(), last_collect.end());
  for (void *v : locations) {
    assert(::std::find(last_collect.begin(), last_collect.end(), v) != last_collect.end());
    AssertThat(::std::find(last_collect.begin(), last_collect.end(), v) != last_collect.end(), IsTrue());
  }
  locations.clear();
}
static void race_condition_test()
{
  ::std::atomic<bool> keep_going{true};
  auto test_thread = [&keep_going]() {
    cgc1::clean_stack();
    CGC1_INITIALIZE_THREAD();
    char *foo = reinterpret_cast<char *>(cgc1::cgc_malloc(100));
    cgc1::secure_zero(foo, 100);
    {
      CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
      locations.push_back(foo);
    }
    while (keep_going) {
      ::std::stringstream ss;
      ss << foo << ::std::endl;
    };

    foo = nullptr;
    {
      CGC1_CONCURRENCY_LOCK_GUARD(debug_mutex);
      for (int i = 0; i < 5000; ++i) {
        foo = reinterpret_cast<char *>(cgc1::cgc_malloc(100));
        cgc1::secure_zero(foo, 100);
        locations.push_back(foo);
      }
    }
    cgc1::cgc_unregister_thread();
    cgc1::clean_stack();
  };
  ::std::thread t1(test_thread);
  ::std::thread t2(test_thread);
  for (int i = 0; i < 0; ++i) {
    cgc1::cgc_force_collect();
    cgc1::details::g_gks.wait_for_finalization();
    auto freed_last = cgc1::details::g_gks._d_freed_in_last_collection();
    assert(freed_last.empty());
    AssertThat(freed_last, HasLength(0));
  }
  keep_going = false;
  t1.join();
  t2.join();
  cgc1::cgc_force_collect();
  cgc1::details::g_gks.wait_for_finalization();
  auto last_collect = cgc1::details::g_gks._d_freed_in_last_collection();
  ::std::sort(locations.begin(), locations.end());
  ::std::sort(last_collect.begin(), last_collect.end());
  for (void *v : locations) {
    bool found = ::std::find(last_collect.begin(), last_collect.end(), v) != last_collect.end();
    assert(found);
    AssertThat(found, IsTrue());
  }
  locations.clear();
}
static void api_tests()
{
  AssertThat(cgc1::cgc_heap_size(), Is().GreaterThan(static_cast<size_t>(0)));
  AssertThat(cgc1::cgc_heap_free(), Is().GreaterThan(static_cast<size_t>(0)));
  cgc1::cgc_disable();
  AssertThat(cgc1::cgc_is_enabled(), Is().False());
  cgc1::cgc_enable();
  AssertThat(cgc1::cgc_is_enabled(), Is().True());
}
void gc_bandit_tests()
{
  describe("GC", []() {
    for (size_t i = 0; i < 10; ++i) {
      it("race condition", []() { cgc1::clean_stack(); race_condition_test();       cgc1::clean_stack();});
    }
    it("linked list test", []() { cgc1::clean_stack(); linked_list_test();       cgc1::clean_stack();});
    it("root", []() { cgc1::clean_stack(); root_test();       cgc1::clean_stack();});
    it("internal pointer", []() { cgc1::clean_stack(); internal_pointer_test();       cgc1::clean_stack();});
    it("finalizers", []() { cgc1::clean_stack(); finalizer_test();       cgc1::clean_stack();});
    it("atomic", []() { cgc1::clean_stack(); atomic_test();       cgc1::clean_stack();});
    it("uncollectable", []() { cgc1::clean_stack(); uncollectable_test();       cgc1::clean_stack();});
    it("api_tests", []() { cgc1::clean_stack(); api_tests();       cgc1::clean_stack();});
    });
}
