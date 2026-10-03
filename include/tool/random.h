#ifndef _TOOL_RANDOM_H
#define _TOOL_RANDOM_H

#include "basics.h"
#include "error.h"
#include "mathematics.h"
#include "vector.h"



//~ Random numbers
//
// Three generators, each a small struct set up by its Init:
// - RandomStream is a sequence. Each *Next getter takes the next value from it.
// - RandomKeyed is a seed. Each *At getter returns the value at a key: an ID, a counter or up to four signed
//   coordinates. The same key always gives the same value, in any order, and keys never affect each other. For
//   several independent values at one key, add an index as one more coordinate: Key(x, y, 1).
// - RandomSpread is a seed for evenly spread points. Its *At getters take an index and return points that cover the
//   space without the clumps and gaps of random points.
//
//     RandomStream stream = {};
//     RandomStreamInit(&stream, seed);
//     i32 roll = RandomI32Next(&stream, 1, 7);
//
//     RandomKeyed keyed = {};
//     RandomKeyedInit(&keyed, seed);
//     b8 tree = RandomB8At(&keyed, Key(x, y));
//     f32 angle = RandomF32At(&keyed, Key(x, y, 1), 0.0f, 6.28f);
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
	// RandomU64At(&keyed, Key(streamID)).
	struct RandomStream
	{
		u64 state[4];
	};

	inline void RandomStreamInit(RandomStream* random, u64 seed);

	//~ Keyed

	// A zeroed keyed generator is valid and equals seed 0.
	struct RandomKeyed
	{
		u64 seed;
	};

	inline void RandomKeyedInit(RandomKeyed* random, u64 seed);

	// The key for an *At getter: four 32-bit slots in two words. Build one with Key.
	struct RandomKey
	{
		u64 low;
		u64 high;
	};

	// Keys from an ID, 1 to 4 signed coordinates, a point, or an ID or point plus an index. Coordinates fill the
	// slots in order and missing ones are 0, so Key(x, y) equals Key(x, y, 0). A 64-bit ID fills the first two slots.
	// There is no float overload, so a float argument is ambiguous and doesn't compile.
	//
	// unsigned long, which size_t is on 64-bit Linux and Mac, keys as the u64 of the same value, which matches the u32
	// key wherever the value fits both. Signed long is deleted: it is 32 bits on Windows and 64 elsewhere, and a
	// negative value keys differently at the two widths. Cast it to i32 or i64.
	constexpr RandomKey Key(i64 id);
	constexpr RandomKey Key(u64 id);
	constexpr RandomKey Key(unsigned long id);
	RandomKey Key(long id) = delete;
	constexpr RandomKey Key(u64 id, i32 index);
	constexpr RandomKey Key(i32 x);
	constexpr RandomKey Key(u32 x);
	constexpr RandomKey Key(i32 x, i32 y);
	constexpr RandomKey Key(i32 x, i32 y, i32 z);
	constexpr RandomKey Key(i32 x, i32 y, i32 z, i32 w);
	inline RandomKey Key(p2 point);
	inline RandomKey Key(p2 point, i32 index);
	inline RandomKey Key(p3 point);
	inline RandomKey Key(p3 point, i32 index);
	inline RandomKey Key(p4 point);

	//~ Spread

	// Halton points in bases 2, 3 and 5. Each seed shifts every digit by its own random amount (mod the base), which
	// keeps Halton's stratification: indices [m * b^k, (m + 1) * b^k) put exactly one point in each 1/b^k interval
	// of a base-b axis, up to the 24-bit rounding of the output. Each axis repeats after roughly 2^24 indices.
	// A zeroed spread is valid and gives plain Halton points.
	struct RandomSpread
	{
		u64 shifts[3];
	};

	void RandomSpreadInit(RandomSpread* random, u64 seed);



	//- Getters

	//~ Floats

	// [0, 1), or [min, max).
	inline f64 RandomF64Next(RandomStream* random);
	inline f64 RandomF64Next(RandomStream* random, f64 min, f64 max);
	inline f64 RandomF64At(const RandomKeyed* random, RandomKey key);
	inline f64 RandomF64At(const RandomKeyed* random, RandomKey key, f64 min, f64 max);

	inline f32 RandomF32Next(RandomStream* random);
	inline f32 RandomF32Next(RandomStream* random, f32 min, f32 max);
	inline f32 RandomF32At(const RandomKeyed* random, RandomKey key);
	inline f32 RandomF32At(const RandomKeyed* random, RandomKey key, f32 min, f32 max);
	f32 RandomF32At(const RandomSpread* random, u64 index);

	//~ Integers

	// Any value of the type, or [min, max).
	inline i64 RandomI64Next(RandomStream* random);
	inline i64 RandomI64Next(RandomStream* random, i64 min, i64 max);
	inline i64 RandomI64At(const RandomKeyed* random, RandomKey key);
	inline i64 RandomI64At(const RandomKeyed* random, RandomKey key, i64 min, i64 max);

	// The raw generator output: every other getter is built on these two.
	inline u64 RandomU64Next(RandomStream* random);
	inline u64 RandomU64Next(RandomStream* random, u64 min, u64 max);
	inline u64 RandomU64At(const RandomKeyed* random, RandomKey key);
	inline u64 RandomU64At(const RandomKeyed* random, RandomKey key, u64 min, u64 max);

	inline i32 RandomI32Next(RandomStream* random);
	inline i32 RandomI32Next(RandomStream* random, i32 min, i32 max);
	inline i32 RandomI32At(const RandomKeyed* random, RandomKey key);
	inline i32 RandomI32At(const RandomKeyed* random, RandomKey key, i32 min, i32 max);

	inline u32 RandomU32Next(RandomStream* random);
	inline u32 RandomU32Next(RandomStream* random, u32 min, u32 max);
	inline u32 RandomU32At(const RandomKeyed* random, RandomKey key);
	inline u32 RandomU32At(const RandomKeyed* random, RandomKey key, u32 min, u32 max);

	inline i16 RandomI16Next(RandomStream* random);
	inline i16 RandomI16Next(RandomStream* random, i16 min, i16 max);
	inline i16 RandomI16At(const RandomKeyed* random, RandomKey key);
	inline i16 RandomI16At(const RandomKeyed* random, RandomKey key, i16 min, i16 max);

	inline u16 RandomU16Next(RandomStream* random);
	inline u16 RandomU16Next(RandomStream* random, u16 min, u16 max);
	inline u16 RandomU16At(const RandomKeyed* random, RandomKey key);
	inline u16 RandomU16At(const RandomKeyed* random, RandomKey key, u16 min, u16 max);

	inline i8 RandomI8Next(RandomStream* random);
	inline i8 RandomI8Next(RandomStream* random, i8 min, i8 max);
	inline i8 RandomI8At(const RandomKeyed* random, RandomKey key);
	inline i8 RandomI8At(const RandomKeyed* random, RandomKey key, i8 min, i8 max);

	inline u8 RandomU8Next(RandomStream* random);
	inline u8 RandomU8Next(RandomStream* random, u8 min, u8 max);
	inline u8 RandomU8At(const RandomKeyed* random, RandomKey key);
	inline u8 RandomU8At(const RandomKeyed* random, RandomKey key, u8 min, u8 max);

	//~ Booleans

	inline b32 RandomB32Next(RandomStream* random);
	inline b32 RandomB32At(const RandomKeyed* random, RandomKey key);
	inline b8 RandomB8Next(RandomStream* random);
	inline b8 RandomB8At(const RandomKeyed* random, RandomKey key);

	//~ Points

	// Inside the square or cube [0, 1) on each axis. Unlike the circle and sphere below, not centered on the origin.
	inline v2 RandomInSquareNext(RandomStream* random);
	inline v2 RandomInSquareAt(const RandomKeyed* random, RandomKey key);
	v2 RandomInSquareAt(const RandomSpread* random, u64 index);

	inline v3 RandomInCubeNext(RandomStream* random);
	inline v3 RandomInCubeAt(const RandomKeyed* random, RandomKey key);
	v3 RandomInCubeAt(const RandomSpread* random, u64 index);

	// On the unit circle or sphere (On), or inside it (In).
	inline v2 RandomOnCircleNext(RandomStream* random);
	inline v2 RandomOnCircleAt(const RandomKeyed* random, RandomKey key);
	inline v2 RandomInCircleNext(RandomStream* random);
	inline v2 RandomInCircleAt(const RandomKeyed* random, RandomKey key);
	inline v3 RandomOnSphereNext(RandomStream* random);
	inline v3 RandomOnSphereAt(const RandomKeyed* random, RandomKey key);
	inline v3 RandomInSphereNext(RandomStream* random);
	inline v3 RandomInSphereAt(const RandomKeyed* random, RandomKey key);

	//~ Gaussian

	// Normal distribution. Tails end near 8.5 deviations for f64 and 5.8 for f32.
	inline f64 RandomGaussianF64Next(RandomStream* random, f64 mean, f64 deviation);
	inline f64 RandomGaussianF64At(const RandomKeyed* random, RandomKey key, f64 mean, f64 deviation);
	inline f32 RandomGaussianF32Next(RandomStream* random, f32 mean, f32 deviation);
	inline f32 RandomGaussianF32At(const RandomKeyed* random, RandomKey key, f32 mean, f32 deviation);

	//~ Collections

	// Fisher-Yates.
	template<typename T>
	void RandomShuffleNext(RandomStream* random, T* items, u64 count);
	template<typename T>
	void RandomShuffleAt(const RandomKeyed* random, RandomKey key, T* items, u64 count);

	// Fills 'size' bytes with successive outputs, each in little-endian byte order.
	void RandomFillNext(RandomStream* random, void* dst, u64 size);



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

	//~ Keys

	constexpr RandomKey Key(i64 id) { return { (u64)id, 0 }; }
	constexpr RandomKey Key(u64 id) { return { id, 0 }; }
	constexpr RandomKey Key(unsigned long id) { return Key((u64)id); }
	constexpr RandomKey Key(u64 id, i32 index) { return { id, (u32)index }; }
	constexpr RandomKey Key(i32 x) { return { (u32)x, 0 }; }
	constexpr RandomKey Key(u32 x) { return { x, 0 }; }
	constexpr RandomKey Key(i32 x, i32 y) { return { (u32)x | ((u64)(u32)y << 32), 0 }; }
	constexpr RandomKey Key(i32 x, i32 y, i32 z) { return { (u32)x | ((u64)(u32)y << 32), (u32)z }; }
	constexpr RandomKey Key(i32 x, i32 y, i32 z, i32 w) { return { (u32)x | ((u64)(u32)y << 32), (u32)z | ((u64)(u32)w << 32) }; }
	inline RandomKey Key(p2 point) { return Key(point.x, point.y); }
	inline RandomKey Key(p2 point, i32 index) { return Key(point.x, point.y, index); }
	inline RandomKey Key(p3 point) { return Key(point.x, point.y, point.z); }
	inline RandomKey Key(p3 point, i32 index) { return Key(point.x, point.y, point.z, index); }
	inline RandomKey Key(p4 point) { return Key(point.x, point.y, point.z, point.w); }

	//~ Generators

	inline void RandomStreamInit(RandomStream* random, u64 seed)
	{
		RandomKeyed keyed = { seed };
		for (u64 i = 0; i < 4; i++)
			random->state[i] = RandomU64At(&keyed, Key(i));
	}

	inline void RandomKeyedInit(RandomKeyed* random, u64 seed)
	{
		random->seed = seed;
	}

	inline u64 RandomU64Next(RandomStream* random)
	{
		u64* s = random->state;
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
	inline u64 RandomU64At(const RandomKeyed* random, RandomKey key)
	{
		u64 inner = RandomMix(key.low ^ random->seed ^ 0x9e3779b97f4a7c15ull);
		if (key.high != 0)
			inner = RandomMix(inner ^ key.high);
		return RandomMix(inner ^ (random->seed * 0xd1b54a32d192ed03ull));
	}

	//~ Bits to values

	// 64-bit ranges use Lemire's multiply with rejection, stretching on a reject, which makes them exact. Ranges up
	// to 32 bits scale all 64 bits by the range without the rejection step, so their bias is at most 2^-32.
	inline u64 RandomBitsU64(u64 bits, u64 min, u64 max)
	{
		TOOL_DEBUG_ASSERT(min < max);
		u64 range = max - min;
		u64 high;
		u64 low = U64MulWide(bits, range, &high);
		if (low < range)
		{
			u64 threshold = (0 - range) % range;
			while (low < threshold)
			{
				bits = RandomStretch(bits);
				low = U64MulWide(bits, range, &high);
			}
		}
		return min + high;
	}

	inline u32 RandomBitsU32(u64 bits, u32 min, u32 max)
	{
		TOOL_DEBUG_ASSERT(min < max);
		u64 range = (u32)(max - min);
		u64 high = (bits >> 32) * range;
		u64 low = (bits & 0xffffffffull) * range;
		return min + (u32)((high + (low >> 32)) >> 32);
	}

	// Signed ranges offset an unsigned range of the same width, so any min < max works without overflow.
	inline i64 RandomBitsI64(u64 bits, i64 min, i64 max)
	{
		TOOL_DEBUG_ASSERT(min < max);
		return (i64)((u64)min + RandomBitsU64(bits, 0, (u64)max - (u64)min));
	}

	inline i32 RandomBitsI32(u64 bits, i32 min, i32 max)
	{
		TOOL_DEBUG_ASSERT(min < max);
		return (i32)((u32)min + RandomBitsU32(bits, 0, (u32)max - (u32)min));
	}

	// From the top 53 or 24 bits, so every value is exact and equally spaced.
	inline f64 RandomBitsF64(u64 bits) { return (f64)(bits >> 11) * 0x1p-53; }
	inline f32 RandomBitsF32(u64 bits) { return (f32)(bits >> 40) * 0x1p-24f; }

	// Scale and offset are separate statements, which Clang and MSVC never fuse into an FMA.
	inline f64 RandomBitsF64(u64 bits, f64 min, f64 max)
	{
		f64 scaled = (max - min) * RandomBitsF64(bits);
		return min + scaled;
	}

	inline f32 RandomBitsF32(u64 bits, f32 min, f32 max)
	{
		f32 scaled = (max - min) * RandomBitsF32(bits);
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
	f64 RandomBitsGaussianF64(u64 bits, f64 mean, f64 deviation);
	f32 RandomBitsGaussianF32(u64 bits, f32 mean, f32 deviation);

	//~ Getters

	inline f64 RandomF64Next(RandomStream* random) { return RandomBitsF64(RandomU64Next(random)); }
	inline f64 RandomF64Next(RandomStream* random, f64 min, f64 max) { return RandomBitsF64(RandomU64Next(random), min, max); }
	inline f64 RandomF64At(const RandomKeyed* random, RandomKey key) { return RandomBitsF64(RandomU64At(random, key)); }
	inline f64 RandomF64At(const RandomKeyed* random, RandomKey key, f64 min, f64 max) { return RandomBitsF64(RandomU64At(random, key), min, max); }
	inline f32 RandomF32Next(RandomStream* random) { return RandomBitsF32(RandomU64Next(random)); }
	inline f32 RandomF32Next(RandomStream* random, f32 min, f32 max) { return RandomBitsF32(RandomU64Next(random), min, max); }
	inline f32 RandomF32At(const RandomKeyed* random, RandomKey key) { return RandomBitsF32(RandomU64At(random, key)); }
	inline f32 RandomF32At(const RandomKeyed* random, RandomKey key, f32 min, f32 max) { return RandomBitsF32(RandomU64At(random, key), min, max); }

	// Full-width values take the top bits.
	inline i64 RandomI64Next(RandomStream* random) { return (i64)RandomU64Next(random); }
	inline i64 RandomI64Next(RandomStream* random, i64 min, i64 max) { return RandomBitsI64(RandomU64Next(random), min, max); }
	inline i64 RandomI64At(const RandomKeyed* random, RandomKey key) { return (i64)RandomU64At(random, key); }
	inline i64 RandomI64At(const RandomKeyed* random, RandomKey key, i64 min, i64 max) { return RandomBitsI64(RandomU64At(random, key), min, max); }
	inline u64 RandomU64Next(RandomStream* random, u64 min, u64 max) { return RandomBitsU64(RandomU64Next(random), min, max); }
	inline u64 RandomU64At(const RandomKeyed* random, RandomKey key, u64 min, u64 max) { return RandomBitsU64(RandomU64At(random, key), min, max); }
	inline i32 RandomI32Next(RandomStream* random) { return (i32)(RandomU64Next(random) >> 32); }
	inline i32 RandomI32Next(RandomStream* random, i32 min, i32 max) { return RandomBitsI32(RandomU64Next(random), min, max); }
	inline i32 RandomI32At(const RandomKeyed* random, RandomKey key) { return (i32)(RandomU64At(random, key) >> 32); }
	inline i32 RandomI32At(const RandomKeyed* random, RandomKey key, i32 min, i32 max) { return RandomBitsI32(RandomU64At(random, key), min, max); }
	inline u32 RandomU32Next(RandomStream* random) { return (u32)(RandomU64Next(random) >> 32); }
	inline u32 RandomU32Next(RandomStream* random, u32 min, u32 max) { return RandomBitsU32(RandomU64Next(random), min, max); }
	inline u32 RandomU32At(const RandomKeyed* random, RandomKey key) { return (u32)(RandomU64At(random, key) >> 32); }
	inline u32 RandomU32At(const RandomKeyed* random, RandomKey key, u32 min, u32 max) { return RandomBitsU32(RandomU64At(random, key), min, max); }
	inline i16 RandomI16Next(RandomStream* random) { return (i16)(RandomU64Next(random) >> 48); }
	inline i16 RandomI16Next(RandomStream* random, i16 min, i16 max) { return (i16)RandomBitsI32(RandomU64Next(random), min, max); }
	inline i16 RandomI16At(const RandomKeyed* random, RandomKey key) { return (i16)(RandomU64At(random, key) >> 48); }
	inline i16 RandomI16At(const RandomKeyed* random, RandomKey key, i16 min, i16 max) { return (i16)RandomBitsI32(RandomU64At(random, key), min, max); }
	inline u16 RandomU16Next(RandomStream* random) { return (u16)(RandomU64Next(random) >> 48); }
	inline u16 RandomU16Next(RandomStream* random, u16 min, u16 max) { return (u16)RandomBitsU32(RandomU64Next(random), min, max); }
	inline u16 RandomU16At(const RandomKeyed* random, RandomKey key) { return (u16)(RandomU64At(random, key) >> 48); }
	inline u16 RandomU16At(const RandomKeyed* random, RandomKey key, u16 min, u16 max) { return (u16)RandomBitsU32(RandomU64At(random, key), min, max); }
	inline i8 RandomI8Next(RandomStream* random) { return (i8)(RandomU64Next(random) >> 56); }
	inline i8 RandomI8Next(RandomStream* random, i8 min, i8 max) { return (i8)RandomBitsI32(RandomU64Next(random), min, max); }
	inline i8 RandomI8At(const RandomKeyed* random, RandomKey key) { return (i8)(RandomU64At(random, key) >> 56); }
	inline i8 RandomI8At(const RandomKeyed* random, RandomKey key, i8 min, i8 max) { return (i8)RandomBitsI32(RandomU64At(random, key), min, max); }
	inline u8 RandomU8Next(RandomStream* random) { return (u8)(RandomU64Next(random) >> 56); }
	inline u8 RandomU8Next(RandomStream* random, u8 min, u8 max) { return (u8)RandomBitsU32(RandomU64Next(random), min, max); }
	inline u8 RandomU8At(const RandomKeyed* random, RandomKey key) { return (u8)(RandomU64At(random, key) >> 56); }
	inline u8 RandomU8At(const RandomKeyed* random, RandomKey key, u8 min, u8 max) { return (u8)RandomBitsU32(RandomU64At(random, key), min, max); }

	inline b32 RandomB32Next(RandomStream* random) { return (b32)(RandomU64Next(random) >> 63); }
	inline b32 RandomB32At(const RandomKeyed* random, RandomKey key) { return (b32)(RandomU64At(random, key) >> 63); }
	inline b8 RandomB8Next(RandomStream* random) { return (b8)(RandomU64Next(random) >> 63); }
	inline b8 RandomB8At(const RandomKeyed* random, RandomKey key) { return (b8)(RandomU64At(random, key) >> 63); }

	inline v2 RandomInSquareNext(RandomStream* random) { return RandomBitsInSquare(RandomU64Next(random)); }
	inline v2 RandomInSquareAt(const RandomKeyed* random, RandomKey key) { return RandomBitsInSquare(RandomU64At(random, key)); }
	inline v3 RandomInCubeNext(RandomStream* random) { return RandomBitsInCube(RandomU64Next(random)); }
	inline v3 RandomInCubeAt(const RandomKeyed* random, RandomKey key) { return RandomBitsInCube(RandomU64At(random, key)); }
	inline v2 RandomOnCircleNext(RandomStream* random) { return RandomBitsOnCircle(RandomU64Next(random)); }
	inline v2 RandomOnCircleAt(const RandomKeyed* random, RandomKey key) { return RandomBitsOnCircle(RandomU64At(random, key)); }
	inline v2 RandomInCircleNext(RandomStream* random) { return RandomBitsInCircle(RandomU64Next(random)); }
	inline v2 RandomInCircleAt(const RandomKeyed* random, RandomKey key) { return RandomBitsInCircle(RandomU64At(random, key)); }
	inline v3 RandomOnSphereNext(RandomStream* random) { return RandomBitsOnSphere(RandomU64Next(random)); }
	inline v3 RandomOnSphereAt(const RandomKeyed* random, RandomKey key) { return RandomBitsOnSphere(RandomU64At(random, key)); }
	inline v3 RandomInSphereNext(RandomStream* random) { return RandomBitsInSphere(RandomU64Next(random)); }
	inline v3 RandomInSphereAt(const RandomKeyed* random, RandomKey key) { return RandomBitsInSphere(RandomU64At(random, key)); }

	inline f64 RandomGaussianF64Next(RandomStream* random, f64 mean, f64 deviation) { return RandomBitsGaussianF64(RandomU64Next(random), mean, deviation); }
	inline f64 RandomGaussianF64At(const RandomKeyed* random, RandomKey key, f64 mean, f64 deviation) { return RandomBitsGaussianF64(RandomU64At(random, key), mean, deviation); }
	inline f32 RandomGaussianF32Next(RandomStream* random, f32 mean, f32 deviation) { return RandomBitsGaussianF32(RandomU64Next(random), mean, deviation); }
	inline f32 RandomGaussianF32At(const RandomKeyed* random, RandomKey key, f32 mean, f32 deviation) { return RandomBitsGaussianF32(RandomU64At(random, key), mean, deviation); }

	// The stream form draws once per swap; the keyed form stretches its word once per swap.
	template<typename T>
	void RandomShuffleNext(RandomStream* random, T* items, u64 count)
	{
		for (u64 i = count; i > 1; i--)
		{
			u64 j = RandomBitsU64(RandomU64Next(random), 0, i);
			T swap = items[i - 1];
			items[i - 1] = items[j];
			items[j] = swap;
		}
	}

	template<typename T>
	void RandomShuffleAt(const RandomKeyed* random, RandomKey key, T* items, u64 count)
	{
		u64 bits = RandomU64At(random, key);
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
