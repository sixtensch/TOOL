#include "random.h"

#include <cmath>



namespace Tool
{
    //- Generators

    //~ Stream

    void RandomFill(Random* rng, void* dst, u64 size)
    {
        u8* bytes = (u8*)dst;
        while (size > 0)
        {
            u64 bits = RandomNext(rng);
            u64 count = size < 8 ? size : 8;
            for (u64 i = 0; i < count; i++)
                bytes[i] = (u8)(bits >> (i * 8));
            bytes += count;
            size -= count;
        }
    }



    //- Distributions

    //~ Gaussian

    // Box-Muller: the radius term takes u in (0, 1], so its log is finite.

    f32 RandomGaussianF32(u64 bits, f32 mean, f32 deviation)
    {
        f32 u = (f32)((bits >> 40) + 1) * 0x1p-24f;
        f32 angle = (f32)(bits & 0xffffffull) * 0x1p-24f * 6.28318530718f;
        f32 radius = std::sqrt(-2.0f * std::log(u));
        f32 scaled = deviation * radius * std::cos(angle);
        return mean + scaled;
    }

    f64 RandomGaussianF64(u64 bits, f64 mean, f64 deviation)
    {
        f64 u = (f64)((bits >> 11) + 1) * 0x1p-53;
        f64 angle = RandomF64(RandomStretch(bits)) * 6.283185307179586;
        f64 radius = std::sqrt(-2.0 * std::log(u));
        f64 scaled = deviation * radius * std::cos(angle);
        return mean + scaled;
    }

    //~ Geometry

    // Each takes two independent 24-bit uniforms from the top and bottom of the word.

    v2 RandomOnCircle(u64 bits)
    {
        f32 angle = RandomF32(bits) * 6.28318530718f;
        return v2 { std::cos(angle), std::sin(angle) };
    }

    v2 RandomInCircle(u64 bits)
    {
        f32 radius = std::sqrt(RandomF32(bits));
        f32 angle = (f32)(bits & 0xffffffull) * 0x1p-24f * 6.28318530718f;
        return v2 { radius * std::cos(angle), radius * std::sin(angle) };
    }

    v3 RandomOnSphere(u64 bits)
    {
        f32 z = 1.0f - 2.0f * RandomF32(bits);
        f32 angle = (f32)(bits & 0xffffffull) * 0x1p-24f * 6.28318530718f;
        f32 squared = 1.0f - z * z;
        f32 radius = squared > 0.0f ? std::sqrt(squared) : 0.0f;
        return v3 { radius * std::cos(angle), radius * std::sin(angle), z };
    }

    v3 RandomInSphere(u64 bits)
    {
        v3 direction = RandomOnSphere(bits);
        f32 radius = std::cbrt(RandomF32(RandomStretch(bits)));
        return v3 { direction.x * radius, direction.y * radius, direction.z * radius };
    }
}
