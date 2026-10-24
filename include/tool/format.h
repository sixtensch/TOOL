#ifndef _TOOL_FORMAT_H
#define _TOOL_FORMAT_H

#include "basics.h"
#include "memory.h"
#include "text.h"



//~ Formatting
//
// Type-safe formatting in std::format's syntax. "{}" takes the next argument and "{1}" a given one, and a spec after
// ':' sets fill and alignment, sign, '#', '0', width, precision and type as std::format defines them: "{:>8.2f}".
// Width and precision can come from arguments: "{:{}.{}f}". "{{" and "}}" write braces. Left out: locale ('L'),
// debug strings ('?'), and display widths, as width counts code points.
//
// Opt-in: not part of tool.h. Format strings are literals, checked against the argument types at compile time
// unless TOOL_NO_FORMAT_VALIDATION is set. Strings made at runtime go through the *Args functions, which check
// as they render.
//
// Formats bool, c8 (as a character), every integer type, unscoped enums (as integers), f32 and f64 (shortest
// round trip by default), s8 and C strings, pointers, and TOOL's vectors, colors and quaternions, whose spec
// applies to each component: "{:.2f}" on a v2 gives "(1.00, 2.00)". Any other type becomes formattable with a
// FormatArgMake overload beside it:
//
//     struct Coin { i32 value; };
//
//     b8 CoinFormat(FormatWriter* writer, const void* value, s8 spec)
//     {
//         FormatWrite(writer, S8("Coin "));
//         return FormatWriteArg(writer, FormatArgMake(((const Coin*)value)->value), spec);
//     }
//
//     inline FormatArg FormatArgMake(const Coin& coin) { return FormatArgCustom(coin, &CoinFormat); }
//
// Captures save a format call to render later, as for logging off the calling thread. FormatCaptureWrite copies
// the arguments, with their strings and custom values, into one block of FormatCaptureSize bytes that can move as
// plain bytes. The block points at its format string and custom functions, so render it before unloading the
// module they live in.



//- Definitions

// A spec's width or precision that comes from no argument
#define TOOL_FORMAT_NO_ARG 0xFFFFFFFFu

// A spec without a precision
#define TOOL_FORMAT_NO_PRECISION 0xFFFFFFFFu

// Alignment of a capture block, and the most a custom value may need
#define TOOL_FORMAT_CAPTURE_ALIGNMENT 16ull



namespace Tool
{
	//- Types

	enum FormatType : u8
	{
		FormatTypeNone,    // No argument
		FormatTypeBool,    // b8
		FormatTypeChar,    // c8, written as a character
		FormatTypeI64,     // Signed integers and unscoped enums, widened
		FormatTypeU64,     // Unsigned integers, widened
		FormatTypeF32,     // f32, whose shortest round trip differs from f64's
		FormatTypeF64,     // f64
		FormatTypeString,  // s8 and C strings
		FormatTypePointer, // Data pointers, written in hexadecimal
		FormatTypeCustom,  // A value written by its own FormatFunction
	};

	enum FormatError : u8
	{
		FormatErrorNone,           // Valid
		FormatErrorUnmatchedOpen,  // A '{' without its '}'
		FormatErrorUnmatchedClose, // A '}' that isn't doubled or closing a field
		FormatErrorArgumentId,     // A malformed argument id
		FormatErrorArgumentRange,  // An argument id past the arguments given
		FormatErrorMixedIndexing,  // Automatic "{}" and numbered "{0}" ids in one format
		FormatErrorSpec,           // A malformed spec
		FormatErrorSpecType,       // A spec option the argument's type doesn't take
		FormatErrorDynamicType,    // A width or precision argument that isn't an integer
		FormatErrorDynamicValue,   // A width or precision argument out of range. Only at runtime
		FormatErrorUnsupported,    // Locale 'L' or debug '?'
		FormatErrorCharacter,      // An integer written as 'c' that isn't a code point. Only at runtime
		FormatErrorCustom,         // A custom formatter refused its spec. Only at runtime
	};

	// A parsed spec: [[fill]align][sign][#][0][width][.precision][type].
	struct FormatSpec
	{
		u32 fill;         // Code point, ' ' by default
		c8 align;         // '<', '^', '>', or 0 for the type's default
		c8 sign;          // '+', '-', ' ', or 0
		c8 type;          // Presentation letter, or 0
		b8 alternate;     // '#'
		b8 zero;          // '0'
		u32 width;        // 0 for none
		u32 widthArg;     // Argument holding the width, or TOOL_FORMAT_NO_ARG
		u32 precision;    // TOOL_FORMAT_NO_PRECISION for none
		u32 precisionArg; // Argument holding the precision, or TOOL_FORMAT_NO_ARG
	};

	// Output being formatted. Bytes past 'capacity' are dropped but counted, so 'size' ends as the full length.
	struct FormatWriter
	{
		c8* destination;
		u64 capacity; // Bytes there is room for
		u64 size;     // Bytes of the whole output so far, those past 'capacity' included
	};

	// Writes a custom value. 'spec' is the text after ':' in its field, empty without one. False if the spec isn't
	// one it takes.
	typedef b8 (*FormatFunction)(FormatWriter* writer, const void* value, s8 spec);

	// One argument, made by FormatArgMake. Strings and custom values are held by pointer, not copied.
	struct FormatArg
	{
		FormatType type;

		union
		{
			b8 boolean;
			c8 character;
			i64 i64Value;
			u64 u64Value;
			f32 f32Value;
			f64 f64Value;
			s8 string;
			const void* pointer;

			struct
			{
				const void* value;
				u32 size;
				u32 alignment;
				FormatFunction function;
			} custom;
		};
	};

	// Header of a captured format call. Its arguments, then copies of their strings and custom values, follow in
	// the same block.
	struct FormatCapture
	{
		u64 size;  // Bytes of the whole block, a multiple of TOOL_FORMAT_CAPTURE_ALIGNMENT
		s8 format; // The format string, by pointer
		u32 count; // Arguments
	};

	// A format string checked against arguments of types A.
	template<typename... A> struct FormatString;

	template<typename T> struct FormatIdentity { using Type = T; };

	// The format string parameter. Keeps the format out of deduction, so the arguments alone decide A.
	template<typename... A> using FormatStringFor = typename FormatIdentity<FormatString<A...>>::Type;

	// Formattable TOOL types, defined in their own headers
	template<typename T> struct Vec2;
	template<typename T> struct Vec3;
	template<typename T> struct Vec4;
	struct Color3U;
	struct Color4U;
	struct Color3F;
	struct Color4F;
	struct Quaternion;



	//- Functions

	//~ Arguments
	// FormatArgMake turns a value into an argument. Overloading it for a type makes the type formattable.

	inline FormatArg FormatArgMake(b8 value);
	inline FormatArg FormatArgMake(c8 value);
	inline FormatArg FormatArgMake(i64 value);
	inline FormatArg FormatArgMake(i32 value);
	inline FormatArg FormatArgMake(i16 value);
	inline FormatArg FormatArgMake(i8 value);
	inline FormatArg FormatArgMake(long value);
	inline FormatArg FormatArgMake(u64 value);
	inline FormatArg FormatArgMake(u32 value);
	inline FormatArg FormatArgMake(u16 value);
	inline FormatArg FormatArgMake(u8 value);
	inline FormatArg FormatArgMake(unsigned long value);
	inline FormatArg FormatArgMake(f64 value);
	inline FormatArg FormatArgMake(f32 value);
	inline FormatArg FormatArgMake(s8 value);
	inline FormatArg FormatArgMake(const c8* value);
	inline FormatArg FormatArgMake(const void* value);
	inline FormatArg FormatArgMake(decltype(nullptr) value);

	FormatArg FormatArgMake(const Vec2<f64>& value);
	FormatArg FormatArgMake(const Vec3<f64>& value);
	FormatArg FormatArgMake(const Vec4<f64>& value);
	FormatArg FormatArgMake(const Vec2<f32>& value);
	FormatArg FormatArgMake(const Vec3<f32>& value);
	FormatArg FormatArgMake(const Vec4<f32>& value);
	FormatArg FormatArgMake(const Vec2<i32>& value);
	FormatArg FormatArgMake(const Vec3<i32>& value);
	FormatArg FormatArgMake(const Vec4<i32>& value);
	FormatArg FormatArgMake(const Vec2<u32>& value);
	FormatArg FormatArgMake(const Vec3<u32>& value);
	FormatArg FormatArgMake(const Vec4<u32>& value);
	FormatArg FormatArgMake(const Color3U& value);
	FormatArg FormatArgMake(const Color4U& value);
	FormatArg FormatArgMake(const Color3F& value);
	FormatArg FormatArgMake(const Color4F& value);
	FormatArg FormatArgMake(const Quaternion& value);

	// An argument written by 'function'. T must be trivially copyable, since captures copy its bytes.
	template<typename T> inline FormatArg FormatArgCustom(const T& value, FormatFunction function);

	//~ Formatting

	// Into a new string from 'allocator', with a terminator after it. Empty with a null str if allocation fails.
	template<typename... A> inline s8 S8Format(MemoryAllocator allocator, FormatStringFor<A...> format, const A&... args);

	// Into a buffer with room for 'capacity' bytes, terminator included. Stops before a character that doesn't fit,
	// always terminates when 'capacity' isn't 0, and returns the bytes written without the terminator.
	template<typename... A> inline u64 CStr8Format(c8* destination, u64 capacity, FormatStringFor<A...> format, const A&... args);

	// As above, for format strings made at runtime, which are checked as they render. An invalid format renders up
	// to the error and records a failure (error.h).
	s8 S8FormatArgs(MemoryAllocator allocator, s8 format, const FormatArg* args, u32 count);
	u64 CStr8FormatArgs(c8* destination, u64 capacity, s8 format, const FormatArg* args, u32 count);

	// Into a writer, after whatever it holds. Doesn't terminate or record failures.
	FormatError FormatRender(FormatWriter* writer, s8 format, const FormatArg* args, u32 count);

	//~ Captures
	// size = FormatCaptureSize(args...);
	// capture = FormatCaptureWrite(ArenaAllocAligned(&arena, size, TOOL_FORMAT_CAPTURE_ALIGNMENT), "{} hit", args...);
	// ...
	// line = S8FormatCapture(allocator, capture);

	// Bytes a capture of these arguments needs.
	template<typename... A> inline u64 FormatCaptureSize(const A&... args);

	// Writes a capture into 'destination', aligned to TOOL_FORMAT_CAPTURE_ALIGNMENT, with FormatCaptureSize bytes
	// of room. The format string must outlive the capture, as literals do.
	template<typename... A> inline FormatCapture* FormatCaptureWrite(void* destination, FormatStringFor<A...> format, const A&... args);

	u64 FormatCaptureSizeArgs(const FormatArg* args, u32 count);
	FormatCapture* FormatCaptureWriteArgs(void* destination, s8 format, const FormatArg* args, u32 count);

	// Renders a capture, as S8FormatArgs, CStr8FormatArgs and FormatRender do.
	s8 S8FormatCapture(MemoryAllocator allocator, const FormatCapture* capture);
	u64 CStr8FormatCapture(c8* destination, u64 capacity, const FormatCapture* capture);
	FormatError FormatRenderCapture(FormatWriter* writer, const FormatCapture* capture);

	//~ Writing, for custom formatters

	void FormatWrite(FormatWriter* writer, s8 text);

	// Writes one argument as a field with this spec would, such as a custom spec passed on to each part of a value.
	// False if the spec doesn't apply to the argument. Widths and precisions from arguments aren't available here.
	b8 FormatWriteArg(FormatWriter* writer, FormatArg arg, s8 spec);

	// Short description of an error, for messages.
	const c8* FormatErrorName(FormatError error);
} //namespace Tool



//- Implementation

namespace Tool
{
	//~ Arguments

	inline FormatArg FormatArgMake(b8 value)
	{
		FormatArg arg;
		arg.type = FormatTypeBool;
		arg.boolean = value;
		return arg;
	}

	inline FormatArg FormatArgMake(c8 value)
	{
		FormatArg arg;
		arg.type = FormatTypeChar;
		arg.character = value;
		return arg;
	}

	inline FormatArg FormatArgMake(i64 value)
	{
		FormatArg arg;
		arg.type = FormatTypeI64;
		arg.i64Value = value;
		return arg;
	}

	inline FormatArg FormatArgMake(i32 value) { return FormatArgMake((i64)value); }
	inline FormatArg FormatArgMake(i16 value) { return FormatArgMake((i64)value); }
	inline FormatArg FormatArgMake(i8 value) { return FormatArgMake((i64)value); }
	inline FormatArg FormatArgMake(long value) { return FormatArgMake((i64)value); }

	inline FormatArg FormatArgMake(u64 value)
	{
		FormatArg arg;
		arg.type = FormatTypeU64;
		arg.u64Value = value;
		return arg;
	}

	inline FormatArg FormatArgMake(u32 value) { return FormatArgMake((u64)value); }
	inline FormatArg FormatArgMake(u16 value) { return FormatArgMake((u64)value); }
	inline FormatArg FormatArgMake(u8 value) { return FormatArgMake((u64)value); }
	inline FormatArg FormatArgMake(unsigned long value) { return FormatArgMake((u64)value); }

	inline FormatArg FormatArgMake(f64 value)
	{
		FormatArg arg;
		arg.type = FormatTypeF64;
		arg.f64Value = value;
		return arg;
	}

	inline FormatArg FormatArgMake(f32 value)
	{
		FormatArg arg;
		arg.type = FormatTypeF32;
		arg.f32Value = value;
		return arg;
	}

	inline FormatArg FormatArgMake(s8 value)
	{
		FormatArg arg;
		arg.type = FormatTypeString;
		arg.string = value;
		return arg;
	}

	inline FormatArg FormatArgMake(const c8* value)
	{
		return FormatArgMake(s8{ (c8*)value, value != nullptr ? CStr8Size(value) : 0 });
	}

	inline FormatArg FormatArgMake(const void* value)
	{
		FormatArg arg;
		arg.type = FormatTypePointer;
		arg.pointer = value;
		return arg;
	}

	inline FormatArg FormatArgMake(decltype(nullptr))
	{
		return FormatArgMake((const void*)nullptr);
	}

	template<typename T>
	inline FormatArg FormatArgCustom(const T& value, FormatFunction function)
	{
		static_assert(__is_trivially_copyable(T), "Captures copy custom values as bytes, so they must be trivially copyable.");
		static_assert(alignof(T) <= TOOL_FORMAT_CAPTURE_ALIGNMENT, "Custom values may not need more than TOOL_FORMAT_CAPTURE_ALIGNMENT.");

		FormatArg arg;
		arg.type = FormatTypeCustom;
		arg.custom.value = &value;
		arg.custom.size = (u32)sizeof(T);
		arg.custom.alignment = (u32)alignof(T);
		arg.custom.function = function;
		return arg;
	}

	//~ Argument types at compile time
	// Mirrors the FormatArgMake overloads above. Anything else is custom, and fails to compile without its overload.

	template<typename T> inline constexpr FormatType FormatTypeOf = __is_enum(T) ? FormatTypeI64 : FormatTypeCustom;
	template<typename T> inline constexpr FormatType FormatTypeOf<T*> = FormatTypePointer;
	template<decltype(sizeof(0)) N> inline constexpr FormatType FormatTypeOf<c8[N]> = FormatTypeString;
	template<> inline constexpr FormatType FormatTypeOf<c8*> = FormatTypeString;
	template<> inline constexpr FormatType FormatTypeOf<const c8*> = FormatTypeString;
	template<> inline constexpr FormatType FormatTypeOf<s8> = FormatTypeString;
	template<> inline constexpr FormatType FormatTypeOf<decltype(nullptr)> = FormatTypePointer;
	template<> inline constexpr FormatType FormatTypeOf<b8> = FormatTypeBool;
	template<> inline constexpr FormatType FormatTypeOf<c8> = FormatTypeChar;
	template<> inline constexpr FormatType FormatTypeOf<i64> = FormatTypeI64;
	template<> inline constexpr FormatType FormatTypeOf<i32> = FormatTypeI64;
	template<> inline constexpr FormatType FormatTypeOf<i16> = FormatTypeI64;
	template<> inline constexpr FormatType FormatTypeOf<i8> = FormatTypeI64;
	template<> inline constexpr FormatType FormatTypeOf<long> = FormatTypeI64;
	template<> inline constexpr FormatType FormatTypeOf<u64> = FormatTypeU64;
	template<> inline constexpr FormatType FormatTypeOf<u32> = FormatTypeU64;
	template<> inline constexpr FormatType FormatTypeOf<u16> = FormatTypeU64;
	template<> inline constexpr FormatType FormatTypeOf<u8> = FormatTypeU64;
	template<> inline constexpr FormatType FormatTypeOf<unsigned long> = FormatTypeU64;
	template<> inline constexpr FormatType FormatTypeOf<f64> = FormatTypeF64;
	template<> inline constexpr FormatType FormatTypeOf<f32> = FormatTypeF32;

	//~ Parsing
	// Shared by the compile-time check and the renderer, so the two can't disagree.

	// Argument numbering across one format: automatic and numbered ids can't mix.
	struct FormatIds
	{
		u32 next;
		b8 automatic;
		b8 numbered;
	};

	constexpr b8 FormatIsDigit(c8 c)
	{
		return c >= '0' && c <= '9';
	}

	constexpr b8 FormatIsAlign(c8 c)
	{
		return c == '<' || c == '>' || c == '^';
	}

	constexpr b8 FormatIsInteger(FormatType type)
	{
		return type == FormatTypeI64 || type == FormatTypeU64;
	}

	// An argument id at *at, or none for the next automatic one.
	constexpr FormatError FormatParseArgId(s8 format, u64* at, FormatIds* ids, u32 count, u32* outIndex)
	{
		u64 i = *at;

		if (i < format.size && FormatIsDigit(format.str[i]))
		{
			if (ids->automatic)
				return FormatErrorMixedIndexing;
			ids->numbered = true;

			u64 value = 0;
			if (format.str[i] == '0')
			{
				i++;
				if (i < format.size && FormatIsDigit(format.str[i]))
					return FormatErrorArgumentId;
			}
			else
			{
				while (i < format.size && FormatIsDigit(format.str[i]))
				{
					value = value * 10 + (u64)(format.str[i] - '0');
					if (value > U32_MAX)
						return FormatErrorArgumentRange;
					i++;
				}
			}

			if (value >= count)
				return FormatErrorArgumentRange;
			*outIndex = (u32)value;
		}
		else
		{
			if (ids->numbered)
				return FormatErrorMixedIndexing;
			ids->automatic = true;

			if (ids->next >= count)
				return FormatErrorArgumentRange;
			*outIndex = ids->next++;
		}

		*at = i;
		return FormatErrorNone;
	}

	// A width or precision: digits, or an argument id in braces.
	constexpr FormatError FormatParseCount(s8 spec, u64* at, FormatIds* ids, u32 count, u32* outValue, u32* outArg)
	{
		u64 i = *at;

		if (i < spec.size && spec.str[i] == '{')
		{
			i++;
			FormatError error = FormatParseArgId(spec, &i, ids, count, outArg);
			if (error != FormatErrorNone)
				return error;
			if (i >= spec.size || spec.str[i] != '}')
				return FormatErrorSpec;
			*at = i + 1;
			return FormatErrorNone;
		}

		u64 value = 0;
		while (i < spec.size && FormatIsDigit(spec.str[i]))
		{
			value = value * 10 + (u64)(spec.str[i] - '0');
			if (value >= U32_MAX)
				return FormatErrorSpec;
			i++;
		}

		*outValue = (u32)value;
		*at = i;
		return FormatErrorNone;
	}

	// The fill code point at the start of a spec, and its length in bytes. Malformed UTF-8 counts as one byte.
	constexpr u32 FormatParseFill(s8 spec, u32* outLength)
	{
		u8 lead = (u8)spec.str[0];
		u32 length = lead < 0x80 ? 1 : (lead & 0xE0) == 0xC0 ? 2 : (lead & 0xF0) == 0xE0 ? 3 : (lead & 0xF8) == 0xF0 ? 4 : 1;
		if (length > spec.size)
			length = 1;

		u32 codepoint = length == 1 ? lead : length == 2 ? (lead & 0x1Fu) : length == 3 ? (lead & 0x0Fu) : (lead & 0x07u);
		for (u32 i = 1; i < length; i++)
			codepoint = (codepoint << 6) | ((u8)spec.str[i] & 0x3Fu);

		*outLength = length;
		return codepoint;
	}

	// The spec between ':' and the field's closing '}'.
	constexpr FormatError FormatSpecParse(s8 spec, FormatIds* ids, u32 count, FormatSpec* outSpec)
	{
		FormatSpec result = {};
		result.fill = ' ';
		result.widthArg = TOOL_FORMAT_NO_ARG;
		result.precision = TOOL_FORMAT_NO_PRECISION;
		result.precisionArg = TOOL_FORMAT_NO_ARG;

		u64 i = 0;
		const c8* s = spec.str;

		if (spec.size > 0)
		{
			u32 fillLength = 0;
			u32 fill = FormatParseFill(spec, &fillLength);

			if (fillLength < spec.size && FormatIsAlign(s[fillLength]))
			{
				if (s[0] == '{' || s[0] == '}')
					return FormatErrorSpec;
				result.fill = fill;
				result.align = s[fillLength];
				i = fillLength + 1;
			}
			else if (FormatIsAlign(s[0]))
			{
				result.align = s[0];
				i = 1;
			}
		}

		if (i < spec.size && (s[i] == '+' || s[i] == '-' || s[i] == ' '))
			result.sign = s[i++];

		if (i < spec.size && s[i] == '#')
		{
			result.alternate = true;
			i++;
		}

		if (i < spec.size && s[i] == '0')
		{
			result.zero = true;
			i++;
		}

		if (i < spec.size && ((s[i] >= '1' && s[i] <= '9') || s[i] == '{'))
		{
			FormatError error = FormatParseCount(spec, &i, ids, count, &result.width, &result.widthArg);
			if (error != FormatErrorNone)
				return error;
		}

		if (i < spec.size && s[i] == '.')
		{
			i++;
			if (i >= spec.size || !(FormatIsDigit(s[i]) || s[i] == '{'))
				return FormatErrorSpec;

			u32 precision = 0;
			FormatError error = FormatParseCount(spec, &i, ids, count, &precision, &result.precisionArg);
			if (error != FormatErrorNone)
				return error;
			if (result.precisionArg == TOOL_FORMAT_NO_ARG)
				result.precision = precision;
		}

		if (i < spec.size && s[i] == 'L')
			return FormatErrorUnsupported;

		if (i < spec.size)
		{
			c8 type = s[i++];
			switch (type)
			{
				case 'a': case 'A': case 'b': case 'B': case 'c': case 'd': case 'e': case 'E':
				case 'f': case 'F': case 'g': case 'G': case 'o': case 'p': case 'P': case 's':
				case 'x': case 'X':
					result.type = type;
					break;

				case '?':
					return FormatErrorUnsupported;

				default:
					return FormatErrorSpec;
			}
		}

		if (i != spec.size)
			return FormatErrorSpec;

		*outSpec = result;
		return FormatErrorNone;
	}

	// Whether a parsed spec applies to an argument type.
	constexpr FormatError FormatSpecCheck(const FormatSpec* spec, FormatType type)
	{
		c8 t = spec->type;
		b8 integer = t == 'b' || t == 'B' || t == 'd' || t == 'o' || t == 'x' || t == 'X';
		b8 precision = spec->precision != TOOL_FORMAT_NO_PRECISION || spec->precisionArg != TOOL_FORMAT_NO_ARG;
		b8 numeric = spec->sign != 0 || spec->alternate || spec->zero;

		switch (type)
		{
			case FormatTypeBool:
				if (t != 0 && t != 's' && !integer)
					return FormatErrorSpecType;
				if (precision || (numeric && !integer))
					return FormatErrorSpecType;
				return FormatErrorNone;

			case FormatTypeChar:
				if (t != 0 && t != 'c' && !integer)
					return FormatErrorSpecType;
				if (precision || (numeric && !integer))
					return FormatErrorSpecType;
				return FormatErrorNone;

			case FormatTypeI64:
			case FormatTypeU64:
				if (t != 0 && t != 'c' && !integer)
					return FormatErrorSpecType;
				if (precision || (numeric && t == 'c'))
					return FormatErrorSpecType;
				return FormatErrorNone;

			case FormatTypeF32:
			case FormatTypeF64:
				if (t != 0 && t != 'a' && t != 'A' && t != 'e' && t != 'E' && t != 'f' && t != 'F' && t != 'g' && t != 'G')
					return FormatErrorSpecType;
				return FormatErrorNone;

			case FormatTypeString:
				if ((t != 0 && t != 's') || numeric)
					return FormatErrorSpecType;
				return FormatErrorNone;

			case FormatTypePointer:
				if ((t != 0 && t != 'p' && t != 'P') || precision || spec->sign != 0 || spec->alternate)
					return FormatErrorSpecType;
				return FormatErrorNone;

			default:
				return FormatErrorNone;
		}
	}

	// Walks a format, handing literal text and fields to 'visitor', which provides:
	//   FormatType Type(u32 index)
	//   void Literal(s8 text)
	//   FormatError Field(u32 index, const FormatSpec* spec, s8 rawSpec)
	// Custom arguments get their spec raw and unparsed. Stops at the first error.
	template<typename V>
	constexpr FormatError FormatWalk(s8 format, u32 count, V* visitor)
	{
		FormatIds ids = {};
		u64 at = 0;
		u64 literal = 0;

		while (at < format.size)
		{
			c8 c = format.str[at];

			if (c == '}')
			{
				if (at + 1 >= format.size || format.str[at + 1] != '}')
					return FormatErrorUnmatchedClose;
				visitor->Literal(s8{ format.str + literal, at + 1 - literal });
				at += 2;
				literal = at;
				continue;
			}

			if (c != '{')
			{
				at++;
				continue;
			}

			if (at + 1 < format.size && format.str[at + 1] == '{')
			{
				visitor->Literal(s8{ format.str + literal, at + 1 - literal });
				at += 2;
				literal = at;
				continue;
			}

			visitor->Literal(s8{ format.str + literal, at - literal });
			at++;

			u32 index = 0;
			FormatError error = FormatParseArgId(format, &at, &ids, count, &index);
			if (error != FormatErrorNone)
				return error;
			if (at >= format.size)
				return FormatErrorUnmatchedOpen;

			s8 rawSpec = { format.str + at, 0 };
			if (format.str[at] == ':')
			{
				// To the '}' closing the field, past the braces of nested width and precision ids
				u64 start = ++at;
				u32 depth = 0;
				while (at < format.size)
				{
					if (format.str[at] == '{')
						depth++;
					else if (format.str[at] == '}')
					{
						if (depth == 0)
							break;
						depth--;
					}
					at++;
				}
				if (at >= format.size)
					return FormatErrorUnmatchedOpen;
				rawSpec = s8{ format.str + start, at - start };
			}
			else if (format.str[at] != '}')
				return FormatErrorArgumentId;
			at++;

			FormatType type = visitor->Type(index);
			FormatSpec spec = {};
			spec.fill = ' ';
			spec.widthArg = TOOL_FORMAT_NO_ARG;
			spec.precision = TOOL_FORMAT_NO_PRECISION;
			spec.precisionArg = TOOL_FORMAT_NO_ARG;

			if (type == FormatTypeCustom)
			{
				for (u64 i = 0; i < rawSpec.size; i++)
				{
					if (rawSpec.str[i] == '{' || rawSpec.str[i] == '}')
						return FormatErrorSpec;
				}
			}
			else if (rawSpec.size != 0)
			{
				error = FormatSpecParse(rawSpec, &ids, count, &spec);
				if (error != FormatErrorNone)
					return error;
				error = FormatSpecCheck(&spec, type);
				if (error != FormatErrorNone)
					return error;
				if (spec.widthArg != TOOL_FORMAT_NO_ARG && !FormatIsInteger(visitor->Type(spec.widthArg)))
					return FormatErrorDynamicType;
				if (spec.precisionArg != TOOL_FORMAT_NO_ARG && !FormatIsInteger(visitor->Type(spec.precisionArg)))
					return FormatErrorDynamicType;
			}

			error = visitor->Field(index, &spec, rawSpec);
			if (error != FormatErrorNone)
				return error;
			literal = at;
		}

		visitor->Literal(s8{ format.str + literal, at - literal });
		return FormatErrorNone;
	}

	//~ Compile-time checking

	struct FormatValidator
	{
		const FormatType* types;

		constexpr FormatType Type(u32 index) const { return types[index]; }
		constexpr void Literal(s8) const {}
		constexpr FormatError Field(u32, const FormatSpec*, s8) const { return FormatErrorNone; }
	};

	// Not constexpr and never defined: reaching one while checking a format stops the build, and the compiler's
	// message names the problem.
	void FormatInvalidUnmatchedOpenBrace();
	void FormatInvalidUnmatchedCloseBrace();
	void FormatInvalidArgumentId();
	void FormatInvalidArgumentIdPastArguments();
	void FormatInvalidMixedAutomaticAndNumberedIds();
	void FormatInvalidSpec();
	void FormatInvalidSpecForArgumentType();
	void FormatInvalidWidthOrPrecisionArgumentNotInteger();
	void FormatInvalidUnsupportedOption();

	constexpr void FormatReportError(FormatError error)
	{
		switch (error)
		{
			case FormatErrorNone: break;
			case FormatErrorUnmatchedOpen: FormatInvalidUnmatchedOpenBrace(); break;
			case FormatErrorUnmatchedClose: FormatInvalidUnmatchedCloseBrace(); break;
			case FormatErrorArgumentId: FormatInvalidArgumentId(); break;
			case FormatErrorArgumentRange: FormatInvalidArgumentIdPastArguments(); break;
			case FormatErrorMixedIndexing: FormatInvalidMixedAutomaticAndNumberedIds(); break;
			case FormatErrorSpec: FormatInvalidSpec(); break;
			case FormatErrorSpecType: FormatInvalidSpecForArgumentType(); break;
			case FormatErrorDynamicType: FormatInvalidWidthOrPrecisionArgumentNotInteger(); break;
			default: FormatInvalidUnsupportedOption(); break;
		}
	}

	template<typename... A>
	struct FormatString
	{
		s8 string;

		template<decltype(sizeof(0)) N>
		consteval FormatString(const c8 (&literal)[N]) : string{ (c8*)literal, N - 1 }
		{
#if !TOOL_NO_FORMAT_VALIDATION
			const FormatType types[sizeof...(A) + 1] = { FormatTypeOf<A>..., FormatTypeNone };
			FormatValidator validator = { types };
			FormatReportError(FormatWalk(string, (u32)sizeof...(A), &validator));
#endif
		}
	};

	//~ Formatting

	template<typename... A>
	inline s8 S8Format(MemoryAllocator allocator, FormatStringFor<A...> format, const A&... args)
	{
		const FormatArg packed[sizeof...(A) + 1] = { FormatArgMake(args)..., FormatArg{} };
		return S8FormatArgs(allocator, format.string, packed, (u32)sizeof...(A));
	}

	template<typename... A>
	inline u64 CStr8Format(c8* destination, u64 capacity, FormatStringFor<A...> format, const A&... args)
	{
		const FormatArg packed[sizeof...(A) + 1] = { FormatArgMake(args)..., FormatArg{} };
		return CStr8FormatArgs(destination, capacity, format.string, packed, (u32)sizeof...(A));
	}

	//~ Captures

	template<typename... A>
	inline u64 FormatCaptureSize(const A&... args)
	{
		const FormatArg packed[sizeof...(A) + 1] = { FormatArgMake(args)..., FormatArg{} };
		return FormatCaptureSizeArgs(packed, (u32)sizeof...(A));
	}

	template<typename... A>
	inline FormatCapture* FormatCaptureWrite(void* destination, FormatStringFor<A...> format, const A&... args)
	{
		const FormatArg packed[sizeof...(A) + 1] = { FormatArgMake(args)..., FormatArg{} };
		return FormatCaptureWriteArgs(destination, format.string, packed, (u32)sizeof...(A));
	}
} //namespace Tool



#endif //_TOOL_FORMAT_H
