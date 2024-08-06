#ifndef _TOOL_BOX_H
#define _TOOL_BOX_H

#include "basics.h"
#include "vector.h"



namespace Tool
{
    //~ Struct declarations
    
    template<typename T>
        struct Box2
    {
        union
        {
            struct
            {
                T x; // Position X component
                T y; // Position Y component
                
                T w; // Width
                T h; // Height
            };
            
            struct
            {
                Vec2<T> position;
                Vec2<T> size;
            };
        };
    };
    
    template<typename T>
        struct Box3
    {
        union
        {
            struct
            {
                T x; // Position X component
                T y; // Position Y component
                T z; // Position Z component
                
                T w; // Width
                T h; // Height
                T d; // Depth
            };
            
            struct
            {
                Vec3<T> position;
                Vec3<T> size;
            };
        };
    };
    
    //~ Acronyms
    
    typedef Box2<f32> b2f;
    typedef Box2<i32> b2i;
    typedef Box2<u32> b2u;
    
    typedef Box3<f32> b3f;
    typedef Box3<i32> b3i;
    typedef Box3<u32> b3u;
    
    
    
    //- Helper function declarations
    
    //~ 2D box
    
    template<typename T>
        b8 B2Touches(Box2<T> a, Box2<T> b);
    template<typename T>
        b8 B2Overlaps(Box2<T> a, Box2<T> b);
    template<typename T>
        b8 B2Contains(Box2<T> a, Box2<T> b);
    template<typename T>
        b8 B2Envelops(Box2<T> a, Box2<T> b);
    
    template<typename T>
        b8 B2Touches(Box2<T> a, Vec2<T> b);
    template<typename T>
        b8 B2Envelops(Box2<T> a, Vec2<T> b);
    
    //~ 3D box
    
    template<typename T>
        b8 B3Touches(Box3<T> a, Box3<T> b);
    template<typename T>
        b8 B3Overlaps(Box3<T> a, Box3<T> b);
    template<typename T>
        b8 B3Contains(Box3<T> a, Box3<T> b);
    template<typename T>
        b8 B3Envelops(Box3<T> a, Box3<T> b);
    
    template<typename T>
        b8 B3Touches(Box3<T> a, Vec3<T> b);
    template<typename T>
        b8 B3Envelops(Box3<T> a, Vec3<T> b);
    
    
    
    //- Helper function definitions
    
    //~ 2D box
    
    template<typename T>
        b8 B2Touches(Box2<T> a, Box2<T> b)
    {
        return !(b.x + b.w < a.x ||
                 b.y + b.h < a.y ||
                 b.x > a.x + a.w ||
                 b.y > a.y + a.h);
    }
    
    template<typename T>
        b8 B2Overlaps(Box2<T> a, Box2<T> b)
    {
        return !(b.x + b.w <= a.x ||
                 b.y + b.h <= a.y ||
                 b.x >= a.x + a.w ||
                 b.y >= a.y + a.h);
    }
    
    template<typename T>
        b8 B2Contains(Box2<T> a, Box2<T> b)
    {
        return !(b.x < a.x ||
                 b.y < a.y ||
                 b.x + b.w > a.x + a.w ||
                 b.y + b.h > a.y + a.h);
    }
    
    template<typename T>
        b8 B2Envelops(Box2<T> a, Box2<T> b)
    {
        return !(b.x <= a.x ||
                 b.y <= a.y ||
                 b.x + b.w >= a.x + a.w ||
                 b.y + b.h >= a.y + a.h);
    }
    
    template<typename T>
        b8 B2Touches(Box2<T> a, Vec2<T> b)
    {
        return !(b.x < a.x ||
                 b.y < a.y ||
                 b.x > a.x + a.w ||
                 b.y > a.y + a.h);
    }
    
    template<typename T>
        b8 B2Envelops(Box2<T> a, Vec2<T> b)
    {
        return !(b.x <= a.x ||
                 b.y <= a.y ||
                 b.x >= a.x + a.w ||
                 b.y >= a.y + a.h);
    }
    
    //~ 3D box
    
    template<typename T>
        b8 B3Touches(Box3<T> a, Box3<T> b)
    {
        return !(b.x + b.w < a.x ||
                 b.y + b.h < a.y ||
                 b.z + b.d < a.z ||
                 b.x > a.x + a.w ||
                 b.y > a.y + a.h ||
                 b.z > a.z + a.d );
    }
    
    template<typename T>
        b8 B3Overlaps(Box3<T> a, Box3<T> b)
    {
        return !(b.x + b.w <= a.x ||
                 b.y + b.h <= a.y ||
                 b.z + b.d <= a.z ||
                 b.x >= a.x + a.w ||
                 b.y >= a.y + a.h ||
                 b.z >= a.z + a.d);
    }
    
    template<typename T>
        b8 B3Contains(Box3<T> a, Box3<T> b)
    {
        return !(b.x < a.x ||
                 b.y < a.y ||
                 b.z < a.z ||
                 b.x + b.w > a.x + a.w ||
                 b.y + b.h > a.y + a.h ||
                 b.z + b.d > a.z + a.d);
    }
    
    template<typename T>
        b8 B3Envelops(Box3<T> a, Box3<T> b)
    {
        return !(b.x <= a.x ||
                 b.y <= a.y ||
                 b.z <= a.z ||
                 b.x + b.w >= a.x + a.w ||
                 b.y + b.h >= a.y + a.h ||
                 b.z + b.d >= a.z + a.d);
    }
    
    template<typename T>
        b8 B3Touches(Box3<T> a, Vec3<T> b)
    {
        return !(b.x < a.x ||
                 b.y < a.y ||
                 b.z < a.z ||
                 b.x > a.x + a.w ||
                 b.y > a.y + a.h ||
                 b.z > a.z + a.d);
    }
    
    template<typename T>
        b8 B3Envelops(Box3<T> a, Vec3<T> b)
    {
        return !(b.x <= a.x ||
                 b.y <= a.y ||
                 b.z <= a.z ||
                 b.x >= a.x + a.w ||
                 b.y >= a.y + a.h ||
                 b.z >= a.z + a.d);
    }
}



//~ Acronym usings

#ifndef TOOL_NO_ACRONYMS

using Tool::b2f;
using Tool::b2i;
using Tool::b2u;

using Tool::b3f;
using Tool::b3i;
using Tool::b3u;

#endif



#endif //_BOX_H
