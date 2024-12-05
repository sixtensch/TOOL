#ifndef _TOOL_QUATERNION_H
#define _TOOL_QUATERNION_H

#include "basics.h"
#include "vector.h"
#include "matrix.h"

namespace Tool
{
    //~ Quaternion
    
    struct Quaternion
    {
        union
        {
            struct 
            {
                f32 x;
                f32 y;
                f32 z;
                f32 w;
            };
            
            v3 im;
            v4 vec;
        };
        
        // Quaternion application. Apply this rotation to a spatial vector.
        v3 operator*(v3 vector);
        v4 operator*(v4 vector);
        
        // Quaternion composition. Perform this rotation, then rotation b.
        Quaternion operator*(Quaternion b); 
        const Quaternion& operator*=(Quaternion b); 
        
        // Comparison
        bool operator==(Quaternion b);
        bool operator!=(Quaternion b);

        // Advanced operations. Do not use if you do not know exactly what you are doing.
        Quaternion operator*(f32 scalar);
        Quaternion operator+(Quaternion b); 
    };
    
    
    
    //~ Acronyms
    
    typedef Quaternion q4; // Quaternion
    
    
    
    //~ Helper function declarations
    
    q4 Q4Identity();
    q4 Q4FromComponents(v4 vector);
    q4 Q4FromComponents(v3 imaginary, f32 real);
    q4 Q4FromComponents(f32 x, f32 y, f32 z, f32 w);
    q4 Q4Axis(v3 normalizedAxis, f32 radians);
    q4 Q4Euler(v3 euler);
    q4 Q4Euler(f32 pitch, f32 yaw, f32 roll);
    q4 Q4FromTo(v3 from, v3 to);
    q4 Q4Direction(v3 direction, v3 up = { 0.0f, 1.0f, 0.0f });
    q4 Q4Deconstruct(const m4& matrix);
    q4 Q4Slerp(q4 from, q4 to, f32 t);
    
    m4 Q4GetMatrix(q4 quaternion);
    m4 Q4GetMatrixNonNormal(q4 quaternion);
    
    f32 Q4Norm(q4 quaternion);
    q4 Q4Conjugate(q4 quaternion);
    q4 Q4Normalize(q4 quaternion);
    q4 Q4Inverse(q4 quaternion);
    void Q4Conjugate(q4* target);
    void Q4Normalize(q4* target);
    void Q4Inverse(q4* target);
}



//~ Acronym usings

#ifndef TOOL_NO_ACRONYMS

using Tool::q4;

#endif



#endif //_QUATERNION_H
