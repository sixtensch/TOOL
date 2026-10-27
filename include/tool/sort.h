#ifndef _TOOL_SORT_H
#define _TOOL_SORT_H

#include "basics.h"
#include "error.h"
#include "memory.h"



//~ Sorting
//
// - Sort: comparison sort, in place, unstable, O(n log n) worst case.
// - SortByKey: stable radix sort by a u64 key per item, O(n), 32 bytes of scratch per item. Wins for many items with
//   integer keys (ids, depths).
//
//     Sort(entries, count, [](const Entry& a, const Entry& b) { return a.id < b.id; });
//     SortByKey(entries, count, [](const Entry& entry) { return entry.id; }, Allocator());
//
// Items move by copy and assignment. Pack multi-field keys most significant first: ((u64)layer << 32) | order. Wider
// keys: one SortByKey pass per part, least significant first. Signed and float values: SortKeyI64/F32/F64.



namespace Tool
{
	//~ Sorting

	// 'less' is b8(const T& a, const T& b), a strict weak ordering
	template<typename T, typename Less>
	void Sort(T* items, u64 count, Less less);

	// 'key' is u64(const T& item), called once per item. False if 'allocator' fails, items untouched.
	template<typename T, typename Key>
	b8 SortByKey(T* items, u64 count, Key key, MemoryAllocator allocator);

	//~ Keys

	// Order-preserving u64 keys. Floats: -0 before +0, NaNs beyond the infinities on their sign's side.
	constexpr u64 SortKeyI64(i64 value);
	constexpr u64 SortKeyF32(f32 value);
	constexpr u64 SortKeyF64(f64 value);

} //namespace Tool



//- Implementation
//
// Sort: introsort. Median-of-three Hoare partition, recursion on the smaller side (O(log n) stack), heapsort past depth
// 2 log2(n), insertion sort at 16 or fewer. SortByKey: (key, index) pairs through an untyped LSD radix sort
// (SortPairsByKey, sort.cpp) that skips bytes all keys share, then the permutation applied in place cycle by cycle.

namespace Tool
{
	//~ Building blocks

	constexpr u64 sortInsertionMax = 16;

	struct SortPair
	{
		u64 key;
		u64 index;
	};

	// Stable. 'scratch' holds 'count' pairs. Returns whichever buffer ends up sorted.
	SortPair* SortPairsByKey(SortPair* pairs, SortPair* scratch, u64 count);

	template<typename T>
	inline void SortSwap(T& a, T& b)
	{
		T held = a;
		a = b;
		b = held;
	}

	template<typename T, typename Less>
	inline void SortInsertion(T* items, u64 count, Less& less)
	{
		for (u64 i = 1; i < count; i++)
		{
			T moving = items[i];
			u64 j = i;
			for (; j > 0 && less(moving, items[j - 1]); j--)
				items[j] = items[j - 1];

			items[j] = moving;
		}
	}

	template<typename T, typename Less>
	inline void SortSiftDown(T* items, u64 root, u64 count, Less& less)
	{
		for (;;)
		{
			u64 child = 2 * root + 1;
			if (child >= count)
				return;

			if (child + 1 < count && less(items[child], items[child + 1]))
				child++;

			if (!less(items[root], items[child]))
				return;

			SortSwap(items[root], items[child]);
			root = child;
		}
	}

	template<typename T, typename Less>
	inline void SortHeap(T* items, u64 count, Less& less)
	{
		for (u64 i = count / 2; i-- > 0;)
			SortSiftDown(items, i, count, less);

		for (u64 end = count - 1; end > 0; end--)
		{
			SortSwap(items[0], items[end]);
			SortSiftDown(items, 0, end, less);
		}
	}

	template<typename T, typename Less>
	inline void SortIntro(T* items, u64 count, u32 depth, Less& less)
	{
		while (count > sortInsertionMax)
		{
			if (depth == 0)
			{
				SortHeap(items, count, less);
				return;
			}

			depth--;

			// Median of three, which also leaves sentinels at both ends for the scans below
			u64 middle = count / 2;
			if (less(items[middle], items[0]))
				SortSwap(items[middle], items[0]);
			if (less(items[count - 1], items[middle]))
			{
				SortSwap(items[count - 1], items[middle]);
				if (less(items[middle], items[0]))
					SortSwap(items[middle], items[0]);
			}

			T pivot = items[middle];
			i64 i = -1;
			i64 j = (i64)count;
			for (;;)
			{
				do
					i++;
				while (less(items[i], pivot));

				do
					j--;
				while (less(pivot, items[j]));

				if (i >= j)
					break;

				SortSwap(items[i], items[j]);
			}

			// [0, j] and [j + 1, count), neither empty
			u64 left = (u64)j + 1;
			u64 right = count - left;
			if (left < right)
			{
				SortIntro(items, left, depth, less);
				items += left;
				count = right;
			}
			else
			{
				SortIntro(items + left, right, depth, less);
				count = left;
			}
		}

		SortInsertion(items, count, less);
	}

	//~ Sorting

	template<typename T, typename Less>
	void Sort(T* items, u64 count, Less less)
	{
		if (count < 2)
			return;

		u32 depth = 0;
		for (u64 n = count; n > 1; n >>= 1)
			depth += 2;

		SortIntro(items, count, depth, less);
	}

	template<typename T, typename Key>
	b8 SortByKey(T* items, u64 count, Key key, MemoryAllocator allocator)
	{
		if (count < 2)
			return true;

		SortPair* pairs = (SortPair*)AllocatorAlloc(allocator, 2 * count, sizeof(SortPair));
		if (pairs == nullptr)
			return TOOL_FAIL("Could not allocate %llu bytes to sort %llu items.", 2 * count * sizeof(SortPair), count);

		for (u64 i = 0; i < count; i++)
			pairs[i] = { (u64)key(items[i]), i };

		SortPair* sorted = SortPairsByKey(pairs, pairs + count, count);

		// Item j takes the one at sorted[j].index. Each cycle is walked once, holding one item aside; finished places
		// point at themselves.
		for (u64 i = 0; i < count; i++)
		{
			if (sorted[i].index == i)
				continue;

			T held = items[i];
			u64 j = i;
			for (;;)
			{
				u64 from = sorted[j].index;
				sorted[j].index = j;
				if (from == i)
				{
					items[j] = held;
					break;
				}

				items[j] = items[from];
				j = from;
			}
		}

		AllocatorDealloc(allocator, pairs);
		return true;
	}

	//~ Keys

	constexpr u64 SortKeyI64(i64 value)
	{
		return (u64)value ^ 0x8000000000000000ull;
	}

	constexpr u64 SortKeyF32(f32 value)
	{
		u32 bits = __builtin_bit_cast(u32, value);

		// Negatives: flip all bits. Positives: flip the sign bit.
		u32 mask = (bits & 0x80000000u) != 0 ? 0xFFFFFFFFu : 0x80000000u;
		return (u64)(bits ^ mask);
	}

	constexpr u64 SortKeyF64(f64 value)
	{
		u64 bits = __builtin_bit_cast(u64, value);

		u64 mask = (bits & 0x8000000000000000ull) != 0 ? 0xFFFFFFFFFFFFFFFFull : 0x8000000000000000ull;
		return bits ^ mask;
	}

} //namespace Tool



#endif //_TOOL_SORT_H
