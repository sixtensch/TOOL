#ifndef _MATH_H
#define _MATH_H



#include <math.h>



//- Definitions

//~ Parametric

#define TOOL_ABS(x) ((x) > 0 ? (x) : -(x))
#define TOOL_MAX(x, y) ((x) > (y) ? (x) : (y))
#define TOOL_MIN(x, y) ((x) < (y) ? (x) : (y))

//~ Static

#ifndef PI
#define PI 3.14159265359f
#endif



/*

//- Standard math function declarations

//~ Float

// Trigonometry
f32 __cdecl cosf(f32 x); // Cosine
f32 __cdecl coshf(f32 x); // Hyperbolic cosine
f32 __cdecl acosf(f32 x); // Arc cosine
f32 __cdecl acoshf(f32 x); // Hyperbolic arc cosine

f32 __cdecl sinf(f32 x); // Sine
f32 __cdecl sinhf(f32 x); // Hyperbolic sine
f32 __cdecl asinf(f32 x); // Arc sine
f32 __cdecl asinhf(f32 x); // Hyperbolic arc sine

f32 __cdecl tanf(f32 x); // Tangent
f32 __cdecl tanhf(f32 x); // Hyperbolic tangent
f32 __cdecl atanf(f32 x); // Arc tangent
f32 __cdecl atan2f(f32 y, f32 x); // Arc tangent of (y / x)
f32 __cdecl atanhf(f32 x); // Hyperbolic arc tangent

// Rounding
f32 __cdecl roundf(f32 x); // Round to nearest whole number
f32 __cdecl ceilf(f32 x); // Ceiling (round up absolute)
f32 __cdecl floorf(f32 x); // Floor (round down absolute)
f32 __cdecl truncf(f32 x); // Truncation. Round towards 0.

// Exponentiation
f32 __cdecl powf(f32 x, f32 y); // Raise x to the power of y 
f32 __cdecl exp2f(f32 x); // Raise 2 to the power of x
f32 __cdecl expf(f32 x); // Raise e to the power of x

f32 __cdecl log2f(f32 x); // 2-logarithm
f32 __cdecl log10f(f32 x); // 10-logarithm
f32 __cdecl logf(f32 x); // e-logarithm

// Root
f32 __cdecl sqrtf(f32 x); // Square root
f32 __cdecl cbrtf(f32 x); // Cube root

// Other
f32 __cdecl fmodf(f32 x, f32 y);

*/



//- Custom math function declarations

//~ Float

f32 clampf(f32 x, f32 min, f32 max);
f32 wrapf(f32 x, f32 min, f32 max);

//~ Int

i32 clampi(i32 x, i32 min, i32 max); // Min inclusive, max exclusive
i32 wrapi(i32 x, i32 min, i32 max); // Min inclusive, max exclusive



//- Custom math function definitions

//~ Float

inline f32 clampf(f32 x, f32 min, f32 max)
{
    bool lesser = x < min;
    bool greater = x >= max;
    
    return 
        min * lesser +
        max * greater +
        x * !(lesser || greater);
}

inline f32 wrapf(f32 x, f32 min, f32 max)
{
    if (x >= min && x <= max)
    {
        return x;
    }
    
    return min + /*fmodf(*/fmodf(x - min, max - min)/*, max - min)*/;
}

//~ Int

inline i32 clampi(i32 x, i32 min, i32 max) // Min inclusive, max exclusive
{
    bool lesser = x < min;
    bool greater = x >= max;
    
    return 
        min * lesser +
        max * greater +
        x * !(lesser || greater);
}

inline i32 wrapi(i32 x, i32 min, i32 max) // Min inclusive, max exclusive
{
    if (x >= min && x < max)
    {
        return x;
    }
    
    return min + ((x - min) % (max - min))/* % (max - min)*/;
}


#endif //_MATH_H
