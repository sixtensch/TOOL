#include "random.h"

#include <cmath>



namespace Tool
{
    //~ Collections

    void RandomFillNext(RandomStream* stream, void* dst, u64 size)
    {
        u8* bytes = (u8*)dst;
        while (size > 0)
        {
            u64 bits = RandomU64Next(stream);
            u64 count = size < 8 ? size : 8;
            for (u64 i = 0; i < count; i++)
                bytes[i] = (u8)(bits >> (i * 8));
            bytes += count;
            size -= count;
        }
    }


    //~ Gaussian

    // Box-Muller: the radius term takes u in (0, 1], so its log is finite.

    f64 RandomBitsGaussianF64(u64 bits, f64 mean, f64 deviation)
    {
        f64 u = (f64)((bits >> 11) + 1) * 0x1p-53;
        f64 angle = RandomBitsF64(RandomStretch(bits)) * 6.283185307179586;
        f64 radius = std::sqrt(-2.0 * std::log(u));
        f64 scaled = deviation * radius * std::cos(angle);
        return mean + scaled;
    }

    f32 RandomBitsGaussianF32(u64 bits, f32 mean, f32 deviation)
    {
        f32 u = (f32)((bits >> 40) + 1) * 0x1p-24f;
        f32 angle = (f32)(bits & 0xffffffull) * 0x1p-24f * 6.28318530718f;
        f32 radius = std::sqrt(-2.0f * std::log(u));
        f32 scaled = deviation * radius * std::cos(angle);
        return mean + scaled;
    }

    //~ Geometry

    // Each takes two independent 24-bit uniforms from the top and bottom of the word.

    v2 RandomBitsOnCircle(u64 bits)
    {
        f32 angle = RandomBitsF32(bits) * 6.28318530718f;
        return v2 { std::cos(angle), std::sin(angle) };
    }

    v2 RandomBitsInCircle(u64 bits)
    {
        f32 radius = std::sqrt(RandomBitsF32(bits));
        f32 angle = (f32)(bits & 0xffffffull) * 0x1p-24f * 6.28318530718f;
        return v2 { radius * std::cos(angle), radius * std::sin(angle) };
    }

    v3 RandomBitsOnSphere(u64 bits)
    {
        f32 z = 1.0f - 2.0f * RandomBitsF32(bits);
        f32 angle = (f32)(bits & 0xffffffull) * 0x1p-24f * 6.28318530718f;
        f32 squared = 1.0f - z * z;
        f32 radius = squared > 0.0f ? std::sqrt(squared) : 0.0f;
        return v3 { radius * std::cos(angle), radius * std::sin(angle), z };
    }

    v3 RandomBitsInSphere(u64 bits)
    {
        v3 direction = RandomBitsOnSphere(bits);
        f32 radius = std::cbrt(RandomBitsF32(RandomStretch(bits)));
        return v3 { direction.x * radius, direction.y * radius, direction.z * radius };
    }

    //~ Spread

    // Base 2 is a bit reversal, and its digit shift is an XOR mask.
    static f32 RandomSpreadBase2(u64 index, u64 shifts)
    {
        u32 x = (u32)index;
        x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
        x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
        x = ((x >> 4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) << 4);
        x = ((x >> 8) & 0x00ff00ffu) | ((x & 0x00ff00ffu) << 8);
        x = (x >> 16) | (x << 16);
        x ^= (u32)shifts;
        return (f32)(x >> 8) * 0x1p-24f;
    }

    // Index digits come off least significant first and are appended to the radical, so the lowest index digit ends
    // up most significant. 'digits' is the fewest with base^digits >= 2^24. Each shift is the next base-b digit of
    // 'shifts' read as a fraction, so all shift values are equally likely.
    static f32 RandomSpreadBase(u64 index, u64 shifts, u32 base, u32 digits)
    {
        u64 radical = 0;
        u64 scale = 1;
        for (u32 i = 0; i < digits; i++)
        {
            u64 high = (shifts >> 32) * base;
            u64 low = (shifts & 0xffffffffull) * base;
            u64 shift = (high + (low >> 32)) >> 32;
            shifts *= base;

            u64 digit = (index % base + shift) % base;
            index /= base;
            radical = radical * base + digit;
            scale *= base;
        }
        // Rounded up, so points on a 1/b^k boundary stay in their own interval, but kept below 1.
        u64 rounded = ((radical << 24) + scale - 1) / scale;
        return (f32)(rounded < 0xffffffull ? rounded : 0xffffffull) * 0x1p-24f;
    }

    // Each axis draws its shifts from its own word. The constant keeps them apart from keyed outputs of the seed.
    void RandomSpreadInit(RandomSpread* spread, u64 seed)
    {
        spread->shifts[0] = RandomMix(seed ^ 0x5d588b656c078965ull);
        spread->shifts[1] = RandomStretch(spread->shifts[0]);
        spread->shifts[2] = RandomStretch(spread->shifts[1]);
    }

    f32 RandomF32At(const RandomSpread* spread, u64 index)
    {
        return RandomSpreadBase2(index, spread->shifts[0]);
    }

    v2 RandomInSquareAt(const RandomSpread* spread, u64 index)
    {
        return v2 { RandomSpreadBase2(index, spread->shifts[0]), RandomSpreadBase(index, spread->shifts[1], 3, 16) };
    }

    v3 RandomInCubeAt(const RandomSpread* spread, u64 index)
    {
        return v3 { RandomSpreadBase2(index, spread->shifts[0]), RandomSpreadBase(index, spread->shifts[1], 3, 16), RandomSpreadBase(index, spread->shifts[2], 5, 11) };
    }
}
