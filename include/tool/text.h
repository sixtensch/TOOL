#ifndef _TOOL_TEXT_H
#define _TOOL_TEXT_H

#include "basics.h"
#include "memory.h"



//~ Text
//
// An s8 is a view: a pointer and a size in bytes, owning nothing and not necessarily zero-terminated. Functions
// that make a new string take an allocator and always add a terminator after the contents, so .str also works as
// a C string. Slices of a string are views into it. Contents are UTF-8 by convention, and nothing validates them.
//
// An s16 exists to talk to Windows. Its size counts 16-bit units.
//
// Numbers are formatted and parsed with '.' as the decimal point whatever the locale, so text written on one
// machine reads back on another.



//- Macros

#ifndef TOOL_NO_UTF
#define UTF8(str) ((const c8*)(u8##str))
#define UTF16(str) ((const c16*)(u##str))
#endif

// Returned by the Find functions when nothing matches.
#define S8_NONE U64_MAX

// Lets GCC and Clang check printf-style arguments against the format.
#if defined(__GNUC__) || defined(__clang__)
#define TOOL_PRINTF_FORMAT(formatIndex, argumentIndex) __attribute__((format(printf, formatIndex, argumentIndex)))
#else
#define TOOL_PRINTF_FORMAT(formatIndex, argumentIndex)
#endif



namespace Tool
{
	//- Types

	struct String8
	{
		c8* str;
		u64 size; // Bytes, not counting a terminator
	};

	struct String16
	{
		c16* str;
		u64 size; // 16-bit units, not counting a terminator
	};

	typedef String8 s8;
	typedef String16 s16;

	enum StringType
	{
		StringTypeNone = 0, // Uninitialized
		StringTypeUTF8,     // Built as c8
		StringTypeUTF16,    // Built as c16
	};

	// Fixed-capacity string assembly. The whole buffer is allocated at init and never grows. The contents are always
	// terminated.
	struct StringBuilder
	{
		StringType type;
		u64 size;     // Of the contents, in bytes
		u64 capacity; // Of the buffer, in bytes, terminator included

		union
		{
			c8* str8;
			c16* str16;
		};
	};



	//- Functions

	//~ Characters
	// ASCII only: other bytes, UTF-8 included, are never spaces, digits or letters, and keep their case.

	inline b8 C8IsSpace(c8 c); // Space, tab, newline, carriage return, vertical tab, form feed
	inline b8 C8IsDigit(c8 c);
	inline b8 C8IsHexDigit(c8 c);
	inline b8 C8IsAlpha(c8 c);
	inline b8 C8IsAlphaNumeric(c8 c);
	inline c8 C8ToLower(c8 c);
	inline c8 C8ToUpper(c8 c);

	//~ Making strings

	constexpr s8 S8(const c8* cstr); // A view of a C string. For literals, the size is worked out at compile time
	s8 S8Copy(s8 string, MemoryAllocator allocator);
	s8 S8Concat(s8 a, s8 b, MemoryAllocator allocator);
	s8 S8Format(MemoryAllocator allocator, const c8* format, ...) TOOL_PRINTF_FORMAT(2, 3); // printf's rules

	//~ Comparing and searching

	b8 S8Equal(s8 a, s8 b);
	b8 S8EqualIgnoreCase(s8 a, s8 b); // ASCII letters only
	i32 S8Compare(s8 a, s8 b);        // Bytewise: negative, 0 or positive, for sorting
	b8 S8StartsWith(s8 string, s8 prefix);
	b8 S8EndsWith(s8 string, s8 suffix);
	u64 S8Find(s8 string, s8 needle, u64 start = 0); // The index of the first match from 'start', or S8_NONE
	u64 S8FindLast(s8 string, s8 needle);            // The index of the last match, or S8_NONE

	//~ Slicing
	// Views into the string. Ranges are clamped to it.

	s8 S8Slice(s8 string, u64 start, u64 end); // From 'start' up to, not including, 'end'
	s8 S8Trim(s8 string);                      // Without ASCII whitespace at either end
	s8 S8TrimStart(s8 string);
	s8 S8TrimEnd(s8 string);

	// Splits at the first 'separator'. False, with all of 'string' in 'outBefore' and nothing in 'outAfter', if
	// there's none. Either output may be null. Repeat on 'outAfter' to split a whole list.
	b8 S8Cut(s8 string, s8 separator, s8* outBefore, s8* outAfter);

	//~ Changing case
	// ASCII letters only, so the size never changes.

	s8 S8ToLower(s8 string, MemoryAllocator allocator);
	s8 S8ToUpper(s8 string, MemoryAllocator allocator);

	//~ Numbers
	// The whole string must be the number, with no spaces around it. False if it isn't, or if it doesn't fit.

	b8 S8ParseU64(s8 string, u64* outValue); // Decimal, or hexadecimal after 0x
	b8 S8ParseI64(s8 string, i64* outValue); // An optional sign, then as S8ParseU64
	b8 S8ParseF64(s8 string, f64* outValue); // As strtod: decimal, exponents, inf and nan

	//~ Code points

	// The code point starting at *index, which moves past it. Invalid or cut-off sequences give U+FFFD and move
	// past at least one byte. *index must be within the string.
	u32 S8DecodeCodepoint(s8 string, u64* index);
	u32 CodepointEncodeUTF8(u32 codepoint, c8* outBytes); // Writes 1 to 4 bytes and returns how many
	u64 S8CodepointCount(s8 string);
	b8 S8IsValidUTF8(s8 string);

	//~ UTF-16
	// Invalid input converts to U+FFFD.

	s16 S16FromS8(s8 string, MemoryAllocator allocator);
	s8 S8FromS16(s16 string, MemoryAllocator allocator);

	// Into a buffer with room for 'capacity' units, terminator included. Stops before a character that doesn't
	// fit, always terminates when 'capacity' isn't 0, and returns the units written without the terminator.
	u64 S8ToUTF16(s8 string, c16* destination, u64 capacity);
	u64 S16ToUTF8(s16 string, c8* destination, u64 capacity);

	//~ C strings
	// Copies and formatting take the destination's capacity, terminator included. They never cut a UTF-8 or
	// UTF-16 character in half, always terminate when 'capacity' isn't 0, and return what they wrote without the
	// terminator.

	u64 CStr8Size(const c8* string);   // Bytes before the terminator
	u64 CStr16Count(const c16* string); // Units before the terminator
	u64 CStr8Copy(c8* destination, const c8* source, u64 capacity);
	u64 CStr16Copy(c16* destination, const c16* source, u64 capacity);
	u64 CStr8Format(c8* destination, u64 capacity, const c8* format, ...) TOOL_PRINTF_FORMAT(3, 4); // printf's rules

	//~ String builder

	// Allocates 'capacity' bytes up front, terminator included. Returns false on failure.
	b8 StringBuilderInit(StringBuilder* builder, u64 capacity, StringType type = StringTypeUTF8);
	void StringBuilderReset(StringBuilder* builder);
	void StringBuilderDestroy(StringBuilder* builder);

	// Appends, converting to the builder's type. Returns false, leaving the contents unchanged, if the result
	// wouldn't fit.
	b8 StringBuilderAdd(StringBuilder* builder, s8 string);
	b8 StringBuilderAdd(StringBuilder* builder, s16 string);
	b8 StringBuilderAdd(StringBuilder* builder, const c8* cstr);
	b8 StringBuilderAdd(StringBuilder* builder, const c16* cstr);
} //namespace Tool



//- Implementation

namespace Tool
{
	//~ Characters

	inline b8 C8IsSpace(c8 c)
	{
		return c == ' ' || (c >= '\t' && c <= '\r');
	}

	inline b8 C8IsDigit(c8 c)
	{
		return c >= '0' && c <= '9';
	}

	inline b8 C8IsHexDigit(c8 c)
	{
		return C8IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
	}

	inline b8 C8IsAlpha(c8 c)
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}

	inline b8 C8IsAlphaNumeric(c8 c)
	{
		return C8IsAlpha(c) || C8IsDigit(c);
	}

	inline c8 C8ToLower(c8 c)
	{
		return (c >= 'A' && c <= 'Z') ? (c8)(c + ('a' - 'A')) : c;
	}

	inline c8 C8ToUpper(c8 c)
	{
		return (c >= 'a' && c <= 'z') ? (c8)(c - ('a' - 'A')) : c;
	}

	//~ Making strings

	constexpr s8 S8(const c8* cstr)
	{
		u64 size = 0;
		if (cstr != nullptr)
			while (cstr[size] != 0)
				size++;

		return { (c8*)cstr, size };
	}
} //namespace Tool



//~ Acronym usings

#ifndef TOOL_NO_ACRONYMS

using Tool::s8;
using Tool::s16;

#endif



#endif //_TOOL_TEXT_H
