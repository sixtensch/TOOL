#include "sort.h"



namespace Tool
{
	//~ Radix sort

	SortPair* SortPairsByKey(SortPair* pairs, SortPair* scratch, u64 count)
	{
		if (count == 0)
			return pairs;

		// How many keys hold each value of each byte, counted for all eight bytes in one pass
		u64 counts[8][256] = {};
		for (u64 i = 0; i < count; i++)
		{
			u64 key = pairs[i].key;
			for (u32 b = 0; b < 8; b++)
				counts[b][(key >> (8 * b)) & 0xFF]++;
		}

		// Least significant byte first, each pass keeping the order of the one before
		SortPair* from = pairs;
		SortPair* to = scratch;
		for (u32 b = 0; b < 8; b++)
		{
			u32 shift = 8 * b;

			// A byte every key shares moves nothing
			if (counts[b][(from[0].key >> shift) & 0xFF] == count)
				continue;

			u64 offsets[256];
			u64 offset = 0;
			for (u32 v = 0; v < 256; v++)
			{
				offsets[v] = offset;
				offset += counts[b][v];
			}

			for (u64 i = 0; i < count; i++)
				to[offsets[(from[i].key >> shift) & 0xFF]++] = from[i];

			SortPair* swap = from;
			from = to;
			to = swap;
		}

		return from;
	}

} //namespace Tool
