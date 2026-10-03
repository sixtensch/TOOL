#ifndef _TOOL_MATHEMATICS_H
#define _TOOL_MATHEMATICS_H

#include "basics.h"



//~ Mathematics
//
// Scalar math for each type, named by its width: F64Sin, F32Clamp, I32Wrap. Types come large to small.
//
// Clamp bounds are inclusive. Wrap is min inclusive, max exclusive, and wraps inputs on either side into range.
// Lerp and Remap don't clamp, so they extrapolate outside their ranges.
//
// The functions over the C math library live in mathematics.cpp, so this header includes no system headers.



//- Definitions

//~ Generic

// Type-generic, but each argument is evaluated twice. The typed functions evaluate once.
#define TOOL_ABS(x) ((x) > 0 ? (x) : -(x))
#define TOOL_MAX(x, y) ((x) > (y) ? (x) : (y))
#define TOOL_MIN(x, y) ((x) < (y) ? (x) : (y))

//~ Constants

#define F64_PI 3.14159265358979324
#define F32_PI 3.14159265358979324f
#define F64_TAU 6.28318530717958648
#define F32_TAU 6.28318530717958648f



namespace Tool
{
	//~ f64

	// Trigonometry
	f64 F64Sin(f64 x); // Sine
	f64 F64Cos(f64 x); // Cosine
	f64 F64Tan(f64 x); // Tangent
	f64 F64Asin(f64 x); // Arc sine
	f64 F64Acos(f64 x); // Arc cosine
	f64 F64Atan(f64 x); // Arc tangent
	f64 F64Atan2(f64 y, f64 x); // Arc tangent of (y / x), using the signs of both for the quadrant
	f64 F64Sinh(f64 x); // Hyperbolic sine
	f64 F64Cosh(f64 x); // Hyperbolic cosine
	f64 F64Tanh(f64 x); // Hyperbolic tangent
	f64 F64Asinh(f64 x); // Hyperbolic arc sine
	f64 F64Acosh(f64 x); // Hyperbolic arc cosine
	f64 F64Atanh(f64 x); // Hyperbolic arc tangent

	// Rounding
	f64 F64Round(f64 x); // To nearest, halves away from 0
	f64 F64Ceil(f64 x); // Up
	f64 F64Floor(f64 x); // Down
	f64 F64Trunc(f64 x); // Towards 0

	// Exponentiation
	f64 F64Pow(f64 x, f64 y); // x to the power of y
	f64 F64Exp(f64 x); // e to the power of x
	f64 F64Exp2(f64 x); // 2 to the power of x
	f64 F64Log(f64 x); // Natural logarithm
	f64 F64Log2(f64 x); // Base 2 logarithm
	f64 F64Log10(f64 x); // Base 10 logarithm

	// Root
	f64 F64Sqrt(f64 x); // Square root
	f64 F64Cbrt(f64 x); // Cube root

	// Other
	f64 F64Mod(f64 x, f64 y); // Remainder of x / y, with the sign of x
	inline f64 F64Abs(f64 x);
	inline f64 F64Sign(f64 x); // -1, 0 or 1
	inline f64 F64Min(f64 a, f64 b);
	inline f64 F64Max(f64 a, f64 b);
	inline f64 F64Clamp(f64 x, f64 min, f64 max);
	inline f64 F64Saturate(f64 x); // Clamp to [0, 1]
	inline f64 F64Wrap(f64 x, f64 min, f64 max);
	inline f64 F64Lerp(f64 a, f64 b, f64 t); // a at 0, b at 1
	inline f64 F64Remap(f64 x, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax); // Maps fromMin to toMin and fromMax to toMax
	inline f64 F64Radians(f64 degrees);
	inline f64 F64Degrees(f64 radians);

	//~ f32

	// Trigonometry
	f32 F32Sin(f32 x); // Sine
	f32 F32Cos(f32 x); // Cosine
	f32 F32Tan(f32 x); // Tangent
	f32 F32Asin(f32 x); // Arc sine
	f32 F32Acos(f32 x); // Arc cosine
	f32 F32Atan(f32 x); // Arc tangent
	f32 F32Atan2(f32 y, f32 x); // Arc tangent of (y / x), using the signs of both for the quadrant
	f32 F32Sinh(f32 x); // Hyperbolic sine
	f32 F32Cosh(f32 x); // Hyperbolic cosine
	f32 F32Tanh(f32 x); // Hyperbolic tangent
	f32 F32Asinh(f32 x); // Hyperbolic arc sine
	f32 F32Acosh(f32 x); // Hyperbolic arc cosine
	f32 F32Atanh(f32 x); // Hyperbolic arc tangent

	// Rounding
	f32 F32Round(f32 x); // To nearest, halves away from 0
	f32 F32Ceil(f32 x); // Up
	f32 F32Floor(f32 x); // Down
	f32 F32Trunc(f32 x); // Towards 0

	// Exponentiation
	f32 F32Pow(f32 x, f32 y); // x to the power of y
	f32 F32Exp(f32 x); // e to the power of x
	f32 F32Exp2(f32 x); // 2 to the power of x
	f32 F32Log(f32 x); // Natural logarithm
	f32 F32Log2(f32 x); // Base 2 logarithm
	f32 F32Log10(f32 x); // Base 10 logarithm

	// Root
	f32 F32Sqrt(f32 x); // Square root
	f32 F32Cbrt(f32 x); // Cube root

	// Other
	f32 F32Mod(f32 x, f32 y); // Remainder of x / y, with the sign of x
	inline f32 F32Abs(f32 x);
	inline f32 F32Sign(f32 x); // -1, 0 or 1
	inline f32 F32Min(f32 a, f32 b);
	inline f32 F32Max(f32 a, f32 b);
	inline f32 F32Clamp(f32 x, f32 min, f32 max);
	inline f32 F32Saturate(f32 x); // Clamp to [0, 1]
	inline f32 F32Wrap(f32 x, f32 min, f32 max);
	inline f32 F32Lerp(f32 a, f32 b, f32 t); // a at 0, b at 1
	inline f32 F32Remap(f32 x, f32 fromMin, f32 fromMax, f32 toMin, f32 toMax); // Maps fromMin to toMin and fromMax to toMax
	inline f32 F32Radians(f32 degrees);
	inline f32 F32Degrees(f32 radians);

	//~ Integers

	// Abs of the type's minimum overflows.
	inline i64 I64Abs(i64 x);
	inline i64 I64Sign(i64 x); // -1, 0 or 1
	inline i64 I64Min(i64 a, i64 b);
	inline i64 I64Max(i64 a, i64 b);
	inline i64 I64Clamp(i64 x, i64 min, i64 max);
	inline i64 I64Wrap(i64 x, i64 min, i64 max);

	inline u64 U64Min(u64 a, u64 b);
	inline u64 U64Max(u64 a, u64 b);
	inline u64 U64Clamp(u64 x, u64 min, u64 max);
	inline u64 U64Wrap(u64 x, u64 min, u64 max);

	// The full 128-bit product: returns the low 64 bits and writes the high 64 bits to 'high'.
	constexpr u64 U64MulWide(u64 a, u64 b, u64* high);

	inline i32 I32Abs(i32 x);
	inline i32 I32Sign(i32 x); // -1, 0 or 1
	inline i32 I32Min(i32 a, i32 b);
	inline i32 I32Max(i32 a, i32 b);
	inline i32 I32Clamp(i32 x, i32 min, i32 max);
	inline i32 I32Wrap(i32 x, i32 min, i32 max);

	inline u32 U32Min(u32 a, u32 b);
	inline u32 U32Max(u32 a, u32 b);
	inline u32 U32Clamp(u32 x, u32 min, u32 max);
	inline u32 U32Wrap(u32 x, u32 min, u32 max);
} //namespace Tool



//- Implementation

// The two multiply intrinsics, declared as <intrin.h> does, to spare every includer that header.
#if defined(_MSC_VER) && !defined(__SIZEOF_INT128__)
extern "C" unsigned __int64 _umul128(unsigned __int64 _Multiplier, unsigned __int64 _Multiplicand, unsigned __int64* _HighProduct);
extern "C" unsigned __int64 __umulh(unsigned __int64 _Multiplier, unsigned __int64 _Multiplicand);
#if defined(_M_X64)
#pragma intrinsic(_umul128)
#elif defined(_M_ARM64)
#pragma intrinsic(__umulh)
#endif
#endif

namespace Tool
{
	//~ f64

	inline f64 F64Abs(f64 x)
	{
		return x > 0.0 ? x : -x;
	}

	inline f64 F64Sign(f64 x)
	{
		return (f64)((x > 0.0) - (x < 0.0));
	}

	inline f64 F64Min(f64 a, f64 b)
	{
		return a < b ? a : b;
	}

	inline f64 F64Max(f64 a, f64 b)
	{
		return a > b ? a : b;
	}

	inline f64 F64Clamp(f64 x, f64 min, f64 max)
	{
		return x < min ? min : x > max ? max : x;
	}

	inline f64 F64Saturate(f64 x)
	{
		return F64Clamp(x, 0.0, 1.0);
	}

	// Mod keeps the sign of its input, so offsets below min move up a period. Adding the period can round up to
	// exactly max, which wraps to min.
	inline f64 F64Wrap(f64 x, f64 min, f64 max)
	{
		if (x >= min && x < max)
			return x;

		f64 period = max - min;
		f64 offset = F64Mod(x - min, period);
		offset = offset < 0.0 ? offset + period : offset;
		f64 result = min + offset;
		return result < max ? result : min;
	}

	inline f64 F64Lerp(f64 a, f64 b, f64 t)
	{
		return a + (b - a) * t;
	}

	inline f64 F64Remap(f64 x, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax)
	{
		return F64Lerp(toMin, toMax, (x - fromMin) / (fromMax - fromMin));
	}

	inline f64 F64Radians(f64 degrees)
	{
		return degrees * (F64_PI / 180.0);
	}

	inline f64 F64Degrees(f64 radians)
	{
		return radians * (180.0 / F64_PI);
	}

	//~ f32

	inline f32 F32Abs(f32 x)
	{
		return x > 0.0f ? x : -x;
	}

	inline f32 F32Sign(f32 x)
	{
		return (f32)((x > 0.0f) - (x < 0.0f));
	}

	inline f32 F32Min(f32 a, f32 b)
	{
		return a < b ? a : b;
	}

	inline f32 F32Max(f32 a, f32 b)
	{
		return a > b ? a : b;
	}

	inline f32 F32Clamp(f32 x, f32 min, f32 max)
	{
		return x < min ? min : x > max ? max : x;
	}

	inline f32 F32Saturate(f32 x)
	{
		return F32Clamp(x, 0.0f, 1.0f);
	}

	// Mod keeps the sign of its input, so offsets below min move up a period. Adding the period can round up to
	// exactly max, which wraps to min.
	inline f32 F32Wrap(f32 x, f32 min, f32 max)
	{
		if (x >= min && x < max)
			return x;

		f32 period = max - min;
		f32 offset = F32Mod(x - min, period);
		offset = offset < 0.0f ? offset + period : offset;
		f32 result = min + offset;
		return result < max ? result : min;
	}

	inline f32 F32Lerp(f32 a, f32 b, f32 t)
	{
		return a + (b - a) * t;
	}

	inline f32 F32Remap(f32 x, f32 fromMin, f32 fromMax, f32 toMin, f32 toMax)
	{
		return F32Lerp(toMin, toMax, (x - fromMin) / (fromMax - fromMin));
	}

	inline f32 F32Radians(f32 degrees)
	{
		return degrees * (F32_PI / 180.0f);
	}

	inline f32 F32Degrees(f32 radians)
	{
		return radians * (180.0f / F32_PI);
	}

	//~ Integers

	// Signed wraps work in unsigned arithmetic, where the period and distances can't overflow. Below min, the
	// distance d down from min maps to the offset (-d) mod period, written so that it stays below the period.

	inline i64 I64Abs(i64 x)
	{
		return x < 0 ? -x : x;
	}

	inline i64 I64Sign(i64 x)
	{
		return (x > 0) - (x < 0);
	}

	inline i64 I64Min(i64 a, i64 b)
	{
		return a < b ? a : b;
	}

	inline i64 I64Max(i64 a, i64 b)
	{
		return a > b ? a : b;
	}

	inline i64 I64Clamp(i64 x, i64 min, i64 max)
	{
		return x < min ? min : x > max ? max : x;
	}

	inline i64 I64Wrap(i64 x, i64 min, i64 max)
	{
		if (x >= min && x < max)
			return x;

		u64 period = (u64)max - (u64)min;
		u64 offset = x >= min ? ((u64)x - (u64)min) % period : period - 1 - ((u64)min - (u64)x - 1) % period;
		return (i64)((u64)min + offset);
	}

	inline u64 U64Min(u64 a, u64 b)
	{
		return a < b ? a : b;
	}

	inline u64 U64Max(u64 a, u64 b)
	{
		return a > b ? a : b;
	}

	inline u64 U64Clamp(u64 x, u64 min, u64 max)
	{
		return x < min ? min : x > max ? max : x;
	}

	inline u64 U64Wrap(u64 x, u64 min, u64 max)
	{
		if (x >= min && x < max)
			return x;

		u64 period = max - min;
		u64 offset = x >= min ? (x - min) % period : period - 1 - (min - x - 1) % period;
		return min + offset;
	}

	// At compile time, and where there is no 128-bit type or intrinsic, the product is built from 32-bit halves.
	constexpr u64 U64MulWide(u64 a, u64 b, u64* high)
	{
#if defined(__SIZEOF_INT128__)
		unsigned __int128 product = (unsigned __int128)a * b;
		*high = (u64)(product >> 64);
		return (u64)product;
#else
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
		if (!__builtin_is_constant_evaluated())
		{
#if defined(_M_X64)
			return _umul128(a, b, high);
#else
			*high = __umulh(a, b);
			return a * b;
#endif
		}
#endif
		u64 ha = a >> 32;
		u64 hb = b >> 32;
		u64 la = a & 0xffffffffull;
		u64 lb = b & 0xffffffffull;
		u64 rh = ha * hb;
		u64 rm0 = ha * lb;
		u64 rm1 = hb * la;
		u64 rl = la * lb;
		u64 t = rl + (rm0 << 32);
		u64 carry = (t < rl) ? 1 : 0;
		u64 low = t + (rm1 << 32);
		carry += (low < t) ? 1 : 0;
		*high = rh + (rm0 >> 32) + (rm1 >> 32) + carry;
		return low;
#endif
	}

	inline i32 I32Abs(i32 x)
	{
		return x < 0 ? -x : x;
	}

	inline i32 I32Sign(i32 x)
	{
		return (x > 0) - (x < 0);
	}

	inline i32 I32Min(i32 a, i32 b)
	{
		return a < b ? a : b;
	}

	inline i32 I32Max(i32 a, i32 b)
	{
		return a > b ? a : b;
	}

	inline i32 I32Clamp(i32 x, i32 min, i32 max)
	{
		return x < min ? min : x > max ? max : x;
	}

	inline i32 I32Wrap(i32 x, i32 min, i32 max)
	{
		if (x >= min && x < max)
			return x;

		u32 period = (u32)max - (u32)min;
		u32 offset = x >= min ? ((u32)x - (u32)min) % period : period - 1 - ((u32)min - (u32)x - 1) % period;
		return (i32)((u32)min + offset);
	}

	inline u32 U32Min(u32 a, u32 b)
	{
		return a < b ? a : b;
	}

	inline u32 U32Max(u32 a, u32 b)
	{
		return a > b ? a : b;
	}

	inline u32 U32Clamp(u32 x, u32 min, u32 max)
	{
		return x < min ? min : x > max ? max : x;
	}

	inline u32 U32Wrap(u32 x, u32 min, u32 max)
	{
		if (x >= min && x < max)
			return x;

		u32 period = max - min;
		u32 offset = x >= min ? (x - min) % period : period - 1 - (min - x - 1) % period;
		return min + offset;
	}
} //namespace Tool



#ifndef TOOL_NO_MATH

// f64
using Tool::F64Sin;
using Tool::F64Cos;
using Tool::F64Tan;
using Tool::F64Asin;
using Tool::F64Acos;
using Tool::F64Atan;
using Tool::F64Atan2;
using Tool::F64Sinh;
using Tool::F64Cosh;
using Tool::F64Tanh;
using Tool::F64Asinh;
using Tool::F64Acosh;
using Tool::F64Atanh;
using Tool::F64Round;
using Tool::F64Ceil;
using Tool::F64Floor;
using Tool::F64Trunc;
using Tool::F64Pow;
using Tool::F64Exp;
using Tool::F64Exp2;
using Tool::F64Log;
using Tool::F64Log2;
using Tool::F64Log10;
using Tool::F64Sqrt;
using Tool::F64Cbrt;
using Tool::F64Mod;
using Tool::F64Abs;
using Tool::F64Sign;
using Tool::F64Min;
using Tool::F64Max;
using Tool::F64Clamp;
using Tool::F64Saturate;
using Tool::F64Wrap;
using Tool::F64Lerp;
using Tool::F64Remap;
using Tool::F64Radians;
using Tool::F64Degrees;

// f32
using Tool::F32Sin;
using Tool::F32Cos;
using Tool::F32Tan;
using Tool::F32Asin;
using Tool::F32Acos;
using Tool::F32Atan;
using Tool::F32Atan2;
using Tool::F32Sinh;
using Tool::F32Cosh;
using Tool::F32Tanh;
using Tool::F32Asinh;
using Tool::F32Acosh;
using Tool::F32Atanh;
using Tool::F32Round;
using Tool::F32Ceil;
using Tool::F32Floor;
using Tool::F32Trunc;
using Tool::F32Pow;
using Tool::F32Exp;
using Tool::F32Exp2;
using Tool::F32Log;
using Tool::F32Log2;
using Tool::F32Log10;
using Tool::F32Sqrt;
using Tool::F32Cbrt;
using Tool::F32Mod;
using Tool::F32Abs;
using Tool::F32Sign;
using Tool::F32Min;
using Tool::F32Max;
using Tool::F32Clamp;
using Tool::F32Saturate;
using Tool::F32Wrap;
using Tool::F32Lerp;
using Tool::F32Remap;
using Tool::F32Radians;
using Tool::F32Degrees;

// Integers
using Tool::I64Abs;
using Tool::I64Sign;
using Tool::I64Min;
using Tool::I64Max;
using Tool::I64Clamp;
using Tool::I64Wrap;
using Tool::U64Min;
using Tool::U64Max;
using Tool::U64Clamp;
using Tool::U64Wrap;
using Tool::U64MulWide;
using Tool::I32Abs;
using Tool::I32Sign;
using Tool::I32Min;
using Tool::I32Max;
using Tool::I32Clamp;
using Tool::I32Wrap;
using Tool::U32Min;
using Tool::U32Max;
using Tool::U32Clamp;
using Tool::U32Wrap;

#endif



#endif
