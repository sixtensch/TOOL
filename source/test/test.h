#ifndef TEST_H
#define TEST_H

#include <cstdio>
#include "tool/basics.h"

#if TOOL_WINDOWS
#define DLL_EXPORT extern "C"  __declspec(dllexport)
#define DLL_IMPORT extern "C"  __declspec(dllimport)
#endif

#if TOOL_UNIX
#define DLL_EXPORT extern "C"  __attribute__((visibility("default")))
#define DLL_IMPORT extern "C"
#endif

#define SCRIPT(name) \
void name##_cpp(int a, int b); \
DLL_EXPORT void name(int a, int b) { name##_cpp(a, b); } \
void name##_cpp(int a, int b)

typedef void (*VoidFunc)();
typedef int (*IntFunc)();

void TestHashMap();
void TestMathematics();
void TestMatrix();
void TestColor();
void TestRandom();
void TestSort();
void TestThreading();
void TestIO();
void TestNetwork();
void TestHttp();
void TestText();
void TestFormat();
u32 TestIntrinsicsRun(const c8** outFirstFailure);

#endif //TEST_H
