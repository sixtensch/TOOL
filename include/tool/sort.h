#ifndef _TOOL_SORT_H
#define _TOOL_SORT_H

#include "basics.h"
#include "error.h"
#include "memory.h"



//~ Sorting
//
// Two sorts, for two kinds of work:
// - Sort orders items by comparing them, in place, with no memory of its own. Not stable: items that compare equal
//   may end up in any order. Never slower than O(n log n), whatever order the items come in.
// - SortByKey orders items by a u64 key each, smallest first, and is stable: items with equal keys keep their order.
//   Its time grows with the count alone, which wins for many items with integer keys, such as ids or depths. It needs
//   memory for two keys and two indices per item while it runs.
//
//     Sort(entries, count, [](const Entry& a, const Entry& b) { return a.id < b.id; });
//     SortByKey(entries, count, [](const Entry& entry) { return entry.id; }, Allocator());
//
// Both move items as plain values, by copy and assignment, so they suit plain structs.
//
// A key of several fields packs the first to sort by highest: ((u64)layer << 32) | (u64)order. A key wider than 64
// bits is sorted a part at a time with SortByKey, least significant part first, as each pass keeps the order of the
// one before. Signed and float values become keys of the same order through SortKeyI64, SortKeyF32 and SortKeyF64.



namespace Tool
{
	//~ Sorting

	// Order 'items' so that less(b, a) holds for no item 'a' before an item 'b'. 'less' is anything callable as
	// b8 less(const T& a, const T& b), such as a lambda, and must be a strict ordering: less(a, a) is false.
	// Quicksort, turning to heapsort when its partitions keep coming out uneven, and to insertion sort for short runs.
	template<typename T, typename Less>
	void Sort(T* items, u64 count, Less less);

	// Order 'items' by key, smallest first, keeping the order of items with equal keys. 'key' is anything callable as
	// u64 key(const T& item), called once per item. A radix sort, a byte at a time, skipping bytes every key shares.
	// Its memory, 32 bytes per item, comes from 'allocator' and goes back to it before returning. False if it can't
	// be had, which is recorded, with the items left as they were.
	template<typename T, typename Key>
	b8 SortByKey(T* items, u64 count, Key key, MemoryAllocator allocator);

	//~ Keys

	// A key whose order as a u64 is the value's own: the smallest value has the smallest key. For floats, -0 comes
	// just before +0, and NaN after +infinity, or before -infinity when its sign is set.
	constexpr u64 SortKeyI64(i64 value);
	constexpr u64 SortKeyF32(f32 value);
	constexpr u64 SortKeyF64(f64 value);

} //namespace Tool



//- Implementation
//
// Sort is an introsort: median-of-three quicksort with a Hoare partition, recursing into the smaller side and
// looping on the larger so the stack stays O(log n), with heapsort past a depth of 2 log2(n) and insertion sort
// below 16 items. SortByKey pairs each item's key with its index, sorts the pairs in an untyped least significant
// digit radix sort (SortPairsByKey, sort.cpp), then moves the items into place by following the permutation's
// cycles, so the items themselves are never copied aside.

namespace Tool
{
	//~ Building blocks

	// Runs this short or shorter are finished by insertion sort
	constexpr u64 sortInsertionMax = 16;

	struct SortPair
	{
		u64 key;
		u64 index;
	};

	// Sort 'pairs' by key, keeping the order of equal keys. 'scratch' holds as many pairs. Returns whichever of the
	// two holds the sorted pairs.
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

			// The median of the first, middle and last, which also leaves an item no greater than the pivot first and
			// one no less last, so neither scan below runs past the ends
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

		// Item j takes the one at sorted[j].index. Each cycle of that permutation is followed once, from its first
		// place, holding only that place's item aside; places done are marked by pointing at themselves.
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

		// Positive values gain the sign bit, so they follow the negative ones, whose bits all flip, so the largest
		// magnitude comes first
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
