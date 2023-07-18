
#include "random.h"



//- Declarations

//~ Parameter struct

struct RandomParameters
{
    i32 exponent;
    
    i32 size128;
    i32 size64;
    i32 size32;
};



//- Preprocessor definitions

//~ Core exponents and their derivatives
// Mersenne prime exponents give periods of n * (2^EXP - 1).

#define MERSENNE_PARAMETERS(exp) \
RandomParameters \
{ \
exp, \
(exp / 128 + 1), \
(exp / 128 + 1) * 2, \
(exp / 128 + 1) * 4, \
}

#define E_DOWN_2 607
#define E_DOWN_1 2281
#define E_DEFAULT 19937
#define E_UP_1 86243
#define E_UP_2 216091

//~ Data

constexpr static RandomParameters presets[] =
{
    MERSENNE_PARAMETERS(E_DOWN_2),
    MERSENNE_PARAMETERS(E_DOWN_1),
    MERSENNE_PARAMETERS(E_DEFAULT),
    MERSENNE_PARAMETERS(E_UP_1),
    MERSENNE_PARAMETERS(E_UP_2)
};





namespace Tool
{
    //- Helper functions
    
    //~ Random getters
    
    void RandomCreate(Random* random, u64 seed)
    {
    }
    
    void RandomSeed(Random* random, u64 seed)
    {
    }
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    i8 RandomGetI8(Random* random, i8 max)
    {
        return 0;
    }
    
    i16 RandomGetI16(Random* random, i16 max)
    {
        return 0;
    }
    
    i32 RandomGetI32(Random* random, i32 max)
    {
        return 0;
    }
    
    i64 RandomGetI64(Random* random, i64 max)
    {
        return 0;
    }
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    i8 RandomGetI8(Random* random, i8 min, i8 max)
    {
        return 0;
    }
    
    i16 RandomGetI16(Random* random, i16 min, i16 max)
    {
        return 0;
    }
    
    i32 RandomGetI32(Random* random, i32 min, i32 max)
    {
        return 0;
    }
    
    i64 RandomGetI64(Random* random, i64 min, i64 max)
    {
        return 0;
    }
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    u8 RandomGetU8(Random* random, u8 max)
    {
        return 0;
    }
    
    u16 RandomGetU16(Random* random, u16 max)
    {
        return 0;
    }
    
    u32 RandomGetU32(Random* random, u32 max)
    {
        return 0;
    }
    
    u64 RandomGetU64(Random* random, u64 max)
    {
        return 0;
    }
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    u8 RandomGetU8(Random* random, u8 min, u8 max)
    {
        return 0;
    }
    
    u16 RandomGetU16(Random* random, u16 min, u16 max)
    {
        return 0;
    }
    
    u32 RandomGetU32(Random* random, u32 min, u32 max)
    {
        return 0;
    }
    
    u64 RandomGetU64(Random* random, u64 min, u64 max)
    {
        return 0;
    }
    
    // NOTE(crazy): Min inclusive, max inclusive. 
    f32 RandomGetF32(Random* random, f32 min, f32 max)
    {
        return 0.0f;
    }
    
    f64 RandomGetF64(Random* random, f64 min, f64 max)
    {
        return 0.0;
    }
    
    // NOTE(crazy): True or false. 
    b8 RandomGetB8(Random* random)
    {
        return false;
    }
    
    b32 RandomGetB32(Random* random)
    {
        return false;
    }
    
    //~ Random fillers
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    void RandomFillI8(Random* random, i8* dst, u64 count, i8 max)
    {
        
    }
    
    void RandomFillI16(Random* random, i16* dst, u64 count, i16 max)
    {
        
    }
    
    void RandomFillI32(Random* random, i32* dst, u64 count,i32 max)
    {
        
    }
    
    void RandomFillI64(Random* random, i64* dst, u64 count,i64 max)
    {
        
    }
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    void RandomFillI8(Random* random, i8* dst, u64 count, i8 min, i8 max)
    {
        
    }
    
    void RandomFillI16(Random* random, i16* dst, u64 count, i16 min, i16 max)
    {
        
    }
    
    void RandomFillI32(Random* random, i32* dst, u64 count, i32 min, i32 max)
    {
        
    }
    
    void RandomFillI64(Random* random, i64* dst, u64 count, i64 min, i64 max)
    {
        
    }
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    void RandomFillU8(Random* random, u8* dst, u64 count, u8 max)
    {
        
    }
    
    void RandomFillU16(Random* random, u16* dst, u64 count, u16 max)
    {
        
    }
    
    void RandomFillU32(Random* random, u32* dst, u64 count, u32 max)
    {
        
    }
    
    void RandomFillU64(Random* random, u64* dst, u64 count, u64 max)
    {
        
    }
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    void RandomFillU8(Random* random, u8* dst, u64 count, u8 min, u8 max)
    {
        
    }
    
    void RandomFillU16(Random* random, u16* dst, u64 count, u16 min, u16 max)
    {
        
    }
    
    void RandomFillU32(Random* random, u32* dst, u64 count, u32 min, u32 max)
    {
        
    }
    
    void RandomFillU64(Random* random, u64* dst, u64 count, u64 min, u64 max)
    {
        
    }
    
    // NOTE(crazy): Min inclusive, max inclusive. 
    void RandomFillF32(Random* random, f32* dst, u64 count, f32 min, f32 max)
    {
        
    }
    
    void RandomFillF64(Random* random, f64* dst, u64 count, f64 min, f64 max)
    {
        
    }
    
    // NOTE(crazy): True or false. 
    void RandomFillB8(Random* random, b8* dst, u64 count)
    {
        
    }
    
    void RandomFillB32(Random* random, b32* dst, u64 count)
    {
        
    }
    
}

