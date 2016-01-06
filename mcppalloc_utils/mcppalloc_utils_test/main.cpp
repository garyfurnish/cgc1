#include <mcppalloc/mcppalloc_utils/backed_ordered_map.hpp>
#include <mcppalloc/mcppalloc_utils/bandit.hpp>
#include <mcppalloc/mcppalloc_utils/literals.hpp>
using namespace bandit;
using namespace ::mcppalloc::literals;
go_bandit([]() {
  describe("backed_ordered_map", []() {
    using bom_t = ::mcppalloc::containers::backed_ordered_map<size_t, size_t>;
    it("backed_ordered_map_test0", []() {
      size_t array[500];
      bom_t map(array, sizeof(array));
      AssertThat(map.size(), Equals(0_sz));
      auto ret = map.insert(::std::make_pair(5, 0));
      AssertThat(map.size(), Equals(1_sz));
      AssertThat(ret.first, Equals(map.begin()));
      AssertThat(ret.second, IsTrue());
      ret = map.insert(::std::make_pair(4, 1));

      AssertThat(map.size(), Equals(2_sz));
      AssertThat(ret.first, Equals(map.begin()));
      AssertThat(ret.second, IsTrue());
      ret = map.insert(::std::make_pair(3, 2));

      AssertThat(map.size(), Equals(3_sz));
      AssertThat(ret.first, Equals(map.begin()));
      AssertThat(ret.second, IsTrue());
      ret = map.insert(::std::make_pair(2, 3));
      AssertThat(map.size(), Equals(4_sz));
      AssertThat(ret.first, Equals(map.begin()));
      AssertThat(ret.second, IsTrue());
      ret = map.insert(::std::make_pair(1, 2));

      AssertThat(map.size(), Equals(5_sz));
      AssertThat(ret.first, Equals(map.begin()));
      AssertThat(ret.second, IsTrue());
      ret = map.insert(::std::make_pair(0, 1));

      AssertThat(map.size(), Equals(6_sz));
      AssertThat(ret.first, Equals(map.begin()));
      AssertThat(ret.second, IsTrue());
    });

  });
});

int main(int argc, char *argv[])
{
  auto ret = bandit::run(argc, argv);
#ifdef _WIN32
  ::std::cin.get();
#endif
  return ret;
}
