#pragma once
#include <cgc1/cgc1_dll.hpp>
#include <new>
#ifdef gc_EXPORTS
extern void *operator new(std::size_t count);
extern void *operator new[](std::size_t count);
extern void *operator new(std::size_t count, const std::nothrow_t &) noexcept;
extern void *operator new[](std::size_t count, const std::nothrow_t &) noexcept;
extern void operator delete(void *ptr) noexcept;
extern void operator delete[](void *ptr) noexcept;
extern void operator delete(void *ptr, const std::nothrow_t &)noexcept;
extern void operator delete[](void *ptr, const std::nothrow_t &) noexcept;
extern void operator delete(void *ptr, std::size_t) noexcept;
extern void operator delete[](void *ptr, std::size_t) noexcept;
extern void operator delete(void *ptr, std::size_t, const std::nothrow_t &)noexcept;
extern void operator delete[](void *ptr, std::size_t, const std::nothrow_t &) noexcept;
#endif
