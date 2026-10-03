#include "tool.h"
#include "test.h"
#include <stdio.h>

using namespace Tool;



// Integer minimums are signed.
static_assert(I32_MIN < 0 && I64_MIN < 0);
static_assert(I32_MIN == (i32)0x80000000u && I64_MIN == (i64)0x8000000000000000ull);

static void TestFloats()
{
	volatile f64 huge = 1e300;
	f64 infinity = huge * huge;

	// Clamp bounds are inclusive, and infinite bounds don't poison the result.
	TOOL_ASSERT(F64Clamp(-0.5, 0.0, 1.0) == 0.0);
	TOOL_ASSERT(F64Clamp(1.0, 0.0, 1.0) == 1.0);
	TOOL_ASSERT(F64Clamp(0.5, -infinity, 1.0) == 0.5);
	TOOL_ASSERT(F64Clamp(2.0, -infinity, 1.0) == 1.0);
	TOOL_ASSERT(F32Clamp(-2.0f, 0.0f, (f32)infinity) == 0.0f);
	TOOL_ASSERT(F32Clamp(1.5f, 0.0f, 1.0f) == 1.0f);
	TOOL_ASSERT(F32Saturate(-0.5f) == 0.0f && F32Saturate(0.25f) == 0.25f && F32Saturate(3.0f) == 1.0f);
	TOOL_ASSERT(F64Saturate(1.5) == 1.0);

	// Wrap is min inclusive, max exclusive, and wraps inputs below min up into range.
	TOOL_ASSERT(F32Wrap(0.25f, 0.0f, 1.0f) == 0.25f);
	TOOL_ASSERT(F32Wrap(1.0f, 0.0f, 1.0f) == 0.0f);
	TOOL_ASSERT(F32Wrap(1.25f, 0.0f, 1.0f) == 0.25f);
	TOOL_ASSERT(F32Wrap(-0.25f, 0.0f, 1.0f) == 0.75f);
	TOOL_ASSERT(F32Wrap(-1.0f, 0.0f, 1.0f) == 0.0f);
	TOOL_ASSERT(F32Wrap(-2.5f, -1.0f, 1.0f) == -0.5f);
	TOOL_ASSERT(F64Wrap(-0.25, 0.0, 1.0) == 0.75);
	TOOL_ASSERT(F64Wrap(370.0, 0.0, 360.0) == 10.0);

	// A tiny negative offset rounds up to the period, which must wrap to min rather than return max.
	TOOL_ASSERT(F32Wrap(-1e-10f, 0.0f, 1.0f) == 0.0f);
	TOOL_ASSERT(F64Wrap(-1e-20, 0.0, 1.0) == 0.0);

	// Abs gives +0 for -0.
	TOOL_ASSERT(F32Abs(-2.0f) == 2.0f && F32Abs(3.0f) == 3.0f);
	TOOL_ASSERT(F64Abs(-0.0) == 0.0 && 1.0 / F64Abs(-0.0) > 0.0);
	TOOL_ASSERT(F32Sign(-3.0f) == -1.0f && F32Sign(0.0f) == 0.0f && F32Sign(2.0f) == 1.0f);
	TOOL_ASSERT(F64Sign(-3.0) == -1.0 && F64Sign(0.0) == 0.0 && F64Sign(2.0) == 1.0);
	TOOL_ASSERT(F32Min(1.0f, 2.0f) == 1.0f && F32Max(1.0f, 2.0f) == 2.0f);
	TOOL_ASSERT(F64Min(-1.0, 2.0) == -1.0 && F64Max(-1.0, 2.0) == 2.0);

	// Lerp and Remap extrapolate.
	TOOL_ASSERT(F32Lerp(2.0f, 4.0f, 0.0f) == 2.0f && F32Lerp(2.0f, 4.0f, 1.0f) == 4.0f);
	TOOL_ASSERT(F32Lerp(2.0f, 4.0f, 0.5f) == 3.0f && F32Lerp(2.0f, 4.0f, 2.0f) == 6.0f);
	TOOL_ASSERT(F64Lerp(-1.0, 1.0, 0.25) == -0.5);
	TOOL_ASSERT(F32Remap(5.0f, 0.0f, 10.0f, 100.0f, 200.0f) == 150.0f);
	TOOL_ASSERT(F32Remap(15.0f, 0.0f, 10.0f, 100.0f, 200.0f) == 250.0f);
	TOOL_ASSERT(F64Remap(0.0, -1.0, 1.0, 1.0, 0.0) == 0.5);

	TOOL_ASSERT(F32Abs(F32Radians(180.0f) - F32_PI) < 1e-6f);
	TOOL_ASSERT(F32Abs(F32Degrees(F32_PI / 2.0f) - 90.0f) < 1e-4f);
	TOOL_ASSERT(F64Abs(F64Radians(90.0) - F64_PI / 2.0) < 1e-15);
	TOOL_ASSERT(F64Abs(F64Degrees(F64_TAU) - 360.0) < 1e-12);

	// The library wrappers reach the right function at each width.
	TOOL_ASSERT(F64Abs(F64Sin(F64_PI / 6.0) - 0.5) < 1e-15);
	TOOL_ASSERT(F64Abs(F64Atan2(1.0, -1.0) - 0.75 * F64_PI) < 1e-15);
	TOOL_ASSERT(F64Abs(F64Sqrt(2.0) * F64Sqrt(2.0) - 2.0) < 1e-15);
	TOOL_ASSERT(F32Abs(F32Cos(F32_PI) + 1.0f) < 1e-6f);
	TOOL_ASSERT(F32Pow(2.0f, 10.0f) == 1024.0f && F64Exp2(-1.0) == 0.5 && F64Log2(8.0) == 3.0);
	TOOL_ASSERT(F32Floor(-1.5f) == -2.0f && F32Ceil(-1.5f) == -1.0f && F32Trunc(-1.5f) == -1.0f && F32Round(-1.5f) == -2.0f);
	TOOL_ASSERT(F64Mod(-7.0, 3.0) == -1.0 && F32Mod(7.0f, 3.0f) == 1.0f);
}

static void TestIntegers()
{
	// Clamp bounds are inclusive.
	TOOL_ASSERT(I32Clamp(-3, 0, 10) == 0 && I32Clamp(10, 0, 10) == 10 && I32Clamp(11, 0, 10) == 10);
	TOOL_ASSERT(I64Clamp(5, 0, 10) == 5 && U32Clamp(1, 2, 4) == 2 && U64Clamp(9, 2, 4) == 4);

	// Wrap is min inclusive, max exclusive, and wraps inputs below min up into range.
	TOOL_ASSERT(I32Wrap(5, 0, 10) == 5);
	TOOL_ASSERT(I32Wrap(10, 0, 10) == 0);
	TOOL_ASSERT(I32Wrap(23, 0, 10) == 3);
	TOOL_ASSERT(I32Wrap(-1, 0, 10) == 9);
	TOOL_ASSERT(I32Wrap(-10, 0, 10) == 0);
	TOOL_ASSERT(I32Wrap(-11, 0, 10) == 9);
	TOOL_ASSERT(I32Wrap(-7, -5, 5) == 3);
	TOOL_ASSERT(I64Wrap(-1, 0, 10) == 9 && I64Wrap(-21, -5, 5) == -1);
	TOOL_ASSERT(U32Wrap(1, 3, 6) == 4 && U32Wrap(9, 3, 6) == 3);
	TOOL_ASSERT(U64Wrap(0, 5, 8) == 6 && U64Wrap(8, 5, 8) == 5);

	// Periods and distances past the type's range don't overflow.
	TOOL_ASSERT(I32Wrap(I32_MIN, 0, I32_MAX) == I32_MAX - 1);
	TOOL_ASSERT(I32Wrap(I32_MAX, I32_MIN, I32_MAX) == I32_MIN);
	TOOL_ASSERT(I32Wrap(I32_MIN, I32_MAX - 3, I32_MAX) == I32_MAX - 3);
	TOOL_ASSERT(I64Wrap(I64_MIN, 0, I64_MAX) == I64_MAX - 1);
	TOOL_ASSERT(I64Wrap(I64_MAX, I64_MIN, I64_MAX) == I64_MIN);
	TOOL_ASSERT(U32Wrap(U32_MAX, 0, U32_MAX) == 0 && U32Wrap(0, 1, U32_MAX) == U32_MAX - 1);
	TOOL_ASSERT(U64Wrap(U64_MAX, 0, 10) == U64_MAX % 10 && U64Wrap(0, 1, U64_MAX) == U64_MAX - 1);

	TOOL_ASSERT(I32Abs(-4) == 4 && I64Abs(-4) == 4 && I32Abs(I32_MAX) == I32_MAX);
	TOOL_ASSERT(I32Sign(-4) == -1 && I32Sign(0) == 0 && I64Sign(I64_MAX) == 1);
	TOOL_ASSERT(I32Min(-1, 1) == -1 && I64Max(-1, 1) == 1 && U32Min(1, 2) == 1 && U64Max(1, 2) == 2);
}

static void TestMacros()
{
	// Arguments are parenthesized.
	TOOL_ASSERT(TOOL_ABS(1 - 3) == 2);
	TOOL_ASSERT(TOOL_MAX(1 + 1, 3) * 2 == 6);
	TOOL_ASSERT(TOOL_MIN(2, 3) - 1 == 1);
}



void TestMathematics()
{
	TestFloats();
	TestIntegers();
	TestMacros();
	printf("Mathematics: passed\n");
}
