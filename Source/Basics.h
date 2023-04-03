#ifndef _BASICS_H
#define _BASICS_H



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
    void Copy(void* destination, const void* source, u64 size);
    void Copy(void* destination, const void* source, u64 count, u64 size);
    
    u64 CStringLength(const c8* string, u64 capacity = 0x10000); 
    u64 CStringCopy(c8* destination, const c8* source, u64 capacity); // Copies up to capacity - 1 characters. 
}


#endif
