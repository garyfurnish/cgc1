#pragma once
#if defined _WIN32 || defined __CYGWIN__
#ifdef mcppalloc_slab_allocator_EXPORTS
#ifdef __GNUC__
#define MCPPALLOC_SLAB_ALLOCATOR_DLL_PUBLIC __attribute__((dllexport))
#else
#define MCPPALLOC_SLAB_ALLOCATOR_DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define MCPPALLOC_SLAB_ALLOCATOR_DLL_PUBLIC __attribute__((dllimport))
#else
#define MCPPALLOC_SLAB_ALLOCATOR_DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define MCPPALLOC_SLAB_ALLOCATOR_DLL_LOCAL
#else
#define MCPPALLOC_SLAB_ALLOCATOR_DLL_PUBLIC __attribute__((visibility("default")))
#define MCPPALLOC_SLAB_ALLOCATOR_DLL_LOCAL __attribute__((visibility("hidden")))
#endif
