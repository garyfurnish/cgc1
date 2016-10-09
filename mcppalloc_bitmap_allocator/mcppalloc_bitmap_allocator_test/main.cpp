#include <mcpputil/mcpputil/declarations.hpp>
// This Must be first.
#include <mcpputil/mcpputil/bandit.hpp>
#ifdef _WIN32
#pragma optimize("", off)
#endif

using namespace bandit;
extern void bitmap_allocator_tests();
go_bandit([]() { bitmap_allocator_tests(); });

int main(int argc, char *argv[])
{
  auto ret = bandit::run(argc, argv);
#ifdef _WIN32
  ::std::cin.get();
#endif
  return ret;
}
