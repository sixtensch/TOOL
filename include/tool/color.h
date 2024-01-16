#ifndef _TOOL_COLOR_H
#define _TOOL_COLOR_H

#include "basics.h"
#include "vector.h"



namespace Tool
{
    //- Struct declarations 
    
    //~ Byte colors
    
    struct Color3U
    {
        u8 r;
        u8 g;
        u8 b;
    };
    
    struct Color4U
    {
        union
        {
            struct
            {
                u8 r;
                u8 g;
                u8 b;
                u8 a;
            };
            
            Color3U c3;
        };
    };
    
    //~ Floating-point colors
    
    struct Color3F
    {
        union
        {
            struct
            {
                f32 r;
                f32 g;
                f32 b;
            };
            
            v3 rgb;
        };
    };
    
    struct Color4F
    {
        union
        {
            struct
            {
                f32 r;
                f32 g;
                f32 b;
                f32 a;
            };
            
            Color3F c3;
            
            v3 rgb;
            v4 rgba;
        };
    };
    
    //~ Acronyms
    
    typedef Color3U c3u;
    typedef Color4U c4u;
    
    typedef Color3F c3f;
    typedef Color4F c4f;
    
    
    
    //- Helper functions
    
    //~ Byte colors
    
    // TODO(crazy): Implement
    
    //~ Floating-point colors
    
    // TODO(crazy): Implement
}



//~ Acronym usings

#ifndef TOOL_NO_ACRONYMS

using Tool::c3u;
using Tool::c4u;

using Tool::c3f;
using Tool::c4f;

#endif



#endif //_COLOR_H
