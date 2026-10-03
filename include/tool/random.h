#ifndef _TOOL_RANDOM_H
#define _TOOL_RANDOM_H

#include "basics.h"
#include "error.h"
#include "hash.h"
#include "vector.h"



//~ Random numbers
//
// Three generators, each a small struct set up by its Init:
// - RandomStream is a sequence. Each *Next getter takes the next value from it.
// - RandomKeyed is a seed. Each *At getter returns the value at a key: an ID, a counter or up to four signed
//   coordinates. The same key always gives the same value, in any order, and keys never affect each other. For
//   several independent values at one key, add an index as one more coordinate: {x, y, 1}.
// - RandomSpread is a seed for evenly spread points. Its *At getters take an index and return points that cover the
//   space without the clumps and gaps of random points.
//
//     RandomStream stream = {};
//     RandomStreamInit(&stream, seed);
//     i32 roll = RandomI32Next(&stream, 1, 7);
//
//     RandomKeyed keyed = {};
//     RandomKeyedInit(&keyed, seed);
//     b8 tree = RandomB8At(&keyed, {x, y});
//     f32 angle = RandomF32At(&keyed, {x, y, 1}, 0.0f, 6.28f);
//
//     RandomSpread spread = {};
//     RandomSpreadInit(&spread, seed);
//     v2 point = RandomInSquareAt(&spread, i);
//
// Integer ranges are min inclusive, max exclusive. Float ranges are [min, max), though rounding can land on max.
//
// Outputs are stable: seeded content depends on them, so changing an algorithm here is a breaking change. Integer
// results and spread points are identical on every platform. Float ranges are too, unless the compiler fuses
// multiply-adds across statements (GCC's default does on FMA targets). Gaussian and geometry results go through
// platform math functions and can differ in the last bits between platforms.



namespace Tool
{
	//- Generators

	//~ Stream

	// xoshiro256** (Blackman and Vigna, public domain). 32 plain bytes: copy it to snapshot or rewind. A zeroed
	// stream is invalid until RandomStreamInit. Independent streams come from independent seeds, such as
	// RandomU64At(&keyed, streamID).
	struct RandomStream
	{
		u64 state[4];
	};

	inline void RandomStreamInit(RandomStream* stream, u64 seed);

	//~ Keyed

	// A zeroed keyed generator is valid and equals seed 0.
	struct RandomKeyed
	{
		u64 seed;
	};

	inline void RandomKeyedInit(RandomKeyed* keyed, u64 seed);

	// The key for an *At getter, converted from what's passed: an ID, 1 to 4 signed coordinates, a point, or an ID
	// or point plus an index. Coordinates fill four 32-bit slots and missing ones are 0, so {x, y} equals
	// {x, y, 0}. A 64-bit ID fills the first two slots. Floats don't convert.
	struct RandomKey
	{
		u64 low;
		u64 high;

		RandomKey() = default;
		constexpr RandomKey(u64 id) :
		    low(id), high(0)
		{
		}
		constexpr RandomKey(i64 id) :
		    low((u64)id), high(0)
		{
		}
		constexpr RandomKey(u32 x) :
		    low(x), high(0)
		{
		}
		constexpr RandomKey(i32 x) :
		    low((u32)x), high(0)
		{
		}
		constexpr RandomKey(u64 id, i32 index) :
		    low(id), high((u32)index)
		{
		}
		constexpr RandomKey(i32 x, i32 y) :
		    low((u32)x | ((u64)(u32)y << 32)), high(0)
		{
		}
		constexpr RandomKey(i32 x, i32 y, i32 z) :
		    low((u32)x | ((u64)(u32)y << 32)), high((u32)z)
		{
		}
		constexpr RandomKey(i32 x, i32 y, i32 z, i32 w) :
		    low((u32)x | ((u64)(u32)y << 32)), high((u32)z | ((u64)(u32)w << 32))
		{
		}
		RandomKey(p2 point) :
		    RandomKey(point.x, point.y)
		{
		}
		RandomKey(p2 point, i32 index) :
		    RandomKey(point.x, point.y, index)
		{
		}
		RandomKey(p3 point) :
		    RandomKey(point.x, point.y, point.z)
		{
		}
		RandomKey(p3 point, i32 index) :
		    RandomKey(point.x, point.y, point.z, index)
		{
		}
		RandomKey(p4 point) :
		    RandomKey(point.x, point.y, point.z, point.w)
		{
		}
	};

	//~ Spread

	// Halton points in bases 2, 3 and 5. Each seed shifts every digit by its own random amount (mod the base), which
	// keeps Halton's stratification: indices [m * b^k, (m + 1) * b^k) put exactly one point in each 1/b^k interval
	// of a base-b axis, up to the 24-bit rounding of the output. Each axis repeats after roughly 2^24 indices.
	// A zeroed spread is valid and gives plain Halton points.
	struct RandomSpread
	{
		u64 shifts[3];
	};

	void RandomSpreadInit(RandomSpread* spread, u64 seed);



	//- Getters

	//~ Floats

	// [0, 1), or [min, max).
	inline f32 RandomF32Next(RandomStream* stream);
	inline f32 RandomF32Next(RandomStream* stream, f32 min, f32 max);
	inline f32 RandomF32At(const RandomKeyed* keyed, RandomKey key);
	inline f32 RandomF32At(const RandomKeyed* keyed, RandomKey key, f32 min, f32 max);
	f32 RandomF32At(const RandomSpread* spread, u64 index);

	inline f64 RandomF64Next(RandomStream* stream);
	inline f64 RandomF64Next(RandomStream* stream, f64 min, f64 max);
	inline f64 RandomF64At(const RandomKeyed* keyed, RandomKey key);
	inline f64 RandomF64At(const RandomKeyed* keyed, RandomKey key, f64 min, f64 max);

	//~ Integers

	// Any value of the type, or [min, max).
	inline i32 RandomI32Next(RandomStream* stream);
	inline i32 RandomI32Next(RandomStream* stream, i32 min, i32 max);
	inline i32 RandomI32At(const RandomKeyed* keyed, RandomKey key);
	inline i32 RandomI32At(const RandomKeyed* keyed, RandomKey key, i32 min, i32 max);

	inline u32 RandomU32Next(RandomStream* stream);
	inline u32 RandomU32Next(RandomStream* stream, u32 min, u32 max);
	inline u32 RandomU32At(const RandomKeyed* keyed, RandomKey key);
	inline u32 RandomU32At(const RandomKeyed* keyed, RandomKey key, u32 min, u32 max);

	inline i64 RandomI64Next(RandomStream* stream);
	inline i64 RandomI64Next(RandomStream* stream, i64 min, i64 max);
	inline i64 RandomI64At(const RandomKeyed* keyed, RandomKey key);
	inline i64 RandomI64At(const RandomKeyed* keyed, RandomKey key, i64 min, i64 max);

	// The raw generator output: every other getter is built on these two.
	inline u64 RandomU64Next(RandomStream* stream);
	inline u64 RandomU64Next(RandomStream* stream, u64 min, u64 max);
	inline u64 RandomU64At(const RandomKeyed* keyed, RandomKey key);
	inline u64 RandomU64At(const RandomKeyed* keyed, RandomKey key, u64 min, u64 max);

	inline i16 RandomI16Next(RandomStream* stream);
	inline i16 RandomI16Next(RandomStream* stream, i16 min, i16 max);
	inline i16 RandomI16At(const RandomKeyed* keyed, RandomKey key);
	inline i16 RandomI16At(const RandomKeyed* keyed, RandomKey key, i16 min, i16 max);

	inline u16 RandomU16Next(RandomStream* stream);
	inline u16 RandomU16Next(RandomStream* stream, u16 min, u16 max);
	inline u16 RandomU16At(const RandomKeyed* keyed, RandomKey key);
	inline u16 RandomU16At(const RandomKeyed* keyed, RandomKey key, u16 min, u16 max);

	inline i8 RandomI8Next(RandomStream* stream);
	inline i8 RandomI8Next(RandomStream* stream, i8 min, i8 max);
	inline i8 RandomI8At(const RandomKeyed* keyed, RandomKey key);
	inline i8 RandomI8At(const RandomKeyed* keyed, RandomKey key, i8 min, i8 max);

	inline u8 RandomU8Next(RandomStream* stream);
	inline u8 RandomU8Next(RandomStream* stream, u8 min, u8 max);
	inline u8 RandomU8At(const RandomKeyed* keyed, RandomKey key);
	inline u8 RandomU8At(const RandomKeyed* keyed, RandomKey key, u8 min, u8 max);

	//~ Booleans

	inline b8 RandomB8Next(RandomStream* stream);
	inline b8 RandomB8At(const RandomKeyed* keyed, RandomKey key);
	inline b32 RandomB32Next(RandomStream* stream);
	inline b32 RandomB32At(const RandomKeyed* keyed, RandomKey key);

	//~ Points

	// Inside the square or cube [0, 1) on each axis. Unlike the circle and sphere below, not centered on the origin.
	inline v2 RandomInSquareNext(RandomStream* stream);
	inline v2 RandomInSquareAt(const RandomKeyed* keyed, RandomKey key);
	v2 RandomInSquareAt(const RandomSpread* spread, u64 index);

	inline v3 RandomInCubeNext(RandomStream* stream);
	inline v3 RandomInCubeAt(const RandomKeyed* keyed, RandomKey key);
	v3 RandomInCubeAt(const RandomSpread* spread, u64 index);

	// On the unit circle or sphere (On), or inside it (In).
	inline v2 RandomOnCircleNext(RandomStream* stream);
	inline v2 RandomOnCircleAt(const RandomKeyed* keyed, RandomKey key);
	inline v2 RandomInCircleNext(RandomStream* stream);
	inline v2 RandomInCircleAt(const RandomKeyed* keyed, RandomKey key);
	inline v3 RandomOnSphereNext(RandomStream* stream);
	inline v3 RandomOnSphereAt(const RandomKeyed* keyed, RandomKey key);
	inline v3 RandomInSphereNext(RandomStream* stream);
	inline v3 RandomInSphereAt(const RandomKeyed* keyed, RandomKey key);

	//~ Gaussian

	// Normal distribution. Tails end near 5.8 deviations for f32 and 8.5 for f64.
	inline f32 RandomGaussianF32Next(RandomStream* stream, f32 mean, f32 deviation);
	inline f32 RandomGaussianF32At(const RandomKeyed* keyed, RandomKey key, f32 mean, f32 deviation);
	inline f64 RandomGaussianF64Next(RandomStream* stream, f64 mean, f64 deviation);
	inline f64 RandomGaussianF64At(const RandomKeyed* keyed, RandomKey key, f64 mean, f64 deviation);

	//~ Collections

	// Fisher-Yates.
	template<typename T>
	void RandomShuffleNext(RandomStream* stream, T* items, u64 count);
	template<typename T>
	void RandomShuffleAt(const RandomKeyed* keyed, RandomKey key, T* items, u64 count);

	// Fills 'size' bytes with successive outputs, each in little-endian byte order.
	void RandomFillNext(RandomStream* stream, void* dst, u64 size);



	//- Building blocks

	// Bijective 64-bit mixer, Pelle Evensen's nasam. Strong even on counter-like inputs, and distinct inputs never
	// collide. It maps 0 to 0.
	constexpr u64 RandomMix(u64 x);

	// The next 64 bits after 'bits'. Getters use it when one output word isn't enough.
	constexpr u64 RandomStretch(u64 bits);
} //namespace Tool



//- Implementation
//
// Every getter draws one 64-bit word (RandomU64Next or RandomU64At) and turns it into its value with a RandomBits
// function. Values that need more than 64 bits stretch the word.

namespace Tool
{
	//~ Building blocks

	constexpr u64 RandomMix(u64 x)
	{
		x ^= ((x >> 25) | (x << 39)) ^ ((x >> 47) | (x << 17));
		x *= 0x9e6c63d0676a9a99ull;
		x ^= (x >> 23) ^ (x >> 51);
		x *= 0x9e6d62d06f6a9a9bull;
		x ^= (x >> 23) ^ (x >> 51);
		return x;
	}

	constexpr u64 RandomStretch(u64 bits)
	{
		return RandomMix(bits + 0x9e3779b97f4a7c15ull);
	}

	//~ Generators

	inline void RandomStreamInit(RandomStream* stream, u64 seed)
	{
		RandomKeyed keyed = { seed };
		for (u64 i = 0; i < 4; i++)
			stream->state[i] = RandomU64At(&keyed, i);
	}

	inline void RandomKeyedInit(RandomKeyed* keyed, u64 seed)
	{
		keyed->seed = seed;
	}

	inline u64 RandomU64Next(RandomStream* stream)
	{
		u64* s = stream->state;
		TOOL_DEBUG_ASSERT((s[0] | s[1] | s[2] | s[3]) != 0, "RandomStream used before RandomStreamInit");

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

	// The seed enters both rounds, so each seed is its own permutation of the keys rather than a shifted copy of
	// another seed's. Keys within the first two slots take two rounds; using the last two adds a third.
	inline u64 RandomU64At(const RandomKeyed* keyed, RandomKey key)
	{
		u64 inner = RandomMix(key.low ^ keyed->seed ^ 0x9e3779b97f4a7c15ull);
		if (key.high != 0)
			inner = RandomMix(inner ^ key.high);
		return RandomMix(inner ^ (keyed->seed * 0xd1b54a32d192ed03ull));
	}

	//~ Bits to values

	// Ranges up to 32 bits scale all 64 bits by the range (Lemire's multiply, without the rejection step), so the
	// bias is at most 2^-32. 64-bit ranges reject and stretch, which makes them exact.
	inline u32 RandomBitsU32(u64 bits, u32 min, u32 max)
	{
		TOOL_DEBUG_ASSERT(min < max);
		u64 range = (u32)(max - min);
		u64 high = (bits >> 32) * range;
		u64 low = (bits & 0xffffffffull) * range;
		return min + (u32)((high + (low >> 32)) >> 32);
	}

	inline u64 RandomBitsU64(u64 bits, u64 min, u64 max)
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
	inline i32 RandomBitsI32(u64 bits, i32 min, i32 max)
	{
		TOOL_DEBUG_ASSERT(min < max);
		return (i32)((u32)min + RandomBitsU32(bits, 0, (u32)max - (u32)min));
	}

	inline i64 RandomBitsI64(u64 bits, i64 min, i64 max)
	{
		TOOL_DEBUG_ASSERT(min < max);
		return (i64)((u64)min + RandomBitsU64(bits, 0, (u64)max - (u64)min));
	}

	// From the top 24 or 53 bits, so every value is exact and equally spaced.
	inline f32 RandomBitsF32(u64 bits) { return (f32)(bits >> 40) * 0x1p-24f; }
	inline f64 RandomBitsF64(u64 bits) { return (f64)(bits >> 11) * 0x1p-53; }

	// Scale and offset are separate statements, which Clang and MSVC never fuse into an FMA.
	inline f32 RandomBitsF32(u64 bits, f32 min, f32 max)
	{
		f32 scaled = (max - min) * RandomBitsF32(bits);
		return min + scaled;
	}

	inline f64 RandomBitsF64(u64 bits, f64 min, f64 max)
	{
		f64 scaled = (max - min) * RandomBitsF64(bits);
		return min + scaled;
	}

	// The second axis takes the low 24 bits, independent of the first's top 24.
	inline v2 RandomBitsInSquare(u64 bits) { return v2 { RandomBitsF32(bits), (f32)(bits & 0xffffffull) * 0x1p-24f }; }
	inline v3 RandomBitsInCube(u64 bits)
	{
		v2 xy = RandomBitsInSquare(bits);
		return v3 { xy.x, xy.y, RandomBitsF32(RandomStretch(bits)) };
	}

	v2 RandomBitsOnCircle(u64 bits);
	v2 RandomBitsInCircle(u64 bits);
	v3 RandomBitsOnSphere(u64 bits);
	v3 RandomBitsInSphere(u64 bits);
	f32 RandomBitsGaussianF32(u64 bits, f32 mean, f32 deviation);
	f64 RandomBitsGaussianF64(u64 bits, f64 mean, f64 deviation);

	//~ Getters

	inline f32 RandomF32Next(RandomStream* stream) { return RandomBitsF32(RandomU64Next(stream)); }
	inline f32 RandomF32Next(RandomStream* stream, f32 min, f32 max) { return RandomBitsF32(RandomU64Next(stream), min, max); }
	inline f32 RandomF32At(const RandomKeyed* keyed, RandomKey key) { return RandomBitsF32(RandomU64At(keyed, key)); }
	inline f32 RandomF32At(const RandomKeyed* keyed, RandomKey key, f32 min, f32 max) { return RandomBitsF32(RandomU64At(keyed, key), min, max); }
	inline f64 RandomF64Next(RandomStream* stream) { return RandomBitsF64(RandomU64Next(stream)); }
	inline f64 RandomF64Next(RandomStream* stream, f64 min, f64 max) { return RandomBitsF64(RandomU64Next(stream), min, max); }
	inline f64 RandomF64At(const RandomKeyed* keyed, RandomKey key) { return RandomBitsF64(RandomU64At(keyed, key)); }
	inline f64 RandomF64At(const RandomKeyed* keyed, RandomKey key, f64 min, f64 max) { return RandomBitsF64(RandomU64At(keyed, key), min, max); }

	// Full-width values take the top bits.
	inline i32 RandomI32Next(RandomStream* stream) { return (i32)(RandomU64Next(stream) >> 32); }
	inline i32 RandomI32Next(RandomStream* stream, i32 min, i32 max) { return RandomBitsI32(RandomU64Next(stream), min, max); }
	inline i32 RandomI32At(const RandomKeyed* keyed, RandomKey key) { return (i32)(RandomU64At(keyed, key) >> 32); }
	inline i32 RandomI32At(const RandomKeyed* keyed, RandomKey key, i32 min, i32 max) { return RandomBitsI32(RandomU64At(keyed, key), min, max); }
	inline u32 RandomU32Next(RandomStream* stream) { return (u32)(RandomU64Next(stream) >> 32); }
	inline u32 RandomU32Next(RandomStream* stream, u32 min, u32 max) { return RandomBitsU32(RandomU64Next(stream), min, max); }
	inline u32 RandomU32At(const RandomKeyed* keyed, RandomKey key) { return (u32)(RandomU64At(keyed, key) >> 32); }
	inline u32 RandomU32At(const RandomKeyed* keyed, RandomKey key, u32 min, u32 max) { return RandomBitsU32(RandomU64At(keyed, key), min, max); }
	inline i64 RandomI64Next(RandomStream* stream) { return (i64)RandomU64Next(stream); }
	inline i64 RandomI64Next(RandomStream* stream, i64 min, i64 max) { return RandomBitsI64(RandomU64Next(stream), min, max); }
	inline i64 RandomI64At(const RandomKeyed* keyed, RandomKey key) { return (i64)RandomU64At(keyed, key); }
	inline i64 RandomI64At(const RandomKeyed* keyed, RandomKey key, i64 min, i64 max) { return RandomBitsI64(RandomU64At(keyed, key), min, max); }
	inline u64 RandomU64Next(RandomStream* stream, u64 min, u64 max) { return RandomBitsU64(RandomU64Next(stream), min, max); }
	inline u64 RandomU64At(const RandomKeyed* keyed, RandomKey key, u64 min, u64 max) { return RandomBitsU64(RandomU64At(keyed, key), min, max); }
	inline i16 RandomI16Next(RandomStream* stream) { return (i16)(RandomU64Next(stream) >> 48); }
	inline i16 RandomI16Next(RandomStream* stream, i16 min, i16 max) { return (i16)RandomBitsI32(RandomU64Next(stream), min, max); }
	inline i16 RandomI16At(const RandomKeyed* keyed, RandomKey key) { return (i16)(RandomU64At(keyed, key) >> 48); }
	inline i16 RandomI16At(const RandomKeyed* keyed, RandomKey key, i16 min, i16 max) { return (i16)RandomBitsI32(RandomU64At(keyed, key), min, max); }
	inline u16 RandomU16Next(RandomStream* stream) { return (u16)(RandomU64Next(stream) >> 48); }
	inline u16 RandomU16Next(RandomStream* stream, u16 min, u16 max) { return (u16)RandomBitsU32(RandomU64Next(stream), min, max); }
	inline u16 RandomU16At(const RandomKeyed* keyed, RandomKey key) { return (u16)(RandomU64At(keyed, key) >> 48); }
	inline u16 RandomU16At(const RandomKeyed* keyed, RandomKey key, u16 min, u16 max) { return (u16)RandomBitsU32(RandomU64At(keyed, key), min, max); }
	inline i8 RandomI8Next(RandomStream* stream) { return (i8)(RandomU64Next(stream) >> 56); }
	inline i8 RandomI8Next(RandomStream* stream, i8 min, i8 max) { return (i8)RandomBitsI32(RandomU64Next(stream), min, max); }
	inline i8 RandomI8At(const RandomKeyed* keyed, RandomKey key) { return (i8)(RandomU64At(keyed, key) >> 56); }
	inline i8 RandomI8At(const RandomKeyed* keyed, RandomKey key, i8 min, i8 max) { return (i8)RandomBitsI32(RandomU64At(keyed, key), min, max); }
	inline u8 RandomU8Next(RandomStream* stream) { return (u8)(RandomU64Next(stream) >> 56); }
	inline u8 RandomU8Next(RandomStream* stream, u8 min, u8 max) { return (u8)RandomBitsU32(RandomU64Next(stream), min, max); }
	inline u8 RandomU8At(const RandomKeyed* keyed, RandomKey key) { return (u8)(RandomU64At(keyed, key) >> 56); }
	inline u8 RandomU8At(const RandomKeyed* keyed, RandomKey key, u8 min, u8 max) { return (u8)RandomBitsU32(RandomU64At(keyed, key), min, max); }

	inline b8 RandomB8Next(RandomStream* stream) { return (b8)(RandomU64Next(stream) >> 63); }
	inline b8 RandomB8At(const RandomKeyed* keyed, RandomKey key) { return (b8)(RandomU64At(keyed, key) >> 63); }
	inline b32 RandomB32Next(RandomStream* stream) { return (b32)(RandomU64Next(stream) >> 63); }
	inline b32 RandomB32At(const RandomKeyed* keyed, RandomKey key) { return (b32)(RandomU64At(keyed, key) >> 63); }

	inline v2 RandomInSquareNext(RandomStream* stream) { return RandomBitsInSquare(RandomU64Next(stream)); }
	inline v2 RandomInSquareAt(const RandomKeyed* keyed, RandomKey key) { return RandomBitsInSquare(RandomU64At(keyed, key)); }
	inline v3 RandomInCubeNext(RandomStream* stream) { return RandomBitsInCube(RandomU64Next(stream)); }
	inline v3 RandomInCubeAt(const RandomKeyed* keyed, RandomKey key) { return RandomBitsInCube(RandomU64At(keyed, key)); }
	inline v2 RandomOnCircleNext(RandomStream* stream) { return RandomBitsOnCircle(RandomU64Next(stream)); }
	inline v2 RandomOnCircleAt(const RandomKeyed* keyed, RandomKey key) { return RandomBitsOnCircle(RandomU64At(keyed, key)); }
	inline v2 RandomInCircleNext(RandomStream* stream) { return RandomBitsInCircle(RandomU64Next(stream)); }
	inline v2 RandomInCircleAt(const RandomKeyed* keyed, RandomKey key) { return RandomBitsInCircle(RandomU64At(keyed, key)); }
	inline v3 RandomOnSphereNext(RandomStream* stream) { return RandomBitsOnSphere(RandomU64Next(stream)); }
	inline v3 RandomOnSphereAt(const RandomKeyed* keyed, RandomKey key) { return RandomBitsOnSphere(RandomU64At(keyed, key)); }
	inline v3 RandomInSphereNext(RandomStream* stream) { return RandomBitsInSphere(RandomU64Next(stream)); }
	inline v3 RandomInSphereAt(const RandomKeyed* keyed, RandomKey key) { return RandomBitsInSphere(RandomU64At(keyed, key)); }

	inline f32 RandomGaussianF32Next(RandomStream* stream, f32 mean, f32 deviation) { return RandomBitsGaussianF32(RandomU64Next(stream), mean, deviation); }
	inline f32 RandomGaussianF32At(const RandomKeyed* keyed, RandomKey key, f32 mean, f32 deviation) { return RandomBitsGaussianF32(RandomU64At(keyed, key), mean, deviation); }
	inline f64 RandomGaussianF64Next(RandomStream* stream, f64 mean, f64 deviation) { return RandomBitsGaussianF64(RandomU64Next(stream), mean, deviation); }
	inline f64 RandomGaussianF64At(const RandomKeyed* keyed, RandomKey key, f64 mean, f64 deviation) { return RandomBitsGaussianF64(RandomU64At(keyed, key), mean, deviation); }

	// The stream form draws once per swap; the keyed form stretches its word once per swap.
	template<typename T>
	void RandomShuffleNext(RandomStream* stream, T* items, u64 count)
	{
		for (u64 i = count; i > 1; i--)
		{
			u64 j = RandomBitsU64(RandomU64Next(stream), 0, i);
			T swap = items[i - 1];
			items[i - 1] = items[j];
			items[j] = swap;
		}
	}

	template<typename T>
	void RandomShuffleAt(const RandomKeyed* keyed, RandomKey key, T* items, u64 count)
	{
		u64 bits = RandomU64At(keyed, key);
		for (u64 i = count; i > 1; i--)
		{
			u64 j = RandomBitsU64(bits, 0, i);
			T swap = items[i - 1];
			items[i - 1] = items[j];
			items[j] = swap;
			bits = RandomStretch(bits);
		}
	}
} //namespace Tool

#endif
