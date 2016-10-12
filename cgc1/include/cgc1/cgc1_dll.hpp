#pragma once
#if defined _WIN32 || defined __CYGWIN__
#ifdef gc_EXPORTS
#ifdef __GNUC__
#define CGC1_DLL_PUBLIC __attribute__((dllexport))
#else
#define CGC1_DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define CGC1_DLL_PUBLIC __attribute__((dllimport))
#else
#define CGC1_DLL_PUBLIC // __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define CGC1_DLL_LOCAL
#else
#define CGC1_DLL_PUBLIC __attribute__((visibility("default")))
#define CGC1_DLL_LOCAL __attribute__((visibility("hidden")))
#endif
