#ifndef _TOOL_INTRINSICS_H
#define _TOOL_INTRINSICS_H

#include "basics.h"
#include "vector.h"



//~ SIMD
//
// 128- and 256-bit lane types with one set of functions that compiles on every target. Opt-in: not part of tool.h.
//
// The backend is picked at compile time:
//   Clang/GCC (clang-cl and Emscripten included): compiler vector extensions. No headers. The compiler lowers each
//     operation to the target's SIMD (SSE/AVX, NEON, wasm simd128 with -msimd128), or to scalar code without it.
//   MSVC x64: SSE/AVX intrinsics, using what the build enables. MSVC defines no SSE4/FMA macros, so SSE3/SSSE3/SSE4.1
//     are taken from /arch:AVX and FMA from /arch:AVX2. Missing instructions are emulated with SSE2.
//   Anything else, or TOOL_SIMD_SCALAR defined before including: per-lane loops.
// There is no runtime CPU dispatch. The build flags decide, so a build must only enable what its target CPUs have.
// Every file in a program must be built with the same flags: these functions are inline, and the linker keeps one
// copy of each, so code built for two instruction sets mixes. On GCC and Clang, 8-wide values are also passed
// differently with and without AVX.
//
// Lane 0 is the lowest address, and the first argument of every Set. Loads and stores are unaligned.
// Integer arithmetic wraps. A mask lane is all ones or all zeros, as Compare returns them.
// Results that depend on the hardware are noted per function. The native register 'm' is backend-specific.



//- Backend selection

#if defined(TOOL_SIMD_SCALAR)
#elif defined(__clang__) || defined(__GNUC__)
#define TOOL_SIMD_VECTOR 1
#elif defined(_MSC_VER) && defined(_M_X64) && !defined(_M_ARM64EC)
#define TOOL_SIMD_X86 1
#if defined(__AVX__)
#define TOOL_SIMD_AVX 1 // SSE3, SSSE3, SSE4.1 and 256-bit floats
#endif
#if defined(__AVX2__)
#define TOOL_SIMD_AVX2 1 // 256-bit integers and FMA
#endif
#if defined(__AVX512VL__) && defined(__AVX512DQ__)
#define TOOL_SIMD_AVX512 1 // 64-bit integer multiply and abs
#endif
#else
#define TOOL_SIMD_SCALAR 1
#endif

// 256-bit types without native registers are two 128-bit halves.
#if defined(TOOL_SIMD_SCALAR) || (defined(TOOL_SIMD_X86) && !defined(TOOL_SIMD_AVX))
#define TOOL_SIMD_HALVES_FLOAT256 1
#endif
#if defined(TOOL_SIMD_SCALAR) || (defined(TOOL_SIMD_X86) && !defined(TOOL_SIMD_AVX2))
#define TOOL_SIMD_HALVES_INT256 1
#endif

#if defined(TOOL_SIMD_X86)
#if defined(TOOL_SIMD_AVX)
#include <immintrin.h>
#else
#include <emmintrin.h>
#endif
#endif



namespace Tool
{
	//- Type definitions

	//~ Native registers

#if defined(TOOL_SIMD_VECTOR)
	typedef f32 SimdF32x4 __attribute__((vector_size(16)));
	typedef f32 SimdF32x8 __attribute__((vector_size(32)));
	typedef f64 SimdF64x2 __attribute__((vector_size(16)));
	typedef f64 SimdF64x4 __attribute__((vector_size(32)));

	typedef i8 SimdI8x16 __attribute__((vector_size(16)));
	typedef i16 SimdI16x8 __attribute__((vector_size(16)));
	typedef i32 SimdI32x4 __attribute__((vector_size(16)));
	typedef i64 SimdI64x2 __attribute__((vector_size(16)));
	typedef i8 SimdI8x32 __attribute__((vector_size(32)));
	typedef i16 SimdI16x16 __attribute__((vector_size(32)));
	typedef i32 SimdI32x8 __attribute__((vector_size(32)));
	typedef i64 SimdI64x4 __attribute__((vector_size(32)));

	// Unsigned lanes, for wrapping arithmetic and bitwise operations
	typedef u8 SimdU8x16 __attribute__((vector_size(16)));
	typedef u16 SimdU16x8 __attribute__((vector_size(16)));
	typedef u32 SimdU32x4 __attribute__((vector_size(16)));
	typedef u64 SimdU64x2 __attribute__((vector_size(16)));
	typedef u8 SimdU8x32 __attribute__((vector_size(32)));
	typedef u16 SimdU16x16 __attribute__((vector_size(32)));
	typedef u32 SimdU32x8 __attribute__((vector_size(32)));
	typedef u64 SimdU64x4 __attribute__((vector_size(32)));

	typedef SimdI64x2 SimdI128; // Integer128 register, recast per lane width
	typedef SimdI64x4 SimdI256;
#elif defined(TOOL_SIMD_X86)
	typedef __m128 SimdF32x4;
	typedef __m128d SimdF64x2;
	typedef __m128i SimdI128;
#if defined(TOOL_SIMD_AVX)
	typedef __m256 SimdF32x8;
	typedef __m256d SimdF64x4;
	typedef __m256i SimdI256;
#endif
#endif

	//~ 128-bit

	union Float32x4
	{
#if !defined(TOOL_SIMD_SCALAR)
		SimdF32x4 m;
#endif

		f32 f[4];

		v2 vector2[2];
		v4 vector4;
	};

	union Float64x2
	{
#if !defined(TOOL_SIMD_SCALAR)
		SimdF64x2 m;
#endif

		f64 f[2];

		v2d vector2d;
	};

	union Integer128
	{
#if !defined(TOOL_SIMD_SCALAR)
		SimdI128 m;
#endif

		i8 int8[16];
		i16 int16[8];
		i32 int32[4];
		i64 int64[2];

		p2 point2[2];
		p4 point4;
	};

	//~ 256-bit

	union Float32x8
	{
#if !defined(TOOL_SIMD_HALVES_FLOAT256)
		SimdF32x8 m;
#endif

		Float32x4 half[2];
		f32 f[8];

		v2 vector2[4];
		v4 vector4[2];
	};

	union Float64x4
	{
#if !defined(TOOL_SIMD_HALVES_FLOAT256)
		SimdF64x4 m;
#endif

		Float64x2 half[2];
		f64 f[4];

		v2d vector2d[2];
		v4d vector4d;
	};

	union Integer256
	{
#if !defined(TOOL_SIMD_SCALAR) && (defined(TOOL_SIMD_VECTOR) || defined(TOOL_SIMD_AVX))
		SimdI256 m;
#endif

		Integer128 half[2];
		i8 int8[32];
		i16 int16[16];
		i32 int32[8];
		i64 int64[4];

		p2 point2[4];
		p4 point4[2];
	};

	//~ Masks

	union Bitmask
	{
		i32 i;

		struct
		{
			b8 b0 : 1;
			b8 b1 : 1;
			b8 b2 : 1;
			b8 b3 : 1;

			b8 b4 : 1;
			b8 b5 : 1;
			b8 b6 : 1;
			b8 b7 : 1;
		};
	};

	//~ Comparison

	// Ordered: a lane where either side is NaN compares false, NotEquals included.
	enum Comparison
	{
		ComparisonEquals,              // a == b
		ComparisonNotEquals,           // a != b
		ComparisonGreaterThan,         // a > b
		ComparisonGreaterThanOrEquals, // a >= b
		ComparisonLessThan,            // a < b
		ComparisonLessThanOrEquals,    // a <= b
	};

	//~ Acronyms

	typedef Float32x4 f32_x4;
	typedef Float32x8 f32_x8;

	typedef Float64x2 f64_x2;
	typedef Float64x4 f64_x4;

	typedef Float32x4 f32_x4_mask;
	typedef Float32x8 f32_x8_mask;
	typedef Float64x2 f64_x2_mask;
	typedef Float64x4 f64_x4_mask;

	typedef Integer128 i8_x16;
	typedef Integer128 i16_x8;
	typedef Integer128 i32_x4;
	typedef Integer128 i64_x2;

	typedef Integer256 i8_x32;
	typedef Integer256 i16_x16;
	typedef Integer256 i32_x8;
	typedef Integer256 i64_x4;



	//- Functions
	//
	// One function per operation and lane type, named by the type: F32x4Add, I16x8Mul.

	//~ Set
	// One value in every lane, or each lane's own value from lane 0 up. Vectors give their components in order.

	inline f32_x4 F32x4Set(f32 all);
	inline f32_x8 F32x8Set(f32 all);
	inline f64_x2 F64x2Set(f64 all);
	inline f64_x4 F64x4Set(f64 all);
	inline i8_x16 I8x16Set(i8 all);
	inline i8_x32 I8x32Set(i8 all);
	inline i16_x8 I16x8Set(i16 all);
	inline i16_x16 I16x16Set(i16 all);
	inline i32_x4 I32x4Set(i32 all);
	inline i32_x8 I32x8Set(i32 all);
	inline i64_x2 I64x2Set(i64 all);
	inline i64_x4 I64x4Set(i64 all);

	inline f32_x4 F32x4Set(f32 f0, f32 f1, f32 f2, f32 f3);
	inline f32_x8 F32x8Set(f32 f0, f32 f1, f32 f2, f32 f3, f32 f4, f32 f5, f32 f6, f32 f7);
	inline f64_x2 F64x2Set(f64 f0, f64 f1);
	inline f64_x4 F64x4Set(f64 f0, f64 f1, f64 f2, f64 f3);
	inline i8_x16 I8x16Set(i8 i0, i8 i1, i8 i2, i8 i3, i8 i4, i8 i5, i8 i6, i8 i7, i8 i8_, i8 i9, i8 i10, i8 i11, i8 i12, i8 i13, i8 i14, i8 i15);
	inline i8_x32 I8x32Set(i8 i0, i8 i1, i8 i2, i8 i3, i8 i4, i8 i5, i8 i6, i8 i7, i8 i8_, i8 i9, i8 i10, i8 i11, i8 i12, i8 i13, i8 i14, i8 i15, i8 i16_, i8 i17, i8 i18, i8 i19, i8 i20, i8 i21, i8 i22, i8 i23, i8 i24, i8 i25, i8 i26, i8 i27, i8 i28, i8 i29, i8 i30, i8 i31);
	inline i16_x8 I16x8Set(i16 i0, i16 i1, i16 i2, i16 i3, i16 i4, i16 i5, i16 i6, i16 i7);
	inline i16_x16 I16x16Set(i16 i0, i16 i1, i16 i2, i16 i3, i16 i4, i16 i5, i16 i6, i16 i7, i16 i8_, i16 i9, i16 i10, i16 i11, i16 i12, i16 i13, i16 i14, i16 i15);
	inline i32_x4 I32x4Set(i32 i0, i32 i1, i32 i2, i32 i3);
	inline i32_x8 I32x8Set(i32 i0, i32 i1, i32 i2, i32 i3, i32 i4, i32 i5, i32 i6, i32 i7);
	inline i64_x2 I64x2Set(i64 i0, i64 i1);
	inline i64_x4 I64x4Set(i64 i0, i64 i1, i64 i2, i64 i3);

	inline f32_x4 F32x4Set(v4 v);
	inline f32_x4 F32x4Set(v2 v0, v2 v1);
	inline f32_x8 F32x8Set(v4 v0, v4 v1);
	inline f32_x8 F32x8Set(v2 v0, v2 v1, v2 v2_, v2 v3);
	inline f64_x2 F64x2Set(v2d v);
	inline f64_x4 F64x4Set(v4d v);
	inline f64_x4 F64x4Set(v2d v0, v2d v1);
	inline i32_x4 I32x4Set(p4 p);
	inline i32_x4 I32x4Set(p2 p0, p2 p1);
	inline i32_x8 I32x8Set(p4 p0, p4 p1);
	inline i32_x8 I32x8Set(p2 p0, p2 p1, p2 p2_, p2 p3);

	//~ Load and store
	// LoadOne puts one value in every lane. Vector loads read the components in order.

	inline f32_x4 F32x4LoadOne(const f32* value);
	inline f64_x2 F64x2LoadOne(const f64* value);

	inline f32_x4 F32x4Load(const f32* values);
	inline f32_x8 F32x8Load(const f32* values);
	inline f64_x2 F64x2Load(const f64* values);
	inline f64_x4 F64x4Load(const f64* values);
	inline i8_x16 I8x16Load(const i8* values);
	inline i8_x32 I8x32Load(const i8* values);
	inline i16_x8 I16x8Load(const i16* values);
	inline i16_x16 I16x16Load(const i16* values);
	inline i32_x4 I32x4Load(const i32* values);
	inline i32_x8 I32x8Load(const i32* values);
	inline i64_x2 I64x2Load(const i64* values);
	inline i64_x4 I64x4Load(const i64* values);

	inline f32_x4 F32x4Load(const v2* vectors);
	inline f32_x4 F32x4Load(const v4* vector);
	inline f32_x8 F32x8Load(const v2* vectors);
	inline f32_x8 F32x8Load(const v4* vectors);
	inline f64_x2 F64x2Load(const v2d* vector);
	inline f64_x4 F64x4Load(const v2d* vectors);
	inline f64_x4 F64x4Load(const v4d* vector);

	inline void F32x4Store(f32_x4 src, f32* dst);
	inline void F32x8Store(f32_x8 src, f32* dst);
	inline void F64x2Store(f64_x2 src, f64* dst);
	inline void F64x4Store(f64_x4 src, f64* dst);
	inline void I8x16Store(i8_x16 src, i8* dst);
	inline void I8x32Store(i8_x32 src, i8* dst);
	inline void I16x8Store(i16_x8 src, i16* dst);
	inline void I16x16Store(i16_x16 src, i16* dst);
	inline void I32x4Store(i32_x4 src, i32* dst);
	inline void I32x8Store(i32_x8 src, i32* dst);
	inline void I64x2Store(i64_x2 src, i64* dst);
	inline void I64x4Store(i64_x4 src, i64* dst);

	//~ Arithmetic
	// There is no 8-bit multiply, and division is floats only.

	inline f32_x4 F32x4Add(f32_x4 a, f32_x4 b);
	inline f32_x8 F32x8Add(f32_x8 a, f32_x8 b);
	inline f64_x2 F64x2Add(f64_x2 a, f64_x2 b);
	inline f64_x4 F64x4Add(f64_x4 a, f64_x4 b);
	inline i8_x16 I8x16Add(i8_x16 a, i8_x16 b);
	inline i8_x32 I8x32Add(i8_x32 a, i8_x32 b);
	inline i16_x8 I16x8Add(i16_x8 a, i16_x8 b);
	inline i16_x16 I16x16Add(i16_x16 a, i16_x16 b);
	inline i32_x4 I32x4Add(i32_x4 a, i32_x4 b);
	inline i32_x8 I32x8Add(i32_x8 a, i32_x8 b);
	inline i64_x2 I64x2Add(i64_x2 a, i64_x2 b);
	inline i64_x4 I64x4Add(i64_x4 a, i64_x4 b);

	inline f32_x4 F32x4Sub(f32_x4 a, f32_x4 b);
	inline f32_x8 F32x8Sub(f32_x8 a, f32_x8 b);
	inline f64_x2 F64x2Sub(f64_x2 a, f64_x2 b);
	inline f64_x4 F64x4Sub(f64_x4 a, f64_x4 b);
	inline i8_x16 I8x16Sub(i8_x16 a, i8_x16 b);
	inline i8_x32 I8x32Sub(i8_x32 a, i8_x32 b);
	inline i16_x8 I16x8Sub(i16_x8 a, i16_x8 b);
	inline i16_x16 I16x16Sub(i16_x16 a, i16_x16 b);
	inline i32_x4 I32x4Sub(i32_x4 a, i32_x4 b);
	inline i32_x8 I32x8Sub(i32_x8 a, i32_x8 b);
	inline i64_x2 I64x2Sub(i64_x2 a, i64_x2 b);
	inline i64_x4 I64x4Sub(i64_x4 a, i64_x4 b);

	inline f32_x4 F32x4Mul(f32_x4 a, f32_x4 b);
	inline f32_x8 F32x8Mul(f32_x8 a, f32_x8 b);
	inline f64_x2 F64x2Mul(f64_x2 a, f64_x2 b);
	inline f64_x4 F64x4Mul(f64_x4 a, f64_x4 b);
	inline i16_x8 I16x8Mul(i16_x8 a, i16_x8 b);
	inline i16_x16 I16x16Mul(i16_x16 a, i16_x16 b);
	inline i32_x4 I32x4Mul(i32_x4 a, i32_x4 b);
	inline i32_x8 I32x8Mul(i32_x8 a, i32_x8 b);
	inline i64_x2 I64x2Mul(i64_x2 a, i64_x2 b);
	inline i64_x4 I64x4Mul(i64_x4 a, i64_x4 b);

	inline f32_x4 F32x4Div(f32_x4 a, f32_x4 b);
	inline f32_x8 F32x8Div(f32_x8 a, f32_x8 b);
	inline f64_x2 F64x2Div(f64_x2 a, f64_x2 b);
	inline f64_x4 F64x4Div(f64_x4 a, f64_x4 b);

	// a * b + c and a * b - c. Fused (one rounding) where the target has FMA, a multiply then an add otherwise.
	inline f32_x4 F32x4MulAdd(f32_x4 a, f32_x4 b, f32_x4 c);
	inline f32_x8 F32x8MulAdd(f32_x8 a, f32_x8 b, f32_x8 c);
	inline f64_x2 F64x2MulAdd(f64_x2 a, f64_x2 b, f64_x2 c);
	inline f64_x4 F64x4MulAdd(f64_x4 a, f64_x4 b, f64_x4 c);

	inline f32_x4 F32x4MulSub(f32_x4 a, f32_x4 b, f32_x4 c);
	inline f32_x8 F32x8MulSub(f32_x8 a, f32_x8 b, f32_x8 c);
	inline f64_x2 F64x2MulSub(f64_x2 a, f64_x2 b, f64_x2 c);
	inline f64_x4 F64x4MulSub(f64_x4 a, f64_x4 b, f64_x4 c);

	// Even lanes subtract, odd lanes add.
	inline f32_x4 F32x4AddSub(f32_x4 a, f32_x4 b);
	inline f32_x8 F32x8AddSub(f32_x8 a, f32_x8 b);
	inline f64_x2 F64x2AddSub(f64_x2 a, f64_x2 b);
	inline f64_x4 F64x4AddSub(f64_x4 a, f64_x4 b);

	//~ Absolute value
	// The most negative value stays itself, as in hardware.

	inline i8_x16 I8x16Abs(i8_x16 a);
	inline i8_x32 I8x32Abs(i8_x32 a);
	inline i16_x8 I16x8Abs(i16_x8 a);
	inline i16_x16 I16x16Abs(i16_x16 a);
	inline i32_x4 I32x4Abs(i32_x4 a);
	inline i32_x8 I32x8Abs(i32_x8 a);
	inline i64_x2 I64x2Abs(i64_x2 a);
	inline i64_x4 I64x4Abs(i64_x4 a);

	//~ Square root and reciprocal
	// InvSqrt (1 / sqrt) and Inv (1 / x) are the hardware estimate on x86, with relative error under 1.5 * 2^-12,
	// and exact elsewhere.

	inline f32_x4 F32x4Sqrt(f32_x4 a);
	inline f32_x8 F32x8Sqrt(f32_x8 a);
	inline f64_x2 F64x2Sqrt(f64_x2 a);
	inline f64_x4 F64x4Sqrt(f64_x4 a);

	inline f32_x4 F32x4InvSqrt(f32_x4 a);
	inline f32_x8 F32x8InvSqrt(f32_x8 a);

	inline f32_x4 F32x4Inv(f32_x4 a);
	inline f32_x8 F32x8Inv(f32_x8 a);

	//~ Rounding
	// Round goes to the nearest whole number, ties to even.

	inline f32_x4 F32x4Ceil(f32_x4 a);
	inline f32_x8 F32x8Ceil(f32_x8 a);
	inline f64_x2 F64x2Ceil(f64_x2 a);
	inline f64_x4 F64x4Ceil(f64_x4 a);

	inline f32_x4 F32x4Floor(f32_x4 a);
	inline f32_x8 F32x8Floor(f32_x8 a);
	inline f64_x2 F64x2Floor(f64_x2 a);
	inline f64_x4 F64x4Floor(f64_x4 a);

	inline f32_x4 F32x4Round(f32_x4 a);
	inline f32_x8 F32x8Round(f32_x8 a);
	inline f64_x2 F64x2Round(f64_x2 a);
	inline f64_x4 F64x4Round(f64_x4 a);

	//~ Comparison and blending
	// Compare sets a mask lane where the comparison holds for that lane. Blend takes the lanes where 'mask' is set
	// from b and the rest from a.

	template<Comparison C> inline f32_x4_mask F32x4Compare(f32_x4 a, f32_x4 b);
	template<Comparison C> inline f32_x8_mask F32x8Compare(f32_x8 a, f32_x8 b);
	template<Comparison C> inline f64_x2_mask F64x2Compare(f64_x2 a, f64_x2 b);
	template<Comparison C> inline f64_x4_mask F64x4Compare(f64_x4 a, f64_x4 b);

	inline f32_x4 F32x4Blend(f32_x4 a, f32_x4 b, f32_x4_mask mask);
	inline f32_x8 F32x8Blend(f32_x8 a, f32_x8 b, f32_x8_mask mask);
	inline f64_x2 F64x2Blend(f64_x2 a, f64_x2 b, f64_x2_mask mask);
	inline f64_x4 F64x4Blend(f64_x4 a, f64_x4 b, f64_x4_mask mask);

	//~ Bitwise
	// AndNot is a & ~b.

	inline f32_x4 F32x4And(f32_x4 a, f32_x4 b);
	inline f32_x8 F32x8And(f32_x8 a, f32_x8 b);
	inline f64_x2 F64x2And(f64_x2 a, f64_x2 b);
	inline f64_x4 F64x4And(f64_x4 a, f64_x4 b);

	inline f32_x4 F32x4AndNot(f32_x4 a, f32_x4 b);
	inline f32_x8 F32x8AndNot(f32_x8 a, f32_x8 b);
	inline f64_x2 F64x2AndNot(f64_x2 a, f64_x2 b);
	inline f64_x4 F64x4AndNot(f64_x4 a, f64_x4 b);

	inline f32_x4 F32x4Or(f32_x4 a, f32_x4 b);
	inline f32_x8 F32x8Or(f32_x8 a, f32_x8 b);
	inline f64_x2 F64x2Or(f64_x2 a, f64_x2 b);
	inline f64_x4 F64x4Or(f64_x4 a, f64_x4 b);

	inline f32_x4 F32x4Xor(f32_x4 a, f32_x4 b);
	inline f32_x8 F32x8Xor(f32_x8 a, f32_x8 b);
	inline f64_x2 F64x2Xor(f64_x2 a, f64_x2 b);
	inline f64_x4 F64x4Xor(f64_x4 a, f64_x4 b);
} //namespace Tool



//- Implementation
//
// Each backend defines every function declared above. 256-bit types without native registers are two 128-bit
// halves, and their functions apply the 128-bit ones to each half.

#ifdef __has_builtin
#define TOOL_SIMD_HAS_BUILTIN(name) __has_builtin(name)
#else
#define TOOL_SIMD_HAS_BUILTIN(name) 0
#endif

// Function body computing each lane of 'Type' from 'Expression', which reads the lane index 'i'.
#define TOOL_SIMD_LANES(Type, Lanes, Count, Expression)                                                                \
    {                                                                                                                  \
        Type r_;                                                                                                       \
        for (u32 i = 0; i < (Count); i++)                                                                              \
            r_.Lanes[i] = (Expression);                                                                                \
        return r_;                                                                                                     \
    }

// Function bodies applying a 128-bit function to both halves of a 256-bit value.
#define TOOL_SIMD_HALVES1(Type, Function, a)                                                                           \
    {                                                                                                                  \
        Type r_;                                                                                                       \
        r_.half[0] = Function(a.half[0]);                                                                              \
        r_.half[1] = Function(a.half[1]);                                                                              \
        return r_;                                                                                                     \
    }

#define TOOL_SIMD_HALVES2(Type, Function, a, b)                                                                        \
    {                                                                                                                  \
        Type r_;                                                                                                       \
        r_.half[0] = Function(a.half[0], b.half[0]);                                                                   \
        r_.half[1] = Function(a.half[1], b.half[1]);                                                                   \
        return r_;                                                                                                     \
    }

#define TOOL_SIMD_HALVES3(Type, Function, a, b, c)                                                                     \
    {                                                                                                                  \
        Type r_;                                                                                                       \
        r_.half[0] = Function(a.half[0], b.half[0], c.half[0]);                                                        \
        r_.half[1] = Function(a.half[1], b.half[1], c.half[1]);                                                        \
        return r_;                                                                                                     \
    }



namespace Tool
{
#if defined(TOOL_SIMD_VECTOR)

	//- Vector extension backend

	//~ Set by value (all same)

	inline f32_x4 F32x4Set(f32 all)                        { return { SimdF32x4{} + all }; }
	inline f32_x8 F32x8Set(f32 all)                        { return { SimdF32x8{} + all }; }
	inline f64_x2 F64x2Set(f64 all)                        { return { SimdF64x2{} + all }; }
	inline f64_x4 F64x4Set(f64 all)                        { return { SimdF64x4{} + all }; }

	inline i8_x16 I8x16Set(i8 all)                         { return { (SimdI128)(SimdI8x16{} + all) }; }
	inline i8_x32 I8x32Set(i8 all)                         { return { (SimdI256)(SimdI8x32{} + all) }; }
	inline i16_x8 I16x8Set(i16 all)                        { return { (SimdI128)(SimdI16x8{} + all) }; }
	inline i16_x16 I16x16Set(i16 all)                      { return { (SimdI256)(SimdI16x16{} + all) }; }
	inline i32_x4 I32x4Set(i32 all)                        { return { (SimdI128)(SimdI32x4{} + all) }; }
	inline i32_x8 I32x8Set(i32 all)                        { return { (SimdI256)(SimdI32x8{} + all) }; }
	inline i64_x2 I64x2Set(i64 all)                        { return { SimdI64x2{} + all }; }
	inline i64_x4 I64x4Set(i64 all)                        { return { SimdI64x4{} + all }; }

	//~ Set by value (individual)

	inline f32_x4 F32x4Set(f32 f0, f32 f1, f32 f2, f32 f3) { return { SimdF32x4{ f0, f1, f2, f3 } }; }
	inline f32_x8 F32x8Set(f32 f0, f32 f1, f32 f2, f32 f3,
	                       f32 f4, f32 f5, f32 f6, f32 f7) { return { SimdF32x8{ f0, f1, f2, f3, f4, f5, f6, f7 } }; }

	inline f64_x2 F64x2Set(f64 f0, f64 f1)                 { return { SimdF64x2{ f0, f1 } }; }
	inline f64_x4 F64x4Set(f64 f0, f64 f1, f64 f2, f64 f3) { return { SimdF64x4{ f0, f1, f2, f3 } }; }

	inline i64_x2 I64x2Set(i64 i0, i64 i1)                 { return { SimdI64x2{ i0, i1 } }; }
	inline i64_x4 I64x4Set(i64 i0, i64 i1, i64 i2, i64 i3) { return { SimdI64x4{ i0, i1, i2, i3 } }; }

	inline i32_x4 I32x4Set(i32 i0, i32 i1, i32 i2, i32 i3) { return { (SimdI128)SimdI32x4{ i0, i1, i2, i3 } }; }
	inline i32_x8 I32x8Set(i32 i0, i32 i1, i32 i2, i32 i3,
	                       i32 i4, i32 i5, i32 i6, i32 i7) { return { (SimdI256)SimdI32x8{ i0, i1, i2, i3, i4, i5, i6, i7 } }; }

	inline i16_x8 I16x8Set(i16 i0, i16 i1, i16 i2, i16 i3,
	                       i16 i4, i16 i5, i16 i6, i16 i7) { return { (SimdI128)SimdI16x8{ i0, i1, i2, i3, i4, i5, i6, i7 } }; }
	inline i16_x16 I16x16Set(i16 i0, i16 i1, i16 i2, i16 i3,
	                         i16 i4, i16 i5, i16 i6, i16 i7,
	                         i16 i8_, i16 i9, i16 i10, i16 i11,
	                         i16 i12, i16 i13, i16 i14, i16 i15)
	{
		return { (SimdI256)SimdI16x16{ i0, i1, i2, i3, i4, i5, i6, i7, i8_, i9, i10, i11, i12, i13, i14, i15 } };
	}

	inline i8_x16 I8x16Set(i8 i0, i8 i1, i8 i2, i8 i3,
	                       i8 i4, i8 i5, i8 i6, i8 i7,
	                       i8 i8_, i8 i9, i8 i10, i8 i11,
	                       i8 i12, i8 i13, i8 i14, i8 i15)
	{
		return { (SimdI128)SimdI8x16{ i0, i1, i2, i3, i4, i5, i6, i7, i8_, i9, i10, i11, i12, i13, i14, i15 } };
	}
	inline i8_x32 I8x32Set(i8 i0, i8 i1, i8 i2, i8 i3,
	                       i8 i4, i8 i5, i8 i6, i8 i7,
	                       i8 i8_, i8 i9, i8 i10, i8 i11,
	                       i8 i12, i8 i13, i8 i14, i8 i15,
	                       i8 i16_, i8 i17, i8 i18, i8 i19,
	                       i8 i20, i8 i21, i8 i22, i8 i23,
	                       i8 i24, i8 i25, i8 i26, i8 i27,
	                       i8 i28, i8 i29, i8 i30, i8 i31)
	{
		return { (SimdI256)SimdI8x32{ i0, i1, i2, i3, i4, i5, i6, i7, i8_, i9, i10, i11, i12, i13, i14, i15,
		                              i16_, i17, i18, i19, i20, i21, i22, i23, i24, i25, i26, i27, i28, i29, i30, i31 } };
	}

	//~ Load

	inline f32_x4 F32x4LoadOne(const f32* value)           { return { SimdF32x4{} + *value }; }
	inline f64_x2 F64x2LoadOne(const f64* value)           { return { SimdF64x2{} + *value }; }

	inline f32_x4 F32x4Load(const f32* values)             { f32_x4 r; __builtin_memcpy(&r, values, 16); return r; }
	inline f32_x8 F32x8Load(const f32* values)             { f32_x8 r; __builtin_memcpy(&r, values, 32); return r; }
	inline f64_x2 F64x2Load(const f64* values)             { f64_x2 r; __builtin_memcpy(&r, values, 16); return r; }
	inline f64_x4 F64x4Load(const f64* values)             { f64_x4 r; __builtin_memcpy(&r, values, 32); return r; }

	inline i8_x16 I8x16Load(const i8* values)              { i8_x16 r; __builtin_memcpy(&r, values, 16); return r; }
	inline i8_x32 I8x32Load(const i8* values)              { i8_x32 r; __builtin_memcpy(&r, values, 32); return r; }
	inline i16_x8 I16x8Load(const i16* values)             { i16_x8 r; __builtin_memcpy(&r, values, 16); return r; }
	inline i16_x16 I16x16Load(const i16* values)           { i16_x16 r; __builtin_memcpy(&r, values, 32); return r; }
	inline i32_x4 I32x4Load(const i32* values)             { i32_x4 r; __builtin_memcpy(&r, values, 16); return r; }
	inline i32_x8 I32x8Load(const i32* values)             { i32_x8 r; __builtin_memcpy(&r, values, 32); return r; }
	inline i64_x2 I64x2Load(const i64* values)             { i64_x2 r; __builtin_memcpy(&r, values, 16); return r; }
	inline i64_x4 I64x4Load(const i64* values)             { i64_x4 r; __builtin_memcpy(&r, values, 32); return r; }

	//~ Store

	inline void F32x4Store(f32_x4 src, f32* dst)           { __builtin_memcpy(dst, &src, 16); }
	inline void F32x8Store(f32_x8 src, f32* dst)           { __builtin_memcpy(dst, &src, 32); }
	inline void F64x2Store(f64_x2 src, f64* dst)           { __builtin_memcpy(dst, &src, 16); }
	inline void F64x4Store(f64_x4 src, f64* dst)           { __builtin_memcpy(dst, &src, 32); }

	inline void I8x16Store(i8_x16 src, i8* dst)            { __builtin_memcpy(dst, &src, 16); }
	inline void I8x32Store(i8_x32 src, i8* dst)            { __builtin_memcpy(dst, &src, 32); }
	inline void I16x8Store(i16_x8 src, i16* dst)           { __builtin_memcpy(dst, &src, 16); }
	inline void I16x16Store(i16_x16 src, i16* dst)         { __builtin_memcpy(dst, &src, 32); }
	inline void I32x4Store(i32_x4 src, i32* dst)           { __builtin_memcpy(dst, &src, 16); }
	inline void I32x8Store(i32_x8 src, i32* dst)           { __builtin_memcpy(dst, &src, 32); }
	inline void I64x2Store(i64_x2 src, i64* dst)           { __builtin_memcpy(dst, &src, 16); }
	inline void I64x4Store(i64_x4 src, i64* dst)           { __builtin_memcpy(dst, &src, 32); }

	//~ Addition

	inline f32_x4  F32x4Add(f32_x4 a, f32_x4 b)            { return { a.m + b.m }; }
	inline f32_x8  F32x8Add(f32_x8 a, f32_x8 b)            { return { a.m + b.m }; }
	inline f64_x2  F64x2Add(f64_x2 a, f64_x2 b)            { return { a.m + b.m }; }
	inline f64_x4  F64x4Add(f64_x4 a, f64_x4 b)            { return { a.m + b.m }; }
	inline i8_x16  I8x16Add(i8_x16 a, i8_x16 b)            { return { (SimdI128)((SimdU8x16)a.m + (SimdU8x16)b.m) }; }
	inline i8_x32  I8x32Add(i8_x32 a, i8_x32 b)            { return { (SimdI256)((SimdU8x32)a.m + (SimdU8x32)b.m) }; }
	inline i16_x8  I16x8Add(i16_x8 a, i16_x8 b)            { return { (SimdI128)((SimdU16x8)a.m + (SimdU16x8)b.m) }; }
	inline i16_x16 I16x16Add(i16_x16 a, i16_x16 b)         { return { (SimdI256)((SimdU16x16)a.m + (SimdU16x16)b.m) }; }
	inline i32_x4  I32x4Add(i32_x4 a, i32_x4 b)            { return { (SimdI128)((SimdU32x4)a.m + (SimdU32x4)b.m) }; }
	inline i32_x8  I32x8Add(i32_x8 a, i32_x8 b)            { return { (SimdI256)((SimdU32x8)a.m + (SimdU32x8)b.m) }; }
	inline i64_x2  I64x2Add(i64_x2 a, i64_x2 b)            { return { (SimdI128)((SimdU64x2)a.m + (SimdU64x2)b.m) }; }
	inline i64_x4  I64x4Add(i64_x4 a, i64_x4 b)            { return { (SimdI256)((SimdU64x4)a.m + (SimdU64x4)b.m) }; }

	//~ Subtraction

	inline f32_x4  F32x4Sub(f32_x4 a, f32_x4 b)            { return { a.m - b.m }; }
	inline f32_x8  F32x8Sub(f32_x8 a, f32_x8 b)            { return { a.m - b.m }; }
	inline f64_x2  F64x2Sub(f64_x2 a, f64_x2 b)            { return { a.m - b.m }; }
	inline f64_x4  F64x4Sub(f64_x4 a, f64_x4 b)            { return { a.m - b.m }; }
	inline i8_x16  I8x16Sub(i8_x16 a, i8_x16 b)            { return { (SimdI128)((SimdU8x16)a.m - (SimdU8x16)b.m) }; }
	inline i8_x32  I8x32Sub(i8_x32 a, i8_x32 b)            { return { (SimdI256)((SimdU8x32)a.m - (SimdU8x32)b.m) }; }
	inline i16_x8  I16x8Sub(i16_x8 a, i16_x8 b)            { return { (SimdI128)((SimdU16x8)a.m - (SimdU16x8)b.m) }; }
	inline i16_x16 I16x16Sub(i16_x16 a, i16_x16 b)         { return { (SimdI256)((SimdU16x16)a.m - (SimdU16x16)b.m) }; }
	inline i32_x4  I32x4Sub(i32_x4 a, i32_x4 b)            { return { (SimdI128)((SimdU32x4)a.m - (SimdU32x4)b.m) }; }
	inline i32_x8  I32x8Sub(i32_x8 a, i32_x8 b)            { return { (SimdI256)((SimdU32x8)a.m - (SimdU32x8)b.m) }; }
	inline i64_x2  I64x2Sub(i64_x2 a, i64_x2 b)            { return { (SimdI128)((SimdU64x2)a.m - (SimdU64x2)b.m) }; }
	inline i64_x4  I64x4Sub(i64_x4 a, i64_x4 b)            { return { (SimdI256)((SimdU64x4)a.m - (SimdU64x4)b.m) }; }

	//~ Multiplication

	inline f32_x4  F32x4Mul(f32_x4 a, f32_x4 b)            { return { a.m * b.m }; }
	inline f32_x8  F32x8Mul(f32_x8 a, f32_x8 b)            { return { a.m * b.m }; }
	inline f64_x2  F64x2Mul(f64_x2 a, f64_x2 b)            { return { a.m * b.m }; }
	inline f64_x4  F64x4Mul(f64_x4 a, f64_x4 b)            { return { a.m * b.m }; }
	inline i16_x8  I16x8Mul(i16_x8 a, i16_x8 b)            { return { (SimdI128)((SimdU16x8)a.m * (SimdU16x8)b.m) }; }
	inline i16_x16 I16x16Mul(i16_x16 a, i16_x16 b)         { return { (SimdI256)((SimdU16x16)a.m * (SimdU16x16)b.m) }; }
	inline i32_x4  I32x4Mul(i32_x4 a, i32_x4 b)            { return { (SimdI128)((SimdU32x4)a.m * (SimdU32x4)b.m) }; }
	inline i32_x8  I32x8Mul(i32_x8 a, i32_x8 b)            { return { (SimdI256)((SimdU32x8)a.m * (SimdU32x8)b.m) }; }
	inline i64_x2  I64x2Mul(i64_x2 a, i64_x2 b)            { return { (SimdI128)((SimdU64x2)a.m * (SimdU64x2)b.m) }; }
	inline i64_x4  I64x4Mul(i64_x4 a, i64_x4 b)            { return { (SimdI256)((SimdU64x4)a.m * (SimdU64x4)b.m) }; }

	//~ Division

	inline f32_x4  F32x4Div(f32_x4 a, f32_x4 b)            { return { a.m / b.m }; }
	inline f32_x8  F32x8Div(f32_x8 a, f32_x8 b)            { return { a.m / b.m }; }
	inline f64_x2  F64x2Div(f64_x2 a, f64_x2 b)            { return { a.m / b.m }; }
	inline f64_x4  F64x4Div(f64_x4 a, f64_x4 b)            { return { a.m / b.m }; }

	//~ Absolute value
	// (a ^ sign) - sign, in wrapping lanes. The most negative value stays itself, as in hardware.

	inline i8_x16 I8x16Abs(i8_x16 a)
	{
		SimdU8x16 s = (SimdU8x16)((SimdI8x16)a.m >> 7);
		return { (SimdI128)(((SimdU8x16)a.m ^ s) - s) };
	}
	inline i8_x32 I8x32Abs(i8_x32 a)
	{
		SimdU8x32 s = (SimdU8x32)((SimdI8x32)a.m >> 7);
		return { (SimdI256)(((SimdU8x32)a.m ^ s) - s) };
	}
	inline i16_x8 I16x8Abs(i16_x8 a)
	{
		SimdU16x8 s = (SimdU16x8)((SimdI16x8)a.m >> 15);
		return { (SimdI128)(((SimdU16x8)a.m ^ s) - s) };
	}
	inline i16_x16 I16x16Abs(i16_x16 a)
	{
		SimdU16x16 s = (SimdU16x16)((SimdI16x16)a.m >> 15);
		return { (SimdI256)(((SimdU16x16)a.m ^ s) - s) };
	}
	inline i32_x4 I32x4Abs(i32_x4 a)
	{
		SimdU32x4 s = (SimdU32x4)((SimdI32x4)a.m >> 31);
		return { (SimdI128)(((SimdU32x4)a.m ^ s) - s) };
	}
	inline i32_x8 I32x8Abs(i32_x8 a)
	{
		SimdU32x8 s = (SimdU32x8)((SimdI32x8)a.m >> 31);
		return { (SimdI256)(((SimdU32x8)a.m ^ s) - s) };
	}
	inline i64_x2 I64x2Abs(i64_x2 a)
	{
		SimdU64x2 s = (SimdU64x2)(a.m >> 63);
		return { (SimdI128)(((SimdU64x2)a.m ^ s) - s) };
	}
	inline i64_x4 I64x4Abs(i64_x4 a)
	{
		SimdU64x4 s = (SimdU64x4)(a.m >> 63);
		return { (SimdI256)(((SimdU64x4)a.m ^ s) - s) };
	}

	//~ Square root

#if TOOL_SIMD_HAS_BUILTIN(__builtin_elementwise_sqrt)
	inline f32_x4  F32x4Sqrt(f32_x4 a)                     { return { __builtin_elementwise_sqrt(a.m) }; }
	inline f32_x8  F32x8Sqrt(f32_x8 a)                     { return { __builtin_elementwise_sqrt(a.m) }; }
	inline f64_x2  F64x2Sqrt(f64_x2 a)                     { return { __builtin_elementwise_sqrt(a.m) }; }
	inline f64_x4  F64x4Sqrt(f64_x4 a)                     { return { __builtin_elementwise_sqrt(a.m) }; }
#else
	inline f32_x4  F32x4Sqrt(f32_x4 a)                     TOOL_SIMD_LANES(f32_x4, f, 4, __builtin_sqrtf(a.f[i]))
	inline f32_x8  F32x8Sqrt(f32_x8 a)                     TOOL_SIMD_LANES(f32_x8, f, 8, __builtin_sqrtf(a.f[i]))
	inline f64_x2  F64x2Sqrt(f64_x2 a)                     TOOL_SIMD_LANES(f64_x2, f, 2, __builtin_sqrt(a.f[i]))
	inline f64_x4  F64x4Sqrt(f64_x4 a)                     TOOL_SIMD_LANES(f64_x4, f, 4, __builtin_sqrt(a.f[i]))
#endif

#if defined(__SSE__)
	inline f32_x4  F32x4InvSqrt(f32_x4 a)                  { return { __builtin_ia32_rsqrtps(a.m) }; }
#else
	inline f32_x4  F32x4InvSqrt(f32_x4 a)                  { return { 1.0f / F32x4Sqrt(a).m }; }
#endif
#if defined(__AVX__)
	inline f32_x8  F32x8InvSqrt(f32_x8 a)                  { return { __builtin_ia32_rsqrtps256(a.m) }; }
#else
	inline f32_x8  F32x8InvSqrt(f32_x8 a)                  { return { 1.0f / F32x8Sqrt(a).m }; }
#endif

	//~ Rounding

#if (defined(__x86_64__) || defined(__i386__)) && !defined(__SSE4_1__)
	// No rounding instruction before SSE4.1, and the compiler would call the C runtime per lane, which may lack
	// roundeven (MSVC's does). Adding and subtracting 2^23 (2^52 for f64) rounds the fraction away, ties to even;
	// magnitudes from there up are already whole. Floor and ceiling step by one where rounding landed on the wrong
	// side, then restore the input's sign, which neither can change.
	template<typename U, typename V, typename S, typename F>
	inline V SimdRoundEven(V a, S signBit, F magic)
	{
		V magnitude = (V)((U)a & ~signBit);
		V rounded = (magnitude + magic) - magic;
		U whole = (U)(magnitude >= magic);
		return (V)((((U)rounded & ~whole) | ((U)magnitude & whole)) | ((U)a & signBit));
	}

	template<typename U, typename V, typename S, typename F>
	inline V SimdFloor(V a, S signBit, F magic)
	{
		V r = SimdRoundEven<U>(a, signBit, magic);
		r = r - (V)((U)(r > a) & (U)(V{} + (F)1));
		return (V)((U)r | ((U)a & signBit));
	}

	template<typename U, typename V, typename S, typename F>
	inline V SimdCeil(V a, S signBit, F magic)
	{
		V r = SimdRoundEven<U>(a, signBit, magic);
		r = r + (V)((U)(r < a) & (U)(V{} + (F)1));
		return (V)((U)r | ((U)a & signBit));
	}

	inline f32_x4  F32x4Ceil(f32_x4 a)                     { return { SimdCeil<SimdU32x4>(a.m, 0x80000000u, 8388608.0f) }; }
	inline f32_x8  F32x8Ceil(f32_x8 a)                     { return { SimdCeil<SimdU32x8>(a.m, 0x80000000u, 8388608.0f) }; }
	inline f64_x2  F64x2Ceil(f64_x2 a)                     { return { SimdCeil<SimdU64x2>(a.m, 0x8000000000000000ull, 4503599627370496.0) }; }
	inline f64_x4  F64x4Ceil(f64_x4 a)                     { return { SimdCeil<SimdU64x4>(a.m, 0x8000000000000000ull, 4503599627370496.0) }; }

	inline f32_x4  F32x4Floor(f32_x4 a)                    { return { SimdFloor<SimdU32x4>(a.m, 0x80000000u, 8388608.0f) }; }
	inline f32_x8  F32x8Floor(f32_x8 a)                    { return { SimdFloor<SimdU32x8>(a.m, 0x80000000u, 8388608.0f) }; }
	inline f64_x2  F64x2Floor(f64_x2 a)                    { return { SimdFloor<SimdU64x2>(a.m, 0x8000000000000000ull, 4503599627370496.0) }; }
	inline f64_x4  F64x4Floor(f64_x4 a)                    { return { SimdFloor<SimdU64x4>(a.m, 0x8000000000000000ull, 4503599627370496.0) }; }

	inline f32_x4  F32x4Round(f32_x4 a)                    { return { SimdRoundEven<SimdU32x4>(a.m, 0x80000000u, 8388608.0f) }; }
	inline f32_x8  F32x8Round(f32_x8 a)                    { return { SimdRoundEven<SimdU32x8>(a.m, 0x80000000u, 8388608.0f) }; }
	inline f64_x2  F64x2Round(f64_x2 a)                    { return { SimdRoundEven<SimdU64x2>(a.m, 0x8000000000000000ull, 4503599627370496.0) }; }
	inline f64_x4  F64x4Round(f64_x4 a)                    { return { SimdRoundEven<SimdU64x4>(a.m, 0x8000000000000000ull, 4503599627370496.0) }; }
#elif TOOL_SIMD_HAS_BUILTIN(__builtin_elementwise_ceil) && TOOL_SIMD_HAS_BUILTIN(__builtin_elementwise_floor) && TOOL_SIMD_HAS_BUILTIN(__builtin_elementwise_roundeven)
	inline f32_x4  F32x4Ceil(f32_x4 a)                     { return { __builtin_elementwise_ceil(a.m) }; }
	inline f32_x8  F32x8Ceil(f32_x8 a)                     { return { __builtin_elementwise_ceil(a.m) }; }
	inline f64_x2  F64x2Ceil(f64_x2 a)                     { return { __builtin_elementwise_ceil(a.m) }; }
	inline f64_x4  F64x4Ceil(f64_x4 a)                     { return { __builtin_elementwise_ceil(a.m) }; }

	inline f32_x4  F32x4Floor(f32_x4 a)                    { return { __builtin_elementwise_floor(a.m) }; }
	inline f32_x8  F32x8Floor(f32_x8 a)                    { return { __builtin_elementwise_floor(a.m) }; }
	inline f64_x2  F64x2Floor(f64_x2 a)                    { return { __builtin_elementwise_floor(a.m) }; }
	inline f64_x4  F64x4Floor(f64_x4 a)                    { return { __builtin_elementwise_floor(a.m) }; }

	inline f32_x4  F32x4Round(f32_x4 a)                    { return { __builtin_elementwise_roundeven(a.m) }; }
	inline f32_x8  F32x8Round(f32_x8 a)                    { return { __builtin_elementwise_roundeven(a.m) }; }
	inline f64_x2  F64x2Round(f64_x2 a)                    { return { __builtin_elementwise_roundeven(a.m) }; }
	inline f64_x4  F64x4Round(f64_x4 a)                    { return { __builtin_elementwise_roundeven(a.m) }; }
#else
	inline f32_x4  F32x4Ceil(f32_x4 a)                     TOOL_SIMD_LANES(f32_x4, f, 4, __builtin_ceilf(a.f[i]))
	inline f32_x8  F32x8Ceil(f32_x8 a)                     TOOL_SIMD_LANES(f32_x8, f, 8, __builtin_ceilf(a.f[i]))
	inline f64_x2  F64x2Ceil(f64_x2 a)                     TOOL_SIMD_LANES(f64_x2, f, 2, __builtin_ceil(a.f[i]))
	inline f64_x4  F64x4Ceil(f64_x4 a)                     TOOL_SIMD_LANES(f64_x4, f, 4, __builtin_ceil(a.f[i]))

	inline f32_x4  F32x4Floor(f32_x4 a)                    TOOL_SIMD_LANES(f32_x4, f, 4, __builtin_floorf(a.f[i]))
	inline f32_x8  F32x8Floor(f32_x8 a)                    TOOL_SIMD_LANES(f32_x8, f, 8, __builtin_floorf(a.f[i]))
	inline f64_x2  F64x2Floor(f64_x2 a)                    TOOL_SIMD_LANES(f64_x2, f, 2, __builtin_floor(a.f[i]))
	inline f64_x4  F64x4Floor(f64_x4 a)                    TOOL_SIMD_LANES(f64_x4, f, 4, __builtin_floor(a.f[i]))

	inline f32_x4  F32x4Round(f32_x4 a)                    TOOL_SIMD_LANES(f32_x4, f, 4, __builtin_roundevenf(a.f[i]))
	inline f32_x8  F32x8Round(f32_x8 a)                    TOOL_SIMD_LANES(f32_x8, f, 8, __builtin_roundevenf(a.f[i]))
	inline f64_x2  F64x2Round(f64_x2 a)                    TOOL_SIMD_LANES(f64_x2, f, 2, __builtin_roundeven(a.f[i]))
	inline f64_x4  F64x4Round(f64_x4 a)                    TOOL_SIMD_LANES(f64_x4, f, 4, __builtin_roundeven(a.f[i]))
#endif

	//~ Inverse (Reciprocal)

#if defined(__SSE__)
	inline f32_x4  F32x4Inv(f32_x4 a)                      { return { __builtin_ia32_rcpps(a.m) }; }
#else
	inline f32_x4  F32x4Inv(f32_x4 a)                      { return { 1.0f / a.m }; }
#endif
#if defined(__AVX__)
	inline f32_x8  F32x8Inv(f32_x8 a)                      { return { __builtin_ia32_rcpps256(a.m) }; }
#else
	inline f32_x8  F32x8Inv(f32_x8 a)                      { return { 1.0f / a.m }; }
#endif

	//~ MulAdd (a * b + c), MulSub (a * b - c)

#if (defined(__FMA__) || defined(__aarch64__) || defined(__ARM_FEATURE_FMA)) && TOOL_SIMD_HAS_BUILTIN(__builtin_elementwise_fma)
	inline f32_x4  F32x4MulAdd(f32_x4 a, f32_x4 b, f32_x4 c)     { return { __builtin_elementwise_fma(a.m, b.m, c.m) }; }
	inline f32_x8  F32x8MulAdd(f32_x8 a, f32_x8 b, f32_x8 c)     { return { __builtin_elementwise_fma(a.m, b.m, c.m) }; }
	inline f64_x2  F64x2MulAdd(f64_x2 a, f64_x2 b, f64_x2 c)     { return { __builtin_elementwise_fma(a.m, b.m, c.m) }; }
	inline f64_x4  F64x4MulAdd(f64_x4 a, f64_x4 b, f64_x4 c)     { return { __builtin_elementwise_fma(a.m, b.m, c.m) }; }

	inline f32_x4  F32x4MulSub(f32_x4 a, f32_x4 b, f32_x4 c)     { return { __builtin_elementwise_fma(a.m, b.m, -c.m) }; }
	inline f32_x8  F32x8MulSub(f32_x8 a, f32_x8 b, f32_x8 c)     { return { __builtin_elementwise_fma(a.m, b.m, -c.m) }; }
	inline f64_x2  F64x2MulSub(f64_x2 a, f64_x2 b, f64_x2 c)     { return { __builtin_elementwise_fma(a.m, b.m, -c.m) }; }
	inline f64_x4  F64x4MulSub(f64_x4 a, f64_x4 b, f64_x4 c)     { return { __builtin_elementwise_fma(a.m, b.m, -c.m) }; }
#else
	inline f32_x4  F32x4MulAdd(f32_x4 a, f32_x4 b, f32_x4 c)     { return { a.m * b.m + c.m }; }
	inline f32_x8  F32x8MulAdd(f32_x8 a, f32_x8 b, f32_x8 c)     { return { a.m * b.m + c.m }; }
	inline f64_x2  F64x2MulAdd(f64_x2 a, f64_x2 b, f64_x2 c)     { return { a.m * b.m + c.m }; }
	inline f64_x4  F64x4MulAdd(f64_x4 a, f64_x4 b, f64_x4 c)     { return { a.m * b.m + c.m }; }

	inline f32_x4  F32x4MulSub(f32_x4 a, f32_x4 b, f32_x4 c)     { return { a.m * b.m - c.m }; }
	inline f32_x8  F32x8MulSub(f32_x8 a, f32_x8 b, f32_x8 c)     { return { a.m * b.m - c.m }; }
	inline f64_x2  F64x2MulSub(f64_x2 a, f64_x2 b, f64_x2 c)     { return { a.m * b.m - c.m }; }
	inline f64_x4  F64x4MulSub(f64_x4 a, f64_x4 b, f64_x4 c)     { return { a.m * b.m - c.m }; }
#endif

	//~ AddSub

	inline f32_x4 F32x4AddSub(f32_x4 a, f32_x4 b)
	{
		SimdU32x4 sign = { 0x80000000u, 0, 0x80000000u, 0 };
		return { a.m + (SimdF32x4)((SimdU32x4)b.m ^ sign) };
	}
	inline f32_x8 F32x8AddSub(f32_x8 a, f32_x8 b)
	{
		SimdU32x8 sign = { 0x80000000u, 0, 0x80000000u, 0, 0x80000000u, 0, 0x80000000u, 0 };
		return { a.m + (SimdF32x8)((SimdU32x8)b.m ^ sign) };
	}
	inline f64_x2 F64x2AddSub(f64_x2 a, f64_x2 b)
	{
		SimdU64x2 sign = { 0x8000000000000000ull, 0 };
		return { a.m + (SimdF64x2)((SimdU64x2)b.m ^ sign) };
	}
	inline f64_x4 F64x4AddSub(f64_x4 a, f64_x4 b)
	{
		SimdU64x4 sign = { 0x8000000000000000ull, 0, 0x8000000000000000ull, 0 };
		return { a.m + (SimdF64x4)((SimdU64x4)b.m ^ sign) };
	}

	//~ Comparison

	template<Comparison C>
	inline f32_x4_mask F32x4Compare(f32_x4 a, f32_x4 b)
	{
		SimdI32x4 r;
		if constexpr (C == ComparisonEquals) r = (SimdI32x4)(a.m == b.m);
		else if constexpr (C == ComparisonNotEquals) r = (SimdI32x4)((a.m < b.m) | (a.m > b.m));
		else if constexpr (C == ComparisonGreaterThan) r = (SimdI32x4)(a.m > b.m);
		else if constexpr (C == ComparisonGreaterThanOrEquals) r = (SimdI32x4)(a.m >= b.m);
		else if constexpr (C == ComparisonLessThan) r = (SimdI32x4)(a.m < b.m);
		else r = (SimdI32x4)(a.m <= b.m);
		return { (SimdF32x4)r };
	}

	template<Comparison C>
	inline f32_x8_mask F32x8Compare(f32_x8 a, f32_x8 b)
	{
		SimdI32x8 r;
		if constexpr (C == ComparisonEquals) r = (SimdI32x8)(a.m == b.m);
		else if constexpr (C == ComparisonNotEquals) r = (SimdI32x8)((a.m < b.m) | (a.m > b.m));
		else if constexpr (C == ComparisonGreaterThan) r = (SimdI32x8)(a.m > b.m);
		else if constexpr (C == ComparisonGreaterThanOrEquals) r = (SimdI32x8)(a.m >= b.m);
		else if constexpr (C == ComparisonLessThan) r = (SimdI32x8)(a.m < b.m);
		else r = (SimdI32x8)(a.m <= b.m);
		return { (SimdF32x8)r };
	}

	template<Comparison C>
	inline f64_x2_mask F64x2Compare(f64_x2 a, f64_x2 b)
	{
		SimdI64x2 r;
		if constexpr (C == ComparisonEquals) r = (SimdI64x2)(a.m == b.m);
		else if constexpr (C == ComparisonNotEquals) r = (SimdI64x2)((a.m < b.m) | (a.m > b.m));
		else if constexpr (C == ComparisonGreaterThan) r = (SimdI64x2)(a.m > b.m);
		else if constexpr (C == ComparisonGreaterThanOrEquals) r = (SimdI64x2)(a.m >= b.m);
		else if constexpr (C == ComparisonLessThan) r = (SimdI64x2)(a.m < b.m);
		else r = (SimdI64x2)(a.m <= b.m);
		return { (SimdF64x2)r };
	}

	template<Comparison C>
	inline f64_x4_mask F64x4Compare(f64_x4 a, f64_x4 b)
	{
		SimdI64x4 r;
		if constexpr (C == ComparisonEquals) r = (SimdI64x4)(a.m == b.m);
		else if constexpr (C == ComparisonNotEquals) r = (SimdI64x4)((a.m < b.m) | (a.m > b.m));
		else if constexpr (C == ComparisonGreaterThan) r = (SimdI64x4)(a.m > b.m);
		else if constexpr (C == ComparisonGreaterThanOrEquals) r = (SimdI64x4)(a.m >= b.m);
		else if constexpr (C == ComparisonLessThan) r = (SimdI64x4)(a.m < b.m);
		else r = (SimdI64x4)(a.m <= b.m);
		return { (SimdF64x4)r };
	}

	//~ Blending

	inline f32_x4 F32x4Blend(f32_x4 a, f32_x4 b, f32_x4_mask mask)
	{
		SimdU32x4 m = (SimdU32x4)mask.m;
		return { (SimdF32x4)(((SimdU32x4)a.m & ~m) | ((SimdU32x4)b.m & m)) };
	}
	inline f32_x8 F32x8Blend(f32_x8 a, f32_x8 b, f32_x8_mask mask)
	{
		SimdU32x8 m = (SimdU32x8)mask.m;
		return { (SimdF32x8)(((SimdU32x8)a.m & ~m) | ((SimdU32x8)b.m & m)) };
	}
	inline f64_x2 F64x2Blend(f64_x2 a, f64_x2 b, f64_x2_mask mask)
	{
		SimdU64x2 m = (SimdU64x2)mask.m;
		return { (SimdF64x2)(((SimdU64x2)a.m & ~m) | ((SimdU64x2)b.m & m)) };
	}
	inline f64_x4 F64x4Blend(f64_x4 a, f64_x4 b, f64_x4_mask mask)
	{
		SimdU64x4 m = (SimdU64x4)mask.m;
		return { (SimdF64x4)(((SimdU64x4)a.m & ~m) | ((SimdU64x4)b.m & m)) };
	}

	//~ Bitwise arithmetic

	inline f32_x4 F32x4And(f32_x4 a, f32_x4 b)             { return { (SimdF32x4)((SimdU32x4)a.m & (SimdU32x4)b.m) }; }
	inline f32_x8 F32x8And(f32_x8 a, f32_x8 b)             { return { (SimdF32x8)((SimdU32x8)a.m & (SimdU32x8)b.m) }; }
	inline f64_x2 F64x2And(f64_x2 a, f64_x2 b)             { return { (SimdF64x2)((SimdU64x2)a.m & (SimdU64x2)b.m) }; }
	inline f64_x4 F64x4And(f64_x4 a, f64_x4 b)             { return { (SimdF64x4)((SimdU64x4)a.m & (SimdU64x4)b.m) }; }

	inline f32_x4 F32x4AndNot(f32_x4 a, f32_x4 b)          { return { (SimdF32x4)((SimdU32x4)a.m & ~(SimdU32x4)b.m) }; }
	inline f32_x8 F32x8AndNot(f32_x8 a, f32_x8 b)          { return { (SimdF32x8)((SimdU32x8)a.m & ~(SimdU32x8)b.m) }; }
	inline f64_x2 F64x2AndNot(f64_x2 a, f64_x2 b)          { return { (SimdF64x2)((SimdU64x2)a.m & ~(SimdU64x2)b.m) }; }
	inline f64_x4 F64x4AndNot(f64_x4 a, f64_x4 b)          { return { (SimdF64x4)((SimdU64x4)a.m & ~(SimdU64x4)b.m) }; }

	inline f32_x4 F32x4Or(f32_x4 a, f32_x4 b)              { return { (SimdF32x4)((SimdU32x4)a.m | (SimdU32x4)b.m) }; }
	inline f32_x8 F32x8Or(f32_x8 a, f32_x8 b)              { return { (SimdF32x8)((SimdU32x8)a.m | (SimdU32x8)b.m) }; }
	inline f64_x2 F64x2Or(f64_x2 a, f64_x2 b)              { return { (SimdF64x2)((SimdU64x2)a.m | (SimdU64x2)b.m) }; }
	inline f64_x4 F64x4Or(f64_x4 a, f64_x4 b)              { return { (SimdF64x4)((SimdU64x4)a.m | (SimdU64x4)b.m) }; }

	inline f32_x4 F32x4Xor(f32_x4 a, f32_x4 b)             { return { (SimdF32x4)((SimdU32x4)a.m ^ (SimdU32x4)b.m) }; }
	inline f32_x8 F32x8Xor(f32_x8 a, f32_x8 b)             { return { (SimdF32x8)((SimdU32x8)a.m ^ (SimdU32x8)b.m) }; }
	inline f64_x2 F64x2Xor(f64_x2 a, f64_x2 b)             { return { (SimdF64x2)((SimdU64x2)a.m ^ (SimdU64x2)b.m) }; }
	inline f64_x4 F64x4Xor(f64_x4 a, f64_x4 b)             { return { (SimdF64x4)((SimdU64x4)a.m ^ (SimdU64x4)b.m) }; }



#elif defined(TOOL_SIMD_X86)

	//- MSVC x64 backend

	//~ SSE2 emulation of SSE4.1 rounding
	// Adding and subtracting 2^23 (2^52 for f64) rounds the fraction away, ties to even. Magnitudes from there up
	// are already whole. Floor and ceiling step the rounded value by one where it landed on the wrong side, then
	// restore the input's sign, which neither can change: ceil(-0.7) is -0, not the +0 that -1 + 1 gives.

#if !defined(TOOL_SIMD_AVX)
	inline __m128 SimdX86RoundEven(__m128 a)
	{
		__m128 sign = _mm_set1_ps(-0.0f);
		__m128 magic = _mm_set1_ps(8388608.0f);
		__m128 magnitude = _mm_andnot_ps(sign, a);
		__m128 rounded = _mm_sub_ps(_mm_add_ps(magnitude, magic), magic);
		__m128 whole = _mm_cmpge_ps(magnitude, magic);
		rounded = _mm_or_ps(_mm_and_ps(whole, magnitude), _mm_andnot_ps(whole, rounded));
		return _mm_or_ps(rounded, _mm_and_ps(a, sign));
	}

	inline __m128d SimdX86RoundEven(__m128d a)
	{
		__m128d sign = _mm_set1_pd(-0.0);
		__m128d magic = _mm_set1_pd(4503599627370496.0);
		__m128d magnitude = _mm_andnot_pd(sign, a);
		__m128d rounded = _mm_sub_pd(_mm_add_pd(magnitude, magic), magic);
		__m128d whole = _mm_cmpge_pd(magnitude, magic);
		rounded = _mm_or_pd(_mm_and_pd(whole, magnitude), _mm_andnot_pd(whole, rounded));
		return _mm_or_pd(rounded, _mm_and_pd(a, sign));
	}

	inline __m128 SimdX86Floor(__m128 a)
	{
		__m128 r = SimdX86RoundEven(a);
		r = _mm_sub_ps(r, _mm_and_ps(_mm_cmpgt_ps(r, a), _mm_set1_ps(1.0f)));
		return _mm_or_ps(r, _mm_and_ps(a, _mm_set1_ps(-0.0f)));
	}

	inline __m128d SimdX86Floor(__m128d a)
	{
		__m128d r = SimdX86RoundEven(a);
		r = _mm_sub_pd(r, _mm_and_pd(_mm_cmpgt_pd(r, a), _mm_set1_pd(1.0)));
		return _mm_or_pd(r, _mm_and_pd(a, _mm_set1_pd(-0.0)));
	}

	inline __m128 SimdX86Ceil(__m128 a)
	{
		__m128 r = SimdX86RoundEven(a);
		r = _mm_add_ps(r, _mm_and_ps(_mm_cmplt_ps(r, a), _mm_set1_ps(1.0f)));
		return _mm_or_ps(r, _mm_and_ps(a, _mm_set1_ps(-0.0f)));
	}

	inline __m128d SimdX86Ceil(__m128d a)
	{
		__m128d r = SimdX86RoundEven(a);
		r = _mm_add_pd(r, _mm_and_pd(_mm_cmplt_pd(r, a), _mm_set1_pd(1.0)));
		return _mm_or_pd(r, _mm_and_pd(a, _mm_set1_pd(-0.0)));
	}
#endif

	//~ 64-bit integer multiply from 32-bit halves
	// a * b mod 2^64 = aLow * bLow + ((aLow * bHigh + aHigh * bLow) << 32)

	inline __m128i SimdX86MulI64(__m128i a, __m128i b)
	{
		__m128i cross = _mm_add_epi64(_mm_mul_epu32(a, _mm_srli_epi64(b, 32)), _mm_mul_epu32(_mm_srli_epi64(a, 32), b));
		return _mm_add_epi64(_mm_mul_epu32(a, b), _mm_slli_epi64(cross, 32));
	}

#if defined(TOOL_SIMD_AVX2)
	inline __m256i SimdX86MulI64(__m256i a, __m256i b)
	{
		__m256i cross = _mm256_add_epi64(_mm256_mul_epu32(a, _mm256_srli_epi64(b, 32)), _mm256_mul_epu32(_mm256_srli_epi64(a, 32), b));
		return _mm256_add_epi64(_mm256_mul_epu32(a, b), _mm256_slli_epi64(cross, 32));
	}
#endif

	//~ Comparison predicates (AVX)

#if defined(TOOL_SIMD_AVX)
	constexpr i32 SimdX86Predicate(Comparison comparison)
	{
		switch (comparison)
		{
		case ComparisonEquals: return _CMP_EQ_OQ;
		case ComparisonNotEquals: return _CMP_NEQ_OQ;
		case ComparisonGreaterThan: return _CMP_GT_OS;
		case ComparisonGreaterThanOrEquals: return _CMP_GE_OS;
		case ComparisonLessThan: return _CMP_LT_OS;
		default: return _CMP_LE_OS;
		}
	}
#endif

	//~ Set by value (all same)

	inline f32_x4 F32x4Set(f32 all)                        { return { _mm_set1_ps(all) }; }
	inline f64_x2 F64x2Set(f64 all)                        { return { _mm_set1_pd(all) }; }
	inline i8_x16 I8x16Set(i8 all)                         { return { _mm_set1_epi8(all) }; }
	inline i16_x8 I16x8Set(i16 all)                        { return { _mm_set1_epi16(all) }; }
	inline i32_x4 I32x4Set(i32 all)                        { return { _mm_set1_epi32(all) }; }
	inline i64_x2 I64x2Set(i64 all)                        { return { _mm_set1_epi64x(all) }; }

	//~ Set by value (individual)

	inline f32_x4 F32x4Set(f32 f0, f32 f1, f32 f2, f32 f3) { return { _mm_setr_ps(f0, f1, f2, f3) }; }
	inline f64_x2 F64x2Set(f64 f0, f64 f1)                 { return { _mm_setr_pd(f0, f1) }; }
	inline i64_x2 I64x2Set(i64 i0, i64 i1)                 { return { _mm_set_epi64x(i1, i0) }; }
	inline i32_x4 I32x4Set(i32 i0, i32 i1, i32 i2, i32 i3) { return { _mm_setr_epi32(i0, i1, i2, i3) }; }
	inline i16_x8 I16x8Set(i16 i0, i16 i1, i16 i2, i16 i3,
	                       i16 i4, i16 i5, i16 i6, i16 i7) { return { _mm_setr_epi16(i0, i1, i2, i3, i4, i5, i6, i7) }; }
	inline i8_x16 I8x16Set(i8 i0, i8 i1, i8 i2, i8 i3,
	                       i8 i4, i8 i5, i8 i6, i8 i7,
	                       i8 i8_, i8 i9, i8 i10, i8 i11,
	                       i8 i12, i8 i13, i8 i14, i8 i15)
	{
		return { _mm_setr_epi8(i0, i1, i2, i3, i4, i5, i6, i7, i8_, i9, i10, i11, i12, i13, i14, i15) };
	}

	//~ Load

	inline f32_x4 F32x4LoadOne(const f32* value)           { return { _mm_load1_ps(value) }; }
	inline f64_x2 F64x2LoadOne(const f64* value)           { return { _mm_load1_pd(value) }; }

	inline f32_x4 F32x4Load(const f32* values)             { return { _mm_loadu_ps(values) }; }
	inline f64_x2 F64x2Load(const f64* values)             { return { _mm_loadu_pd(values) }; }
	inline i8_x16 I8x16Load(const i8* values)              { return { _mm_loadu_si128((const __m128i*)values) }; }
	inline i16_x8 I16x8Load(const i16* values)             { return { _mm_loadu_si128((const __m128i*)values) }; }
	inline i32_x4 I32x4Load(const i32* values)             { return { _mm_loadu_si128((const __m128i*)values) }; }
	inline i64_x2 I64x2Load(const i64* values)             { return { _mm_loadu_si128((const __m128i*)values) }; }

	//~ Store

	inline void F32x4Store(f32_x4 src, f32* dst)           { _mm_storeu_ps(dst, src.m); }
	inline void F64x2Store(f64_x2 src, f64* dst)           { _mm_storeu_pd(dst, src.m); }
	inline void I8x16Store(i8_x16 src, i8* dst)            { _mm_storeu_si128((__m128i*)dst, src.m); }
	inline void I16x8Store(i16_x8 src, i16* dst)           { _mm_storeu_si128((__m128i*)dst, src.m); }
	inline void I32x4Store(i32_x4 src, i32* dst)           { _mm_storeu_si128((__m128i*)dst, src.m); }
	inline void I64x2Store(i64_x2 src, i64* dst)           { _mm_storeu_si128((__m128i*)dst, src.m); }

	//~ Arithmetic

	inline f32_x4  F32x4Add(f32_x4 a, f32_x4 b)            { return { _mm_add_ps(a.m, b.m) }; }
	inline f64_x2  F64x2Add(f64_x2 a, f64_x2 b)            { return { _mm_add_pd(a.m, b.m) }; }
	inline i8_x16  I8x16Add(i8_x16 a, i8_x16 b)            { return { _mm_add_epi8(a.m, b.m) }; }
	inline i16_x8  I16x8Add(i16_x8 a, i16_x8 b)            { return { _mm_add_epi16(a.m, b.m) }; }
	inline i32_x4  I32x4Add(i32_x4 a, i32_x4 b)            { return { _mm_add_epi32(a.m, b.m) }; }
	inline i64_x2  I64x2Add(i64_x2 a, i64_x2 b)            { return { _mm_add_epi64(a.m, b.m) }; }

	inline f32_x4  F32x4Sub(f32_x4 a, f32_x4 b)            { return { _mm_sub_ps(a.m, b.m) }; }
	inline f64_x2  F64x2Sub(f64_x2 a, f64_x2 b)            { return { _mm_sub_pd(a.m, b.m) }; }
	inline i8_x16  I8x16Sub(i8_x16 a, i8_x16 b)            { return { _mm_sub_epi8(a.m, b.m) }; }
	inline i16_x8  I16x8Sub(i16_x8 a, i16_x8 b)            { return { _mm_sub_epi16(a.m, b.m) }; }
	inline i32_x4  I32x4Sub(i32_x4 a, i32_x4 b)            { return { _mm_sub_epi32(a.m, b.m) }; }
	inline i64_x2  I64x2Sub(i64_x2 a, i64_x2 b)            { return { _mm_sub_epi64(a.m, b.m) }; }

	inline f32_x4  F32x4Mul(f32_x4 a, f32_x4 b)            { return { _mm_mul_ps(a.m, b.m) }; }
	inline f64_x2  F64x2Mul(f64_x2 a, f64_x2 b)            { return { _mm_mul_pd(a.m, b.m) }; }
	inline i16_x8  I16x8Mul(i16_x8 a, i16_x8 b)            { return { _mm_mullo_epi16(a.m, b.m) }; }
#if defined(TOOL_SIMD_AVX)
	inline i32_x4  I32x4Mul(i32_x4 a, i32_x4 b)            { return { _mm_mullo_epi32(a.m, b.m) }; }
#else
	inline i32_x4 I32x4Mul(i32_x4 a, i32_x4 b)
	{
		__m128i even = _mm_mul_epu32(a.m, b.m);
		__m128i odd = _mm_mul_epu32(_mm_srli_epi64(a.m, 32), _mm_srli_epi64(b.m, 32));
		return { _mm_unpacklo_epi32(_mm_shuffle_epi32(even, _MM_SHUFFLE(0, 0, 2, 0)), _mm_shuffle_epi32(odd, _MM_SHUFFLE(0, 0, 2, 0))) };
	}
#endif
#if defined(TOOL_SIMD_AVX512)
	inline i64_x2  I64x2Mul(i64_x2 a, i64_x2 b)            { return { _mm_mullo_epi64(a.m, b.m) }; }
#else
	inline i64_x2  I64x2Mul(i64_x2 a, i64_x2 b)            { return { SimdX86MulI64(a.m, b.m) }; }
#endif

	inline f32_x4  F32x4Div(f32_x4 a, f32_x4 b)            { return { _mm_div_ps(a.m, b.m) }; }
	inline f64_x2  F64x2Div(f64_x2 a, f64_x2 b)            { return { _mm_div_pd(a.m, b.m) }; }

	//~ Absolute value

#if defined(TOOL_SIMD_AVX)
	inline i8_x16  I8x16Abs(i8_x16 a)                      { return { _mm_abs_epi8(a.m) }; }
	inline i16_x8  I16x8Abs(i16_x8 a)                      { return { _mm_abs_epi16(a.m) }; }
	inline i32_x4  I32x4Abs(i32_x4 a)                      { return { _mm_abs_epi32(a.m) }; }
#else
	inline i8_x16 I8x16Abs(i8_x16 a)
	{
		__m128i s = _mm_cmpgt_epi8(_mm_setzero_si128(), a.m);
		return { _mm_sub_epi8(_mm_xor_si128(a.m, s), s) };
	}
	inline i16_x8 I16x8Abs(i16_x8 a)
	{
		__m128i s = _mm_srai_epi16(a.m, 15);
		return { _mm_sub_epi16(_mm_xor_si128(a.m, s), s) };
	}
	inline i32_x4 I32x4Abs(i32_x4 a)
	{
		__m128i s = _mm_srai_epi32(a.m, 31);
		return { _mm_sub_epi32(_mm_xor_si128(a.m, s), s) };
	}
#endif
#if defined(TOOL_SIMD_AVX512)
	inline i64_x2  I64x2Abs(i64_x2 a)                      { return { _mm_abs_epi64(a.m) }; }
#else
	inline i64_x2 I64x2Abs(i64_x2 a)
	{
		__m128i s = _mm_srai_epi32(_mm_shuffle_epi32(a.m, _MM_SHUFFLE(3, 3, 1, 1)), 31);
		return { _mm_sub_epi64(_mm_xor_si128(a.m, s), s) };
	}
#endif

	//~ Square root, inverse

	inline f32_x4  F32x4Sqrt(f32_x4 a)                     { return { _mm_sqrt_ps(a.m) }; }
	inline f64_x2  F64x2Sqrt(f64_x2 a)                     { return { _mm_sqrt_pd(a.m) }; }
	inline f32_x4  F32x4InvSqrt(f32_x4 a)                  { return { _mm_rsqrt_ps(a.m) }; }
	inline f32_x4  F32x4Inv(f32_x4 a)                      { return { _mm_rcp_ps(a.m) }; }

	//~ Rounding

#if defined(TOOL_SIMD_AVX)
	inline f32_x4  F32x4Ceil(f32_x4 a)                     { return { _mm_ceil_ps(a.m) }; }
	inline f64_x2  F64x2Ceil(f64_x2 a)                     { return { _mm_ceil_pd(a.m) }; }
	inline f32_x4  F32x4Floor(f32_x4 a)                    { return { _mm_floor_ps(a.m) }; }
	inline f64_x2  F64x2Floor(f64_x2 a)                    { return { _mm_floor_pd(a.m) }; }
	inline f32_x4  F32x4Round(f32_x4 a)                    { return { _mm_round_ps(a.m, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC) }; }
	inline f64_x2  F64x2Round(f64_x2 a)                    { return { _mm_round_pd(a.m, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC) }; }
#else
	inline f32_x4  F32x4Ceil(f32_x4 a)                     { return { SimdX86Ceil(a.m) }; }
	inline f64_x2  F64x2Ceil(f64_x2 a)                     { return { SimdX86Ceil(a.m) }; }
	inline f32_x4  F32x4Floor(f32_x4 a)                    { return { SimdX86Floor(a.m) }; }
	inline f64_x2  F64x2Floor(f64_x2 a)                    { return { SimdX86Floor(a.m) }; }
	inline f32_x4  F32x4Round(f32_x4 a)                    { return { SimdX86RoundEven(a.m) }; }
	inline f64_x2  F64x2Round(f64_x2 a)                    { return { SimdX86RoundEven(a.m) }; }
#endif

	//~ MulAdd (a * b + c), MulSub (a * b - c)
	// Fused (one rounding) with AVX2, a multiply then an add otherwise.

#if defined(TOOL_SIMD_AVX2)
	inline f32_x4  F32x4MulAdd(f32_x4 a, f32_x4 b, f32_x4 c)     { return { _mm_fmadd_ps(a.m, b.m, c.m) }; }
	inline f64_x2  F64x2MulAdd(f64_x2 a, f64_x2 b, f64_x2 c)     { return { _mm_fmadd_pd(a.m, b.m, c.m) }; }
	inline f32_x4  F32x4MulSub(f32_x4 a, f32_x4 b, f32_x4 c)     { return { _mm_fmsub_ps(a.m, b.m, c.m) }; }
	inline f64_x2  F64x2MulSub(f64_x2 a, f64_x2 b, f64_x2 c)     { return { _mm_fmsub_pd(a.m, b.m, c.m) }; }
#else
	inline f32_x4  F32x4MulAdd(f32_x4 a, f32_x4 b, f32_x4 c)     { return { _mm_add_ps(_mm_mul_ps(a.m, b.m), c.m) }; }
	inline f64_x2  F64x2MulAdd(f64_x2 a, f64_x2 b, f64_x2 c)     { return { _mm_add_pd(_mm_mul_pd(a.m, b.m), c.m) }; }
	inline f32_x4  F32x4MulSub(f32_x4 a, f32_x4 b, f32_x4 c)     { return { _mm_sub_ps(_mm_mul_ps(a.m, b.m), c.m) }; }
	inline f64_x2  F64x2MulSub(f64_x2 a, f64_x2 b, f64_x2 c)     { return { _mm_sub_pd(_mm_mul_pd(a.m, b.m), c.m) }; }
#endif

	//~ AddSub

#if defined(TOOL_SIMD_AVX)
	inline f32_x4 F32x4AddSub(f32_x4 a, f32_x4 b)          { return { _mm_addsub_ps(a.m, b.m) }; }
	inline f64_x2 F64x2AddSub(f64_x2 a, f64_x2 b)          { return { _mm_addsub_pd(a.m, b.m) }; }
#else
	inline f32_x4 F32x4AddSub(f32_x4 a, f32_x4 b)
	{
		__m128 sign = _mm_castsi128_ps(_mm_setr_epi32((i32)0x80000000, 0, (i32)0x80000000, 0));
		return { _mm_add_ps(a.m, _mm_xor_ps(b.m, sign)) };
	}
	inline f64_x2 F64x2AddSub(f64_x2 a, f64_x2 b)
	{
		__m128d sign = _mm_castsi128_pd(_mm_set_epi64x(0, (i64)0x8000000000000000ull));
		return { _mm_add_pd(a.m, _mm_xor_pd(b.m, sign)) };
	}
#endif

	//~ Comparison

#if defined(TOOL_SIMD_AVX)
	template<Comparison C>
	inline f32_x4_mask F32x4Compare(f32_x4 a, f32_x4 b)    { return { _mm_cmp_ps(a.m, b.m, SimdX86Predicate(C)) }; }

	template<Comparison C>
	inline f64_x2_mask F64x2Compare(f64_x2 a, f64_x2 b)    { return { _mm_cmp_pd(a.m, b.m, SimdX86Predicate(C)) }; }
#else
	template<Comparison C>
	inline f32_x4_mask F32x4Compare(f32_x4 a, f32_x4 b)
	{
		if constexpr (C == ComparisonEquals) return { _mm_cmpeq_ps(a.m, b.m) };
		else if constexpr (C == ComparisonNotEquals) return { _mm_and_ps(_mm_cmpneq_ps(a.m, b.m), _mm_cmpord_ps(a.m, b.m)) };
		else if constexpr (C == ComparisonGreaterThan) return { _mm_cmpgt_ps(a.m, b.m) };
		else if constexpr (C == ComparisonGreaterThanOrEquals) return { _mm_cmpge_ps(a.m, b.m) };
		else if constexpr (C == ComparisonLessThan) return { _mm_cmplt_ps(a.m, b.m) };
		else return { _mm_cmple_ps(a.m, b.m) };
	}

	template<Comparison C>
	inline f64_x2_mask F64x2Compare(f64_x2 a, f64_x2 b)
	{
		if constexpr (C == ComparisonEquals) return { _mm_cmpeq_pd(a.m, b.m) };
		else if constexpr (C == ComparisonNotEquals) return { _mm_and_pd(_mm_cmpneq_pd(a.m, b.m), _mm_cmpord_pd(a.m, b.m)) };
		else if constexpr (C == ComparisonGreaterThan) return { _mm_cmpgt_pd(a.m, b.m) };
		else if constexpr (C == ComparisonGreaterThanOrEquals) return { _mm_cmpge_pd(a.m, b.m) };
		else if constexpr (C == ComparisonLessThan) return { _mm_cmplt_pd(a.m, b.m) };
		else return { _mm_cmple_pd(a.m, b.m) };
	}
#endif

	//~ Blending

#if defined(TOOL_SIMD_AVX)
	inline f32_x4 F32x4Blend(f32_x4 a, f32_x4 b, f32_x4_mask mask) { return { _mm_blendv_ps(a.m, b.m, mask.m) }; }
	inline f64_x2 F64x2Blend(f64_x2 a, f64_x2 b, f64_x2_mask mask) { return { _mm_blendv_pd(a.m, b.m, mask.m) }; }
#else
	inline f32_x4 F32x4Blend(f32_x4 a, f32_x4 b, f32_x4_mask mask) { return { _mm_or_ps(_mm_andnot_ps(mask.m, a.m), _mm_and_ps(mask.m, b.m)) }; }
	inline f64_x2 F64x2Blend(f64_x2 a, f64_x2 b, f64_x2_mask mask) { return { _mm_or_pd(_mm_andnot_pd(mask.m, a.m), _mm_and_pd(mask.m, b.m)) }; }
#endif

	//~ Bitwise arithmetic

	inline f32_x4 F32x4And(f32_x4 a, f32_x4 b)             { return { _mm_and_ps(a.m, b.m) }; }
	inline f64_x2 F64x2And(f64_x2 a, f64_x2 b)             { return { _mm_and_pd(a.m, b.m) }; }
	inline f32_x4 F32x4AndNot(f32_x4 a, f32_x4 b)          { return { _mm_andnot_ps(b.m, a.m) }; }
	inline f64_x2 F64x2AndNot(f64_x2 a, f64_x2 b)          { return { _mm_andnot_pd(b.m, a.m) }; }
	inline f32_x4 F32x4Or(f32_x4 a, f32_x4 b)              { return { _mm_or_ps(a.m, b.m) }; }
	inline f64_x2 F64x2Or(f64_x2 a, f64_x2 b)              { return { _mm_or_pd(a.m, b.m) }; }
	inline f32_x4 F32x4Xor(f32_x4 a, f32_x4 b)             { return { _mm_xor_ps(a.m, b.m) }; }
	inline f64_x2 F64x2Xor(f64_x2 a, f64_x2 b)             { return { _mm_xor_pd(a.m, b.m) }; }

	//~ 256-bit floats (AVX)

#if defined(TOOL_SIMD_AVX)
	inline f32_x8 F32x8Set(f32 all)                        { return { _mm256_set1_ps(all) }; }
	inline f64_x4 F64x4Set(f64 all)                        { return { _mm256_set1_pd(all) }; }
	inline f32_x8 F32x8Set(f32 f0, f32 f1, f32 f2, f32 f3,
	                       f32 f4, f32 f5, f32 f6, f32 f7) { return { _mm256_setr_ps(f0, f1, f2, f3, f4, f5, f6, f7) }; }
	inline f64_x4 F64x4Set(f64 f0, f64 f1, f64 f2, f64 f3) { return { _mm256_setr_pd(f0, f1, f2, f3) }; }

	inline f32_x8 F32x8Load(const f32* values)             { return { _mm256_loadu_ps(values) }; }
	inline f64_x4 F64x4Load(const f64* values)             { return { _mm256_loadu_pd(values) }; }
	inline void F32x8Store(f32_x8 src, f32* dst)           { _mm256_storeu_ps(dst, src.m); }
	inline void F64x4Store(f64_x4 src, f64* dst)           { _mm256_storeu_pd(dst, src.m); }

	inline f32_x8  F32x8Add(f32_x8 a, f32_x8 b)            { return { _mm256_add_ps(a.m, b.m) }; }
	inline f64_x4  F64x4Add(f64_x4 a, f64_x4 b)            { return { _mm256_add_pd(a.m, b.m) }; }
	inline f32_x8  F32x8Sub(f32_x8 a, f32_x8 b)            { return { _mm256_sub_ps(a.m, b.m) }; }
	inline f64_x4  F64x4Sub(f64_x4 a, f64_x4 b)            { return { _mm256_sub_pd(a.m, b.m) }; }
	inline f32_x8  F32x8Mul(f32_x8 a, f32_x8 b)            { return { _mm256_mul_ps(a.m, b.m) }; }
	inline f64_x4  F64x4Mul(f64_x4 a, f64_x4 b)            { return { _mm256_mul_pd(a.m, b.m) }; }
	inline f32_x8  F32x8Div(f32_x8 a, f32_x8 b)            { return { _mm256_div_ps(a.m, b.m) }; }
	inline f64_x4  F64x4Div(f64_x4 a, f64_x4 b)            { return { _mm256_div_pd(a.m, b.m) }; }

	inline f32_x8  F32x8Sqrt(f32_x8 a)                     { return { _mm256_sqrt_ps(a.m) }; }
	inline f64_x4  F64x4Sqrt(f64_x4 a)                     { return { _mm256_sqrt_pd(a.m) }; }
	inline f32_x8  F32x8InvSqrt(f32_x8 a)                  { return { _mm256_rsqrt_ps(a.m) }; }
	inline f32_x8  F32x8Inv(f32_x8 a)                      { return { _mm256_rcp_ps(a.m) }; }

	inline f32_x8  F32x8Ceil(f32_x8 a)                     { return { _mm256_ceil_ps(a.m) }; }
	inline f64_x4  F64x4Ceil(f64_x4 a)                     { return { _mm256_ceil_pd(a.m) }; }
	inline f32_x8  F32x8Floor(f32_x8 a)                    { return { _mm256_floor_ps(a.m) }; }
	inline f64_x4  F64x4Floor(f64_x4 a)                    { return { _mm256_floor_pd(a.m) }; }
	inline f32_x8  F32x8Round(f32_x8 a)                    { return { _mm256_round_ps(a.m, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC) }; }
	inline f64_x4  F64x4Round(f64_x4 a)                    { return { _mm256_round_pd(a.m, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC) }; }

#if defined(TOOL_SIMD_AVX2)
	inline f32_x8  F32x8MulAdd(f32_x8 a, f32_x8 b, f32_x8 c)     { return { _mm256_fmadd_ps(a.m, b.m, c.m) }; }
	inline f64_x4  F64x4MulAdd(f64_x4 a, f64_x4 b, f64_x4 c)     { return { _mm256_fmadd_pd(a.m, b.m, c.m) }; }
	inline f32_x8  F32x8MulSub(f32_x8 a, f32_x8 b, f32_x8 c)     { return { _mm256_fmsub_ps(a.m, b.m, c.m) }; }
	inline f64_x4  F64x4MulSub(f64_x4 a, f64_x4 b, f64_x4 c)     { return { _mm256_fmsub_pd(a.m, b.m, c.m) }; }
#else
	inline f32_x8  F32x8MulAdd(f32_x8 a, f32_x8 b, f32_x8 c)     { return { _mm256_add_ps(_mm256_mul_ps(a.m, b.m), c.m) }; }
	inline f64_x4  F64x4MulAdd(f64_x4 a, f64_x4 b, f64_x4 c)     { return { _mm256_add_pd(_mm256_mul_pd(a.m, b.m), c.m) }; }
	inline f32_x8  F32x8MulSub(f32_x8 a, f32_x8 b, f32_x8 c)     { return { _mm256_sub_ps(_mm256_mul_ps(a.m, b.m), c.m) }; }
	inline f64_x4  F64x4MulSub(f64_x4 a, f64_x4 b, f64_x4 c)     { return { _mm256_sub_pd(_mm256_mul_pd(a.m, b.m), c.m) }; }
#endif

	inline f32_x8 F32x8AddSub(f32_x8 a, f32_x8 b)          { return { _mm256_addsub_ps(a.m, b.m) }; }
	inline f64_x4 F64x4AddSub(f64_x4 a, f64_x4 b)          { return { _mm256_addsub_pd(a.m, b.m) }; }

	template<Comparison C>
	inline f32_x8_mask F32x8Compare(f32_x8 a, f32_x8 b)    { return { _mm256_cmp_ps(a.m, b.m, SimdX86Predicate(C)) }; }

	template<Comparison C>
	inline f64_x4_mask F64x4Compare(f64_x4 a, f64_x4 b)    { return { _mm256_cmp_pd(a.m, b.m, SimdX86Predicate(C)) }; }

	inline f32_x8 F32x8Blend(f32_x8 a, f32_x8 b, f32_x8_mask mask) { return { _mm256_blendv_ps(a.m, b.m, mask.m) }; }
	inline f64_x4 F64x4Blend(f64_x4 a, f64_x4 b, f64_x4_mask mask) { return { _mm256_blendv_pd(a.m, b.m, mask.m) }; }

	inline f32_x8 F32x8And(f32_x8 a, f32_x8 b)             { return { _mm256_and_ps(a.m, b.m) }; }
	inline f64_x4 F64x4And(f64_x4 a, f64_x4 b)             { return { _mm256_and_pd(a.m, b.m) }; }
	inline f32_x8 F32x8AndNot(f32_x8 a, f32_x8 b)          { return { _mm256_andnot_ps(b.m, a.m) }; }
	inline f64_x4 F64x4AndNot(f64_x4 a, f64_x4 b)          { return { _mm256_andnot_pd(b.m, a.m) }; }
	inline f32_x8 F32x8Or(f32_x8 a, f32_x8 b)              { return { _mm256_or_ps(a.m, b.m) }; }
	inline f64_x4 F64x4Or(f64_x4 a, f64_x4 b)              { return { _mm256_or_pd(a.m, b.m) }; }
	inline f32_x8 F32x8Xor(f32_x8 a, f32_x8 b)             { return { _mm256_xor_ps(a.m, b.m) }; }
	inline f64_x4 F64x4Xor(f64_x4 a, f64_x4 b)             { return { _mm256_xor_pd(a.m, b.m) }; }
#endif

	//~ 256-bit integers (AVX2)

#if defined(TOOL_SIMD_AVX2)
	inline i8_x32 I8x32Set(i8 all)                         { return { _mm256_set1_epi8(all) }; }
	inline i16_x16 I16x16Set(i16 all)                      { return { _mm256_set1_epi16(all) }; }
	inline i32_x8 I32x8Set(i32 all)                        { return { _mm256_set1_epi32(all) }; }
	inline i64_x4 I64x4Set(i64 all)                        { return { _mm256_set1_epi64x(all) }; }

	inline i64_x4 I64x4Set(i64 i0, i64 i1, i64 i2, i64 i3) { return { _mm256_setr_epi64x(i0, i1, i2, i3) }; }
	inline i32_x8 I32x8Set(i32 i0, i32 i1, i32 i2, i32 i3,
	                       i32 i4, i32 i5, i32 i6, i32 i7) { return { _mm256_setr_epi32(i0, i1, i2, i3, i4, i5, i6, i7) }; }
	inline i16_x16 I16x16Set(i16 i0, i16 i1, i16 i2, i16 i3,
	                         i16 i4, i16 i5, i16 i6, i16 i7,
	                         i16 i8_, i16 i9, i16 i10, i16 i11,
	                         i16 i12, i16 i13, i16 i14, i16 i15)
	{
		return { _mm256_setr_epi16(i0, i1, i2, i3, i4, i5, i6, i7, i8_, i9, i10, i11, i12, i13, i14, i15) };
	}
	inline i8_x32 I8x32Set(i8 i0, i8 i1, i8 i2, i8 i3,
	                       i8 i4, i8 i5, i8 i6, i8 i7,
	                       i8 i8_, i8 i9, i8 i10, i8 i11,
	                       i8 i12, i8 i13, i8 i14, i8 i15,
	                       i8 i16_, i8 i17, i8 i18, i8 i19,
	                       i8 i20, i8 i21, i8 i22, i8 i23,
	                       i8 i24, i8 i25, i8 i26, i8 i27,
	                       i8 i28, i8 i29, i8 i30, i8 i31)
	{
		return { _mm256_setr_epi8(i0, i1, i2, i3, i4, i5, i6, i7, i8_, i9, i10, i11, i12, i13, i14, i15,
		                          i16_, i17, i18, i19, i20, i21, i22, i23, i24, i25, i26, i27, i28, i29, i30, i31) };
	}

	inline i8_x32 I8x32Load(const i8* values)              { return { _mm256_loadu_si256((const __m256i*)values) }; }
	inline i16_x16 I16x16Load(const i16* values)           { return { _mm256_loadu_si256((const __m256i*)values) }; }
	inline i32_x8 I32x8Load(const i32* values)             { return { _mm256_loadu_si256((const __m256i*)values) }; }
	inline i64_x4 I64x4Load(const i64* values)             { return { _mm256_loadu_si256((const __m256i*)values) }; }

	inline void I8x32Store(i8_x32 src, i8* dst)            { _mm256_storeu_si256((__m256i*)dst, src.m); }
	inline void I16x16Store(i16_x16 src, i16* dst)         { _mm256_storeu_si256((__m256i*)dst, src.m); }
	inline void I32x8Store(i32_x8 src, i32* dst)           { _mm256_storeu_si256((__m256i*)dst, src.m); }
	inline void I64x4Store(i64_x4 src, i64* dst)           { _mm256_storeu_si256((__m256i*)dst, src.m); }

	inline i8_x32  I8x32Add(i8_x32 a, i8_x32 b)            { return { _mm256_add_epi8(a.m, b.m) }; }
	inline i16_x16 I16x16Add(i16_x16 a, i16_x16 b)         { return { _mm256_add_epi16(a.m, b.m) }; }
	inline i32_x8  I32x8Add(i32_x8 a, i32_x8 b)            { return { _mm256_add_epi32(a.m, b.m) }; }
	inline i64_x4  I64x4Add(i64_x4 a, i64_x4 b)            { return { _mm256_add_epi64(a.m, b.m) }; }

	inline i8_x32  I8x32Sub(i8_x32 a, i8_x32 b)            { return { _mm256_sub_epi8(a.m, b.m) }; }
	inline i16_x16 I16x16Sub(i16_x16 a, i16_x16 b)         { return { _mm256_sub_epi16(a.m, b.m) }; }
	inline i32_x8  I32x8Sub(i32_x8 a, i32_x8 b)            { return { _mm256_sub_epi32(a.m, b.m) }; }
	inline i64_x4  I64x4Sub(i64_x4 a, i64_x4 b)            { return { _mm256_sub_epi64(a.m, b.m) }; }

	inline i16_x16 I16x16Mul(i16_x16 a, i16_x16 b)         { return { _mm256_mullo_epi16(a.m, b.m) }; }
	inline i32_x8  I32x8Mul(i32_x8 a, i32_x8 b)            { return { _mm256_mullo_epi32(a.m, b.m) }; }
#if defined(TOOL_SIMD_AVX512)
	inline i64_x4  I64x4Mul(i64_x4 a, i64_x4 b)            { return { _mm256_mullo_epi64(a.m, b.m) }; }
#else
	inline i64_x4  I64x4Mul(i64_x4 a, i64_x4 b)            { return { SimdX86MulI64(a.m, b.m) }; }
#endif

	inline i8_x32  I8x32Abs(i8_x32 a)                      { return { _mm256_abs_epi8(a.m) }; }
	inline i16_x16 I16x16Abs(i16_x16 a)                    { return { _mm256_abs_epi16(a.m) }; }
	inline i32_x8  I32x8Abs(i32_x8 a)                      { return { _mm256_abs_epi32(a.m) }; }
#if defined(TOOL_SIMD_AVX512)
	inline i64_x4  I64x4Abs(i64_x4 a)                      { return { _mm256_abs_epi64(a.m) }; }
#else
	inline i64_x4 I64x4Abs(i64_x4 a)
	{
		__m256i s = _mm256_srai_epi32(_mm256_shuffle_epi32(a.m, _MM_SHUFFLE(3, 3, 1, 1)), 31);
		return { _mm256_sub_epi64(_mm256_xor_si256(a.m, s), s) };
	}
#endif
#endif



#else

	//- Scalar backend
	// Per-lane loops. Rounding and square roots call out of line into the C runtime.

	f32 SimdScalarSqrt(f32 x);
	f64 SimdScalarSqrt(f64 x);
	f32 SimdScalarFloor(f32 x);
	f64 SimdScalarFloor(f64 x);
	f32 SimdScalarCeil(f32 x);
	f64 SimdScalarCeil(f64 x);
	f32 SimdScalarRoundEven(f32 x);
	f64 SimdScalarRoundEven(f64 x);

	// An all-ones or all-zeros lane, for masks
	inline f32 SimdScalarMask32(b8 set)
	{
		union { u32 u; f32 f; } bits = { set ? 0xffffffffu : 0u };
		return bits.f;
	}

	inline f64 SimdScalarMask64(b8 set)
	{
		union { u64 u; f64 f; } bits = { set ? 0xffffffffffffffffull : 0ull };
		return bits.f;
	}

	inline u32 SimdScalarBits(f32 x)
	{
		union { f32 f; u32 u; } bits = { x };
		return bits.u;
	}

	inline u64 SimdScalarBits(f64 x)
	{
		union { f64 f; u64 u; } bits = { x };
		return bits.u;
	}

	inline f32 SimdScalarFloat(u32 x)
	{
		union { u32 u; f32 f; } bits = { x };
		return bits.f;
	}

	inline f64 SimdScalarFloat(u64 x)
	{
		union { u64 u; f64 f; } bits = { x };
		return bits.f;
	}

	template<Comparison C, typename T>
	inline b8 SimdScalarCompare(T a, T b)
	{
		if constexpr (C == ComparisonEquals) return a == b;
		else if constexpr (C == ComparisonNotEquals) return a < b || a > b;
		else if constexpr (C == ComparisonGreaterThan) return a > b;
		else if constexpr (C == ComparisonGreaterThanOrEquals) return a >= b;
		else if constexpr (C == ComparisonLessThan) return a < b;
		else return a <= b;
	}

	//~ Set

	inline f32_x4 F32x4Set(f32 all)                        TOOL_SIMD_LANES(f32_x4, f, 4, all)
	inline f64_x2 F64x2Set(f64 all)                        TOOL_SIMD_LANES(f64_x2, f, 2, all)
	inline i8_x16 I8x16Set(i8 all)                         TOOL_SIMD_LANES(i8_x16, int8, 16, all)
	inline i16_x8 I16x8Set(i16 all)                        TOOL_SIMD_LANES(i16_x8, int16, 8, all)
	inline i32_x4 I32x4Set(i32 all)                        TOOL_SIMD_LANES(i32_x4, int32, 4, all)
	inline i64_x2 I64x2Set(i64 all)                        TOOL_SIMD_LANES(i64_x2, int64, 2, all)

	inline f32_x4 F32x4Set(f32 f0, f32 f1, f32 f2, f32 f3) { f32_x4 r; r.f[0] = f0; r.f[1] = f1; r.f[2] = f2; r.f[3] = f3; return r; }
	inline f64_x2 F64x2Set(f64 f0, f64 f1)                 { f64_x2 r; r.f[0] = f0; r.f[1] = f1; return r; }
	inline i64_x2 I64x2Set(i64 i0, i64 i1)                 { i64_x2 r; r.int64[0] = i0; r.int64[1] = i1; return r; }
	inline i32_x4 I32x4Set(i32 i0, i32 i1, i32 i2, i32 i3) { i32_x4 r; r.int32[0] = i0; r.int32[1] = i1; r.int32[2] = i2; r.int32[3] = i3; return r; }
	inline i16_x8 I16x8Set(i16 i0, i16 i1, i16 i2, i16 i3,
	                       i16 i4, i16 i5, i16 i6, i16 i7)
	{
		i16 lanes[8] = { i0, i1, i2, i3, i4, i5, i6, i7 };
		TOOL_SIMD_LANES(i16_x8, int16, 8, lanes[i])
	}
	inline i8_x16 I8x16Set(i8 i0, i8 i1, i8 i2, i8 i3,
	                       i8 i4, i8 i5, i8 i6, i8 i7,
	                       i8 i8_, i8 i9, i8 i10, i8 i11,
	                       i8 i12, i8 i13, i8 i14, i8 i15)
	{
		i8 lanes[16] = { i0, i1, i2, i3, i4, i5, i6, i7, i8_, i9, i10, i11, i12, i13, i14, i15 };
		TOOL_SIMD_LANES(i8_x16, int8, 16, lanes[i])
	}

	//~ Load and store

	inline f32_x4 F32x4LoadOne(const f32* value)           TOOL_SIMD_LANES(f32_x4, f, 4, *value)
	inline f64_x2 F64x2LoadOne(const f64* value)           TOOL_SIMD_LANES(f64_x2, f, 2, *value)

	inline f32_x4 F32x4Load(const f32* values)             TOOL_SIMD_LANES(f32_x4, f, 4, values[i])
	inline f64_x2 F64x2Load(const f64* values)             TOOL_SIMD_LANES(f64_x2, f, 2, values[i])
	inline i8_x16 I8x16Load(const i8* values)              TOOL_SIMD_LANES(i8_x16, int8, 16, values[i])
	inline i16_x8 I16x8Load(const i16* values)             TOOL_SIMD_LANES(i16_x8, int16, 8, values[i])
	inline i32_x4 I32x4Load(const i32* values)             TOOL_SIMD_LANES(i32_x4, int32, 4, values[i])
	inline i64_x2 I64x2Load(const i64* values)             TOOL_SIMD_LANES(i64_x2, int64, 2, values[i])

	inline void F32x4Store(f32_x4 src, f32* dst)           { for (u32 i = 0; i < 4; i++) dst[i] = src.f[i]; }
	inline void F64x2Store(f64_x2 src, f64* dst)           { for (u32 i = 0; i < 2; i++) dst[i] = src.f[i]; }
	inline void I8x16Store(i8_x16 src, i8* dst)            { for (u32 i = 0; i < 16; i++) dst[i] = src.int8[i]; }
	inline void I16x8Store(i16_x8 src, i16* dst)           { for (u32 i = 0; i < 8; i++) dst[i] = src.int16[i]; }
	inline void I32x4Store(i32_x4 src, i32* dst)           { for (u32 i = 0; i < 4; i++) dst[i] = src.int32[i]; }
	inline void I64x2Store(i64_x2 src, i64* dst)           { for (u32 i = 0; i < 2; i++) dst[i] = src.int64[i]; }

	//~ Arithmetic
	// Integer lanes go through unsigned arithmetic, which wraps.

	inline f32_x4  F32x4Add(f32_x4 a, f32_x4 b)            TOOL_SIMD_LANES(f32_x4, f, 4, a.f[i] + b.f[i])
	inline f64_x2  F64x2Add(f64_x2 a, f64_x2 b)            TOOL_SIMD_LANES(f64_x2, f, 2, a.f[i] + b.f[i])
	inline i8_x16  I8x16Add(i8_x16 a, i8_x16 b)            TOOL_SIMD_LANES(i8_x16, int8, 16, (i8)((u8)a.int8[i] + (u8)b.int8[i]))
	inline i16_x8  I16x8Add(i16_x8 a, i16_x8 b)            TOOL_SIMD_LANES(i16_x8, int16, 8, (i16)((u16)a.int16[i] + (u16)b.int16[i]))
	inline i32_x4  I32x4Add(i32_x4 a, i32_x4 b)            TOOL_SIMD_LANES(i32_x4, int32, 4, (i32)((u32)a.int32[i] + (u32)b.int32[i]))
	inline i64_x2  I64x2Add(i64_x2 a, i64_x2 b)            TOOL_SIMD_LANES(i64_x2, int64, 2, (i64)((u64)a.int64[i] + (u64)b.int64[i]))

	inline f32_x4  F32x4Sub(f32_x4 a, f32_x4 b)            TOOL_SIMD_LANES(f32_x4, f, 4, a.f[i] - b.f[i])
	inline f64_x2  F64x2Sub(f64_x2 a, f64_x2 b)            TOOL_SIMD_LANES(f64_x2, f, 2, a.f[i] - b.f[i])
	inline i8_x16  I8x16Sub(i8_x16 a, i8_x16 b)            TOOL_SIMD_LANES(i8_x16, int8, 16, (i8)((u8)a.int8[i] - (u8)b.int8[i]))
	inline i16_x8  I16x8Sub(i16_x8 a, i16_x8 b)            TOOL_SIMD_LANES(i16_x8, int16, 8, (i16)((u16)a.int16[i] - (u16)b.int16[i]))
	inline i32_x4  I32x4Sub(i32_x4 a, i32_x4 b)            TOOL_SIMD_LANES(i32_x4, int32, 4, (i32)((u32)a.int32[i] - (u32)b.int32[i]))
	inline i64_x2  I64x2Sub(i64_x2 a, i64_x2 b)            TOOL_SIMD_LANES(i64_x2, int64, 2, (i64)((u64)a.int64[i] - (u64)b.int64[i]))

	inline f32_x4  F32x4Mul(f32_x4 a, f32_x4 b)            TOOL_SIMD_LANES(f32_x4, f, 4, a.f[i] * b.f[i])
	inline f64_x2  F64x2Mul(f64_x2 a, f64_x2 b)            TOOL_SIMD_LANES(f64_x2, f, 2, a.f[i] * b.f[i])
	inline i16_x8  I16x8Mul(i16_x8 a, i16_x8 b)            TOOL_SIMD_LANES(i16_x8, int16, 8, (i16)((u32)(u16)a.int16[i] * (u16)b.int16[i]))
	inline i32_x4  I32x4Mul(i32_x4 a, i32_x4 b)            TOOL_SIMD_LANES(i32_x4, int32, 4, (i32)((u32)a.int32[i] * (u32)b.int32[i]))
	inline i64_x2  I64x2Mul(i64_x2 a, i64_x2 b)            TOOL_SIMD_LANES(i64_x2, int64, 2, (i64)((u64)a.int64[i] * (u64)b.int64[i]))

	inline f32_x4  F32x4Div(f32_x4 a, f32_x4 b)            TOOL_SIMD_LANES(f32_x4, f, 4, a.f[i] / b.f[i])
	inline f64_x2  F64x2Div(f64_x2 a, f64_x2 b)            TOOL_SIMD_LANES(f64_x2, f, 2, a.f[i] / b.f[i])

	inline i8_x16  I8x16Abs(i8_x16 a)                      TOOL_SIMD_LANES(i8_x16, int8, 16, (i8)(a.int8[i] < 0 ? (u8)0 - (u8)a.int8[i] : (u8)a.int8[i]))
	inline i16_x8  I16x8Abs(i16_x8 a)                      TOOL_SIMD_LANES(i16_x8, int16, 8, (i16)(a.int16[i] < 0 ? (u16)0 - (u16)a.int16[i] : (u16)a.int16[i]))
	inline i32_x4  I32x4Abs(i32_x4 a)                      TOOL_SIMD_LANES(i32_x4, int32, 4, (i32)(a.int32[i] < 0 ? 0u - (u32)a.int32[i] : (u32)a.int32[i]))
	inline i64_x2  I64x2Abs(i64_x2 a)                      TOOL_SIMD_LANES(i64_x2, int64, 2, (i64)(a.int64[i] < 0 ? 0ull - (u64)a.int64[i] : (u64)a.int64[i]))

	//~ Square root, inverse, rounding
	// InvSqrt and Inv are exact here.

	inline f32_x4  F32x4Sqrt(f32_x4 a)                     TOOL_SIMD_LANES(f32_x4, f, 4, SimdScalarSqrt(a.f[i]))
	inline f64_x2  F64x2Sqrt(f64_x2 a)                     TOOL_SIMD_LANES(f64_x2, f, 2, SimdScalarSqrt(a.f[i]))
	inline f32_x4  F32x4InvSqrt(f32_x4 a)                  TOOL_SIMD_LANES(f32_x4, f, 4, 1.0f / SimdScalarSqrt(a.f[i]))
	inline f32_x4  F32x4Inv(f32_x4 a)                      TOOL_SIMD_LANES(f32_x4, f, 4, 1.0f / a.f[i])

	inline f32_x4  F32x4Ceil(f32_x4 a)                     TOOL_SIMD_LANES(f32_x4, f, 4, SimdScalarCeil(a.f[i]))
	inline f64_x2  F64x2Ceil(f64_x2 a)                     TOOL_SIMD_LANES(f64_x2, f, 2, SimdScalarCeil(a.f[i]))
	inline f32_x4  F32x4Floor(f32_x4 a)                    TOOL_SIMD_LANES(f32_x4, f, 4, SimdScalarFloor(a.f[i]))
	inline f64_x2  F64x2Floor(f64_x2 a)                    TOOL_SIMD_LANES(f64_x2, f, 2, SimdScalarFloor(a.f[i]))
	inline f32_x4  F32x4Round(f32_x4 a)                    TOOL_SIMD_LANES(f32_x4, f, 4, SimdScalarRoundEven(a.f[i]))
	inline f64_x2  F64x2Round(f64_x2 a)                    TOOL_SIMD_LANES(f64_x2, f, 2, SimdScalarRoundEven(a.f[i]))

	//~ MulAdd, MulSub, AddSub
	// A multiply then an add here.

	inline f32_x4  F32x4MulAdd(f32_x4 a, f32_x4 b, f32_x4 c)     TOOL_SIMD_LANES(f32_x4, f, 4, a.f[i] * b.f[i] + c.f[i])
	inline f64_x2  F64x2MulAdd(f64_x2 a, f64_x2 b, f64_x2 c)     TOOL_SIMD_LANES(f64_x2, f, 2, a.f[i] * b.f[i] + c.f[i])
	inline f32_x4  F32x4MulSub(f32_x4 a, f32_x4 b, f32_x4 c)     TOOL_SIMD_LANES(f32_x4, f, 4, a.f[i] * b.f[i] - c.f[i])
	inline f64_x2  F64x2MulSub(f64_x2 a, f64_x2 b, f64_x2 c)     TOOL_SIMD_LANES(f64_x2, f, 2, a.f[i] * b.f[i] - c.f[i])

	inline f32_x4 F32x4AddSub(f32_x4 a, f32_x4 b)          TOOL_SIMD_LANES(f32_x4, f, 4, (i % 2 == 0) ? a.f[i] - b.f[i] : a.f[i] + b.f[i])
	inline f64_x2 F64x2AddSub(f64_x2 a, f64_x2 b)          TOOL_SIMD_LANES(f64_x2, f, 2, (i % 2 == 0) ? a.f[i] - b.f[i] : a.f[i] + b.f[i])

	//~ Comparison, blending, bitwise

	template<Comparison C>
	inline f32_x4_mask F32x4Compare(f32_x4 a, f32_x4 b)    TOOL_SIMD_LANES(f32_x4, f, 4, SimdScalarMask32(SimdScalarCompare<C>(a.f[i], b.f[i])))

	template<Comparison C>
	inline f64_x2_mask F64x2Compare(f64_x2 a, f64_x2 b)    TOOL_SIMD_LANES(f64_x2, f, 2, SimdScalarMask64(SimdScalarCompare<C>(a.f[i], b.f[i])))

	inline f32_x4 F32x4Blend(f32_x4 a, f32_x4 b, f32_x4_mask mask)
		TOOL_SIMD_LANES(f32_x4, f, 4, SimdScalarFloat((SimdScalarBits(a.f[i]) & ~SimdScalarBits(mask.f[i])) | (SimdScalarBits(b.f[i]) & SimdScalarBits(mask.f[i]))))
	inline f64_x2 F64x2Blend(f64_x2 a, f64_x2 b, f64_x2_mask mask)
		TOOL_SIMD_LANES(f64_x2, f, 2, SimdScalarFloat((SimdScalarBits(a.f[i]) & ~SimdScalarBits(mask.f[i])) | (SimdScalarBits(b.f[i]) & SimdScalarBits(mask.f[i]))))

	inline f32_x4 F32x4And(f32_x4 a, f32_x4 b)             TOOL_SIMD_LANES(f32_x4, f, 4, SimdScalarFloat(SimdScalarBits(a.f[i]) & SimdScalarBits(b.f[i])))
	inline f64_x2 F64x2And(f64_x2 a, f64_x2 b)             TOOL_SIMD_LANES(f64_x2, f, 2, SimdScalarFloat(SimdScalarBits(a.f[i]) & SimdScalarBits(b.f[i])))
	inline f32_x4 F32x4AndNot(f32_x4 a, f32_x4 b)          TOOL_SIMD_LANES(f32_x4, f, 4, SimdScalarFloat(SimdScalarBits(a.f[i]) & ~SimdScalarBits(b.f[i])))
	inline f64_x2 F64x2AndNot(f64_x2 a, f64_x2 b)          TOOL_SIMD_LANES(f64_x2, f, 2, SimdScalarFloat(SimdScalarBits(a.f[i]) & ~SimdScalarBits(b.f[i])))
	inline f32_x4 F32x4Or(f32_x4 a, f32_x4 b)              TOOL_SIMD_LANES(f32_x4, f, 4, SimdScalarFloat(SimdScalarBits(a.f[i]) | SimdScalarBits(b.f[i])))
	inline f64_x2 F64x2Or(f64_x2 a, f64_x2 b)              TOOL_SIMD_LANES(f64_x2, f, 2, SimdScalarFloat(SimdScalarBits(a.f[i]) | SimdScalarBits(b.f[i])))
	inline f32_x4 F32x4Xor(f32_x4 a, f32_x4 b)             TOOL_SIMD_LANES(f32_x4, f, 4, SimdScalarFloat(SimdScalarBits(a.f[i]) ^ SimdScalarBits(b.f[i])))
	inline f64_x2 F64x2Xor(f64_x2 a, f64_x2 b)             TOOL_SIMD_LANES(f64_x2, f, 2, SimdScalarFloat(SimdScalarBits(a.f[i]) ^ SimdScalarBits(b.f[i])))

#endif



	//- 256-bit from 128-bit halves

#if defined(TOOL_SIMD_HALVES_FLOAT256)

	//~ Floats

	inline f32_x8 F32x8Set(f32 all)                        { f32_x8 r; r.half[0] = r.half[1] = F32x4Set(all); return r; }
	inline f64_x4 F64x4Set(f64 all)                        { f64_x4 r; r.half[0] = r.half[1] = F64x2Set(all); return r; }
	inline f32_x8 F32x8Set(f32 f0, f32 f1, f32 f2, f32 f3,
	                       f32 f4, f32 f5, f32 f6, f32 f7)
	{
		f32_x8 r;
		r.half[0] = F32x4Set(f0, f1, f2, f3);
		r.half[1] = F32x4Set(f4, f5, f6, f7);
		return r;
	}
	inline f64_x4 F64x4Set(f64 f0, f64 f1, f64 f2, f64 f3)
	{
		f64_x4 r;
		r.half[0] = F64x2Set(f0, f1);
		r.half[1] = F64x2Set(f2, f3);
		return r;
	}

	inline f32_x8 F32x8Load(const f32* values)             { f32_x8 r; r.half[0] = F32x4Load(values); r.half[1] = F32x4Load(values + 4); return r; }
	inline f64_x4 F64x4Load(const f64* values)             { f64_x4 r; r.half[0] = F64x2Load(values); r.half[1] = F64x2Load(values + 2); return r; }
	inline void F32x8Store(f32_x8 src, f32* dst)           { F32x4Store(src.half[0], dst); F32x4Store(src.half[1], dst + 4); }
	inline void F64x4Store(f64_x4 src, f64* dst)           { F64x2Store(src.half[0], dst); F64x2Store(src.half[1], dst + 2); }

	inline f32_x8  F32x8Add(f32_x8 a, f32_x8 b)            TOOL_SIMD_HALVES2(f32_x8, F32x4Add, a, b)
	inline f64_x4  F64x4Add(f64_x4 a, f64_x4 b)            TOOL_SIMD_HALVES2(f64_x4, F64x2Add, a, b)
	inline f32_x8  F32x8Sub(f32_x8 a, f32_x8 b)            TOOL_SIMD_HALVES2(f32_x8, F32x4Sub, a, b)
	inline f64_x4  F64x4Sub(f64_x4 a, f64_x4 b)            TOOL_SIMD_HALVES2(f64_x4, F64x2Sub, a, b)
	inline f32_x8  F32x8Mul(f32_x8 a, f32_x8 b)            TOOL_SIMD_HALVES2(f32_x8, F32x4Mul, a, b)
	inline f64_x4  F64x4Mul(f64_x4 a, f64_x4 b)            TOOL_SIMD_HALVES2(f64_x4, F64x2Mul, a, b)
	inline f32_x8  F32x8Div(f32_x8 a, f32_x8 b)            TOOL_SIMD_HALVES2(f32_x8, F32x4Div, a, b)
	inline f64_x4  F64x4Div(f64_x4 a, f64_x4 b)            TOOL_SIMD_HALVES2(f64_x4, F64x2Div, a, b)

	inline f32_x8  F32x8Sqrt(f32_x8 a)                     TOOL_SIMD_HALVES1(f32_x8, F32x4Sqrt, a)
	inline f64_x4  F64x4Sqrt(f64_x4 a)                     TOOL_SIMD_HALVES1(f64_x4, F64x2Sqrt, a)
	inline f32_x8  F32x8InvSqrt(f32_x8 a)                  TOOL_SIMD_HALVES1(f32_x8, F32x4InvSqrt, a)
	inline f32_x8  F32x8Inv(f32_x8 a)                      TOOL_SIMD_HALVES1(f32_x8, F32x4Inv, a)
	inline f32_x8  F32x8Ceil(f32_x8 a)                     TOOL_SIMD_HALVES1(f32_x8, F32x4Ceil, a)
	inline f64_x4  F64x4Ceil(f64_x4 a)                     TOOL_SIMD_HALVES1(f64_x4, F64x2Ceil, a)
	inline f32_x8  F32x8Floor(f32_x8 a)                    TOOL_SIMD_HALVES1(f32_x8, F32x4Floor, a)
	inline f64_x4  F64x4Floor(f64_x4 a)                    TOOL_SIMD_HALVES1(f64_x4, F64x2Floor, a)
	inline f32_x8  F32x8Round(f32_x8 a)                    TOOL_SIMD_HALVES1(f32_x8, F32x4Round, a)
	inline f64_x4  F64x4Round(f64_x4 a)                    TOOL_SIMD_HALVES1(f64_x4, F64x2Round, a)

	inline f32_x8  F32x8MulAdd(f32_x8 a, f32_x8 b, f32_x8 c)     TOOL_SIMD_HALVES3(f32_x8, F32x4MulAdd, a, b, c)
	inline f64_x4  F64x4MulAdd(f64_x4 a, f64_x4 b, f64_x4 c)     TOOL_SIMD_HALVES3(f64_x4, F64x2MulAdd, a, b, c)
	inline f32_x8  F32x8MulSub(f32_x8 a, f32_x8 b, f32_x8 c)     TOOL_SIMD_HALVES3(f32_x8, F32x4MulSub, a, b, c)
	inline f64_x4  F64x4MulSub(f64_x4 a, f64_x4 b, f64_x4 c)     TOOL_SIMD_HALVES3(f64_x4, F64x2MulSub, a, b, c)
	inline f32_x8  F32x8AddSub(f32_x8 a, f32_x8 b)               TOOL_SIMD_HALVES2(f32_x8, F32x4AddSub, a, b)
	inline f64_x4  F64x4AddSub(f64_x4 a, f64_x4 b)               TOOL_SIMD_HALVES2(f64_x4, F64x2AddSub, a, b)

	template<Comparison C>
	inline f32_x8_mask F32x8Compare(f32_x8 a, f32_x8 b)    TOOL_SIMD_HALVES2(f32_x8, F32x4Compare<C>, a, b)

	template<Comparison C>
	inline f64_x4_mask F64x4Compare(f64_x4 a, f64_x4 b)    TOOL_SIMD_HALVES2(f64_x4, F64x2Compare<C>, a, b)

	inline f32_x8 F32x8Blend(f32_x8 a, f32_x8 b, f32_x8_mask mask) TOOL_SIMD_HALVES3(f32_x8, F32x4Blend, a, b, mask)
	inline f64_x4 F64x4Blend(f64_x4 a, f64_x4 b, f64_x4_mask mask) TOOL_SIMD_HALVES3(f64_x4, F64x2Blend, a, b, mask)

	inline f32_x8 F32x8And(f32_x8 a, f32_x8 b)             TOOL_SIMD_HALVES2(f32_x8, F32x4And, a, b)
	inline f64_x4 F64x4And(f64_x4 a, f64_x4 b)             TOOL_SIMD_HALVES2(f64_x4, F64x2And, a, b)
	inline f32_x8 F32x8AndNot(f32_x8 a, f32_x8 b)          TOOL_SIMD_HALVES2(f32_x8, F32x4AndNot, a, b)
	inline f64_x4 F64x4AndNot(f64_x4 a, f64_x4 b)          TOOL_SIMD_HALVES2(f64_x4, F64x2AndNot, a, b)
	inline f32_x8 F32x8Or(f32_x8 a, f32_x8 b)              TOOL_SIMD_HALVES2(f32_x8, F32x4Or, a, b)
	inline f64_x4 F64x4Or(f64_x4 a, f64_x4 b)              TOOL_SIMD_HALVES2(f64_x4, F64x2Or, a, b)
	inline f32_x8 F32x8Xor(f32_x8 a, f32_x8 b)             TOOL_SIMD_HALVES2(f32_x8, F32x4Xor, a, b)
	inline f64_x4 F64x4Xor(f64_x4 a, f64_x4 b)             TOOL_SIMD_HALVES2(f64_x4, F64x2Xor, a, b)

#endif

#if defined(TOOL_SIMD_HALVES_INT256)

	//~ Integers

	inline i8_x32 I8x32Set(i8 all)                         { i8_x32 r; r.half[0] = r.half[1] = I8x16Set(all); return r; }
	inline i16_x16 I16x16Set(i16 all)                      { i16_x16 r; r.half[0] = r.half[1] = I16x8Set(all); return r; }
	inline i32_x8 I32x8Set(i32 all)                        { i32_x8 r; r.half[0] = r.half[1] = I32x4Set(all); return r; }
	inline i64_x4 I64x4Set(i64 all)                        { i64_x4 r; r.half[0] = r.half[1] = I64x2Set(all); return r; }

	inline i64_x4 I64x4Set(i64 i0, i64 i1, i64 i2, i64 i3)
	{
		i64_x4 r;
		r.half[0] = I64x2Set(i0, i1);
		r.half[1] = I64x2Set(i2, i3);
		return r;
	}
	inline i32_x8 I32x8Set(i32 i0, i32 i1, i32 i2, i32 i3,
	                       i32 i4, i32 i5, i32 i6, i32 i7)
	{
		i32_x8 r;
		r.half[0] = I32x4Set(i0, i1, i2, i3);
		r.half[1] = I32x4Set(i4, i5, i6, i7);
		return r;
	}
	inline i16_x16 I16x16Set(i16 i0, i16 i1, i16 i2, i16 i3,
	                         i16 i4, i16 i5, i16 i6, i16 i7,
	                         i16 i8_, i16 i9, i16 i10, i16 i11,
	                         i16 i12, i16 i13, i16 i14, i16 i15)
	{
		i16_x16 r;
		r.half[0] = I16x8Set(i0, i1, i2, i3, i4, i5, i6, i7);
		r.half[1] = I16x8Set(i8_, i9, i10, i11, i12, i13, i14, i15);
		return r;
	}
	inline i8_x32 I8x32Set(i8 i0, i8 i1, i8 i2, i8 i3,
	                       i8 i4, i8 i5, i8 i6, i8 i7,
	                       i8 i8_, i8 i9, i8 i10, i8 i11,
	                       i8 i12, i8 i13, i8 i14, i8 i15,
	                       i8 i16_, i8 i17, i8 i18, i8 i19,
	                       i8 i20, i8 i21, i8 i22, i8 i23,
	                       i8 i24, i8 i25, i8 i26, i8 i27,
	                       i8 i28, i8 i29, i8 i30, i8 i31)
	{
		i8_x32 r;
		r.half[0] = I8x16Set(i0, i1, i2, i3, i4, i5, i6, i7, i8_, i9, i10, i11, i12, i13, i14, i15);
		r.half[1] = I8x16Set(i16_, i17, i18, i19, i20, i21, i22, i23, i24, i25, i26, i27, i28, i29, i30, i31);
		return r;
	}

	inline i8_x32 I8x32Load(const i8* values)              { i8_x32 r; r.half[0] = I8x16Load(values); r.half[1] = I8x16Load(values + 16); return r; }
	inline i16_x16 I16x16Load(const i16* values)           { i16_x16 r; r.half[0] = I16x8Load(values); r.half[1] = I16x8Load(values + 8); return r; }
	inline i32_x8 I32x8Load(const i32* values)             { i32_x8 r; r.half[0] = I32x4Load(values); r.half[1] = I32x4Load(values + 4); return r; }
	inline i64_x4 I64x4Load(const i64* values)             { i64_x4 r; r.half[0] = I64x2Load(values); r.half[1] = I64x2Load(values + 2); return r; }

	inline void I8x32Store(i8_x32 src, i8* dst)            { I8x16Store(src.half[0], dst); I8x16Store(src.half[1], dst + 16); }
	inline void I16x16Store(i16_x16 src, i16* dst)         { I16x8Store(src.half[0], dst); I16x8Store(src.half[1], dst + 8); }
	inline void I32x8Store(i32_x8 src, i32* dst)           { I32x4Store(src.half[0], dst); I32x4Store(src.half[1], dst + 4); }
	inline void I64x4Store(i64_x4 src, i64* dst)           { I64x2Store(src.half[0], dst); I64x2Store(src.half[1], dst + 2); }

	inline i8_x32  I8x32Add(i8_x32 a, i8_x32 b)            TOOL_SIMD_HALVES2(i8_x32, I8x16Add, a, b)
	inline i16_x16 I16x16Add(i16_x16 a, i16_x16 b)         TOOL_SIMD_HALVES2(i16_x16, I16x8Add, a, b)
	inline i32_x8  I32x8Add(i32_x8 a, i32_x8 b)            TOOL_SIMD_HALVES2(i32_x8, I32x4Add, a, b)
	inline i64_x4  I64x4Add(i64_x4 a, i64_x4 b)            TOOL_SIMD_HALVES2(i64_x4, I64x2Add, a, b)

	inline i8_x32  I8x32Sub(i8_x32 a, i8_x32 b)            TOOL_SIMD_HALVES2(i8_x32, I8x16Sub, a, b)
	inline i16_x16 I16x16Sub(i16_x16 a, i16_x16 b)         TOOL_SIMD_HALVES2(i16_x16, I16x8Sub, a, b)
	inline i32_x8  I32x8Sub(i32_x8 a, i32_x8 b)            TOOL_SIMD_HALVES2(i32_x8, I32x4Sub, a, b)
	inline i64_x4  I64x4Sub(i64_x4 a, i64_x4 b)            TOOL_SIMD_HALVES2(i64_x4, I64x2Sub, a, b)

	inline i16_x16 I16x16Mul(i16_x16 a, i16_x16 b)         TOOL_SIMD_HALVES2(i16_x16, I16x8Mul, a, b)
	inline i32_x8  I32x8Mul(i32_x8 a, i32_x8 b)            TOOL_SIMD_HALVES2(i32_x8, I32x4Mul, a, b)
	inline i64_x4  I64x4Mul(i64_x4 a, i64_x4 b)            TOOL_SIMD_HALVES2(i64_x4, I64x2Mul, a, b)

	inline i8_x32  I8x32Abs(i8_x32 a)                      TOOL_SIMD_HALVES1(i8_x32, I8x16Abs, a)
	inline i16_x16 I16x16Abs(i16_x16 a)                    TOOL_SIMD_HALVES1(i16_x16, I16x8Abs, a)
	inline i32_x8  I32x8Abs(i32_x8 a)                      TOOL_SIMD_HALVES1(i32_x8, I32x4Abs, a)
	inline i64_x4  I64x4Abs(i64_x4 a)                      TOOL_SIMD_HALVES1(i64_x4, I64x2Abs, a)

#endif



	//- Every backend

	//~ Set by value (vector)

	inline f32_x4 F32x4Set(v4 v)                           { return F32x4Set(v.x, v.y, v.z, v.w); }
	inline f32_x4 F32x4Set(v2 v0, v2 v1)                   { return F32x4Set(v0.x, v0.y, v1.x, v1.y); }

	inline f32_x8 F32x8Set(v4 v0, v4 v1)                   { return F32x8Set(v0.x, v0.y, v0.z, v0.w, v1.x, v1.y, v1.z, v1.w); }
	inline f32_x8 F32x8Set(v2 v0, v2 v1, v2 v2_, v2 v3)    { return F32x8Set(v0.x, v0.y, v1.x, v1.y, v2_.x, v2_.y, v3.x, v3.y); }

	inline f64_x2 F64x2Set(v2d v)                          { return F64x2Set(v.x, v.y); }

	inline f64_x4 F64x4Set(v4d v)                          { return F64x4Set(v.x, v.y, v.z, v.w); }
	inline f64_x4 F64x4Set(v2d v0, v2d v1)                 { return F64x4Set(v0.x, v0.y, v1.x, v1.y); }

	inline i32_x4 I32x4Set(p4 p)                           { return I32x4Set(p.x, p.y, p.z, p.w); }
	inline i32_x4 I32x4Set(p2 p0, p2 p1)                   { return I32x4Set(p0.x, p0.y, p1.x, p1.y); }

	inline i32_x8 I32x8Set(p4 p0, p4 p1)                   { return I32x8Set(p0.x, p0.y, p0.z, p0.w, p1.x, p1.y, p1.z, p1.w); }
	inline i32_x8 I32x8Set(p2 p0, p2 p1, p2 p2_, p2 p3)    { return I32x8Set(p0.x, p0.y, p1.x, p1.y, p2_.x, p2_.y, p3.x, p3.y); }

	//~ Load (vector)

	inline f32_x4 F32x4Load(const v2* vectors)             { return F32x4Load((const f32*)vectors); }
	inline f32_x4 F32x4Load(const v4* vector)              { return F32x4Load((const f32*)vector); }
	inline f32_x8 F32x8Load(const v2* vectors)             { return F32x8Load((const f32*)vectors); }
	inline f32_x8 F32x8Load(const v4* vectors)             { return F32x8Load((const f32*)vectors); }

	inline f64_x2 F64x2Load(const v2d* vector)             { return F64x2Load((const f64*)vector); }
	inline f64_x4 F64x4Load(const v2d* vectors)            { return F64x4Load((const f64*)vectors); }
	inline f64_x4 F64x4Load(const v4d* vector)             { return F64x4Load((const f64*)vector); }
} //namespace Tool



//~ Acronym usings

#ifndef TOOL_NO_ACRONYMS

using Tool::f32_x4;
using Tool::f32_x8;

using Tool::f64_x2;
using Tool::f64_x4;

using Tool::i8_x16;
using Tool::i16_x8;
using Tool::i32_x4;
using Tool::i64_x2;

using Tool::i8_x32;
using Tool::i16_x16;
using Tool::i32_x8;
using Tool::i64_x4;

#endif



#endif //_TOOL_INTRINSICS_H
