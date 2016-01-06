#include <cgc1/cgc1.hpp>
#include <mcppalloc/mcppalloc_utils/bandit.hpp>

using namespace bandit;
extern void gc_bandit_tests();
extern void gc_bitmap_tests();

go_bandit([]() {
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
