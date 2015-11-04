#include <cgc1/cgc1.hpp>
#include <mcppalloc_utils/bandit.hpp>
#ifdef _WIN32
#pragma optimize("", off)
#endif

using namespace bandit;
extern void gc_bandit_tests();
extern void gc_bitmap_tests();
extern void slab_allocator_bandit_tests();

go_bandit([]() {
  slab_allocator_bandit_tests();
  gc_bitmap_tests();
  gc_bandit_tests();
});

int main(int argc, char *argv[])
{
  CGC1_INITIALIZE_THREAD();
  auto ret = bandit::run(argc, argv);
#ifdef _WIN32
  ::std::cin.get();
#endif
  cgc1::cgc_shutdown();
  return ret;
}
