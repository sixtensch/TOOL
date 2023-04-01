#ifndef _COLOR_H
#define _COLOR_H

#include "Basics.h"
#include "Vector.h"



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
            
            struct
            {
                Color3U rgb;
                u8 a;
            };
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
            
            v3 v3;
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
            
            struct
            {
                Color3F rgb;
                f32 a;
            };
            
            struct
            {
                v3 v3;
                f32 a;
            };
            
            v4 v4;
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
