#ifndef _TOOL_KEY_H
#define _TOOL_KEY_H

#include "basics.h"
#include "vector.h"



//~ Packed keys
//
// Key packs an ID, up to four signed coordinates, or a point, plus an optional index, into four 32-bit slots in
// two words, without hashing. The packing is lossless, so two keys are equal exactly when their inputs are. Keyed
// random getters take one, and so do hash maps of packed keys.
//
//     b8 tree = RandomB8At(&keyed, Key(x, y));
//     Chunk* chunk = HashMapFind<Chunk>(&chunks, Key(cell));



namespace Tool
{
	struct PackedKey
	{
		u64 low;
		u64 high;
	};

	// Coordinates fill the slots in order and missing ones are 0, so Key(x, y) equals Key(x, y, 0). A 64-bit ID
	// fills the first two slots. There is no float overload, so a float argument is ambiguous and doesn't compile.
	//
	// unsigned long, which size_t is on 64-bit Linux and Mac, keys as the u64 of the same value, which matches the u32
	// key wherever the value fits both. Signed long is deleted: it is 32 bits on Windows and 64 elsewhere, and a
	// negative value keys differently at the two widths. Cast it to i32 or i64.
	constexpr PackedKey Key(i64 id);
	constexpr PackedKey Key(u64 id);
	constexpr PackedKey Key(unsigned long id);
	PackedKey Key(long id) = delete;
	constexpr PackedKey Key(u64 id, i32 index);
	constexpr PackedKey Key(i32 x);
	constexpr PackedKey Key(u32 x);
	constexpr PackedKey Key(i32 x, i32 y);
	constexpr PackedKey Key(i32 x, i32 y, i32 z);
	constexpr PackedKey Key(i32 x, i32 y, i32 z, i32 w);
	inline PackedKey Key(p2 point);
	inline PackedKey Key(p2 point, i32 index);
	inline PackedKey Key(p3 point);
	inline PackedKey Key(p3 point, i32 index);
	inline PackedKey Key(p4 point);

	constexpr b8 operator==(PackedKey a, PackedKey b);
	constexpr b8 operator!=(PackedKey a, PackedKey b);
} //namespace Tool



//- Implementation

namespace Tool
{
	constexpr PackedKey Key(i64 id) { return { (u64)id, 0 }; }
	constexpr PackedKey Key(u64 id) { return { id, 0 }; }
	constexpr PackedKey Key(unsigned long id) { return Key((u64)id); }
	constexpr PackedKey Key(u64 id, i32 index) { return { id, (u32)index }; }
	constexpr PackedKey Key(i32 x) { return { (u32)x, 0 }; }
	constexpr PackedKey Key(u32 x) { return { x, 0 }; }
	constexpr PackedKey Key(i32 x, i32 y) { return { (u32)x | ((u64)(u32)y << 32), 0 }; }
	constexpr PackedKey Key(i32 x, i32 y, i32 z) { return { (u32)x | ((u64)(u32)y << 32), (u32)z }; }
	constexpr PackedKey Key(i32 x, i32 y, i32 z, i32 w) { return { (u32)x | ((u64)(u32)y << 32), (u32)z | ((u64)(u32)w << 32) }; }
	inline PackedKey Key(p2 point) { return Key(point.x, point.y); }
	inline PackedKey Key(p2 point, i32 index) { return Key(point.x, point.y, index); }
	inline PackedKey Key(p3 point) { return Key(point.x, point.y, point.z); }
	inline PackedKey Key(p3 point, i32 index) { return Key(point.x, point.y, point.z, index); }
	inline PackedKey Key(p4 point) { return Key(point.x, point.y, point.z, point.w); }

	constexpr b8 operator==(PackedKey a, PackedKey b) { return a.low == b.low && a.high == b.high; }
	constexpr b8 operator!=(PackedKey a, PackedKey b) { return !(a == b); }
} //namespace Tool



#endif //_TOOL_KEY_H
