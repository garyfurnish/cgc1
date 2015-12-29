#include "../cgc1/src/internal_allocator.hpp"
#include <mcppalloc/mcppalloc_slab_allocator/slab_allocator.hpp>
#include <mcppalloc/mcppalloc_utils/bandit.hpp>
#include <mcppalloc/mcppalloc_utils/container.hpp>
#include <mcppalloc/mcppalloc_utils/unsafe_cast.hpp>
using namespace bandit;
using ::mcppalloc::unsafe_cast;
using ::mcppalloc::align;
void slab_allocator_bandit_tests()
{
  describe("Slab Allocator", []() {
    it("sa_test1", []() {
      using ::mcppalloc::slab_allocator::details::slab_allocator_object_t;
      ::mcppalloc::slab_allocator::details::slab_allocator_t slab(500000, 5000000);
      using slab_type = decltype(slab);
      uint8_t *alloc1 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      AssertThat(alloc1 == slab.begin() + slab_type::cs_header_sz, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1, slab_type::alignment())->next() ==
                     &*slab._u_object_current_end(),
                 IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1, slab_type::alignment())->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1, slab_type::alignment())->next()->next_valid(), IsFalse());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1, slab_type::alignment())->next()->not_available(), IsFalse());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1, slab_type::alignment())->not_available(), IsTrue());

      uint8_t *alloc2 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      AssertThat(static_cast<uintptr_t>(alloc2 - slab.begin()),
                 Equals(2 * slab_type::cs_header_sz + align(100, slab_type::alignment())));
      AssertThat(slab_allocator_object_t::from_object_start(alloc2, slab_type::alignment())->next() ==
                     &*slab._u_object_current_end(),
                 IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1, slab_type::alignment())->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2, slab_type::alignment())->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2, slab_type::alignment())->next()->next_valid(), IsFalse());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2, slab_type::alignment())->not_available(), IsTrue());

      uint8_t *alloc3 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      AssertThat(static_cast<uintptr_t>(alloc3 - slab.begin()),
                 Equals(3 * slab_type::cs_header_sz + 2 * align(100, slab_type::alignment())));

      uint8_t *alloc4 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      AssertThat(static_cast<uintptr_t>(alloc4 - slab.begin()),
                 Equals(4 * slab_type::cs_header_sz + 3 * align(100, slab_type::alignment())));
      slab.deallocate_raw(alloc4);
      AssertThat(slab_allocator_object_t::from_object_start(alloc3, slab_type::alignment())->next() ==
                     &*slab._u_object_current_end(),
                 IsTrue());
      alloc4 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      AssertThat(static_cast<uintptr_t>(alloc4 - slab.begin()),
                 Equals(4 * slab_type::cs_header_sz + 3 * align(100, slab_type::alignment())));
      slab.deallocate_raw(alloc4);
      slab.deallocate_raw(alloc3);
      AssertThat(slab_allocator_object_t::from_object_start(alloc2, slab_type::alignment())->next() ==
                     &*slab._u_object_current_end(),
                 IsTrue());
      alloc3 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      alloc4 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      AssertThat(static_cast<uintptr_t>(alloc3 - slab.begin()),
                 Equals(3 * slab_type::cs_header_sz + 2 * align(100, slab_type::alignment())));
      AssertThat(static_cast<uintptr_t>(alloc4 - slab.begin()),
                 Equals(4 * slab_type::cs_header_sz + 3 * align(100, slab_type::alignment())));
      slab.deallocate_raw(alloc3);
      slab.deallocate_raw(alloc4);
      AssertThat(slab_allocator_object_t::from_object_start(alloc3, slab_type::alignment())->next() ==
                     &*slab._u_object_current_end(),
                 IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc3, slab_type::alignment())->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc3, slab_type::alignment())->next()->next_valid(), IsFalse());
      alloc3 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      alloc4 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      AssertThat(static_cast<uintptr_t>(alloc3 - slab.begin()),
                 Equals(3 * slab_type::cs_header_sz + 2 * align(100, slab_type::alignment())));
      AssertThat(static_cast<uintptr_t>(alloc4 - slab.begin()),
                 Equals(4 * slab_type::cs_header_sz + 3 * align(100, slab_type::alignment())));

      AssertThat(slab_allocator_object_t::from_object_start(alloc4, slab_type::alignment())->next() ==
                     &*slab._u_object_current_end(),
                 IsTrue());
      uint8_t *alloc5 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      AssertThat(static_cast<uintptr_t>(alloc5 - slab.begin()),
                 Equals(5 * slab_type::cs_header_sz + 4 * align(100, slab_type::alignment())));
      AssertThat(slab_allocator_object_t::from_object_start(alloc5, slab_type::alignment())->next() ==
                     &*slab._u_object_current_end(),
                 IsTrue());
      slab.deallocate_raw(alloc4);
      slab.deallocate_raw(alloc3);
      AssertThat(slab_allocator_object_t::from_object_start(alloc3, slab_type::alignment())->next_valid(), IsTrue());
      alloc3 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      alloc4 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      AssertThat(static_cast<uintptr_t>(alloc3 - slab.begin()),
                 Equals(3 * slab_type::cs_header_sz + 2 * align(100, slab_type::alignment())));
      AssertThat(static_cast<uintptr_t>(alloc4 - slab.begin()),
                 Equals(4 * slab_type::cs_header_sz + 3 * align(100, slab_type::alignment())));

      AssertThat(slab_allocator_object_t::from_object_start(alloc3, slab_type::alignment())->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc4, slab_type::alignment())->next_valid(), IsTrue());
      // Now test coalescing.
      slab.deallocate_raw(alloc3);
      slab.deallocate_raw(alloc4);
      alloc3 = reinterpret_cast<uint8_t *>(slab.allocate_raw(239));
      AssertThat(slab_allocator_object_t::from_object_start(alloc3, slab_type::alignment())->next_valid(), IsTrue());
      // Now test splitting.
      slab.deallocate_raw(alloc3);
      alloc3 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      alloc4 = reinterpret_cast<uint8_t *>(slab.allocate_raw(100));
      AssertThat(slab_allocator_object_t::from_object_start(alloc3, slab_type::alignment())->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc4, slab_type::alignment())->next_valid(), IsTrue());

    });
    it("test2,", []() {
      ::mcppalloc::rebind_vector_t<size_t, cgc1::cgc_internal_slab_allocator_t<int>> vec;
      ::mcppalloc::rebind_vector_t<size_t, cgc1::cgc_internal_slab_allocator_t<int>> vec2;
      ::mcppalloc::rebind_vector_t<size_t, cgc1::cgc_internal_slab_allocator_t<int>> vec3;
      ::mcppalloc::rebind_vector_t<size_t, cgc1::cgc_internal_slab_allocator_t<int>> vec4;
      for (size_t i = 0; i < 1000; ++i) {
        vec.push_back(i);
        vec2.push_back(i);
        vec3.push_back(i);
        vec4.push_back(i);
      }
      for (size_t i = 0; i < 1000; ++i) {
        AssertThat(vec[i], Equals(i));
        AssertThat(vec2[i], Equals(i));
        AssertThat(vec3[i], Equals(i));
        AssertThat(vec4[i], Equals(i));
      }
    });
  });
}
