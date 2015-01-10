#include "bandit.hpp"
#include "../cgc1/src/slab_allocator.hpp"
#include "../cgc1/src/allocator_block.hpp"
using namespace bandit;
void slab_allocator_bandit_tests()
{
  describe("Slab Allocator", []() {
    it("test1", []() {
      using cgc1::details::slab_allocator_object_t;
      cgc1::details::slab_allocator_t slab(500000, 5000000);
      void *alloc1 = slab.allocate_raw(100);
      AssertThat(alloc1 == slab.begin() + 16, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->next() == &*slab._u_object_current_end(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->next()->next_valid(), IsFalse());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->next()->not_available(), IsFalse());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->not_available(), IsTrue());
      void *alloc2 = slab.allocate_raw(100);
      AssertThat(alloc2 == slab.begin() + 144, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2)->next() == &*slab._u_object_current_end(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc1)->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2)->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2)->next()->next_valid(), IsFalse());
      AssertThat(slab_allocator_object_t::from_object_start(alloc2)->not_available(), IsTrue());
      void *alloc3 = slab.allocate_raw(100);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      void *alloc4 = slab.allocate_raw(100);
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
      slab.deallocate_raw(alloc4);
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next() == &*slab._u_object_current_end(), IsTrue());
      alloc4 = slab.allocate_raw(100);
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
      slab.deallocate_raw(alloc4);
      slab.deallocate_raw(alloc3);
      AssertThat(slab_allocator_object_t::from_object_start(alloc2)->next() == &*slab._u_object_current_end(), IsTrue());
      alloc3 = slab.allocate_raw(100);
      alloc4 = slab.allocate_raw(100);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
      slab.deallocate_raw(alloc3);
      slab.deallocate_raw(alloc4);
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next() == &*slab._u_object_current_end(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next()->next_valid(), IsFalse());
      alloc3 = slab.allocate_raw(100);
      alloc4 = slab.allocate_raw(100);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc4)->next() == &*slab._u_object_current_end(), IsTrue());
      void *alloc5 = slab.allocate_raw(100);
      AssertThat(alloc5 == slab.begin() + 528, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc5)->next() == &*slab._u_object_current_end(), IsTrue());
      slab.deallocate_raw(alloc4);
      slab.deallocate_raw(alloc3);
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next_valid(), IsTrue());
      alloc3 = slab.allocate_raw(100);
      alloc4 = slab.allocate_raw(100);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc3)->next_valid(), IsTrue());
      AssertThat(slab_allocator_object_t::from_object_start(alloc4)->next_valid(), IsTrue());
      // Now test coalescing.
      slab.deallocate_raw(alloc3);
      slab.deallocate_raw(alloc4);
      alloc3 = slab.allocate_raw(239);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      // Now test splitting.
      slab.deallocate_raw(alloc3);
      alloc3 = slab.allocate_raw(100);
      alloc4 = slab.allocate_raw(100);
      AssertThat(alloc3 == slab.begin() + 272, IsTrue());
      AssertThat(alloc4 == slab.begin() + 400, IsTrue());
    });
    it("test2,", []() {
      cgc1::rebind_vector_t<size_t, cgc1::cgc_internal_slab_allocator_t<int>> vec;
      cgc1::rebind_vector_t<size_t, cgc1::cgc_internal_slab_allocator_t<int>> vec2;
      cgc1::rebind_vector_t<size_t, cgc1::cgc_internal_slab_allocator_t<int>> vec3;
      cgc1::rebind_vector_t<size_t, cgc1::cgc_internal_slab_allocator_t<int>> vec4;
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
