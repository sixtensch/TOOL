#ifndef _TOOL_MATHEMATICS_H
#define _TOOL_MATHEMATICS_H

#include "basics.h"



//- Definitions

//~ Parametric

#define TOOL_ABS(x) ((x) > 0 ? (x) : -(x))
#define TOOL_MAX(x, y) ((x) > (y) ? (x) : (y))
#define TOOL_MIN(x, y) ((x) < (y) ? (x) : (y))

// Angle conversion
#define TOOL_R2D(r) (r * 57.2957795130f)
#define TOOL_D2R(d) (d * 0.0174532925199f)

//~ Static

#ifndef PI
#define PI 3.14159265359f
#define PI_D 3.14159265359
#endif



namespace Tool
{
    //- Standard math aliases
    
    //~ Float
    
    // Trigonometry
    f32 FCos(f32 x); // Cosine
    f32 FCosh(f32 x); // Hyperbolic cosine
    f32 FAcos(f32 x); // Arc cosine
    f32 FAcosh(f32 x); // Hyperbolic arc cosine
    
    f32 FSin(f32 x); // Sine
    f32 FSinh(f32 x); // Hyperbolic sine
    f32 FAsin(f32 x); // Arc sine
    f32 FAsinh(f32 x); // Hyperbolic arc sine
    
    f32 FTan(f32 x); // Tangent
    f32 FTanh(f32 x); // Hyperbolic tangent
    f32 FAtan(f32 x); // Arc tangent
    f32 FAtan2(f32 y, f32 x); // Arc tangent of (y / x)
    f32 FAtanh(f32 x); // Hyperbolic arc tangent
    
    // Rounding
    f32 FRound(f32 x); // Round to nearest whole number
    f32 FCeil(f32 x); // Ceiling (round up absolute)
    f32 FFloor(f32 x); // Floor (round down absolute)
    f32 FTrunc(f32 x); // Truncation. Round towards 0.
    
    // Exponentiation
    f32 FPow(f32 x, f32 y); // Raise x to the power of y 
    f32 FExp2(f32 x); // Raise 2 to the power of x
    f32 FExp(f32 x); // Raise e to the power of x
    
    f32 FLog2(f32 x); // 2-logarithm
    f32 FLog10(f32 x); // 10-logarithm
    f32 FLog(f32 x); // e-logarithm
    
    // Root
    f32 FSqrt(f32 x); // Square root
    f32 FCbrt(f32 x); // Cube root
    
    // Other
    f32 FMod(f32 x, f32 y); // Modulo
    
    
    
    //- Custom math function declarations
    
    //~ Float
    
    f32 FClamp(f32 x, f32 min, f32 max);
    f32 FWrap(f32 x, f32 min, f32 max);
    
    //~ Int
    
    i32 IClamp(i32 x, i32 min, i32 max); // Min inclusive, max exclusive
    i32 IWrap(i32 x, i32 min, i32 max); // Min inclusive, max exclusive
    
    
    
    //- Custom math function definitions
    
    //~ Float
    
    inline f32 FClamp(f32 x, f32 min, f32 max)
    {
        bool lesser = x < min;
        bool greater = x >= max;
        
        return 
            min * lesser +
            max * greater +
            x * !(lesser || greater);
    }
    
    inline f32 FWrap(f32 x, f32 min, f32 max)
    {
        if (x >= min && x <= max)
        {
            return x;
        }
        
        return min + /*fmodf(*/FMod(x - min, max - min)/*, max - min)*/;
    }
    
    //~ Int
    
    inline i32 IClamp(i32 x, i32 min, i32 max) // Min inclusive, max exclusive
    {
        bool lesser = x < min;
        bool greater = x >= max;
        
        return 
            min * lesser +
            max * greater +
            x * !(lesser || greater);
    }
    
    inline i32 IWrap(i32 x, i32 min, i32 max) // Min inclusive, max exclusive
    {
        if (x >= min && x < max)
        {
            return x;
        }
        
        return min + ((x - min) % (max - min))/* % (max - min)*/;
    }
    
}



#ifndef TOOL_NO_MATH

// Trigonometry
using Tool::FCos; // Cosine
using Tool::FCosh; // Hyperbolic cosine
using Tool::FAcos; // Arc cosine
using Tool::FAcosh; // Hyperbolic arc cosine

using Tool::FSin; // Sine
using Tool::FSinh; // Hyperbolic sine
using Tool::FAsin; // Arc sine
using Tool::FAsinh; // Hyperbolic arc sine

using Tool::FTan; // Tangent
using Tool::FTanh; // Hyperbolic tangent
using Tool::FAtan; // Arc tangent
using Tool::FAtan2; // Arc tangent of (y / x)
using Tool::FAtanh; // Hyperbolic arc tangent

// Rounding
using Tool::FRound; // Round to nearest whole number
using Tool::FCeil; // Ceiling (round up absolute)
using Tool::FFloor; // Floor (round down absolute)
using Tool::FTrunc; // Truncation. Round towards 0.

// Exponentiation
using Tool::FPow; // Raise x to the power of y 
using Tool::FExp2; // Raise 2 to the power of x
using Tool::FExp; // Raise e to the power of x

using Tool::FLog2; // 2-logarithm
using Tool::FLog10; // 10-logarithm
using Tool::FLog; // e-logarithm

// Root
using Tool::FSqrt; // Square root
using Tool::FCbrt; // Cube root

// Other
using Tool::FMod; // Modulo

// Custom
using Tool::FClamp;
using Tool::FWrap;
using Tool::IClamp; // Min inclusive, max exclusive
using Tool::IWrap; // Min inclusive, max exclusive

#endif



#endif //_MATH_H
