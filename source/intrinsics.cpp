#include "basics.h"

#include <cmath>



//- Scalar SIMD backend helpers
// Defined whichever backend this library was built with, so a translation unit that forces TOOL_SIMD_SCALAR still
// links.

namespace Tool
{
    f32 SimdScalarSqrt(f32 x)      { return std::sqrt(x); }
    f64 SimdScalarSqrt(f64 x)      { return std::sqrt(x); }
    f32 SimdScalarFloor(f32 x)     { return std::floor(x); }
    f64 SimdScalarFloor(f64 x)     { return std::floor(x); }
    f32 SimdScalarCeil(f32 x)      { return std::ceil(x); }
    f64 SimdScalarCeil(f64 x)      { return std::ceil(x); }

    // Rounds in the current mode, which is ties to even unless a caller changed it.
    f32 SimdScalarRoundEven(f32 x) { return std::nearbyint(x); }
    f64 SimdScalarRoundEven(f64 x) { return std::nearbyint(x); }
}
