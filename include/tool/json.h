#ifndef _TOOL_JSON_H
#define _TOOL_JSON_H

#include "basics.h"
#include "memory.h"
#include "text.h"



//~ JSON
//
// Reads RFC 8259 text into a tree, strictly: one value, nothing after it. Everything is copied into the allocator,
// nothing per value is freed, so give it an arena.
//
// Arena arena = {};
// ArenaInit(&arena, Allocator());
//
// const JsonValue* root = JsonParse(text, Allocator(&arena));
// if (root != nullptr)
// {
//     const JsonValue* name = JsonFind(root, S8("name"));
//     for (const JsonValue* item = JsonFind(root, S8("items"))->first; item != nullptr; item = item->next)
//         ...
// }
//
// ArenaDeInit(&arena);



//~ Definitions

// Deepest nesting JsonParse takes
#define TOOL_JSON_DEPTH_MAX 128



namespace Tool
{
	//- Types

	enum JsonType
	{
		JsonTypeNull,   // null
		JsonTypeBool,   // true or false, in 'boolean'
		JsonTypeNumber, // In 'number', and as written in 'text', which keeps what an f64 can't
		JsonTypeString, // Decoded, in 'text'
		JsonTypeArray,  // Elements from 'first', in order
		JsonTypeObject, // Members from 'first', in order, repeated keys kept
	};

	struct JsonValue
	{
		JsonType type;
		b8 boolean;
		f64 number;
		s8 text;

		s8 key;          // Within an object
		JsonValue* first; // Of an array or object
		JsonValue* next;  // Within an array or object
		u64 count;        // Children of an array or object
	};



	//- Functions

	// The tree, or null with the reason and byte offset recorded
	const JsonValue* JsonParse(s8 text, MemoryAllocator allocator);

	// The first member named 'key'. Null if there's none, or 'object' is null or isn't an object.
	const JsonValue* JsonFind(const JsonValue* object, s8 key);

	// 'text' as the inside of a JSON string, without its quotes. UTF-8 passes through.
	s8 JsonEscape(s8 text, MemoryAllocator allocator);
}

#endif //_TOOL_JSON_H
