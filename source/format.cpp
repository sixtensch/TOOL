#include "format.h"
#include "color.h"
#include "error.h"
#include "quaternion.h"
#include "vector.h"

#include <charconv>
#include <string.h>



namespace Tool
{
	//- Writing

	void FormatWrite(FormatWriter* writer, s8 text)
	{
		if (writer->size < writer->capacity && text.size != 0)
		{
			u64 room = writer->capacity - writer->size;
			memcpy(writer->destination + writer->size, text.str, text.size < room ? text.size : room);
		}
		writer->size += text.size;
	}

	static void FormatWriteBytes(FormatWriter* writer, const c8* bytes, u64 size)
	{
		FormatWrite(writer, s8{ (c8*)bytes, size });
	}

	static void FormatWriteFill(FormatWriter* writer, u32 fill, u64 count)
	{
		if (count == 0)
			return;

		c8 bytes[4];
		u32 length = CodepointEncodeUTF8(fill, bytes);

		if (length == 1)
		{
			if (writer->size < writer->capacity)
			{
				u64 room = writer->capacity - writer->size;
				memset(writer->destination + writer->size, bytes[0], count < room ? count : room);
			}
			writer->size += count;
			return;
		}

		for (u64 i = 0; i < count; i++)
			FormatWriteBytes(writer, bytes, length);
	}

	// Code points in UTF-8 text, counting lead bytes
	static u64 FormatCodepointCount(const c8* text, u64 size)
	{
		u64 count = 0;
		for (u64 i = 0; i < size; i++)
			count += ((u8)text[i] & 0xC0) != 0x80;
		return count;
	}

	// Bytes in the first 'count' code points of UTF-8 text
	static u64 FormatCodepointPrefix(const c8* text, u64 size, u64 count)
	{
		u64 seen = 0;
		for (u64 i = 0; i < size; i++)
		{
			if (((u8)text[i] & 0xC0) != 0x80)
			{
				if (seen == count)
					return i;
				seen++;
			}
		}
		return size;
	}

	// The length of UTF-8 text cut to 'size' bytes, without a character the cut split.
	static u64 FormatTrimPartial(const c8* text, u64 size)
	{
		for (u64 back = 1; back <= 4 && back <= size; back++)
		{
			u8 byte = (u8)text[size - back];
			if ((byte & 0xC0) == 0x80)
				continue;

			u64 length = byte < 0x80 ? 1 : (byte & 0xE0) == 0xC0 ? 2 : (byte & 0xF0) == 0xE0 ? 3 : (byte & 0xF8) == 0xF0 ? 4 : 1;
			return length > back ? size - back : size;
		}
		return size;
	}

	// Writes content of 'length' code points padded to the spec's width.
	static void FormatWritePadded(FormatWriter* writer, const c8* bytes, u64 size, u64 length, const FormatSpec* spec, c8 defaultAlign)
	{
		u64 padding = spec->width > length ? spec->width - length : 0;
		c8 align = spec->align != 0 ? spec->align : defaultAlign;
		u64 before = align == '>' ? padding : align == '^' ? padding / 2 : 0;

		FormatWriteFill(writer, spec->fill, before);
		FormatWriteBytes(writer, bytes, size);
		FormatWriteFill(writer, spec->fill, padding - before);
	}

	// Writes a number as prefix and body. With '0' and no alignment, zeros go between them; otherwise the fill pads
	// both, aligned right by default.
	static void FormatWriteNumber(FormatWriter* writer, const c8* prefix, u64 prefixSize, const c8* body, u64 bodySize, const FormatSpec* spec, b8 zeroAllowed)
	{
		u64 length = prefixSize + bodySize;

		if (spec->zero && spec->align == 0 && zeroAllowed)
		{
			FormatWriteBytes(writer, prefix, prefixSize);
			FormatWriteFill(writer, '0', spec->width > length ? spec->width - length : 0);
			FormatWriteBytes(writer, body, bodySize);
			return;
		}

		u64 padding = spec->width > length ? spec->width - length : 0;
		c8 align = spec->align != 0 ? spec->align : '>';
		u64 before = align == '>' ? padding : align == '^' ? padding / 2 : 0;

		FormatWriteFill(writer, spec->fill, before);
		FormatWriteBytes(writer, prefix, prefixSize);
		FormatWriteBytes(writer, body, bodySize);
		FormatWriteFill(writer, spec->fill, padding - before);
	}



	//- Values

	//~ Integers

	static const c8 formatDigitPairs[] =
		"00010203040506070809101112131415161718192021222324252627282930313233343536373839"
		"40414243444546474849505152535455565758596061626364656667686970717273747576777879"
		"8081828384858687888990919293949596979899";

	// Writes the digits backwards from 'end' and returns where they start.
	static c8* FormatDigits(c8* end, u64 value, u32 base, b8 upper)
	{
		c8* at = end;

		if (base == 10)
		{
			while (value >= 100)
			{
				u64 pair = (value % 100) * 2;
				value /= 100;
				*--at = formatDigitPairs[pair + 1];
				*--at = formatDigitPairs[pair];
			}
			if (value >= 10)
			{
				*--at = formatDigitPairs[value * 2 + 1];
				*--at = formatDigitPairs[value * 2];
			}
			else
				*--at = (c8)('0' + value);
			return at;
		}

		const c8* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
		u32 shift = base == 16 ? 4 : base == 8 ? 3 : 1;
		do
		{
			*--at = digits[value & (base - 1)];
			value >>= shift;
		} while (value != 0);
		return at;
	}

	static void FormatWriteInteger(FormatWriter* writer, u64 magnitude, b8 negative, const FormatSpec* spec)
	{
		c8 type = spec->type;
		u32 base = (type == 'x' || type == 'X') ? 16 : type == 'o' ? 8 : (type == 'b' || type == 'B') ? 2 : 10;

		c8 digits[64];
		c8* end = digits + sizeof(digits);
		c8* start = FormatDigits(end, magnitude, base, type == 'X');

		c8 prefix[3];
		u64 prefixSize = 0;
		if (negative)
			prefix[prefixSize++] = '-';
		else if (spec->sign == '+' || spec->sign == ' ')
			prefix[prefixSize++] = spec->sign;

		if (spec->alternate)
		{
			if (base == 16 || base == 2)
			{
				prefix[prefixSize++] = '0';
				prefix[prefixSize++] = type;
			}
			else if (base == 8 && magnitude != 0)
				prefix[prefixSize++] = '0';
		}

		FormatWriteNumber(writer, prefix, prefixSize, start, (u64)(end - start), spec, true);
	}

	static FormatError FormatWriteCodepoint(FormatWriter* writer, u64 codepoint, const FormatSpec* spec)
	{
		if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
			return FormatErrorCharacter;

		c8 bytes[4];
		u32 size = CodepointEncodeUTF8((u32)codepoint, bytes);
		FormatWritePadded(writer, bytes, size, 1, spec, '<');
		return FormatErrorNone;
	}

	//~ Floats

	// Fixed notation of the largest f64 has 309 digits before the point.
	static constexpr u64 formatFloatBaseSize = 340;

	static FormatError FormatWriteFloat(FormatWriter* writer, f64 value, b8 single, const FormatSpec* spec)
	{
		// Converting to f64 keeps the sign of an f32, NaN and negative zero included
		u64 bits;
		memcpy(&bits, &value, sizeof(bits));
		b8 negative = (bits >> 63) != 0;
		f64 magnitude = negative ? -value : value;
		c8 type = spec->type;
		b8 upper = type == 'A' || type == 'E' || type == 'F' || type == 'G';
		b8 hasPrecision = spec->precision != TOOL_FORMAT_NO_PRECISION;
		i32 precision = hasPrecision ? (i32)(spec->precision > (u32)I32_MAX ? (u32)I32_MAX : spec->precision) : 6;

		c8 prefix[1];
		u64 prefixSize = 0;
		if (negative)
			prefix[prefixSize++] = '-';
		else if (spec->sign == '+' || spec->sign == ' ')
			prefix[prefixSize++] = spec->sign;

		if (magnitude != magnitude || magnitude > 1.7976931348623157e308)
		{
			const c8* body = magnitude != magnitude ? (upper ? "NAN" : "nan") : (upper ? "INF" : "inf");
			FormatWriteNumber(writer, prefix, prefixSize, body, 3, spec, false);
			return FormatErrorNone;
		}

		// Room for the digits, plus what '#' may add: a point and the zeros general notation keeps
		u64 capacity = formatFloatBaseSize + (hasPrecision ? (u64)precision * 2 : 0);
		c8 stack[512];
		c8* buffer = stack;
		if (capacity > sizeof(stack))
		{
			buffer = (c8*)ClassicAlloc(capacity);
			if (buffer == nullptr)
				return FormatErrorDynamicValue;
		}

		c8* first = buffer;
		c8* last = buffer + capacity - (hasPrecision ? (u64)precision : 0) - 8;
		std::to_chars_result result = {};
		// '#' keeps general notation's trailing zeros for 'g' and 'G' only, not for a precision without a type
		b8 general = false;

		f32 small = (f32)magnitude;
		switch (type)
		{
			case 0:
				if (!hasPrecision)
					result = single ? std::to_chars(first, last, small) : std::to_chars(first, last, magnitude);
				else
					result = single ? std::to_chars(first, last, small, std::chars_format::general, precision) : std::to_chars(first, last, magnitude, std::chars_format::general, precision);
				break;

			case 'e':
			case 'E':
				result = single ? std::to_chars(first, last, small, std::chars_format::scientific, precision) : std::to_chars(first, last, magnitude, std::chars_format::scientific, precision);
				break;

			case 'f':
			case 'F':
				result = single ? std::to_chars(first, last, small, std::chars_format::fixed, precision) : std::to_chars(first, last, magnitude, std::chars_format::fixed, precision);
				break;

			case 'g':
			case 'G':
				general = true;
				result = single ? std::to_chars(first, last, small, std::chars_format::general, precision) : std::to_chars(first, last, magnitude, std::chars_format::general, precision);
				break;

			default: // 'a', 'A'
				if (!hasPrecision)
					result = single ? std::to_chars(first, last, small, std::chars_format::hex) : std::to_chars(first, last, magnitude, std::chars_format::hex);
				else
					result = single ? std::to_chars(first, last, small, std::chars_format::hex, precision) : std::to_chars(first, last, magnitude, std::chars_format::hex, precision);
				break;
		}

		if (result.ec != std::errc())
		{
			if (buffer != stack)
				ClassicDealloc(buffer);
			return FormatErrorDynamicValue;
		}

		u64 size = (u64)(result.ptr - first);

		if (spec->alternate)
		{
			// '#' keeps the point, and with 'g' the trailing zeros up to the precision
			c8 exponentMark = (type == 'a' || type == 'A') ? 'p' : 'e';
			u64 exponent = size;
			b8 point = false;
			for (u64 i = 0; i < size; i++)
			{
				if (buffer[i] == '.')
					point = true;
				else if (buffer[i] == exponentMark)
				{
					exponent = i;
					break;
				}
			}

			u64 zeros = 0;
			if (general)
			{
				u64 significant = 0;
				b8 leading = true;
				for (u64 i = 0; i < exponent; i++)
				{
					if (buffer[i] == '.')
						continue;
					if (leading && buffer[i] == '0')
						continue;
					leading = false;
					significant++;
				}
				if (leading)
					significant = 1;

				u64 wanted = precision == 0 ? 1 : (u64)precision;
				zeros = wanted > significant ? wanted - significant : 0;
			}

			u64 insert = (point ? 0 : 1) + zeros;
			if (insert != 0)
			{
				memmove(buffer + exponent + insert, buffer + exponent, size - exponent);
				u64 at = exponent;
				if (!point)
					buffer[at++] = '.';
				for (u64 i = 0; i < zeros; i++)
					buffer[at++] = '0';
				size += insert;
			}
		}

		if (upper)
		{
			for (u64 i = 0; i < size; i++)
				buffer[i] = C8ToUpper(buffer[i]);
		}

		FormatWriteNumber(writer, prefix, prefixSize, buffer, size, spec, true);

		if (buffer != stack)
			ClassicDealloc(buffer);
		return FormatErrorNone;
	}

	//~ One value

	static FormatError FormatWriteValue(FormatWriter* writer, const FormatArg* arg, const FormatSpec* spec, s8 rawSpec)
	{
		c8 type = spec->type;

		switch (arg->type)
		{
			case FormatTypeBool:
				if (type == 0 || type == 's')
				{
					s8 text = arg->boolean ? S8("true") : S8("false");
					FormatWritePadded(writer, text.str, text.size, text.size, spec, '<');
					return FormatErrorNone;
				}
				FormatWriteInteger(writer, arg->boolean ? 1 : 0, false, spec);
				return FormatErrorNone;

			case FormatTypeChar:
				if (type == 0 || type == 'c')
				{
					FormatWritePadded(writer, &arg->character, 1, 1, spec, '<');
					return FormatErrorNone;
				}
				FormatWriteInteger(writer, (u8)arg->character, false, spec);
				return FormatErrorNone;

			case FormatTypeI64:
			{
				i64 value = arg->i64Value;
				if (type == 'c')
					return value < 0 ? FormatErrorCharacter : FormatWriteCodepoint(writer, (u64)value, spec);
				u64 magnitude = value < 0 ? (u64)0 - (u64)value : (u64)value;
				FormatWriteInteger(writer, magnitude, value < 0, spec);
				return FormatErrorNone;
			}

			case FormatTypeU64:
				if (type == 'c')
					return FormatWriteCodepoint(writer, arg->u64Value, spec);
				FormatWriteInteger(writer, arg->u64Value, false, spec);
				return FormatErrorNone;

			case FormatTypeF32:
				return FormatWriteFloat(writer, (f64)arg->f32Value, true, spec);

			case FormatTypeF64:
				return FormatWriteFloat(writer, arg->f64Value, false, spec);

			case FormatTypeString:
			{
				s8 text = arg->string;
				if (spec->precision != TOOL_FORMAT_NO_PRECISION)
					text.size = FormatCodepointPrefix(text.str, text.size, spec->precision);
				u64 length = spec->width != 0 ? FormatCodepointCount(text.str, text.size) : 0;
				FormatWritePadded(writer, text.str, text.size, length, spec, '<');
				return FormatErrorNone;
			}

			case FormatTypePointer:
			{
				c8 digits[16];
				c8* end = digits + sizeof(digits);
				c8* start = FormatDigits(end, (u64)arg->pointer, 16, type == 'P');
				const c8* prefix = type == 'P' ? "0X" : "0x";
				FormatWriteNumber(writer, prefix, 2, start, (u64)(end - start), spec, true);
				return FormatErrorNone;
			}

			case FormatTypeCustom:
				return arg->custom.function(writer, arg->custom.value, rawSpec) ? FormatErrorNone : FormatErrorCustom;

			default:
				return FormatErrorArgumentRange;
		}
	}

	b8 FormatWriteArg(FormatWriter* writer, FormatArg arg, s8 spec)
	{
		FormatSpec parsed = {};
		parsed.fill = ' ';
		parsed.widthArg = TOOL_FORMAT_NO_ARG;
		parsed.precision = TOOL_FORMAT_NO_PRECISION;
		parsed.precisionArg = TOOL_FORMAT_NO_ARG;

		if (arg.type != FormatTypeCustom)
		{
			FormatIds ids = {};
			if (FormatSpecParse(spec, &ids, 0, &parsed) != FormatErrorNone || FormatSpecCheck(&parsed, arg.type) != FormatErrorNone)
				return false;
		}

		return FormatWriteValue(writer, &arg, &parsed, spec) == FormatErrorNone;
	}



	//- Rendering

	// An argument as given, or from a capture, whose strings and custom values sit at offsets from the capture.
	static FormatArg FormatArgResolve(const FormatArg* arg, const u8* base)
	{
		FormatArg resolved = *arg;
		if (base != nullptr)
		{
			if (resolved.type == FormatTypeString)
				resolved.string.str = (c8*)(base + (u64)resolved.string.str);
			else if (resolved.type == FormatTypeCustom)
				resolved.custom.value = base + (u64)resolved.custom.value;
		}
		return resolved;
	}

	// A width or precision from an argument
	static FormatError FormatDynamicCount(const FormatArg* arg, u32* outValue)
	{
		if (arg->type == FormatTypeI64)
		{
			if (arg->i64Value < 0 || arg->i64Value >= (i64)U32_MAX)
				return FormatErrorDynamicValue;
			*outValue = (u32)arg->i64Value;
			return FormatErrorNone;
		}

		if (arg->type == FormatTypeU64)
		{
			if (arg->u64Value >= U32_MAX)
				return FormatErrorDynamicValue;
			*outValue = (u32)arg->u64Value;
			return FormatErrorNone;
		}

		return FormatErrorDynamicType;
	}

	struct FormatRenderer
	{
		FormatWriter* writer;
		const FormatArg* args;
		const u8* base;

		FormatType Type(u32 index) const
		{
			return args[index].type;
		}

		void Literal(s8 text) const
		{
			FormatWrite(writer, text);
		}

		FormatError Field(u32 index, const FormatSpec* spec, s8 rawSpec) const
		{
			FormatSpec resolved = *spec;

			if (spec->widthArg != TOOL_FORMAT_NO_ARG)
			{
				FormatError error = FormatDynamicCount(&args[spec->widthArg], &resolved.width);
				if (error != FormatErrorNone)
					return error;
			}

			if (spec->precisionArg != TOOL_FORMAT_NO_ARG)
			{
				FormatError error = FormatDynamicCount(&args[spec->precisionArg], &resolved.precision);
				if (error != FormatErrorNone)
					return error;
			}

			FormatArg arg = FormatArgResolve(&args[index], base);
			return FormatWriteValue(writer, &arg, &resolved, rawSpec);
		}
	};

	static FormatError FormatRenderFrom(FormatWriter* writer, s8 format, const FormatArg* args, u32 count, const u8* base)
	{
		FormatRenderer renderer = { writer, args, base };
		return FormatWalk(format, count, &renderer);
	}

	static void FormatFail(FormatError error, s8 format)
	{
		TOOL_FAIL("Invalid format (%s): \"%.*s\"", FormatErrorName(error), (i32)(format.size > 256 ? 256 : format.size), format.str);
	}

	// Renders into a new string: into the stack first, then again into the allocation if that was too small.
	static s8 S8FormatFrom(MemoryAllocator allocator, s8 format, const FormatArg* args, u32 count, const u8* base)
	{
		c8 stack[1024];
		FormatWriter writer = { stack, sizeof(stack), 0 };
		FormatError error = FormatRenderFrom(&writer, format, args, count, base);
		if (error != FormatErrorNone)
			FormatFail(error, format);

		u64 size = writer.size;
		c8* str = (c8*)AllocatorAlloc(allocator, size + 1);
		if (str == nullptr)
		{
			TOOL_FAIL("Could not allocate a formatted string of %llu bytes.", (unsigned long long)size + 1);
			return {};
		}

		if (size <= sizeof(stack))
		{
			memcpy(str, stack, size);
		}
		else
		{
			writer = { str, size, 0 };
			FormatRenderFrom(&writer, format, args, count, base);
			size = writer.size < size ? writer.size : size;
		}

		str[size] = 0;
		return { str, size };
	}

	static u64 CStr8FormatFrom(c8* destination, u64 capacity, s8 format, const FormatArg* args, u32 count, const u8* base)
	{
		if (capacity == 0)
			return 0;

		FormatWriter writer = { destination, capacity - 1, 0 };
		FormatError error = FormatRenderFrom(&writer, format, args, count, base);
		if (error != FormatErrorNone)
			FormatFail(error, format);

		u64 size = writer.size;
		if (size > writer.capacity)
			size = FormatTrimPartial(destination, writer.capacity);

		destination[size] = 0;
		return size;
	}

	s8 S8FormatArgs(MemoryAllocator allocator, s8 format, const FormatArg* args, u32 count)
	{
		return S8FormatFrom(allocator, format, args, count, nullptr);
	}

	u64 CStr8FormatArgs(c8* destination, u64 capacity, s8 format, const FormatArg* args, u32 count)
	{
		return CStr8FormatFrom(destination, capacity, format, args, count, nullptr);
	}

	FormatError FormatRender(FormatWriter* writer, s8 format, const FormatArg* args, u32 count)
	{
		return FormatRenderFrom(writer, format, args, count, nullptr);
	}



	//- Captures
	// Layout: the header, the arguments, then string and custom value bytes. Argument pointers to those bytes are
	// stored as offsets from the header, so the block can move.

	static u64 FormatAlignUp(u64 value, u64 alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	u64 FormatCaptureSizeArgs(const FormatArg* args, u32 count)
	{
		u64 size = sizeof(FormatCapture) + (u64)count * sizeof(FormatArg);

		for (u32 i = 0; i < count; i++)
		{
			if (args[i].type == FormatTypeString)
				size += args[i].string.size;
			else if (args[i].type == FormatTypeCustom)
				size = FormatAlignUp(size, args[i].custom.alignment) + args[i].custom.size;
		}

		return FormatAlignUp(size, TOOL_FORMAT_CAPTURE_ALIGNMENT);
	}

	FormatCapture* FormatCaptureWriteArgs(void* destination, s8 format, const FormatArg* args, u32 count)
	{
		TOOL_DEBUG_ASSERT(((u64)destination & (TOOL_FORMAT_CAPTURE_ALIGNMENT - 1)) == 0, "Format captures must be aligned to %llu bytes.", TOOL_FORMAT_CAPTURE_ALIGNMENT);

		u8* base = (u8*)destination;
		FormatCapture* capture = (FormatCapture*)base;
		FormatArg* stored = (FormatArg*)(capture + 1);
		u64 at = sizeof(FormatCapture) + (u64)count * sizeof(FormatArg);

		for (u32 i = 0; i < count; i++)
		{
			stored[i] = args[i];

			if (args[i].type == FormatTypeString)
			{
				if (args[i].string.size != 0)
					memcpy(base + at, args[i].string.str, args[i].string.size);
				stored[i].string.str = (c8*)at;
				at += args[i].string.size;
			}
			else if (args[i].type == FormatTypeCustom)
			{
				at = FormatAlignUp(at, args[i].custom.alignment);
				memcpy(base + at, args[i].custom.value, args[i].custom.size);
				stored[i].custom.value = (const void*)at;
				at += args[i].custom.size;
			}
		}

		capture->size = FormatAlignUp(at, TOOL_FORMAT_CAPTURE_ALIGNMENT);
		capture->format = format;
		capture->count = count;
		return capture;
	}

	s8 S8FormatCapture(MemoryAllocator allocator, const FormatCapture* capture)
	{
		return S8FormatFrom(allocator, capture->format, (const FormatArg*)(capture + 1), capture->count, (const u8*)capture);
	}

	u64 CStr8FormatCapture(c8* destination, u64 capacity, const FormatCapture* capture)
	{
		return CStr8FormatFrom(destination, capacity, capture->format, (const FormatArg*)(capture + 1), capture->count, (const u8*)capture);
	}

	FormatError FormatRenderCapture(FormatWriter* writer, const FormatCapture* capture)
	{
		return FormatRenderFrom(writer, capture->format, (const FormatArg*)(capture + 1), capture->count, (const u8*)capture);
	}



	//- Errors

	const c8* FormatErrorName(FormatError error)
	{
		switch (error)
		{
			case FormatErrorNone: return "none";
			case FormatErrorUnmatchedOpen: return "unmatched '{'";
			case FormatErrorUnmatchedClose: return "unmatched '}'";
			case FormatErrorArgumentId: return "malformed argument id";
			case FormatErrorArgumentRange: return "argument id past the arguments";
			case FormatErrorMixedIndexing: return "automatic and numbered ids mixed";
			case FormatErrorSpec: return "malformed spec";
			case FormatErrorSpecType: return "spec doesn't apply to the argument";
			case FormatErrorDynamicType: return "width or precision argument isn't an integer";
			case FormatErrorDynamicValue: return "width or precision out of range";
			case FormatErrorUnsupported: return "unsupported option";
			case FormatErrorCharacter: return "not a code point";
			case FormatErrorCustom: return "custom formatter refused its spec";
			default: return "unknown";
		}
	}



	//- TOOL types
	// Components in parentheses, each formatted with the field's spec.

	template<typename T>
	static b8 FormatComponents(FormatWriter* writer, const T* components, u32 count, s8 spec)
	{
		FormatWrite(writer, S8("("));
		for (u32 i = 0; i < count; i++)
		{
			if (i != 0)
				FormatWrite(writer, S8(", "));
			if (!FormatWriteArg(writer, FormatArgMake(components[i]), spec))
				return false;
		}
		FormatWrite(writer, S8(")"));
		return true;
	}

	template<typename T>
	static b8 FormatVec2(FormatWriter* writer, const void* value, s8 spec)
	{
		const Vec2<T>* v = (const Vec2<T>*)value;
		T components[] = { v->x, v->y };
		return FormatComponents(writer, components, 2, spec);
	}

	template<typename T>
	static b8 FormatVec3(FormatWriter* writer, const void* value, s8 spec)
	{
		const Vec3<T>* v = (const Vec3<T>*)value;
		T components[] = { v->x, v->y, v->z };
		return FormatComponents(writer, components, 3, spec);
	}

	template<typename T>
	static b8 FormatVec4(FormatWriter* writer, const void* value, s8 spec)
	{
		const Vec4<T>* v = (const Vec4<T>*)value;
		T components[] = { v->x, v->y, v->z, v->w };
		return FormatComponents(writer, components, 4, spec);
	}

	static b8 FormatColor3U(FormatWriter* writer, const void* value, s8 spec)
	{
		const Color3U* c = (const Color3U*)value;
		u8 components[] = { c->r, c->g, c->b };
		return FormatComponents(writer, components, 3, spec);
	}

	static b8 FormatColor4U(FormatWriter* writer, const void* value, s8 spec)
	{
		const Color4U* c = (const Color4U*)value;
		u8 components[] = { c->r, c->g, c->b, c->a };
		return FormatComponents(writer, components, 4, spec);
	}

	static b8 FormatColor3F(FormatWriter* writer, const void* value, s8 spec)
	{
		const Color3F* c = (const Color3F*)value;
		f32 components[] = { c->r, c->g, c->b };
		return FormatComponents(writer, components, 3, spec);
	}

	static b8 FormatColor4F(FormatWriter* writer, const void* value, s8 spec)
	{
		const Color4F* c = (const Color4F*)value;
		f32 components[] = { c->r, c->g, c->b, c->a };
		return FormatComponents(writer, components, 4, spec);
	}

	static b8 FormatQuaternion(FormatWriter* writer, const void* value, s8 spec)
	{
		const Quaternion* q = (const Quaternion*)value;
		f32 components[] = { q->x, q->y, q->z, q->w };
		return FormatComponents(writer, components, 4, spec);
	}

	FormatArg FormatArgMake(const Vec2<f64>& value) { return FormatArgCustom(value, &FormatVec2<f64>); }
	FormatArg FormatArgMake(const Vec3<f64>& value) { return FormatArgCustom(value, &FormatVec3<f64>); }
	FormatArg FormatArgMake(const Vec4<f64>& value) { return FormatArgCustom(value, &FormatVec4<f64>); }
	FormatArg FormatArgMake(const Vec2<f32>& value) { return FormatArgCustom(value, &FormatVec2<f32>); }
	FormatArg FormatArgMake(const Vec3<f32>& value) { return FormatArgCustom(value, &FormatVec3<f32>); }
	FormatArg FormatArgMake(const Vec4<f32>& value) { return FormatArgCustom(value, &FormatVec4<f32>); }
	FormatArg FormatArgMake(const Vec2<i32>& value) { return FormatArgCustom(value, &FormatVec2<i32>); }
	FormatArg FormatArgMake(const Vec3<i32>& value) { return FormatArgCustom(value, &FormatVec3<i32>); }
	FormatArg FormatArgMake(const Vec4<i32>& value) { return FormatArgCustom(value, &FormatVec4<i32>); }
	FormatArg FormatArgMake(const Vec2<u32>& value) { return FormatArgCustom(value, &FormatVec2<u32>); }
	FormatArg FormatArgMake(const Vec3<u32>& value) { return FormatArgCustom(value, &FormatVec3<u32>); }
	FormatArg FormatArgMake(const Vec4<u32>& value) { return FormatArgCustom(value, &FormatVec4<u32>); }
	FormatArg FormatArgMake(const Color3U& value) { return FormatArgCustom(value, &FormatColor3U); }
	FormatArg FormatArgMake(const Color4U& value) { return FormatArgCustom(value, &FormatColor4U); }
	FormatArg FormatArgMake(const Color3F& value) { return FormatArgCustom(value, &FormatColor3F); }
	FormatArg FormatArgMake(const Color4F& value) { return FormatArgCustom(value, &FormatColor4F); }
	FormatArg FormatArgMake(const Quaternion& value) { return FormatArgCustom(value, &FormatQuaternion); }
}
