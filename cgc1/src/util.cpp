#include <atomic>
#include <cgc1/declarations.hpp>
#include <mcpputil/mcpputil/security.hpp>
namespace cgc1
{
#ifndef _WIN32
  template <size_t bytes>
  MCPPALLOC_NO_INLINE void clean_stack(size_t, size_t, size_t, size_t, size_t)
  {
    // this nukes all registers and forces spills.
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
        : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%xmm0",
          "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7", "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13",
          "%xmm14", "%xmm15");
    ::std::atomic_thread_fence(::std::memory_order_acq_rel);
    // zero the stack.
    int *array = reinterpret_cast<int *>(alloca(sizeof(int) * bytes));
    ::mcpputil::secure_zero(array, bytes);
    assert(*array == 0);
    ::std::atomic_thread_fence(::std::memory_order_acq_rel);
  }
#else
  template <size_t bytes>
  MCPPALLOC_NO_INLINE void clean_stack(size_t, size_t, size_t, size_t, size_t)
  {
    ::std::atomic_thread_fence(::std::memory_order_acq_rel);
    // zero the stack.
    int *array = reinterpret_cast<int *>(alloca(sizeof(int) * bytes));
    ::mcpputil::secure_zero(array, bytes);
    assert(*array == 0);
    ::std::atomic_thread_fence(::std::memory_order_acq_rel);
  }
#endif
  template void clean_stack<5000>(size_t, size_t, size_t, size_t, size_t);
}
