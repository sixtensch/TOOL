#ifndef _TOOL_RANDOM_H
#define _TOOL_RANDOM_H

#include "basics.h"
#include "error.h"
#include "hash.h"
#include "vector.h"



//~ Random numbers
//
// Two generators feed one set of distributions.
// - Random is a stream, xoshiro256** (Blackman and Vigna, public domain). Its state is 32 plain bytes, so it can sit in
//   persistent memory and be copied to snapshot or rewind. A zeroed Random is invalid: seed it with RandomInit.
// - RandomKey turns a seed and a key (a counter, an ID, or up to four signed coordinates) into 64 random bits, with no
//   state. Results don't depend on call order, so adding a draw somewhere never shifts any other. For one seed,
//   distinct keys of the same shape never give the same bits, and different seeds give unrelated results.
//
// Every distribution takes either a Random* or 64 bits from RandomKey. A Random* call consumes exactly one RandomNext,
// and a distribution that needs more than 64 bits stretches them deterministically. Integer ranges are min inclusive,
// max exclusive. Float ranges are [min, max), though rounding can land on max.
//
// Outputs are stable: seeded content depends on them, so changing an algorithm here is a breaking change. Integer
// results are identical on every platform. Float ranges are too, unless the compiler fuses multiply-adds across
// statements (GCC's default does on FMA targets). Gaussian and geometry results go through platform math functions
// and can differ in the last bits between platforms.



namespace Tool
{
    //- Generators

    //~ Mixer

    // Bijective 64-bit mixer, Pelle Evensen's nasam. Strong even on counter-like inputs, and distinct inputs never
    // collide. It maps 0 to 0.
    constexpr u64 RandomMix(u64 x)
    {
        x ^= ((x >> 25) | (x << 39)) ^ ((x >> 47) | (x << 17));
        x *= 0x9e6c63d0676a9a99ull;
        x ^= (x >> 23) ^ (x >> 51);
        x *= 0x9e6d62d06f6a9a9bull;
        x ^= (x >> 23) ^ (x >> 51);
        return x;
    }

    // The next 64 bits after 'bits', for distributions that need more than one word.
    constexpr u64 RandomStretch(u64 bits)
    {
        return RandomMix(bits + 0x9e3779b97f4a7c15ull);
    }

    //~ Keyed

    // The seed enters both rounds, so each seed is its own permutation of the keys rather than a shifted copy of
    // another seed's. Signed coordinates pack as their 32-bit patterns, two to a word.

    constexpr u64 RandomKey(u64 seed, u64 key)
    {
        u64 inner = RandomMix(key ^ seed ^ 0x9e3779b97f4a7c15ull);
        return RandomMix(inner ^ (seed * 0xd1b54a32d192ed03ull));
    }

    constexpr u64 RandomKey(u64 seed, i32 x, i32 y)
    {
        return RandomKey(seed, (u64)(u32)x | ((u64)(u32)y << 32));
    }

    constexpr u64 RandomKey(u64 seed, i32 x, i32 y, i32 z, i32 w)
    {
        u64 inner = RandomMix(((u64)(u32)x | ((u64)(u32)y << 32)) ^ seed ^ 0x9e3779b97f4a7c15ull);
        inner = RandomMix(inner ^ ((u64)(u32)z | ((u64)(u32)w << 32)));
        return RandomMix(inner ^ (seed * 0xd1b54a32d192ed03ull));
    }

    // Same as the four-coordinate form with w = 0.
    constexpr u64 RandomKey(u64 seed, i32 x, i32 y, i32 z)
    {
        return RandomKey(seed, x, y, z, 0);
    }

    inline u64 RandomKey(u64 seed, p2 point) { return RandomKey(seed, point.x, point.y); }
    inline u64 RandomKey(u64 seed, p3 point) { return RandomKey(seed, point.x, point.y, point.z); }
    inline u64 RandomKey(u64 seed, p4 point) { return RandomKey(seed, point.x, point.y, point.z, point.w); }

    //~ Stream

    struct Random
    {
        u64 state[4];
    };

    // Independent streams come from independent seeds, e.g. RandomKey(seed, streamID).
    inline void RandomInit(Random* rng, u64 seed)
    {
        for (u64 i = 0; i < 4; i++)
            rng->state[i] = RandomKey(seed, i);
    }

    inline u64 RandomNext(Random* rng)
    {
        u64* s = rng->state;
        TOOL_DEBUG_ASSERT((s[0] | s[1] | s[2] | s[3]) != 0, "Random used before RandomInit");

        u64 x = s[1] * 5;
        u64 result = ((x << 7) | (x >> 57)) * 9;
        u64 t = s[1] << 17;
        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = (s[3] << 45) | (s[3] >> 19);
        return result;
    }

    // Fills 'size' bytes, each RandomNext word in little-endian byte order.
    void RandomFill(Random* rng, void* dst, u64 size);



    //- Distributions

    //~ Full-width integers

    inline u8  RandomU8(u64 bits)  { return (u8)(bits >> 56); }
    inline u16 RandomU16(u64 bits) { return (u16)(bits >> 48); }
    inline u32 RandomU32(u64 bits) { return (u32)(bits >> 32); }
    inline u64 RandomU64(u64 bits) { return bits; }
    inline i8  RandomI8(u64 bits)  { return (i8)(bits >> 56); }
    inline i16 RandomI16(u64 bits) { return (i16)(bits >> 48); }
    inline i32 RandomI32(u64 bits) { return (i32)(bits >> 32); }
    inline i64 RandomI64(u64 bits) { return (i64)bits; }

    inline u8  RandomU8(Random* rng)  { return RandomU8(RandomNext(rng)); }
    inline u16 RandomU16(Random* rng) { return RandomU16(RandomNext(rng)); }
    inline u32 RandomU32(Random* rng) { return RandomU32(RandomNext(rng)); }
    inline u64 RandomU64(Random* rng) { return RandomU64(RandomNext(rng)); }
    inline i8  RandomI8(Random* rng)  { return RandomI8(RandomNext(rng)); }
    inline i16 RandomI16(Random* rng) { return RandomI16(RandomNext(rng)); }
    inline i32 RandomI32(Random* rng) { return RandomI32(RandomNext(rng)); }
    inline i64 RandomI64(Random* rng) { return RandomI64(RandomNext(rng)); }

    //~ Integer ranges

    // Ranges up to 32 bits scale all 64 bits by the range (Lemire's multiply, without the rejection step), so the
    // bias is at most 2^-32. 64-bit ranges reject and stretch, which makes them exact.

    inline u32 RandomRangeU32(u64 bits, u32 min, u32 max)
    {
        TOOL_DEBUG_ASSERT(min < max);
        u64 range = (u32)(max - min);
        u64 high = (bits >> 32) * range;
        u64 low = (bits & 0xffffffffull) * range;
        return min + (u32)((high + (low >> 32)) >> 32);
    }

    inline u64 RandomRangeU64(u64 bits, u64 min, u64 max)
    {
        TOOL_DEBUG_ASSERT(min < max);
        u64 range = max - min;
        u64 low = bits;
        u64 high = range;
        HashRapidMultiply(&low, &high);
        if (low < range)
        {
            u64 threshold = (0 - range) % range;
            while (low < threshold)
            {
                bits = RandomStretch(bits);
                low = bits;
                high = range;
                HashRapidMultiply(&low, &high);
            }
        }
        return min + high;
    }

    // Signed ranges offset an unsigned range of the same width, so any min < max works without overflow.
    inline i32 RandomRangeI32(u64 bits, i32 min, i32 max)
    {
        TOOL_DEBUG_ASSERT(min < max);
        return (i32)((u32)min + RandomRangeU32(bits, 0, (u32)max - (u32)min));
    }

    inline i64 RandomRangeI64(u64 bits, i64 min, i64 max)
    {
        TOOL_DEBUG_ASSERT(min < max);
        return (i64)((u64)min + RandomRangeU64(bits, 0, (u64)max - (u64)min));
    }

    inline u8  RandomRangeU8(u64 bits, u8 min, u8 max)    { return (u8)RandomRangeU32(bits, min, max); }
    inline u16 RandomRangeU16(u64 bits, u16 min, u16 max) { return (u16)RandomRangeU32(bits, min, max); }
    inline i8  RandomRangeI8(u64 bits, i8 min, i8 max)    { return (i8)RandomRangeI32(bits, min, max); }
    inline i16 RandomRangeI16(u64 bits, i16 min, i16 max) { return (i16)RandomRangeI32(bits, min, max); }

    inline u8  RandomRangeU8(Random* rng, u8 min, u8 max)    { return RandomRangeU8(RandomNext(rng), min, max); }
    inline u16 RandomRangeU16(Random* rng, u16 min, u16 max) { return RandomRangeU16(RandomNext(rng), min, max); }
    inline u32 RandomRangeU32(Random* rng, u32 min, u32 max) { return RandomRangeU32(RandomNext(rng), min, max); }
    inline u64 RandomRangeU64(Random* rng, u64 min, u64 max) { return RandomRangeU64(RandomNext(rng), min, max); }
    inline i8  RandomRangeI8(Random* rng, i8 min, i8 max)    { return RandomRangeI8(RandomNext(rng), min, max); }
    inline i16 RandomRangeI16(Random* rng, i16 min, i16 max) { return RandomRangeI16(RandomNext(rng), min, max); }
    inline i32 RandomRangeI32(Random* rng, i32 min, i32 max) { return RandomRangeI32(RandomNext(rng), min, max); }
    inline i64 RandomRangeI64(Random* rng, i64 min, i64 max) { return RandomRangeI64(RandomNext(rng), min, max); }

    //~ Floats

    // [0, 1), from the top 24 or 53 bits, so every value is exact and equally spaced.
    inline f32 RandomF32(u64 bits) { return (f32)(bits >> 40) * 0x1p-24f; }
    inline f64 RandomF64(u64 bits) { return (f64)(bits >> 11) * 0x1p-53; }

    // Scale and offset are separate statements, which Clang and MSVC never fuse into an FMA.
    inline f32 RandomF32(u64 bits, f32 min, f32 max)
    {
        f32 scaled = (max - min) * RandomF32(bits);
        return min + scaled;
    }

    inline f64 RandomF64(u64 bits, f64 min, f64 max)
    {
        f64 scaled = (max - min) * RandomF64(bits);
        return min + scaled;
    }

    inline f32 RandomF32(Random* rng)                   { return RandomF32(RandomNext(rng)); }
    inline f64 RandomF64(Random* rng)                   { return RandomF64(RandomNext(rng)); }
    inline f32 RandomF32(Random* rng, f32 min, f32 max) { return RandomF32(RandomNext(rng), min, max); }
    inline f64 RandomF64(Random* rng, f64 min, f64 max) { return RandomF64(RandomNext(rng), min, max); }

    //~ Booleans

    inline b8  RandomB8(u64 bits)  { return (b8)(bits >> 63); }
    inline b32 RandomB32(u64 bits) { return (b32)(bits >> 63); }

    inline b8  RandomB8(Random* rng)  { return RandomB8(RandomNext(rng)); }
    inline b32 RandomB32(Random* rng) { return RandomB32(RandomNext(rng)); }

    //~ Gaussian

    // Normal distribution by Box-Muller. Tails end near 5.8 deviations for f32 and 8.5 for f64.
    f32 RandomGaussianF32(u64 bits, f32 mean, f32 deviation);
    f64 RandomGaussianF64(u64 bits, f64 mean, f64 deviation);

    inline f32 RandomGaussianF32(Random* rng, f32 mean, f32 deviation) { return RandomGaussianF32(RandomNext(rng), mean, deviation); }
    inline f64 RandomGaussianF64(Random* rng, f64 mean, f64 deviation) { return RandomGaussianF64(RandomNext(rng), mean, deviation); }

    //~ Geometry

    // Uniform over the unit circle or sphere (On), or the area or volume inside it (In).
    v2 RandomOnCircle(u64 bits);
    v2 RandomInCircle(u64 bits);
    v3 RandomOnSphere(u64 bits);
    v3 RandomInSphere(u64 bits);

    inline v2 RandomOnCircle(Random* rng) { return RandomOnCircle(RandomNext(rng)); }
    inline v2 RandomInCircle(Random* rng) { return RandomInCircle(RandomNext(rng)); }
    inline v3 RandomOnSphere(Random* rng) { return RandomOnSphere(RandomNext(rng)); }
    inline v3 RandomInSphere(Random* rng) { return RandomInSphere(RandomNext(rng)); }

    //~ Shuffle

    // Fisher-Yates. The Random* form draws once per swap, the bits form stretches once per swap.

    template<typename T>
    inline void RandomShuffle(Random* rng, T* items, u64 count)
    {
        for (u64 i = count; i > 1; i--)
        {
            u64 j = RandomRangeU64(RandomNext(rng), 0, i);
            T swap = items[i - 1];
            items[i - 1] = items[j];
            items[j] = swap;
        }
    }

    template<typename T>
    inline void RandomShuffle(u64 bits, T* items, u64 count)
    {
        for (u64 i = count; i > 1; i--)
        {
            u64 j = RandomRangeU64(bits, 0, i);
            T swap = items[i - 1];
            items[i - 1] = items[j];
            items[j] = swap;
            bits = RandomStretch(bits);
        }
    }
}

#endif
