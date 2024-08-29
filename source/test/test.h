#ifndef TEST_H
#define TEST_H

#include <cstdio>

#ifdef TOOL_WINDOWS
#define DLL_EXPORT extern "C"  __declspec(dllexport)
#define DLL_IMPORT extern "C"  __declspec(dllimport)
#endif

#ifdef TOOL_UNIX
#define DLL_EXPORT extern "C"  __attribute__((visibility("default")))
#endif

#define SCRIPT(name) \
void name##_cpp(int a, int b); \
DLL_EXPORT void name(int a, int b) { name##_cpp(a, b); } \
void name##_cpp(int a, int b)

typedef void (*VoidFunc)();
typedef int (*IntFunc)();

#endif //TEST_H
