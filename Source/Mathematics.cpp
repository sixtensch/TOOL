
#include "Mathematics.h"
#include <math.h>

namespace Tool
{
    //- Standard math aliases
    
    //~ Trigonometry
    
    f32 FCos(f32 x) // Cosine
    {
        return cosf(x);
    }
    
    f32 FCosh(f32 x) // Hyperbolic cosine
    {
        return coshf(x);
    }
    
    f32 FAcos(f32 x) // Arc cosine
    {
        return acosf(x);
    }
    
    f32 FAcosh(f32 x) // Hyperbolic arc cosine
    {
        return acoshf(x);
    }
    
    f32 FSin(f32 x) // Sine
    {
        return sinf(x);
    }
    
    f32 FSinh(f32 x) // Hyperbolic sine
    {
        return sinhf(x);
    }
    
    f32 FAsin(f32 x) // Arc sine
    {
        return asinf(x);
    }
    
    f32 FAsinh(f32 x) // Hyperbolic arc sine
    {
        return asinhf(x);
    }
    
    f32 FTan(f32 x) // Tangent
    {
        return tanf(x);
    }
    
    f32 FTanh(f32 x) // Hyperbolic tangent
    {
        return tanhf(x);
    }
    
    f32 FAtan(f32 x) // Arc tangent
    {
        return atanf(x);
    }
    
    f32 FAtan2(f32 y, f32 x) // Arc tangent of (y / x)
    {
        return atan2f(y, x);
    }
    
    f32 FAtanh(f32 x) // Hyperbolic arc tangent
    {
        return atanhf(x);
    }
    
    //~ Rounding
    
    f32 FRound(f32 x) // Round to nearest whole number
    {
        return roundf(x);
    }
    
    f32 FCeil(f32 x) // Ceiling (round up absolute)
    {
        return ceilf(x);
    }
    
    f32 FFloor(f32 x) // Floor (round down absolute)
    {
        return floorf(x);
    }
    
    f32 FTrunc(f32 x) // Truncation. Round towards 0.
    {
        return truncf(x);
    }
    
    //~ Exponentiation
    
    f32 FPow(f32 x, f32 y) // Raise x to the power of y 
    {
        return powf(x, y);
    }
    
    f32 FExp2(f32 x) // Raise 2 to the power of x
    {
        return exp2f(x);
    }
    
    f32 FExp(f32 x) // Raise e to the power of x
    {
        return expf(x);
    }
    
    f32 FLog2(f32 x) // 2-logarithm
    {
        return log2f(x);
    }
    
    f32 FLog10(f32 x) // 10-logarithm
    {
        return log10f(x);
    }
    
    f32 FLog(f32 x) // e-logarithm
    {
        return logf(x);
    }
    
    //~ Root
    
    f32 FSqrt(f32 x) // Square root
    {
        return sqrtf(x);
    }
    
    f32 FCbrt(f32 x) // Cube root
    {
        return cbrtf(x);
    }
    
    //~ Other
    
    f32 FMod(f32 x, f32 y) 
    {
        return fmodf(x, y);
    }
    
}