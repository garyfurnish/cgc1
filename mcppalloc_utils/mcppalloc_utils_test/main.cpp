#include <mcppalloc/mcppalloc_utils/backed_ordered_map.hpp>
#include <mcppalloc/mcppalloc_utils/bandit.hpp>
#include <mcppalloc/mcppalloc_utils/literals.hpp>
using namespace bandit;
using namespace ::mcppalloc::literals;
go_bandit([]() {
  describe("backed_ordered_multimap", []() {
    using bom_t = ::mcppalloc::containers::backed_ordered_multimap<size_t, size_t>;
    it("backed_ordered_multimap_test0", []() {
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
      AssertThat(map.find(3_sz), Equals(map.begin() + 3));
      AssertThat(map.find(88_sz), Equals(map.end()));
      AssertThat(map.erase(3_sz), Equals(1_sz));
      AssertThat(map.find(3_sz), Equals(map.end()));

      ret = map.insert(::std::make_pair(3, 2));
      AssertThat(ret.first, Equals(map.begin() + 3));
      AssertThat(ret.second, IsTrue());
      ::std::vector<::std::pair<size_t, size_t>> t1{{0, 1}, {1, 2}, {2, 3}, {3, 2}, {4, 1}, {5, 0}};
      AssertThat(::std::equal(map.begin(), map.end(), t1.begin()), IsTrue());
      map.move(0, {4, 3});
      ::std::vector<::std::pair<size_t, size_t>> t2{{1, 2}, {2, 3}, {3, 2}, {4, 1}, {4, 3}, {5, 0}};
      AssertThat(::std::equal(map.begin(), map.end(), t2.begin()), IsTrue());
      map.move(3, {0, 50});
      ::std::vector<::std::pair<size_t, size_t>> t3{{0, 50}, {1, 2}, {2, 3}, {4, 1}, {4, 3}, {5, 0}};
      AssertThat(::std::equal(map.begin(), map.end(), t3.begin()), IsTrue());
      map.move(0, {0, 51});
      ::std::vector<::std::pair<size_t, size_t>> t4{{0, 51}, {1, 2}, {2, 3}, {4, 1}, {4, 3}, {5, 0}};
      AssertThat(::std::equal(map.begin(), map.end(), t4.begin()), IsTrue());
      /*      for (auto &&obj : map) {
        ::std::cout << "(" << obj.first << "," << obj.second << ")";
      }
      ::std::cout << "\n";*/
    });
    it("backed_ordered_multimap_test1", []() {
      uint8_t array[31];
      bom_t map(array, sizeof(array));
      AssertThat(map.capacity(), Equals(1_sz));
      auto ret = map.insert(::std::make_pair(5, 0));
      AssertThat(map.size(), Equals(1_sz));
      AssertThat(ret.second, IsTrue());
      ret = map.insert(::std::make_pair(5, 1));
      AssertThat(map.size(), Equals(1_sz));
      AssertThat(ret.second, IsFalse());
      ::std::vector<::std::pair<size_t, size_t>> t1{{5, 0}, {5, 1}};
      AssertThat(::std::equal(map.begin(), map.end(), t1.begin()), IsTrue());

    });
    it("backed_ordered_multimap_test1", []() {
      size_t array[500];
      bom_t map(array, sizeof(array));
      auto ret = map.insert(::std::make_pair(5, 0));
      ret = map.insert(::std::make_pair(5, 1));
      ::std::vector<::std::pair<size_t, size_t>> t1{{5, 0}, {5, 1}};
      AssertThat(::std::equal(map.begin(), map.end(), t1.begin()), IsTrue());

    });
    it("backed_ordered_multimap_test2", []() {
      size_t array[500];
      bom_t map(array, sizeof(array));
      map.insert(::std::make_pair(0, 5));
      AssertThat(map.lower_bound(0), Equals(map.begin()));
      AssertThat(map.find(0), Equals(map.begin()));

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
