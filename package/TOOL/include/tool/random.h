#ifndef _TOOL_RANDOM_H
#define _TOOL_RANDOM_H

#include "basics.h"

namespace Tool
{
    //- Struct definitions
    
    //~ Random
    
    enum RandomMagnitude
    {
        // Defines @state width of 607 bits (76 bytes, 19 128-bit integers).
        // This gives a period of >5.3E+180
        RandomMagnitudeDown2,
        
        // Defines @state width of 2281 bits (160 bytes, 40 integers).
        RandomMagnitudeDown1,
        
        // Defines @state width of 19937 bits (624 bytes).
        RandomMagnitudeStandard,
        
        // Defines @state width of 86243 bits (.
        RandomMagnitudeUp1,
        
        // Defines @state width of 216091 bits.
        RandomMagnitudeUp2
    };
    
    struct Random
    {
        RandomMagnitude magnitude;
        
        u32* state;
        u64 seed;
    };
    
    
    
    //- Helper functions
    
    //~ Random getters
    
    void RandomCreate(Random* random, u64 seed = 0);
    void RandomSeed(Random* random, u64 seed = 0);
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    i8 RandomGetI8(Random* random, i8 max);
    i16 RandomGetI16(Random* random, i16 max);
    i32 RandomGetI32(Random* random, i32 max);
    i64 RandomGetI64(Random* random, i64 max);
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    i8 RandomGetI8(Random* random, i8 min, i8 max);
    i16 RandomGetI16(Random* random, i16 min, i16 max);
    i32 RandomGetI32(Random* random, i32 min, i32 max);
    i64 RandomGetI64(Random* random, i64 min, i64 max);
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    u8 RandomGetU8(Random* random, u8 max);
    u16 RandomGetU16(Random* random, u16 max);
    u32 RandomGetU32(Random* random, u32 max);
    u64 RandomGetU64(Random* random, u64 max);
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    u8 RandomGetU8(Random* random, u8 min, u8 max);
    u16 RandomGetU16(Random* random, u16 min, u16 max);
    u32 RandomGetU32(Random* random, u32 min, u32 max);
    u64 RandomGetU64(Random* random, u64 min, u64 max);
    
    // NOTE(crazy): Min inclusive, max inclusive. 
    f32 RandomGetF32(Random* random, f32 min, f32 max);
    f64 RandomGetF64(Random* random, f64 min, f64 max);
    
    // NOTE(crazy): True or false. 
    b8 RandomGetB8(Random* random);
    b32 RandomGetB32(Random* random);
    
    //~ Random fillers
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    void RandomFillI8(Random* random, i8* dst, u64 count, i8 max);
    void RandomFillI16(Random* random, i16* dst, u64 count, i16 max);
    void RandomFillI32(Random* random, i32* dst, u64 count, i32 max);
    void RandomFillI64(Random* random, i64* dst, u64 count, i64 max);
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    void RandomFillI8(Random* random, i8* dst, u64 count, i8 min, i8 max);
    void RandomFillI16(Random* random, i16* dst, u64 count, i16 min, i16 max);
    void RandomFillI32(Random* random, i32* dst, u64 count, i32 min, i32 max);
    void RandomFillI64(Random* random, i64* dst, u64 count, i64 min, i64 max);
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    void RandomFillU8(Random* random, u8* dst, u64 count, u8 max);
    void RandomFillU16(Random* random, u16* dst, u64 count, u16 max);
    void RandomFillU32(Random* random, u32* dst, u64 count, u32 max);
    void RandomFillU64(Random* random, u64* dst, u64 count, u64 max);
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    void RandomFillU8(Random* random, u8* dst, u64 count, u8 min, u8 max);
    void RandomFillU16(Random* random, u16* dst, u64 count, u16 min, u16 max);
    void RandomFillU32(Random* random, u32* dst, u64 count, u32 min, u32 max);
    void RandomFillU64(Random* random, u64* dst, u64 count, u64 min, u64 max);
    
    // NOTE(crazy): Min inclusive, max inclusive. 
    void RandomFillF32(Random* random, f32* dst, u64 count, f32 min, f32 max);
    void RandomFillF64(Random* random, f64* dst, u64 count, f64 min, f64 max);
    
    // NOTE(crazy): True or false. 
    void RandomFillB8(Random* random, b8* dst, u64 count);
    void RandomFillB32(Random* random, b32* dst, u64 count);
    
}

#endif //_RANDOM_H
