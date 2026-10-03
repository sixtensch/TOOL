#ifndef _TOOL_HASH_H
#define _TOOL_HASH_H

#include "basics.h"
#include "mathematics.h"
#include "text.h"



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

	constexpr u64 HashFnv1a(const c8* data, u64 size);
	constexpr u64 HashFnv1a(s8 string);
	constexpr u64 HashFnv1a(u64 value);
	inline u64 HashFnv1a(const void* data, u64 size);

	//- rapidhash V3

	// Up to 16 bytes in one multiply, 112 bytes per step beyond. A port of the default variant with seed 0, under
	// the license in the implementation below.

	constexpr u64 HashRapid(const c8* data, u64 size);
	constexpr u64 HashRapid(s8 string);
	constexpr u64 HashRapid(u64 value);
	inline u64 HashRapid(const void* data, u64 size);

	//- Compile-time string hashes

	// Null-terminated literal, terminator excluded. Equal to the runtime hash of the same bytes.
	consteval u64 ConstHashFnv1a(const c8* string);
	consteval u64 ConstHashRapid(const c8* string);
} //namespace Tool



//- Implementation

namespace Tool
{
	//~ FNV-1a 64

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



	//~ rapidhash V3
	//
	// From https://github.com/Nicoshev/rapidhash:
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

	constexpr u64 HashRapidMix(u64 a, u64 b)
	{
		a = U64MulWide(a, b, &b);
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

#if defined(__clang__) || defined(__GNUC__)
		u64 value;
		__builtin_memcpy(&value, p, 8);
		return value;
#else
		// MSVC does no type-based alias analysis, and its targets allow unaligned loads.
		return *(const u64*)p;
#endif
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

#if defined(__clang__) || defined(__GNUC__)
		u32 value;
		__builtin_memcpy(&value, p, 4);
		return value;
#else
		return *(const u32*)p;
#endif
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
		a = U64MulWide(a, b, &b);
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
		a = U64MulWide(a, b, &b);
		return HashRapidMix(a ^ secret[7], b ^ secret[1] ^ 8);
	}

	inline u64 HashRapid(const void* data, u64 size)
	{
		return HashRapid((const c8*)data, size);
	}



	//~ Compile-time string hashes

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
} //namespace Tool



#endif //_TOOL_HASH_H
