#include <cgc1/cgc1.hpp>
#include <mcpputil/mcpputil/bandit.hpp>
#include <mcpputil/mcpputil/thread_id_manager.hpp>

using namespace bandit;
extern void gc_bandit_tests();
extern void gc_bitmap_tests();

go_bandit([]() {
  gc_bitmap_tests();
  gc_bandit_tests();
});

int main(int argc, char *argv[])
{
  auto manager = ::std::make_unique<mcpputil::thread_id_manager_t>();
  manager->set_max_threads(100);
  manager->set_max_tls_pointers(10);
  manager.release();
  CGC1_INITIALIZE_THREAD();
  auto ret = bandit::run(argc, argv);
#ifdef _WIN32
  ::std::cin.get();
#endif
  cgc1::cgc_shutdown();
  return ret;
}
