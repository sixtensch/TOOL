#ifndef _TOOL_HASH_H
#define _TOOL_HASH_H

#include "basics.h"
#include "text.h"

#include <string.h>

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



//~ Hash functions
//
// Non-cryptographic 64-bit hashes, named by algorithm so a name always means the same output. Every function is
// constexpr: called in a constant expression it runs at compile time, otherwise it takes the fast runtime path.
// The ConstHash variants are consteval and force a string literal to hash at compile time.
//
// Integer overloads hash the value's 8 little-endian bytes, matching the byte overloads on the same bytes.
// Outputs are stable for a given algorithm, but nothing here is versioned: persist a hash only where the
// algorithm is pinned on purpose.



namespace Tool
{
    //- FNV-1a 64

    // One byte per step through a multiply chain. Tiny, but slow on long inputs. The low bits of the result only
    // depend on the low bits of the input bytes, so reduce it by its high bits, as HashMap does.

    constexpr u64 HashFnv1a(const c8* data, u64 size)
    {
        u64 hash = 0xcbf29ce484222325ull;
        for (u64 i = 0; i < size; i++)
        {
            hash ^= (u8)data[i];
            hash *= 0x100000001b3ull;
        }
        return hash;
    }

    constexpr u64 HashFnv1a(s8 string)
    {
        return HashFnv1a(string.str, string.size);
    }

    constexpr u64 HashFnv1a(u64 value)
    {
        u64 hash = 0xcbf29ce484222325ull;
        for (u32 i = 0; i < 8; i++)
        {
            hash ^= (value >> (i * 8)) & 0xff;
            hash *= 0x100000001b3ull;
        }
        return hash;
    }

    inline u64 HashFnv1a(const void* data, u64 size)
    {
        return HashFnv1a((const c8*)data, size);
    }



    //- rapidhash V3
    //
    // Up to 16 bytes in one multiply, 112 bytes per step beyond. Port of the default (compact, fast) variant
    // with seed 0, from https://github.com/Nicoshev/rapidhash:
    //
    //   rapidhash V3 - Very fast, high quality, platform-independent hashing algorithm.
    //   Based on 'wyhash', by Wang Yi <godspeed_china@yeah.net>
    //   Copyright (C) 2025 Nicolas De Carli
    //
    //   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
    //   associated documentation files (the "Software"), to deal in the Software without restriction, including
    //   without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    //   copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
    //   following conditions:
    //
    //   The above copyright notice and this permission notice shall be included in all copies or substantial
    //   portions of the Software.
    //
    //   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
    //   LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
    //   EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    //   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
    //   THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    //~ Building blocks

    // 64x64 -> 128-bit multiply: low half into 'a', high half into 'b'.
    // Operands are loaded into locals first: MSVC 19.51 crashes constant-evaluating a shift of '*a' directly.
    constexpr void HashRapidMultiply(u64* a, u64* b)
    {
        u64 x = *a;
        u64 y = *b;

#if defined(__SIZEOF_INT128__)
        unsigned __int128 product = (unsigned __int128)x * y;
        *a = (u64)product;
        *b = (u64)(product >> 64);
#else
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
        if (!__builtin_is_constant_evaluated())
        {
#if defined(_M_X64)
            u64 high;
            *a = _umul128(x, y, &high);
            *b = high;
#else
            *a = x * y;
            *b = __umulh(x, y);
#endif
            return;
        }
#endif
        u64 ha = x >> 32;
        u64 hb = y >> 32;
        u64 la = x & 0xffffffffull;
        u64 lb = y & 0xffffffffull;
        u64 rh = ha * hb;
        u64 rm0 = ha * lb;
        u64 rm1 = hb * la;
        u64 rl = la * lb;
        u64 t = rl + (rm0 << 32);
        u64 carry = (t < rl) ? 1 : 0;
        u64 low = t + (rm1 << 32);
        carry += (low < t) ? 1 : 0;
        *a = low;
        *b = rh + (rm0 >> 32) + (rm1 >> 32) + carry;
#endif
    }

    constexpr u64 HashRapidMix(u64 a, u64 b)
    {
        HashRapidMultiply(&a, &b);
        return a ^ b;
    }

    // Little-endian reads. Byte by byte at compile time, one unaligned load at runtime.
    constexpr u64 HashRapidRead64(const c8* p)
    {
        if (__builtin_is_constant_evaluated())
        {
            u64 value = 0;
            for (u32 i = 0; i < 8; i++)
                value |= (u64)(u8)p[i] << (i * 8);
            return value;
        }

        u64 value;
        memcpy(&value, p, 8);
        return value;
    }

    constexpr u64 HashRapidRead32(const c8* p)
    {
        if (__builtin_is_constant_evaluated())
        {
            u64 value = 0;
            for (u32 i = 0; i < 4; i++)
                value |= (u64)(u8)p[i] << (i * 8);
            return value;
        }

        u32 value;
        memcpy(&value, p, 4);
        return value;
    }

    inline constexpr u64 hashRapidSecret[8] = {
        0x2d358dccaa6c78a5ull,
        0x8bb84b93962eacc9ull,
        0x4b33a62ed433d4a3ull,
        0x4d5a2da51de1aa47ull,
        0xa0761d6478bd642full,
        0xe7037ed1a0b428dbull,
        0x90ed1765281c388cull,
        0xaaaaaaaaaaaaaaaaull,
    };

    //~ Hash

    constexpr u64 HashRapid(const c8* data, u64 size)
    {
        const u64* secret = hashRapidSecret;
        const c8* p = data;

        u64 seed = HashRapidMix(secret[2], secret[1]);
        u64 a = 0;
        u64 b = 0;
        u64 i = size;

        if (size <= 16)
        {
            if (size >= 4)
            {
                seed ^= size;
                if (size >= 8)
                {
                    a = HashRapidRead64(p);
                    b = HashRapidRead64(p + size - 8);
                }
                else
                {
                    a = HashRapidRead32(p);
                    b = HashRapidRead32(p + size - 4);
                }
            }
            else if (size > 0)
            {
                a = ((u64)(u8)p[0] << 45) | (u8)p[size - 1];
                b = (u8)p[size >> 1];
            }
        }
        else
        {
            if (size > 112)
            {
                u64 see1 = seed, see2 = seed, see3 = seed, see4 = seed, see5 = seed, see6 = seed;
                do
                {
                    seed = HashRapidMix(HashRapidRead64(p) ^ secret[0], HashRapidRead64(p + 8) ^ seed);
                    see1 = HashRapidMix(HashRapidRead64(p + 16) ^ secret[1], HashRapidRead64(p + 24) ^ see1);
                    see2 = HashRapidMix(HashRapidRead64(p + 32) ^ secret[2], HashRapidRead64(p + 40) ^ see2);
                    see3 = HashRapidMix(HashRapidRead64(p + 48) ^ secret[3], HashRapidRead64(p + 56) ^ see3);
                    see4 = HashRapidMix(HashRapidRead64(p + 64) ^ secret[4], HashRapidRead64(p + 72) ^ see4);
                    see5 = HashRapidMix(HashRapidRead64(p + 80) ^ secret[5], HashRapidRead64(p + 88) ^ see5);
                    see6 = HashRapidMix(HashRapidRead64(p + 96) ^ secret[6], HashRapidRead64(p + 104) ^ see6);
                    p += 112;
                    i -= 112;
                } while (i > 112);

                seed ^= see1;
                see2 ^= see3;
                see4 ^= see5;
                seed ^= see6;
                see2 ^= see4;
                seed ^= see2;
            }

            if (i > 16)
            {
                seed = HashRapidMix(HashRapidRead64(p) ^ secret[2], HashRapidRead64(p + 8) ^ seed);
                if (i > 32)
                {
                    seed = HashRapidMix(HashRapidRead64(p + 16) ^ secret[2], HashRapidRead64(p + 24) ^ seed);
                    if (i > 48)
                    {
                        seed = HashRapidMix(HashRapidRead64(p + 32) ^ secret[1], HashRapidRead64(p + 40) ^ seed);
                        if (i > 64)
                        {
                            seed = HashRapidMix(HashRapidRead64(p + 48) ^ secret[1], HashRapidRead64(p + 56) ^ seed);
                            if (i > 80)
                            {
                                seed = HashRapidMix(HashRapidRead64(p + 64) ^ secret[2], HashRapidRead64(p + 72) ^ seed);
                                if (i > 96)
                                    seed = HashRapidMix(HashRapidRead64(p + 80) ^ secret[1], HashRapidRead64(p + 88) ^ seed);
                            }
                        }
                    }
                }
            }

            a = HashRapidRead64(p + i - 16) ^ i;
            b = HashRapidRead64(p + i - 8);
        }

        a ^= secret[1];
        b ^= seed;
        HashRapidMultiply(&a, &b);
        return HashRapidMix(a ^ secret[7], b ^ secret[1] ^ i);
    }

    constexpr u64 HashRapid(s8 string)
    {
        return HashRapid(string.str, string.size);
    }

    // The 8-byte path of HashRapid, with the value as both reads.
    constexpr u64 HashRapid(u64 value)
    {
        const u64* secret = hashRapidSecret;
        u64 seed = HashRapidMix(secret[2], secret[1]) ^ 8;

        u64 a = value ^ secret[1];
        u64 b = value ^ seed;
        HashRapidMultiply(&a, &b);
        return HashRapidMix(a ^ secret[7], b ^ secret[1] ^ 8);
    }

    inline u64 HashRapid(const void* data, u64 size)
    {
        return HashRapid((const c8*)data, size);
    }



    //- Compile-time string hashes

    // Null-terminated literal, terminator excluded. Equal to the runtime hash of the same bytes.
    consteval u64 ConstHashFnv1a(const c8* string)
    {
        u64 size = 0;
        while (string[size] != '\0')
            size++;
        return HashFnv1a(string, size);
    }

    consteval u64 ConstHashRapid(const c8* string)
    {
        u64 size = 0;
        while (string[size] != '\0')
            size++;
        return HashRapid(string, size);
    }
}



#endif //_TOOL_HASH_H
