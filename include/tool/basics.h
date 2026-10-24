#ifndef _TOOL_BASICS_H
#define _TOOL_BASICS_H



//~ Basics
//
// The foundation every other header includes: what the target is and can do, the options a program sets, the
// limits of each integer type, and the short type names used throughout (i8 to i64, u8 to u64, f32, f64, b8, b32,
// c8, c16, c32).



//- Definitions

//~ Target
// Facts about the target: the operating system, compiler and architecture this is compiled for. Each is derived
// here, and only here, from what the compiler itself says, which always describes the target and never the machine
// compiling, so a cross-compile infers them as a native one does. A target TOOL doesn't know is an error rather
// than a guess. Instruction sets, and the SIMD backend built on them, are detected in intrinsics.h.
//
// Every TOOL define is always defined, to 1 or 0, and tested by its value, as in '#if TOOL_WINDOWS', never with
// #ifdef or defined(), so a misspelled name is an error where -Wundef is on rather than silently false. Nothing
// outside detection tests the compiler's own macros. What a system or library header defines to say what it
// provides, such as O_DIRECT, is tested where it is used.

// Operating system: exactly one of these

#if defined(_WIN32)
#define TOOL_WINDOWS 1 // Windows
#else
#define TOOL_WINDOWS 0
#endif

#if defined(__linux__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
#define TOOL_LINUX 1 // Linux, but not Android, which says it is Linux too
#else
#define TOOL_LINUX 0
#endif

#if defined(__APPLE__)
#define TOOL_APPLE 1 // Apple's systems, macOS
#else
#define TOOL_APPLE 0
#endif

#if defined(__EMSCRIPTEN__)
#define TOOL_WEB 1 // A browser, through Emscripten
#else
#define TOOL_WEB 0
#endif

#if TOOL_WINDOWS + TOOL_LINUX + TOOL_APPLE + TOOL_WEB != 1
#error "TOOL doesn't know the operating system this is compiled for. Add it to basics.h."
#endif

// The POSIX layer: Linux, Apple's systems, and the web, which Emscripten gives one
#define TOOL_UNIX (TOOL_LINUX || TOOL_APPLE || TOOL_WEB)

// Compiler: exactly one of these

#if defined(__clang__)
#define TOOL_CLANG 1 // Clang, clang-cl and Emscripten's included
#else
#define TOOL_CLANG 0
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define TOOL_MSVC 1 // Microsoft's own compiler, not clang-cl
#else
#define TOOL_MSVC 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define TOOL_GCC 1 // GCC, not Clang, which says it is GCC too
#else
#define TOOL_GCC 0
#endif

#if TOOL_CLANG + TOOL_MSVC + TOOL_GCC != 1
#error "TOOL doesn't know the compiler. Add it to basics.h."
#endif

// Architecture: exactly one of these

#if (defined(__x86_64__) || defined(_M_X64)) && !defined(_M_ARM64EC)
#define TOOL_X64 1 // 64-bit x86
#else
#define TOOL_X64 0
#endif

#if defined(__i386__) || defined(_M_IX86)
#define TOOL_X86 1 // 32-bit x86
#else
#define TOOL_X86 0
#endif

#if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#define TOOL_ARM64 1 // 64-bit ARM, Windows' x64-compatible ARM64EC included
#else
#define TOOL_ARM64 0
#endif

#if defined(__arm__) || defined(_M_ARM)
#define TOOL_ARM32 1 // 32-bit ARM
#else
#define TOOL_ARM32 0
#endif

#if defined(__wasm__)
#define TOOL_WASM 1 // WebAssembly
#else
#define TOOL_WASM 0
#endif

#if TOOL_X64 + TOOL_X86 + TOOL_ARM64 + TOOL_ARM32 + TOOL_WASM != 1
#error "TOOL doesn't know the architecture. Add it to basics.h."
#endif

//~ Capabilities
// What the target can do, where code needs a capability rather than a particular platform, so a platform lacking
// it compiles that code out

// More than one thread: everywhere but a web build without pthreads
#if TOOL_WEB && !defined(__EMSCRIPTEN_PTHREADS__)
#define TOOL_THREADS 0
#else
#define TOOL_THREADS 1
#endif

// Address space can be reserved without backing it, and committed page by page later
#define TOOL_VIRTUAL_MEMORY (TOOL_WINDOWS || TOOL_LINUX || TOOL_APPLE)

// The same physical pages can be mapped at two virtual addresses
#define TOOL_MIRRORED_MEMORY TOOL_WINDOWS

// A 128-bit integer type
#if defined(__SIZEOF_INT128__)
#define TOOL_INT128 1
#else
#define TOOL_INT128 0
#endif

//~ Options
// Set from outside, by the build or by a program using TOOL, to 1 before including any TOOL header. Each is 0
// unless set.
//
//   TOOL_DEBUG_ASSERTS          Check TOOL_ASSERT and TOOL's own internal checks. TOOL's CMake sets it in Debug.
//   TOOL_NO_ACRONYMS            Keep the short names (i32, f32, v2, m4, ...) inside namespace Tool, out of the global one.
//   TOOL_NO_UTF                 Leave out the UTF8(str) and UTF16(str) macros.
//   TOOL_NO_MATH                Keep the math functions (F64Sin, ...) inside namespace Tool, out of the global one.
//   TOOL_NO_FORMAT_VALIDATION   Don't check format strings against their arguments at compile time.
//   TOOL_NO_SIMD                Build the SIMD types from per-lane loops, whatever the target has.

#ifndef TOOL_DEBUG_ASSERTS
#define TOOL_DEBUG_ASSERTS 0
#endif

#ifndef TOOL_NO_ACRONYMS
#define TOOL_NO_ACRONYMS 0
#endif

#ifndef TOOL_NO_UTF
#define TOOL_NO_UTF 0
#endif

#ifndef TOOL_NO_MATH
#define TOOL_NO_MATH 0
#endif

#ifndef TOOL_NO_FORMAT_VALIDATION
#define TOOL_NO_FORMAT_VALIDATION 0
#endif

#ifndef TOOL_NO_SIMD
#define TOOL_NO_SIMD 0
#endif

//~ Maximums and minimums

#define I8_MAX 0x7F
#define I16_MAX 0x7FFF
#define I32_MAX 0x7FFFFFFF
#define I64_MAX 0x7FFFFFFFFFFFFFFF

// Written as -MAX - 1, since the literal 0x80000000 is unsigned and negating it stays unsigned.
#define I8_MIN (-0x7F - 1)
#define I16_MIN (-0x7FFF - 1)
#define I32_MIN (-0x7FFFFFFF - 1)
#define I64_MIN (-0x7FFFFFFFFFFFFFFF - 1)

#define U8_MAX 0xFFu
#define U16_MAX 0xFFFFu
#define U32_MAX 0xFFFFFFFFu
#define U64_MAX 0xFFFFFFFFFFFFFFFFu

#define U8_MIN 0u
#define U16_MIN 0u
#define U32_MIN 0u
#define U64_MIN 0u

#define B8_FALSE ((b8)0u)
#define B8_TRUE ((b8)1u)

#define B32_FALSE ((b32)0u)
#define B32_TRUE ((b32)1u)



//- Acronyms

//~ Acronym definitions

namespace Tool
{
#if TOOL_MSVC
    typedef signed __int8     i8;
    typedef signed __int16    i16;
    typedef signed __int32    i32;
    typedef signed __int64    i64;
    
    typedef unsigned __int8   u8;
    typedef unsigned __int16  u16;
    typedef unsigned __int32  u32;
    typedef unsigned __int64  u64;
#else
    typedef signed char        i8;
    typedef signed short       i16;
    typedef signed int         i32;
    typedef signed long long   i64;
    
    typedef unsigned char      u8;
    typedef unsigned short     u16;
    typedef unsigned int       u32;
    typedef unsigned long long u64;
#endif
    
    typedef float             f32;
    typedef double            f64;
    
    typedef bool              b8;
    typedef int               b32;
    
    typedef char              c8;
    typedef u16               c16;
    typedef u32               c32;

    // The names promise these sizes, and the standard doesn't guarantee them for the types above
    static_assert(sizeof(i8) == 1 && sizeof(i16) == 2 && sizeof(i32) == 4 && sizeof(i64) == 8, "Signed integer sizes");
    static_assert(sizeof(u8) == 1 && sizeof(u16) == 2 && sizeof(u32) == 4 && sizeof(u64) == 8, "Unsigned integer sizes");
    static_assert(sizeof(f32) == 4 && sizeof(f64) == 8, "Float sizes");
    static_assert(sizeof(b8) == 1 && sizeof(b32) == 4, "Boolean sizes");
}

//~ Acronym usings

#if !TOOL_NO_ACRONYMS

using Tool::i8;
using Tool::i16;
using Tool::i32;
using Tool::i64;

using Tool::u8;
using Tool::u16;
using Tool::u32;
using Tool::u64;

using Tool::f32;
using Tool::f64;

using Tool::b8;
using Tool::b32;

using Tool::c8;
using Tool::c16;
using Tool::c32;

#endif //TOOL_NO_ACRONYMS



//- Most basic functions

namespace Tool
{
    
    //~ Memory transformation
    
    void Copy(void* destination, const void* source, u64 size);
    void Copy(void* destination, const void* source, u64 count, u64 size);
    
}



#endif //_TOOL_BASICS_H
