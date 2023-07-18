#ifndef _STRING_H
#define _STRING_H

#include "basics.h"
#include "memory.h"



//- Preprocessor definitions

//~ Macro functions

#ifndef TOOL_NO_UTF
#define UTF8(str)  ((const c8*)(u8##str))
#define UTF16(str) ((const c16*)(u##str))
#endif



namespace Tool 
{
    //- Struct definitions
    
    //~ 8-bit string (narrow string)
    // Suitable for ASCII or UTF-8 encoded strings
    struct String8
    {
        c8* str;
        u64 size;
    };
    
    //~ 16-bit string (wide string)
    // Suitable for UTF-16 encoded strings.
    struct String16
    {
        c16* str; // 16-bit buffer. Has a capacity of size + 1, with a null terminator after the contents.
        u64 size; // Number of 16-bit characters, not including null terminator
    };
    
    //~ 32-bit string
    // Suitable for UTF-32 encoded strings
    struct String32
    {
        c32* str;
        u64 size; // Number of -bit characters
    };
    
    //~ Acronyms
    
    typedef String8 s8;
    typedef String16 s16;
    typedef String32 s32;
    
    
    
    //- Helper function declarations
    
    //~ C-string
    
    u64 CStr8Size(const c8* string, u64 capacity = 0x10000);           // Size in bytes 
    u64 CStr8Copy(c8* destination, const c8* source, u64 capacity);    // Copies up to (capacity - 1) 8-bit characters. 
    
    u64 CStr16Size(const c16* string, u64 capacity = 0x10000);         // Size in bytes
    u64 CStr16Count(const c16* string, u64 capacity = 0x10000);        // Number of 16-bit characters
    u64 CStr16Copy(c16* destination, const c16* source, u64 capacity); // Copies up to (capacity - 1) 16-bit characters. 
    
    //~ 8-bit string
    
    s8 S8FromS16(s16 str, MemoryAllocator a);
    s8 S8FromCStr8(const c8* cstr, MemoryAllocator a, u64 capacity = 0x10000);
    s8 S8FromCStr16(const c16* cstr, MemoryAllocator a, u64 capacity = 0x10000);
    
    // TODO(crazy): Implement
    
    //~ 16-bit string
    
    s16 S16FromS8(s8 str, MemoryAllocator a);
    s16 S16FromCStr8(const c8* cstr, MemoryAllocator a, u64 capacity = 0x10000);
    s16 S16FromCStr16(const c16* cstr, MemoryAllocator a, u64 capacity = 0x10000);
    
    // TODO(crazy): Implement
    
    //~ 32-bit string
    
    // TODO(crazy): Implement
    
}



//~ Acronym usings

#ifndef TOOL_NO_ACRONYMS

using Tool::s8;
using Tool::s16;
using Tool::s32;

#endif



#endif //_STRING_H
