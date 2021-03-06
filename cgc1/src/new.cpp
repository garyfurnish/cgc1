#include <cstdlib>
#include <iostream>
#include <new>
void *operator new(size_t count)
{
  return ::malloc(count);
}
void *operator new[](size_t count)
{
  return ::malloc(count);
}
void *operator new(size_t count, const std::nothrow_t &) noexcept
{
  return ::malloc(count);
}
void *operator new[](size_t count, const std::nothrow_t &) noexcept
{
  return ::malloc(count);
}
void operator delete(void *ptr) noexcept
{
  ::free(ptr);
}
void operator delete[](void *ptr) noexcept
{
  ::free(ptr);
}
void operator delete(void *ptr, const std::nothrow_t &)noexcept
{
  ::free(ptr);
}
void operator delete[](void *ptr, const std::nothrow_t &) noexcept
{
  ::free(ptr);
}
void operator delete(void *ptr, size_t) noexcept
{
  ::free(ptr);
}
void operator delete[](void *ptr, size_t) noexcept
{
  ::free(ptr);
}
void operator delete(void *ptr, size_t, const std::nothrow_t &)noexcept
{
  ::free(ptr);
}

void operator delete[](void *ptr, size_t, const std::nothrow_t &) noexcept
{
  ::free(ptr);
}
