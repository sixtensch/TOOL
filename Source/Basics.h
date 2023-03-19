
#ifndef _BASICS_H
#define _BASICS_H



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
    
    typedef bool               b8;
    typedef int                b32;
}



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

#endif //TOOL_NO_ACRONYMS



#endif
