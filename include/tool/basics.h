#ifndef _TOOL_BASICS_H
#define _TOOL_BASICS_H



//- Definitions

//~ Platform capabilities
// The build defines the platform (TOOL_WINDOWS, TOOL_UNIX). Code for one OS keys off those; code that needs a
// memory capability keys off the defines below, so a platform lacking it compiles that code out.

// Emscripten. Also counts as TOOL_UNIX, whose POSIX layer it emulates.
#if defined(__EMSCRIPTEN__) && !defined(TOOL_WEB)
#define TOOL_WEB 1
#endif

// Address space can be reserved without backing it, and committed page by page later.
#if defined(TOOL_WINDOWS) || (defined(TOOL_UNIX) && !defined(TOOL_WEB))
#define TOOL_VIRTUAL_MEMORY 1
#endif

// The same physical pages can be mapped at two virtual addresses.
#if defined(TOOL_WINDOWS)
#define TOOL_MIRRORED_MEMORY 1
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
#ifdef _MSC_VER
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
}

//~ Acronym usings

#ifndef TOOL_NO_ACRONYMS

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



#endif
