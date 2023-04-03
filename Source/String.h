#ifndef _STRING_H
#define _STRING_H

#include "Basics.h"



namespace Tool 
{
    //- Struct definitions
    
    //~ 8-bit string
    // Suitable for ASCII or UTF-8 encoded strings
    
    struct String8
    {
        c8* str;
        u64 size;
    };
    
    //~ 16-bit string
    // Suitable for UTF-16 encoded strings
    
    struct String16
    {
        c16* str;
        u64 size;
    };
    
    //~ 32-bit string
    // Suitable for UTF-32 encoded strings
    
    struct String32
    {
        c32* str;
        u64 size;
    };
    
    //~ Acronyms
    
    typedef String8 s8;
    typedef String16 s16;
    typedef String32 s32;
    
    
    
    //- Helper function declarations
    
    //~ 8-bit string
    
    // TODO(crazy): Implement
    
    //~ 16-bit string
    
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
