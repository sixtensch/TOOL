#ifndef _RANDOM_H
#define _RANDOM_H

namespace Tool
{
    //- Struct definitions
    
    //~ Random
    
    struct EngineRandom
    {
        void* data;
        u64 seed;
    };
    
    
    
    //- Helper functions
    
    //~ Random getters
    
    void EngineRandomCreate(EngineRandom* random, u64 seed = 0);
    void EngineRandomSeed(EngineRandom* random, u64 seed = 0);
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    i8 EngineRandomGetI8(EngineRandom* random, i8 max);
    i16 EngineRandomGetI16(EngineRandom* random, i16 max);
    i32 EngineRandomGetI32(EngineRandom* random, i32 max);
    i64 EngineRandomGetI64(EngineRandom* random, i64 max);
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    i8 EngineRandomGetI8(EngineRandom* random, i8 min, i8 max);
    i16 EngineRandomGetI16(EngineRandom* random, i16 min, i16 max);
    i32 EngineRandomGetI32(EngineRandom* random, i32 min, i32 max);
    i64 EngineRandomGetI64(EngineRandom* random, i64 min, i64 max);
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    u8 EngineRandomGetU8(EngineRandom* random, u8 max);
    u16 EngineRandomGetU16(EngineRandom* random, u16 max);
    u32 EngineRandomGetU32(EngineRandom* random, u32 max);
    u64 EngineRandomGetU64(EngineRandom* random, u64 max);
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    u8 EngineRandomGetU8(EngineRandom* random, u8 min, u8 max);
    u16 EngineRandomGetU16(EngineRandom* random, u16 min, u16 max);
    u32 EngineRandomGetU32(EngineRandom* random, u32 min, u32 max);
    u64 EngineRandomGetU64(EngineRandom* random, u64 min, u64 max);
    
    // NOTE(crazy): Min inclusive, max inclusive. 
    f32 EngineRandomGetF32(EngineRandom* random, f32 min, f32 max);
    f64 EngineRandomGetF64(EngineRandom* random, f64 min, f64 max);
    
    // NOTE(crazy): True or false. 
    b8 EngineRandomGetB8(EngineRandom* random);
    b32 EngineRandomGetB32(EngineRandom* random);
    
    //~ Random fillers
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    i8 EngineRandomFillI8(EngineRandom* random, i8* dst, u64 count, i8 max);
    i16 EngineRandomFillI16(EngineRandom* random, i16* dst, u64 count, i16 max);
    i32 EngineRandomFillI32(EngineRandom* random, i32* dst, u64 count,i32 max);
    i64 EngineRandomFillI64(EngineRandom* random, i64* dst, u64 count,i64 max);
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    i8 EngineRandomFillI8(EngineRandom* random, i8* dst, u64 count, i8 min, i8 max);
    i16 EngineRandomFillI16(EngineRandom* random, i16* dst, u64 count, i16 min, i16 max);
    i32 EngineRandomFillI32(EngineRandom* random, i32* dst, u64 count, i32 min, i32 max);
    i64 EngineRandomFillI64(EngineRandom* random, i64* dst, u64 count, i64 min, i64 max);
    
    // NOTE(crazy): Min is 0 (inclusive), max exclusive.
    u8 EngineRandomFillU8(EngineRandom* random, u8* dst, u64 count, u8 max);
    u16 EngineRandomFillU16(EngineRandom* random, u16* dst, u64 count, u16 max);
    u32 EngineRandomFillU32(EngineRandom* random, u32* dst, u64 count, u32 max);
    u64 EngineRandomFillU64(EngineRandom* random, u64* dst, u64 count, u64 max);
    
    // NOTE(crazy): Min inclusive, max exclusive. 
    u8 EngineRandomFillU8(EngineRandom* random, u8* dst, u64 count, u8 min, u8 max);
    u16 EngineRandomFillU16(EngineRandom* random, u16* dst, u64 count, u16 min, u16 max);
    u32 EngineRandomFillU32(EngineRandom* random, u32* dst, u64 count, u32 min, u32 max);
    u64 EngineRandomFillU64(EngineRandom* random, u64* dst, u64 count, u64 min, u64 max);
    
    // NOTE(crazy): Min inclusive, max inclusive. 
    f32 EngineRandomFillF32(EngineRandom* random, f32* dst, u64 count, f32 min, f32 max);
    f64 EngineRandomFillF64(EngineRandom* random, f64* dst, u64 count, f64 min, f64 max);
    
    // NOTE(crazy): True or false. 
    b8 EngineRandomFillB8(EngineRandom* random, b8* dst, u64 count);
    b32 EngineRandomFillB32(EngineRandom* random, b32* dst, u64 count);
    
}

#endif //_RANDOM_H
