#include "mathematics.h"

#include <math.h>



namespace Tool
{
	//~ f64

	// Trigonometry

	f64 F64Sin(f64 x)
	{
		return sin(x);
	}

	f64 F64Cos(f64 x)
	{
		return cos(x);
	}

	f64 F64Tan(f64 x)
	{
		return tan(x);
	}

	f64 F64Asin(f64 x)
	{
		return asin(x);
	}

	f64 F64Acos(f64 x)
	{
		return acos(x);
	}

	f64 F64Atan(f64 x)
	{
		return atan(x);
	}

	f64 F64Atan2(f64 y, f64 x)
	{
		return atan2(y, x);
	}

	f64 F64Sinh(f64 x)
	{
		return sinh(x);
	}

	f64 F64Cosh(f64 x)
	{
		return cosh(x);
	}

	f64 F64Tanh(f64 x)
	{
		return tanh(x);
	}

	f64 F64Asinh(f64 x)
	{
		return asinh(x);
	}

	f64 F64Acosh(f64 x)
	{
		return acosh(x);
	}

	f64 F64Atanh(f64 x)
	{
		return atanh(x);
	}

	// Rounding

	f64 F64Round(f64 x)
	{
		return round(x);
	}

	f64 F64Ceil(f64 x)
	{
		return ceil(x);
	}

	f64 F64Floor(f64 x)
	{
		return floor(x);
	}

	f64 F64Trunc(f64 x)
	{
		return trunc(x);
	}

	// Exponentiation

	f64 F64Pow(f64 x, f64 y)
	{
		return pow(x, y);
	}

	f64 F64Exp(f64 x)
	{
		return exp(x);
	}

	f64 F64Exp2(f64 x)
	{
		return exp2(x);
	}

	f64 F64Log(f64 x)
	{
		return log(x);
	}

	f64 F64Log2(f64 x)
	{
		return log2(x);
	}

	f64 F64Log10(f64 x)
	{
		return log10(x);
	}

	// Root

	f64 F64Sqrt(f64 x)
	{
		return sqrt(x);
	}

	f64 F64Cbrt(f64 x)
	{
		return cbrt(x);
	}

	// Other

	f64 F64Mod(f64 x, f64 y)
	{
		return fmod(x, y);
	}


	//~ f32

	// Trigonometry

	f32 F32Sin(f32 x)
	{
		return sinf(x);
	}

	f32 F32Cos(f32 x)
	{
		return cosf(x);
	}

	f32 F32Tan(f32 x)
	{
		return tanf(x);
	}

	f32 F32Asin(f32 x)
	{
		return asinf(x);
	}

	f32 F32Acos(f32 x)
	{
		return acosf(x);
	}

	f32 F32Atan(f32 x)
	{
		return atanf(x);
	}

	f32 F32Atan2(f32 y, f32 x)
	{
		return atan2f(y, x);
	}

	f32 F32Sinh(f32 x)
	{
		return sinhf(x);
	}

	f32 F32Cosh(f32 x)
	{
		return coshf(x);
	}

	f32 F32Tanh(f32 x)
	{
		return tanhf(x);
	}

	f32 F32Asinh(f32 x)
	{
		return asinhf(x);
	}

	f32 F32Acosh(f32 x)
	{
		return acoshf(x);
	}

	f32 F32Atanh(f32 x)
	{
		return atanhf(x);
	}

	// Rounding

	f32 F32Round(f32 x)
	{
		return roundf(x);
	}

	f32 F32Ceil(f32 x)
	{
		return ceilf(x);
	}

	f32 F32Floor(f32 x)
	{
		return floorf(x);
	}

	f32 F32Trunc(f32 x)
	{
		return truncf(x);
	}

	// Exponentiation

	f32 F32Pow(f32 x, f32 y)
	{
		return powf(x, y);
	}

	f32 F32Exp(f32 x)
	{
		return expf(x);
	}

	f32 F32Exp2(f32 x)
	{
		return exp2f(x);
	}

	f32 F32Log(f32 x)
	{
		return logf(x);
	}

	f32 F32Log2(f32 x)
	{
		return log2f(x);
	}

	f32 F32Log10(f32 x)
	{
		return log10f(x);
	}

	// Root

	f32 F32Sqrt(f32 x)
	{
		return sqrtf(x);
	}

	f32 F32Cbrt(f32 x)
	{
		return cbrtf(x);
	}

	// Other

	f32 F32Mod(f32 x, f32 y)
	{
		return fmodf(x, y);
	}
}
