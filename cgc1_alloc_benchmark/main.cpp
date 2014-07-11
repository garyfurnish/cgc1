#include "../cgc1/src/internal_declarations.hpp"
#include <cgc1/cgc1.hpp>
#include <cgc1/posix_slab.hpp>
#include <cgc1/posix.hpp>
#include <thread>
#include <signal.h>
#include "../cgc1/src/allocator_block.hpp"
#include "../cgc1/src/allocator.hpp"

int main()
{
  //  CGC1_INITIALIZE_THREAD();
  cgc1::details::allocator_t<> allocator;
  allocator.initialize(cgc1::details::pow2(32), cgc1::details::pow2(32));
  auto &ts = allocator.initialize_thread();
  for (size_t i = 0; i < 10000000; ++i)
  //  for(size_t i = 0; i < 100000; ++i)
  {
    ts.allocate(100);
  }
  /*size_t iter = 10000000;
  size_t sz = iter * 200;
  void *memory = malloc(sz);
  for (int i = 0; i < 1; ++i) {
    cgc1::details::allocator_block_t<std::allocator<void>> block(memory, sz, 16);
    cgc1::details::allocator_block_set_t<std::allocator<void>> allocator(16,1000);
    allocator.add_block(::std::move(block));
    for (size_t i = 0; i < iter; ++i)
      allocator.allocate(100);
  }
  free(memory);*/
  return 0;
}
