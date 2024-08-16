#ifndef _TOOL_INTRINSICS_H
#define _TOOL_INTRINSICS_H

#ifndef __AVX__
//#error AVX intrinsics not supported by the compiler.
#endif

// Try to include AVX intrinsics
#include <immintrin.h>

#include "basics.h"
#include "vector.h"
#include "matrix.h"



namespace Tool
{
    //- Type definitions
    
    //~ 128-bit intrinsics
    
    union Float32x4
    {
        __m128 m;
        
        f32 f[4];
        
        v2 vector2[2];
        v4 vector4;
    };
    
    union Float64x2
    {
        __m128d m;
        
        f64 f[2];
        
        v2d vector2d;
    };
    
    union Integer128
    {
        __m128i m;
        
        i8  int8[16];
        i16 int16[8];
        i32 int32[4];
        i64 int64[2];
        
        p2  point2[2];
        p4  point4;
    };
    
    //~ 256-bit intrinsics
    
    union Float32x8
    {
        __m256 m;
        
        f32 f[8];
        
        v2 vector2[4];
        v4 vector4[2];
    };
    
    union Float64x4
    {
        __m256d m;
        
        f64 f[4];
        
        v2d vector2d[2];
        v4d vector4d;
    };
    
    union Integer256
    {
        __m256i m;
        
        i8  int8[32];
        i16 int16[16];
        i32 int32[8];
        i64 int64[4];
        
        p2  point2[4];
        p4  point4[2];
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
    
    // Uses ordered signaling comparison (exceptions raised when operating on NaN values)
    enum Comparison : const int
    {
        ComparisonEquals              = _CMP_EQ_OS,
        ComparisonNotEquals           = _CMP_NEQ_OS,
        ComparisonGreaterThan         = _CMP_GT_OS,
        ComparisonGreaterThanOrEquals = _CMP_GE_OS,
        ComparisonLessThan            = _CMP_LT_OS,
        ComparisonLessThanOrEquals    = _CMP_LE_OS
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
    
    
    
    //- Initialization (common)
    
    
    
    //~ Set by value (all same)
    
    inline f32_x4 F32x4Set(f32 all)                        { return { _mm_set1_ps(all) }; }
    inline f32_x8 F32x8Set(f32 all)                        { return { _mm256_set1_ps(all) }; }
    inline f64_x2 F64x2Set(f64 all)                        { return { _mm_set1_pd(all) }; }
    inline f64_x4 F64x4Set(f64 all)                        { return { _mm256_set1_pd(all) }; }
    
    inline i8_x16 I8x16Set(i8 all)                         { return { _mm_set1_epi8(all) }; }
    inline i8_x32 I8x32Set(i8 all)                         { return { _mm256_set1_epi8(all) }; }
    inline i16_x8 I16x8Set(i16 all)                        { return { _mm_set1_epi16(all) }; }
    inline i16_x16 I16x16Set(i16 all)                      { return { _mm256_set1_epi16(all) }; }
    inline i32_x4 I32x4Set(i32 all)                        { return { _mm_set1_epi32(all) }; }
    inline i32_x8 I32x8Set(i32 all)                        { return { _mm256_set1_epi32(all) }; }
    inline i64_x2 I64x2Set(i64 all)                        { return { _mm_set1_epi64x(all) }; }
    inline i64_x4 I64x4Set(i64 all)                        { return { _mm256_set1_epi64x(all) }; }
    
    
    
    //~ Load
    
    // All same
    inline f32_x4 F32x4LoadOne(f32* value)                 { return { _mm_load1_ps(value) }; }
    inline f64_x2 F64x2LoadOne(f64* value)                 { return { _mm_load1_pd(value) }; }
    
    // All different
    inline f32_x4 F32x4Load(f32* values)                   { return { _mm_load_ps(values) }; }
    inline f32_x8 F32x8Load(f32* values)                   { return { _mm256_load_ps(values) }; }
    inline f64_x2 F64x2Load(f64* values)                   { return { _mm_load_pd(values) }; }
    inline f64_x4 F64x4Load(f64* values)                   { return { _mm256_load_pd(values) }; }
    
    inline f32_x4 F32x4Load(v2* vectors)                   { return { _mm_load_ps((f32*)vectors) }; }
    inline f32_x4 F32x4Load(v4* vector)                    { return { _mm_load_ps((f32*)vector) }; }
    inline f32_x8 F32x8Load(v2* vectors)                   { return { _mm256_load_ps((f32*)vectors) }; }
    inline f32_x8 F32x8Load(v4* vectors)                   { return { _mm256_load_ps((f32*)vectors) }; }
    
    inline f64_x2 F64x2Load(v2d* vector)                   { return { _mm_load_pd((f64*)vector) }; }
    inline f64_x4 F64x4Load(v2d* vectors)                  { return { _mm256_load_pd((f64*)vectors) }; }
    inline f64_x4 F64x4Load(v4d* vector)                   { return { _mm256_load_pd((f64*)vector) }; }
    
    
    
    //- Storing
    
    
    
    //~ Store
    
    inline void F32x4Store(f32_x4 src, f32* dst)           { _mm_storeu_ps(dst, src.m); }
    inline void F32x8Store(f32_x8 src, f32* dst)           { _mm256_storeu_ps(dst, src.m); }
    inline void F64x2Store(f64_x2 src, f64* dst)           { _mm_storeu_pd(dst, src.m); }
    inline void F64x4Store(f64_x4 src, f64* dst)           { _mm256_storeu_pd(dst, src.m); }
    
    inline void I8x16Store(i8_x16 src, i8* dst)            { _mm_storeu_si128((__m128i*)dst, src.m); }
    inline void I8x32Store(i8_x32 src, i8* dst)            { _mm256_storeu_si256((__m256i*)dst, src.m); }
    inline void I16x8Store(i16_x8 src, i16* dst)           { _mm_storeu_si128((__m128i*)dst, src.m); }
    inline void I16x16Store(i16_x16 src, i16* dst)         { _mm256_storeu_si256((__m256i*)dst, src.m); }
    inline void I32x4Store(i32_x4 src, i32* dst)           { _mm_storeu_si128((__m128i*)dst, src.m); }
    inline void I32x8Store(i32_x8 src, i32* dst)           { _mm256_storeu_si256((__m256i*)dst, src.m); }
    inline void I64x2Store(i64_x2 src, i64* dst)           { _mm_storeu_si128((__m128i*)dst, src.m); }
    inline void I64x4Store(i64_x4 src, i64* dst)           { _mm256_storeu_si256((__m256i*)dst, src.m); }
    
    
    
    //- Basic arithmetic
    
    
    
    //~ Addition
    
    inline f32_x4  F32x4Add(f32_x4 a, f32_x4 b)            { return { _mm_add_ps(a.m, b.m) }; }
    inline f32_x8  F32x8Add(f32_x8 a, f32_x8 b)            { return { _mm256_add_ps(a.m, b.m) }; }
    inline f64_x2  F64x2Add(f64_x2 a, f64_x2 b)            { return { _mm_add_pd(a.m, b.m) }; }
    inline f64_x4  F64x4Add(f64_x4 a, f64_x4 b)            { return { _mm256_add_pd(a.m, b.m) }; }
    inline i8_x16  I8x16Add(i8_x16  a, i8_x16  b)          { return { _mm_add_epi8(a.m, b.m) }; }
    inline i8_x32  I8x32Add(i8_x32  a, i8_x32  b)          { return { _mm256_add_epi8(a.m, b.m) }; }
    inline i16_x8  I16x8Add(i16_x8  a, i16_x8  b)          { return { _mm_add_epi16(a.m, b.m) }; }
    inline i16_x16 I16x16Add(i16_x16 a, i16_x16 b)         { return { _mm256_add_epi16(a.m, b.m) }; }
    inline i32_x4  I32x4Add(i32_x4  a, i32_x4  b)          { return { _mm_add_epi32(a.m, b.m) }; }
    inline i32_x8  I32x8Add(i32_x8  a, i32_x8  b)          { return { _mm256_add_epi32(a.m, b.m) }; }
    inline i64_x2  I64x2Add(i64_x2  a, i64_x2  b)          { return { _mm_add_epi64(a.m, b.m) }; }
    inline i64_x4  I64x4Add(i64_x4  a, i64_x4  b)          { return { _mm256_add_epi64(a.m, b.m) }; }
    
    
    
    //~ Subtraction
    
    inline f32_x4  F32x4Sub(f32_x4 a, f32_x4 b)            { return { _mm_sub_ps(a.m, b.m) }; }
    inline f32_x8  F32x8Sub(f32_x8 a, f32_x8 b)            { return { _mm256_sub_ps(a.m, b.m) }; }
    inline f64_x2  F64x2Sub(f64_x2 a, f64_x2 b)            { return { _mm_sub_pd(a.m, b.m) }; }
    inline f64_x4  F64x4Sub(f64_x4 a, f64_x4 b)            { return { _mm256_sub_pd(a.m, b.m) }; }
    inline i8_x16  I8x16Sub(i8_x16  a, i8_x16  b)          { return { _mm_sub_epi8(a.m, b.m) }; }
    inline i8_x32  I8x32Sub(i8_x32  a, i8_x32  b)          { return { _mm256_sub_epi8(a.m, b.m) }; }
    inline i16_x8  I16x8Sub(i16_x8  a, i16_x8  b)          { return { _mm_sub_epi16(a.m, b.m) }; }
    inline i16_x16 I16x16Sub(i16_x16 a, i16_x16 b)         { return { _mm256_sub_epi16(a.m, b.m) }; }
    inline i32_x4  I32x4Sub(i32_x4  a, i32_x4  b)          { return { _mm_sub_epi32(a.m, b.m) }; }
    inline i32_x8  I32x8Sub(i32_x8  a, i32_x8  b)          { return { _mm256_sub_epi32(a.m, b.m) }; }
    inline i64_x2  I64x2Sub(i64_x2  a, i64_x2  b)          { return { _mm_sub_epi64(a.m, b.m) }; }
    inline i64_x4  I64x4Sub(i64_x4  a, i64_x4  b)          { return { _mm256_sub_epi64(a.m, b.m) }; }
    
    
    
    //~ Multiplication
    
    inline f32_x4  F32x4Mul(f32_x4 a, f32_x4 b)            { return { _mm_mul_ps(a.m, b.m) }; }
    inline f32_x8  F32x8Mul(f32_x8 a, f32_x8 b)            { return { _mm256_mul_ps(a.m, b.m) }; }
    inline f64_x2  F64x2Mul(f64_x2 a, f64_x2 b)            { return { _mm_mul_pd(a.m, b.m) }; }
    inline f64_x4  F64x4Mul(f64_x4 a, f64_x4 b)            { return { _mm256_mul_pd(a.m, b.m) }; }
    inline i16_x8  I16x8Mul(i16_x8  a, i16_x8  b)          { return { _mm_mullo_epi16(a.m, b.m) }; }
    inline i16_x16 I16x16Mul(i16_x16 a, i16_x16 b)         { return { _mm256_mullo_epi16(a.m, b.m) }; }
    inline i32_x4  I32x4Mul(i32_x4  a, i32_x4  b)          { return { _mm_mullo_epi32(a.m, b.m) }; }
    inline i32_x8  I32x8Mul(i32_x8  a, i32_x8  b)          { return { _mm256_mullo_epi32(a.m, b.m) }; }
    inline i64_x2  I64x2Mul(i64_x2  a, i64_x2  b)          { return { _mm_mullo_epi64(a.m, b.m) }; }
    inline i64_x4  I64x4Mul(i64_x4  a, i64_x4  b)          { return { _mm256_mullo_epi64(a.m, b.m) }; }
    
    
    
    //~ Division
    
    inline f32_x4  F32x4Div(f32_x4 a, f32_x4 b)            { return { _mm_div_ps(a.m, b.m) }; }
    inline f32_x8  F32x8Div(f32_x8 a, f32_x8 b)            { return { _mm256_div_ps(a.m, b.m) }; }
    inline f64_x2  F64x2Div(f64_x2 a, f64_x2 b)            { return { _mm_div_pd(a.m, b.m) }; }
    inline f64_x4  F64x4Div(f64_x4 a, f64_x4 b)            { return { _mm256_div_pd(a.m, b.m) }; }
    
    // Seemingly unsupported by Clang
    
    // inline i8_x16  I8x16Div(i8_x16  a, i8_x16  b)          { return { _mm_div_epi8(a.m, b.m) }; }
    // inline i8_x32  I8x32Div(i8_x32  a, i8_x32  b)          { return { _mm256_div_epi8(a.m, b.m) }; }
    // inline i16_x8  I16x8Div(i16_x8  a, i16_x8  b)          { return { _mm_div_epi16(a.m, b.m) }; }
    // inline i16_x16 I16x16Div(i16_x16 a, i16_x16 b)         { return { _mm256_div_epi16(a.m, b.m) }; }
    // inline i32_x4  I32x4Div(i32_x4  a, i32_x4  b)          { return { _mm_div_epi32(a.m, b.m) }; }
    // inline i32_x8  I32x8Div(i32_x8  a, i32_x8  b)          { return { _mm256_div_epi32(a.m, b.m) }; }
    // inline i64_x2  I64x2Div(i64_x2  a, i64_x2  b)          { return { _mm_div_epi64(a.m, b.m) }; }
    // inline i64_x4  I64x4Div(i64_x4  a, i64_x4  b)          { return { _mm256_div_epi64(a.m, b.m) }; }
    
    
    
    //- Unary operations
    
    
    
    //~ Absolute value
    
    inline i8_x16  I8x16Abs(i8_x16  a)                     { return { _mm_abs_epi8(a.m) }; }
    inline i8_x32  I8x32Abs(i8_x32  a)                     { return { _mm256_abs_epi8(a.m) }; }
    inline i16_x8  I16x8Abs(i16_x8  a)                     { return { _mm_abs_epi16(a.m) }; }
    inline i16_x16 I16x16Abs(i16_x16 a)                    { return { _mm256_abs_epi16(a.m) }; }
    inline i32_x4  I32x4Abs(i32_x4  a)                     { return { _mm_abs_epi32(a.m) }; }
    inline i32_x8  I32x8Abs(i32_x8  a)                     { return { _mm256_abs_epi32(a.m) }; }
    inline i64_x2  I64x2Abs(i64_x2  a)                     { return { _mm_abs_epi64(a.m) }; }
    inline i64_x4  I64x4Abs(i64_x4  a)                     { return { _mm256_abs_epi64(a.m) }; }
    
    
    
    //~ Square root
    
    inline f32_x4  F32x4Sqrt(f32_x4 a)                     { return { _mm_sqrt_ps(a.m) }; }
    inline f32_x8  F32x8Sqrt(f32_x8 a)                     { return { _mm256_sqrt_ps(a.m) }; }
    inline f64_x2  F64x2Sqrt(f64_x2 a)                     { return { _mm_sqrt_pd(a.m) }; }
    inline f64_x4  F64x4Sqrt(f64_x4 a)                     { return { _mm256_sqrt_pd(a.m) }; }
    
    inline f32_x4  F32x4InvSqrt(f32_x4 a)                  { return { _mm_rsqrt_ps(a.m) }; }
    inline f32_x8  F32x8InvSqrt(f32_x8 a)                  { return { _mm256_rsqrt_ps(a.m) }; }
    
    
    
    //~ Ceiling
    
    inline f32_x4  F32x4Ceil(f32_x4 a)                     { return { _mm_ceil_ps(a.m) }; }
    inline f32_x8  F32x8Ceil(f32_x8 a)                     { return { _mm256_ceil_ps(a.m) }; }
    inline f64_x2  F64x2Ceil(f64_x2 a)                     { return { _mm_ceil_pd(a.m) }; }
    inline f64_x4  F64x4Ceil(f64_x4 a)                     { return { _mm256_ceil_pd(a.m) }; }
    
    
    
    //~ Floor
    
    inline f32_x4  F32x4Floor(f32_x4 a)                    { return { _mm_floor_ps(a.m) }; }
    inline f32_x8  F32x8Floor(f32_x8 a)                    { return { _mm256_floor_ps(a.m) }; }
    inline f64_x2  F64x2Floor(f64_x2 a)                    { return { _mm_floor_pd(a.m) }; }
    inline f64_x4  F64x4Floor(f64_x4 a)                    { return { _mm256_floor_pd(a.m) }; }
    
    
    
    //~ Round
    
    inline f32_x4  F32x4Round(f32_x4 a)                    { return { _mm_round_ps(a.m, _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC) }; }
    inline f32_x8  F32x8Round(f32_x8 a)                    { return { _mm256_round_ps(a.m, _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC) }; }
    inline f64_x2  F64x2Round(f64_x2 a)                    { return { _mm_round_pd(a.m, _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC) }; }
    inline f64_x4  F64x4Round(f64_x4 a)                    { return { _mm256_round_pd(a.m, _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC) }; }
    
    
    
    //~ Inverse (Reciprocal)
    
    inline f32_x4  F32x4Inv(f32_x4 a)                      { return { _mm_rcp_ps(a.m) }; }
    inline f32_x8  F32x8Inv(f32_x8 a)                      { return { _mm256_rcp_ps(a.m) }; }
    
    
    
    //- Exotic arithmetic
    
    
    
    //~ MulAdd (a * b + c)
    
    inline f32_x4  F32x4MulAdd(f32_x4 a, f32_x4 b, f32_x4 c)     { return { _mm_fmadd_ps(a.m, b.m, c.m) }; }
    inline f32_x8  F32x8MulAdd(f32_x8 a, f32_x8 b, f32_x8 c)     { return { _mm256_fmadd_ps(a.m, b.m, c.m) }; }
    inline f64_x2  F64x2MulAdd(f64_x2 a, f64_x2 b, f64_x2 c)     { return { _mm_fmadd_pd(a.m, b.m, c.m) }; }
    inline f64_x4  F64x4MulAdd(f64_x4 a, f64_x4 b, f64_x4 c)     { return { _mm256_fmadd_pd(a.m, b.m, c.m) }; }
    
    
    
    //~ MulSub (a * b - c)
    
    inline f32_x4  F32x4MulSub(f32_x4 a, f32_x4 b, f32_x4 c)     { return { _mm_fmsub_ps(a.m, b.m, c.m) }; }
    inline f32_x8  F32x8MulSub(f32_x8 a, f32_x8 b, f32_x8 c)     { return { _mm256_fmsub_ps(a.m, b.m, c.m) }; }
    inline f64_x2  F64x2MulSub(f64_x2 a, f64_x2 b, f64_x2 c)     { return { _mm_fmsub_pd(a.m, b.m, c.m) }; }
    inline f64_x4  F64x4MulSub(f64_x4 a, f64_x4 b, f64_x4 c)     { return { _mm256_fmsub_pd(a.m, b.m, c.m) }; }
    
    
    
    //~ AddSub
    
    // Even indices add, odd indices subtract
    inline f32_x4 F32x4AddSub(f32_x4 a, f32_x4 b)          { return { _mm_addsub_ps(a.m, b.m) }; }
    inline f32_x8 F32x8AddSub(f32_x8 a, f32_x8 b)          { return { _mm256_addsub_ps(a.m, b.m) }; }
    inline f64_x2 F64x2AddSub(f64_x2 a, f64_x2 b)          { return { _mm_addsub_pd(a.m, b.m) }; }
    inline f64_x4 F64x4AddSub(f64_x4 a, f64_x4 b)          { return { _mm256_addsub_pd(a.m, b.m) }; }
    
    
    
    //- Selecting, comparing, and masking
    
    
    
    //~ Comparison
    
    template <Comparison C>
        inline f32_x4_mask F32x4Compare(f32_x4 a, f32_x4 b)    { return { _mm_cmp_ps(a.m, b.m, C) }; }
    
    template <Comparison C>
        inline f32_x8_mask F32x8Compare(f32_x8 a, f32_x8 b)    { return { _mm256_cmp_ps(a.m, b.m, C) }; }
    
    template <Comparison C>
        inline f64_x2_mask F64x2Compare(f64_x2 a, f64_x2 b)    { return { _mm_cmp_pd(a.m, b.m, C) }; }
    
    template <Comparison C>
        inline f64_x4_mask F64x4Compare(f64_x4 a, f64_x4 b)    { return { _mm256_cmp_pd(a.m, b.m, C) }; }
    
    
    
    //~ Blending
    
    inline f32_x4 F32x4Blend(f32_x4 a, f32_x4 b)           { return { _mm_add_ps(a.m, b.m) }; }
    inline f32_x8 F32x8Blend(f32_x8 a, f32_x8 b)           { return { _mm256_add_ps(a.m, b.m) }; }
    inline f64_x2 F64x2Blend(f64_x2 a, f64_x2 b)           { return { _mm_add_pd(a.m, b.m) }; }
    inline f64_x4 F64x4Blend(f64_x4 a, f64_x4 b)           { return { _mm256_add_pd(a.m, b.m) }; }
    
    
    //~ Bitwise arithmetic
    
    // Bitwise AND [ 11 ]
    inline f32_x4 F32x4And(f32_x4 a, f32_x4 b)             { return { _mm_and_ps(a.m, b.m) }; }
    inline f32_x8 F32x8And(f32_x8 a, f32_x8 b)             { return { _mm256_and_ps(a.m, b.m) }; }
    inline f64_x2 F64x2And(f64_x2 a, f64_x2 b)             { return { _mm_and_pd(a.m, b.m) }; }
    inline f64_x4 F64x4And(f64_x4 a, f64_x4 b)             { return { _mm256_and_pd(a.m, b.m) }; }
    
    // Bitwise AND NOT [ 10 ]
    inline f32_x4 F32x4AndNot(f32_x4 a, f32_x4 b)          { return { _mm_andnot_ps(b.m, a.m) }; }
    inline f32_x8 F32x8AndNot(f32_x8 a, f32_x8 b)          { return { _mm256_andnot_ps(b.m, a.m) }; }
    inline f64_x2 F64x2AndNot(f64_x2 a, f64_x2 b)          { return { _mm_andnot_pd(b.m, a.m) }; }
    inline f64_x4 F64x4AndNot(f64_x4 a, f64_x4 b)          { return { _mm256_andnot_pd(b.m, a.m) }; }
    
    //// Bitwise OR [ 01, 10, 11 ]
    //inline f32_x4 F32x4Or(f32_x4 a, f32_x4 b)              { return { _mm_or_ps(a.m, b.m) }; }
    //inline f32_x8 F32x8Or(f32_x8 a, f32_x8 b)              { return { _mm256_or_ps(a.m, b.m) }; }
    //inline f64_x2 F64x2Or(f64_x2 a, f64_x2 b)              { return { _mm_or_pd(a.m, b.m) }; }
    //inline f64_x4 F64x4Or(f64_x4 a, f64_x4 b)              { return { _mm256_or_pd(a.m, b.m) }; }
    
    //// Bitwise XOR [ 01, 10 ]
    //inline f32_x4 F32x4Xor(f32_x4 a, f32_x4 b)             { return { _mm_xor_ps(a.m, b.m) }; }
    //inline f32_x8 F32x8Xor(f32_x8 a, f32_x8 b)             { return { _mm256_xor_ps(a.m, b.m) }; }
    //inline f64_x2 F64x2Xor(f64_x2 a, f64_x2 b)             { return { _mm_xor_pd(a.m, b.m) }; }
    //inline f64_x4 F64x4Xor(f64_x4 a, f64_x4 b)             { return { _mm256_xor_pd(a.m, b.m) }; }
    
    
    
    //- Initialization (uncommon)
    
    
    
    //~ Set by value (individual)
    
    inline f32_x4 F32x4Set(f32 f0, f32 f1, f32 f2, f32 f3) { return { _mm_setr_ps(f0, f1, f2, f3) }; }
    inline f32_x8 F32x8Set(f32 f0, f32 f1, f32 f2, f32 f3,
                           f32 f4, f32 f5, f32 f6, f32 f7) { return { _mm256_setr_ps(f0, f1, f2, f3, 
                                                                                     f4, f5, f6, f7) }; }
    
    inline f64_x2 F64x4Set(f64 f0, f64 f1)                 { return { _mm_setr_pd(f0, f1) }; }
    inline f64_x4 F64x8Set(f64 f0, f64 f1, f64 f2, f64 f3) { return { _mm256_setr_pd(f0, f1, f2, f3) }; }
    
    inline i64_x2 I64x2Set(i64 i0, i64 i1)                 { return { _mm_set_epi64x(i0, i1) }; }
    inline i64_x4 I64x4Set(i64 i0, i64 i1, i64 i2, i64 i3) { return { _mm256_setr_epi64x(i0, i1, i2, i3) }; }
    
    inline i32_x4 I32x4Set(i32 i0, i32 i1, i32 i2, i32 i3) { return { _mm_set_epi32(i0, i1, i2, i3) }; }
    inline i32_x8 I32x8Set(i32 i0, i32 i1, i32 i2, i32 i3,
                           i32 i4, i32 i5, i32 i6, i32 i7) { return { _mm256_set_epi32(i0, i1, i2, i3,
                                                                                       i4, i5, i6, i7) }; }
    
    inline i16_x8 I16x8Set(i16 i0, i16 i1, i16 i2, i16 i3,
                           i16 i4, i16 i5, i16 i6, i16 i7) { return { _mm_set_epi16(i0, i1, i2, i3,
                                                                                    i4, i5, i6, i7) }; }
    inline i16_x16 I16x16Set(i16 i0,  i16 i1,  i16 i2,  i16 i3,
                             i16 i4,  i16 i5,  i16 i6,  i16 i7,
                             i16 i16_, i16 i9,  i16 i10, i16 i11,
                             i16 i12, i16 i13, i16 i14, i16 i15) { return { _mm256_set_epi16(i0, i1, i2, i3,
                                                                                             i4, i5, i6, i7,
                                                                                             i16_, i9, i10, i11,
                                                                                             i12, i13, i14, i15) }; }
    
    inline i8_x16 I8x16Set(i8 i0,  i8 i1,  i8 i2,  i8 i3,
                           i8 i4,  i8 i5,  i8 i6,  i8 i7,
                           i8 i8_, i8 i9,  i8 i10, i8 i11,
                           i8 i12, i8 i13, i8 i14, i8 i15) { return { _mm_set_epi8(i0, i1, i2, i3,
                                                                                   i4, i5, i6, i7,
                                                                                   i8_, i9, i10, i11,
                                                                                   i12, i13, i14, i15) }; }
    inline i8_x32 I8x32Set(i8 i0,   i8 i1,  i8 i2,  i8 i3,
                           i8 i4,   i8 i5,  i8 i6,  i8 i7,
                           i8 i8_,  i8 i9,  i8 i10, i8 i11,
                           i8 i12,  i8 i13, i8 i14, i8 i15,
                           i8 i16_, i8 i17, i8 i18, i8 i19,
                           i8 i20,  i8 i21, i8 i22, i8 i23,
                           i8 i24,  i8 i25, i8 i26, i8 i27,
                           i8 i28,  i8 i29, i8 i30, i8 i31) { return { _mm256_set_epi8(i0, i1, i2, i3,
                                                                                       i4, i5, i6, i7,
                                                                                       i8_, i9, i10, i11,
                                                                                       i12, i13, i14, i15,
                                                                                       i16_, i17, i18, i19,
                                                                                       i20, i21, i22, i23,
                                                                                       i24, i25, i26, i27,
                                                                                       i28, i29, i30, i31) }; }
    
    
    
    //~ Set by value (vector)
    
    inline f32_x4 F32x4Set(v4 v)                           { return { _mm_set_ps(v.x, v.y, v.z, v.w) }; }
    inline f32_x4 F32x4Set(v2 v0, v2 v1)                   { return { _mm_set_ps(v0.x, v0.y, v1.x, v1.y) }; }
    
    inline f32_x8 F32x8Set(v4 v0, v4 v1)                   { return { _mm256_set_ps(v0.x, v0.y, v0.z, v0.w,
                                                                                    v1.x, v1.y, v1.z, v1.w) }; }
    inline f32_x8 F32x8Set(v2 v20, v2 v21, v2 v22, v2 v23) { return { _mm256_set_ps(v20.x, v20.y, v21.x, v21.y,
                                                                                    v22.x, v22.y, v23.x, v23.y) }; }
    
    inline f64_x2 F64x2Set(v2d v)                          { return { _mm_set_pd(v.x, v.y) }; }
    
    inline f64_x4 F64x4Set(v4d v)                          { return { _mm256_set_pd(v.x, v.y, v.z, v.w) }; }
    inline f64_x4 F64x4Set(v2d v0, v2d v1)                 { return { _mm256_set_pd(v0.x, v0.y, v1.x, v1.y) }; }
    
    inline i32_x4 I32x4Set(p4 p)                           { return { _mm_set_epi32(p.x, p.y, p.z, p.w) }; }
    inline i32_x4 I32x4Set(p2 p0, p2 p1)                   { return { _mm_set_epi32(p0.x, p0.y, p1.x, p1.y) }; }
    
    inline i32_x8 I32x8Set(p4 p0, p4 p1)                   { return { _mm256_set_epi32(p0.x, p0.y, p0.z, p0.w,
                                                                                       p1.x, p1.y, p1.z, p1.w) }; }
    inline i32_x8 I32x8Set(p2 p0, p2 p1, p2 p2_, p2 p3_)   { return { _mm256_set_epi32(p0.x, p0.y, p1.x, p1.y,
                                                                                       p2_.x, p2_.y, p3_.x, p3_.y) };}
}



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



#endif //_INTRINSICS_H
