#include "tool.h"
#include "test.h"
#include <stdio.h>
#include <algorithm>
#include <limits>
#include <vector>

using namespace Tool;



//~ Inputs

// The orders sorts are known to stumble on, besides random ones
enum SortInput
{
	SortInputRandom,     // Uniform over all u32
	SortInputFewValues,  // Many duplicates: values 0..7
	SortInputSorted,     // Already in order
	SortInputReversed,   // In reverse order
	SortInputEqual,      // One value throughout
	SortInputOrganPipe,  // Rising, then falling
	SortInputSawtooth,   // Rising runs of 32

	SortInputCount, // <-- Keep last!
};

static void SortInputMake(SortInput input, u32* values, u64 count, RandomStream* random)
{
	for (u64 i = 0; i < count; i++)
	{
		switch (input)
		{
		case SortInputRandom:     values[i] = RandomU32Next(random); break;
		case SortInputFewValues:  values[i] = RandomU32Next(random) & 7; break;
		case SortInputSorted:     values[i] = (u32)i; break;
		case SortInputReversed:   values[i] = (u32)(count - i); break;
		case SortInputEqual:      values[i] = 42; break;
		case SortInputOrganPipe:  values[i] = (u32)(i < count / 2 ? i : count - i); break;
		case SortInputSawtooth:   values[i] = (u32)(i % 32); break;
		default: break;
		}
	}
}

static const u64 sortCounts[] = { 0, 1, 2, 3, 15, 16, 17, 31, 100, 1000, 4097, 100000 };



//~ Sort

static void TestSortValues()
{
	RandomStream random = {};
	RandomStreamInit(&random, 0x5047);

	for (u32 input = 0; input < SortInputCount; input++)
	{
		for (u64 count : sortCounts)
		{
			std::vector<u32> values(count + 1);
			SortInputMake((SortInput)input, values.data(), count, &random);
			std::vector<u32> expected(values.begin(), values.begin() + count);
			std::sort(expected.begin(), expected.end());

			Sort(values.data(), count, [](const u32& a, const u32& b) { return a < b; });
			for (u64 i = 0; i < count; i++)
				TOOL_ASSERT(values[i] == expected[i]);
		}
	}
}

// Every item kept, in the order asked, for items whose order is not all of them
static void TestSortRecords()
{
	struct Record
	{
		u32 key;
		u32 payload;
	};

	RandomStream random = {};
	RandomStreamInit(&random, 0x5048);

	const u64 count = 5000;
	std::vector<Record> records(count);
	u64 payloadSum = 0;
	for (u64 i = 0; i < count; i++)
	{
		records[i] = { RandomU32Next(&random) % 100, (u32)i };
		payloadSum += i;
	}

	// Descending, to sort by something other than <
	Sort(records.data(), count, [](const Record& a, const Record& b) { return a.key > b.key; });

	u64 sum = 0;
	for (u64 i = 0; i < count; i++)
	{
		TOOL_ASSERT(i == 0 || records[i - 1].key >= records[i].key);
		sum += records[i].payload;
	}

	TOOL_ASSERT(sum == payloadSum);
}

// The heapsort fallback, on its own, since a quicksort with a median of three rarely falls back to it
static void TestSortHeap()
{
	RandomStream random = {};
	RandomStreamInit(&random, 0x5049);

	for (u64 count : sortCounts)
	{
		std::vector<u32> values(count + 1);
		SortInputMake(SortInputRandom, values.data(), count, &random);
		std::vector<u32> expected(values.begin(), values.begin() + count);
		std::sort(expected.begin(), expected.end());

		auto less = [](const u32& a, const u32& b) { return a < b; };
		if (count > 1)
			SortHeap(values.data(), count, less);

		for (u64 i = 0; i < count; i++)
			TOOL_ASSERT(values[i] == expected[i]);
	}
}

// Comparisons stay within O(n log n) on an input made to defeat a median of three
static void TestSortBound()
{
	const u64 count = 1 << 16;
	std::vector<u32> values(count);

	// Musser's median-of-3 killer
	u64 half = count / 2;
	for (u64 i = 0; i < half; i++)
	{
		values[i] = (u32)(i % 2 == 0 ? i + 1 : half + i);
		values[half + i] = (u32)(2 * (i + 1));
	}

	u64 comparisons = 0;
	Sort(values.data(), count, [&comparisons](const u32& a, const u32& b) { comparisons++; return a < b; });

	for (u64 i = 1; i < count; i++)
		TOOL_ASSERT(values[i - 1] <= values[i]);

	// 16 levels: quicksort's 2n log n with room to spare, far below the n^2 / 4 of a quadratic run
	TOOL_ASSERT(comparisons < 8 * count * 16);
}



//~ SortByKey

static void TestSortByKeyStable()
{
	struct Item
	{
		u64 key;
		u64 order;
	};

	RandomStream random = {};
	RandomStreamInit(&random, 0x504A);

	// Keys spread over every byte, keys sharing their upper bytes, and one key throughout, which sorts nothing
	const u64 masks[] = { 0xFFFFFFFFFFFFFFFFull, 0xFFull, 0x0F0F000000000000ull, 0 };

	for (u64 mask : masks)
	{
		for (u64 count : sortCounts)
		{
			std::vector<Item> items(count + 1);
			for (u64 i = 0; i < count; i++)
				items[i] = { RandomU64Next(&random) & mask, i };

			std::vector<Item> expected(items.begin(), items.begin() + count);
			std::stable_sort(expected.begin(), expected.end(), [](const Item& a, const Item& b) { return a.key < b.key; });

			b8 sorted = SortByKey(items.data(), count, [](const Item& item) { return item.key; }, Allocator());
			TOOL_ASSERT(sorted);

			for (u64 i = 0; i < count; i++)
				TOOL_ASSERT(items[i].key == expected[i].key && items[i].order == expected[i].order);
		}
	}
}

// Memory from an arena, which takes nothing back, and items bigger than their keys
static void TestSortByKeyArena()
{
	struct Wide
	{
		u32 key;
		u8 bytes[60];
	};

	Arena arena = {};
	ArenaInit(&arena, Allocator(), 4096, "Sort test");

	RandomStream random = {};
	RandomStreamInit(&random, 0x504B);

	const u64 count = 3000;
	std::vector<Wide> items(count);
	for (u64 i = 0; i < count; i++)
	{
		items[i].key = RandomU32Next(&random);
		for (u32 b = 0; b < 60; b++)
			items[i].bytes[b] = (u8)(items[i].key + b);
	}

	TOOL_ASSERT(SortByKey(items.data(), count, [](const Wide& item) { return (u64)item.key; }, Allocator(&arena)));

	for (u64 i = 0; i < count; i++)
	{
		TOOL_ASSERT(i == 0 || items[i - 1].key <= items[i].key);
		for (u32 b = 0; b < 60; b++)
			TOOL_ASSERT(items[i].bytes[b] == (u8)(items[i].key + b));
	}

	ArenaDeInit(&arena);
}



//~ Keys

static_assert(SortKeyI64(-1) < SortKeyI64(0));
static_assert(SortKeyI64(I64_MIN) < SortKeyI64(-1));
static_assert(SortKeyI64(0) < SortKeyI64(I64_MAX));
static_assert(SortKeyF32(-1.0f) < SortKeyF32(-0.0f) && SortKeyF32(-0.0f) < SortKeyF32(0.0f));
static_assert(SortKeyF64(0.5) < SortKeyF64(2.0));

static void TestSortKeys()
{
	const f32 infinity32 = std::numeric_limits<f32>::infinity();
	const f32 floats[] = { -infinity32, -3.0e38f, -1.0f, -1.0e-40f, -0.0f, 0.0f, 1.0e-40f, 1.0f, 3.0e38f, infinity32 };
	for (u32 i = 1; i < sizeof(floats) / sizeof(floats[0]); i++)
		TOOL_ASSERT(SortKeyF32(floats[i - 1]) < SortKeyF32(floats[i]));

	const f64 infinity64 = std::numeric_limits<f64>::infinity();
	const f64 doubles[] = { -infinity64, -1.0e300, -1.0, -1.0e-320, -0.0, 0.0, 1.0e-320, 1.0, 1.0e300, infinity64 };
	for (u32 i = 1; i < sizeof(doubles) / sizeof(doubles[0]); i++)
		TOOL_ASSERT(SortKeyF64(doubles[i - 1]) < SortKeyF64(doubles[i]));

	// Depths sorted through their keys come out in the order of the values
	RandomStream random = {};
	RandomStreamInit(&random, 0x504C);

	const u64 count = 1000;
	std::vector<f32> depths(count);
	for (u64 i = 0; i < count; i++)
		depths[i] = RandomF32Next(&random, -100.0f, 100.0f);

	TOOL_ASSERT(SortByKey(depths.data(), count, [](const f32& depth) { return SortKeyF32(depth); }, Allocator()));
	for (u64 i = 1; i < count; i++)
		TOOL_ASSERT(depths[i - 1] <= depths[i]);
}



void TestSort()
{
	TestSortValues();
	TestSortRecords();
	TestSortHeap();
	TestSortBound();
	TestSortByKeyStable();
	TestSortByKeyArena();
	TestSortKeys();

	printf("Sort: passed\n");
}
