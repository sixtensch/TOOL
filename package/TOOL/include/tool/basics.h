#ifndef _TOOL_BASICS_H
#define _TOOL_BASICS_H



//- Definitions

//~ Maximums and minimums

#define I8_MAX 0x7F
#define I16_MAX 0x7FFF
#define I32_MAX 0x7FFFFFFF
#define I64_MAX 0x7FFFFFFFFFFFFFFF

#define I8_MIN -0x80
#define I16_MIN -0x8000
#define I32_MIN -0x80000000
#define I64_MIN -0x8000000000000000

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
    typedef signed char        i8;
    typedef signed short       i16;
    typedef signed int         i32;
    typedef signed long long   i64;
    
    typedef unsigned char      u8;
    typedef unsigned short     u16;
    typedef unsigned int       u32;
    typedef unsigned long long u64;
    
    typedef float              f32;
    typedef double             f64;
    typedef long double        f128;
    
    typedef bool               b8;
    typedef int                b32;
    
    typedef char               c8;
    typedef unsigned short     c16;
    typedef unsigned int       c32;
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
using Tool::f128;

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
