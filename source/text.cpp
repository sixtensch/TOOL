#include "text.h"
#include "error.h"

#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__)
#include <xlocale.h>
#endif



namespace Tool
{
    //- UTF encoding
    // Decoders return CodepointInvalid for bytes that aren't a valid character, so callers can tell them from a
    // U+FFFD that was in the text. Public functions turn it into U+FFFD.

    static constexpr u32 CodepointReplacement = 0xFFFD;
    static constexpr u32 CodepointInvalid = 0xFFFFFFFF;
    static constexpr u32 CodepointMax = 0x10FFFF;

    static b8 CodepointIsValid(u32 c)
    {
        return c <= CodepointMax && (c < 0xD800 || c > 0xDFFF);
    }

    //~ UTF-8

    static u32 UTF8Length(u32 c)
    {
        return c < 0x80 ? 1 : c < 0x800 ? 2 : c < 0x10000 ? 3 : 4;
    }

    // Consumes the lead byte and every continuation byte that fits it, so a broken sequence is skipped as one.
    static u32 UTF8Decode(const c8* bytes, u64 size, u64* index)
    {
        const u8* s = (const u8*)bytes;
        u64 i = *index;
        u8 lead = s[i];

        u32 length = 0;
        u32 c = 0;
        if (lead < 0x80)
        {
            *index = i + 1;
            return lead;
        }
        else if ((lead & 0xE0) == 0xC0)
        {
            length = 2;
            c = lead & 0x1F;
        }
        else if ((lead & 0xF0) == 0xE0)
        {
            length = 3;
            c = lead & 0x0F;
        }
        else if ((lead & 0xF8) == 0xF0)
        {
            length = 4;
            c = lead & 0x07;
        }
        else
        {
            *index = i + 1; // A stray continuation byte, or a lead byte no character uses
            return CodepointInvalid;
        }

        u64 j = i + 1;
        for (u32 k = 1; k < length; k++, j++)
        {
            if (j >= size || (s[j] & 0xC0) != 0x80)
            {
                *index = j;
                return CodepointInvalid;
            }

            c = (c << 6) | (s[j] & 0x3F);
        }

        *index = j;

        // Longer than needed, a UTF-16 surrogate, or past Unicode
        if (UTF8Length(c) != length || !CodepointIsValid(c))
            return CodepointInvalid;

        return c;
    }

    static u32 UTF8Encode(u32 c, c8* out)
    {
        u8* o = (u8*)out;
        switch (UTF8Length(c))
        {
            case 1:
                o[0] = (u8)c;
                return 1;
            case 2:
                o[0] = (u8)(0xC0 | (c >> 6));
                o[1] = (u8)(0x80 | (c & 0x3F));
                return 2;
            case 3:
                o[0] = (u8)(0xE0 | (c >> 12));
                o[1] = (u8)(0x80 | ((c >> 6) & 0x3F));
                o[2] = (u8)(0x80 | (c & 0x3F));
                return 3;
            default:
                o[0] = (u8)(0xF0 | (c >> 18));
                o[1] = (u8)(0x80 | ((c >> 12) & 0x3F));
                o[2] = (u8)(0x80 | ((c >> 6) & 0x3F));
                o[3] = (u8)(0x80 | (c & 0x3F));
                return 4;
        }
    }

    // The longest start of 'bytes' that doesn't end partway into a character.
    static u64 UTF8TrimPartial(const c8* bytes, u64 size)
    {
        const u8* s = (const u8*)bytes;

        u64 lead = size;
        while (lead > 0 && size - lead < 4 && (s[lead - 1] & 0xC0) == 0x80)
            lead--;

        if (lead == 0 || size - lead >= 4)
            return size; // No lead byte in reach: not UTF-8 to protect

        lead--;
        u8 first = s[lead];
        u64 length = first < 0x80 ? 1 : (first & 0xE0) == 0xC0 ? 2 : (first & 0xF0) == 0xE0 ? 3 : (first & 0xF8) == 0xF0 ? 4 : 1;
        return size - lead < length ? lead : size;
    }

    //~ UTF-16

    static u32 UTF16Length(u32 c)
    {
        return c >= 0x10000 ? 2 : 1;
    }

    static u32 UTF16Decode(const c16* units, u64 count, u64* index)
    {
        u64 i = *index;
        u32 high = units[i];
        *index = i + 1;

        if (high < 0xD800 || high > 0xDFFF)
            return high;

        // A low surrogate first, or a high one with no low after it
        if (high >= 0xDC00 || i + 1 >= count)
            return CodepointInvalid;

        u32 low = units[i + 1];
        if (low < 0xDC00 || low > 0xDFFF)
            return CodepointInvalid;

        *index = i + 2;
        return 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
    }

    static u32 UTF16Encode(u32 c, c16* out)
    {
        if (c < 0x10000)
        {
            out[0] = (c16)c;
            return 1;
        }

        c -= 0x10000;
        out[0] = (c16)(0xD800 | (c >> 10));
        out[1] = (c16)(0xDC00 | (c & 0x3FF));
        return 2;
    }

    //~ Conversion
    // Into 'capacity' units, without a terminator, stopping before a character that doesn't fit. Returns the units
    // written, or with a null destination, the units the whole conversion needs.

    static u64 UTF8ToUTF16(const c8* source, u64 size, c16* destination, u64 capacity)
    {
        u64 written = 0;
        for (u64 i = 0; i < size;)
        {
            u32 c = UTF8Decode(source, size, &i);
            if (c == CodepointInvalid)
                c = CodepointReplacement;

            u32 length = UTF16Length(c);
            if (destination != nullptr)
            {
                if (written + length > capacity)
                    break;
                UTF16Encode(c, destination + written);
            }
            written += length;
        }

        return written;
    }

    static u64 UTF16ToUTF8(const c16* source, u64 count, c8* destination, u64 capacity)
    {
        u64 written = 0;
        for (u64 i = 0; i < count;)
        {
            u32 c = UTF16Decode(source, count, &i);
            if (c == CodepointInvalid)
                c = CodepointReplacement;

            u32 length = UTF8Length(c);
            if (destination != nullptr)
            {
                if (written + length > capacity)
                    break;
                UTF8Encode(c, destination + written);
            }
            written += length;
        }

        return written;
    }



    //- Locale
    // printf and strtod follow the C library's locale, which an application (Qt on Linux, for one) may set to use ','
    // for decimals. Every call here uses the "C" locale instead.

#if defined(TOOL_WINDOWS)

    static _locale_t LocaleC()
    {
        static _locale_t locale = _create_locale(LC_ALL, "C");
        return locale;
    }

    // vsnprintf with C99's return: the full length, however much of it fit.
    static i32 FormatList(c8* destination, u64 capacity, const c8* format, va_list arguments)
    {
        return __stdio_common_vsprintf(_CRT_INTERNAL_LOCAL_PRINTF_OPTIONS | _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR,
                                       destination, (size_t)capacity, format, LocaleC(), arguments);
    }

    static f64 ParseDouble(const c8* cstr, c8** end)
    {
        return _strtod_l(cstr, end, LocaleC());
    }

#else

    static locale_t LocaleC()
    {
        static locale_t locale = newlocale(LC_ALL_MASK, "C", (locale_t)0);
        return locale;
    }

    static i32 FormatList(c8* destination, u64 capacity, const c8* format, va_list arguments)
    {
        locale_t previous = uselocale(LocaleC());
        i32 length = vsnprintf(destination, (size_t)capacity, format, arguments);
        uselocale(previous);
        return length;
    }

    static f64 ParseDouble(const c8* cstr, c8** end)
    {
        locale_t previous = uselocale(LocaleC());
        f64 value = strtod(cstr, end);
        uselocale(previous);
        return value;
    }

#endif



    //- Helpers

    static s8 S8Allocate(u64 size, MemoryAllocator allocator)
    {
        c8* str = (c8*)AllocatorAlloc(allocator, size + 1);
        if (str == nullptr)
        {
            TOOL_FAIL("Could not allocate a string of %llu bytes.", (unsigned long long)size + 1);
            return {};
        }

        str[size] = 0;
        return { str, size };
    }

    static b8 BytesEqual(const c8* a, const c8* b, u64 size)
    {
        return size == 0 || memcmp(a, b, size) == 0;
    }



    //- Making strings

    s8 S8Copy(s8 string, MemoryAllocator allocator)
    {
        s8 copy = S8Allocate(string.size, allocator);
        if (copy.str != nullptr && string.size != 0)
            memcpy(copy.str, string.str, string.size);

        return copy;
    }

    s8 S8Concat(s8 a, s8 b, MemoryAllocator allocator)
    {
        s8 result = S8Allocate(a.size + b.size, allocator);
        if (result.str == nullptr)
            return result;

        if (a.size != 0)
            memcpy(result.str, a.str, a.size);
        if (b.size != 0)
            memcpy(result.str + a.size, b.str, b.size);

        return result;
    }

    s8 S8Format(MemoryAllocator allocator, const c8* format, ...)
    {
        va_list arguments;
        va_list again;
        va_start(arguments, format);
        va_copy(again, arguments);

        // Measure, then write
        i32 length = FormatList(nullptr, 0, format, arguments);
        va_end(arguments);

        s8 result = {};
        if (length < 0)
            TOOL_FAIL("Could not format \"%s\".", format);
        else
            result = S8Allocate((u64)length, allocator);

        if (result.str != nullptr)
            FormatList(result.str, (u64)length + 1, format, again);

        va_end(again);
        return result;
    }



    //- Comparing and searching

    b8 S8Equal(s8 a, s8 b)
    {
        return a.size == b.size && BytesEqual(a.str, b.str, a.size);
    }

    b8 S8EqualIgnoreCase(s8 a, s8 b)
    {
        if (a.size != b.size)
            return false;

        for (u64 i = 0; i < a.size; i++)
            if (C8ToLower(a.str[i]) != C8ToLower(b.str[i]))
                return false;

        return true;
    }

    i32 S8Compare(s8 a, s8 b)
    {
        u64 shared = a.size < b.size ? a.size : b.size;
        i32 order = shared == 0 ? 0 : memcmp(a.str, b.str, shared);
        if (order != 0)
            return order < 0 ? -1 : 1;

        return a.size < b.size ? -1 : a.size > b.size ? 1 : 0;
    }

    b8 S8StartsWith(s8 string, s8 prefix)
    {
        return prefix.size <= string.size && BytesEqual(string.str, prefix.str, prefix.size);
    }

    b8 S8EndsWith(s8 string, s8 suffix)
    {
        return suffix.size <= string.size && BytesEqual(string.str + string.size - suffix.size, suffix.str, suffix.size);
    }

    u64 S8Find(s8 string, s8 needle, u64 start)
    {
        if (start > string.size || needle.size > string.size - start)
            return S8_NONE;

        if (needle.size == 0)
            return start;

        for (u64 i = start; i <= string.size - needle.size; i++)
            if (string.str[i] == needle.str[0] && BytesEqual(string.str + i, needle.str, needle.size))
                return i;

        return S8_NONE;
    }

    u64 S8FindLast(s8 string, s8 needle)
    {
        if (needle.size > string.size)
            return S8_NONE;

        for (u64 i = string.size - needle.size + 1; i > 0; i--)
            if (BytesEqual(string.str + i - 1, needle.str, needle.size))
                return i - 1;

        return S8_NONE;
    }



    //- Slicing

    s8 S8Slice(s8 string, u64 start, u64 end)
    {
        if (end > string.size)
            end = string.size;
        if (start > end)
            start = end;

        return { string.str + start, end - start };
    }

    s8 S8TrimStart(s8 string)
    {
        u64 start = 0;
        while (start < string.size && C8IsSpace(string.str[start]))
            start++;

        return { string.str + start, string.size - start };
    }

    s8 S8TrimEnd(s8 string)
    {
        u64 size = string.size;
        while (size > 0 && C8IsSpace(string.str[size - 1]))
            size--;

        return { string.str, size };
    }

    s8 S8Trim(s8 string)
    {
        return S8TrimEnd(S8TrimStart(string));
    }

    b8 S8Cut(s8 string, s8 separator, s8* outBefore, s8* outAfter)
    {
        u64 at = S8Find(string, separator);
        s8 before = string;
        s8 after = { string.str + string.size, 0 };

        if (at != S8_NONE)
        {
            before = { string.str, at };
            after = { string.str + at + separator.size, string.size - at - separator.size };
        }

        if (outBefore != nullptr)
            *outBefore = before;
        if (outAfter != nullptr)
            *outAfter = after;

        return at != S8_NONE;
    }



    //- Changing case

    s8 S8ToLower(s8 string, MemoryAllocator allocator)
    {
        s8 result = S8Allocate(string.size, allocator);
        for (u64 i = 0; result.str != nullptr && i < string.size; i++)
            result.str[i] = C8ToLower(string.str[i]);

        return result;
    }

    s8 S8ToUpper(s8 string, MemoryAllocator allocator)
    {
        s8 result = S8Allocate(string.size, allocator);
        for (u64 i = 0; result.str != nullptr && i < string.size; i++)
            result.str[i] = C8ToUpper(string.str[i]);

        return result;
    }



    //- Numbers

    b8 S8ParseU64(s8 string, u64* outValue)
    {
        u64 base = 10;
        u64 i = 0;
        if (string.size > 2 && string.str[0] == '0' && (string.str[1] == 'x' || string.str[1] == 'X'))
        {
            base = 16;
            i = 2;
        }

        if (i >= string.size)
            return false;

        u64 value = 0;
        for (; i < string.size; i++)
        {
            c8 c = string.str[i];
            u64 digit = 0;
            if (C8IsDigit(c))
                digit = (u64)(c - '0');
            else if (base == 16 && C8IsHexDigit(c))
                digit = (u64)(C8ToLower(c) - 'a' + 10);
            else
                return false;

            if (value > (U64_MAX - digit) / base)
                return false; // Too large

            value = value * base + digit;
        }

        *outValue = value;
        return true;
    }

    b8 S8ParseI64(s8 string, i64* outValue)
    {
        b8 negative = string.size > 0 && string.str[0] == '-';
        b8 sign = negative || (string.size > 0 && string.str[0] == '+');

        u64 magnitude = 0;
        if (!S8ParseU64(S8Slice(string, sign ? 1 : 0, string.size), &magnitude))
            return false;

        // The magnitude of I64_MIN is one more than I64_MAX
        if (magnitude > (u64)I64_MAX + (negative ? 1 : 0))
            return false;

        *outValue = negative ? (i64)(0 - magnitude) : (i64)magnitude;
        return true;
    }

    b8 S8ParseF64(s8 string, f64* outValue)
    {
        // strtod needs a terminator, and skips leading spaces, which aren't allowed here
        c8 buffer[512];
        if (string.size == 0 || string.size >= sizeof(buffer) || C8IsSpace(string.str[0]))
            return false;

        memcpy(buffer, string.str, string.size);
        buffer[string.size] = 0;

        c8* end = nullptr;
        errno = 0;
        f64 value = ParseDouble(buffer, &end);
        if (end != buffer + string.size)
            return false;

        // Too large in magnitude. Too small rounds toward zero, which is kept.
        if (errno == ERANGE && (value > 1.0 || value < -1.0))
            return false;

        *outValue = value;
        return true;
    }



    //- Code points

    u32 S8DecodeCodepoint(s8 string, u64* index)
    {
        u32 c = UTF8Decode(string.str, string.size, index);
        return c == CodepointInvalid ? CodepointReplacement : c;
    }

    u32 CodepointEncodeUTF8(u32 codepoint, c8* outBytes)
    {
        return UTF8Encode(CodepointIsValid(codepoint) ? codepoint : CodepointReplacement, outBytes);
    }

    u64 S8CodepointCount(s8 string)
    {
        u64 count = 0;
        for (u64 i = 0; i < string.size; count++)
            UTF8Decode(string.str, string.size, &i);

        return count;
    }

    b8 S8IsValidUTF8(s8 string)
    {
        for (u64 i = 0; i < string.size;)
            if (UTF8Decode(string.str, string.size, &i) == CodepointInvalid)
                return false;

        return true;
    }



    //- UTF-16

    s16 S16FromS8(s8 string, MemoryAllocator allocator)
    {
        u64 count = UTF8ToUTF16(string.str, string.size, nullptr, 0);
        c16* str = (c16*)AllocatorAlloc(allocator, (count + 1) * sizeof(c16));
        if (str == nullptr)
        {
            TOOL_FAIL("Could not allocate a string of %llu bytes.", (unsigned long long)(count + 1) * sizeof(c16));
            return {};
        }

        UTF8ToUTF16(string.str, string.size, str, count);
        str[count] = 0;
        return { str, count };
    }

    s8 S8FromS16(s16 string, MemoryAllocator allocator)
    {
        u64 size = UTF16ToUTF8(string.str, string.size, nullptr, 0);
        s8 result = S8Allocate(size, allocator);
        if (result.str != nullptr)
            UTF16ToUTF8(string.str, string.size, result.str, size);

        return result;
    }

    u64 S8ToUTF16(s8 string, c16* destination, u64 capacity)
    {
        if (capacity == 0)
            return 0;

        u64 written = UTF8ToUTF16(string.str, string.size, destination, capacity - 1);
        destination[written] = 0;
        return written;
    }

    u64 S16ToUTF8(s16 string, c8* destination, u64 capacity)
    {
        if (capacity == 0)
            return 0;

        u64 written = UTF16ToUTF8(string.str, string.size, destination, capacity - 1);
        destination[written] = 0;
        return written;
    }



    //- C strings

    u64 CStr8Size(const c8* string)
    {
        return string == nullptr ? 0 : strlen(string);
    }

    u64 CStr16Count(const c16* string)
    {
        u64 count = 0;
        if (string != nullptr)
            while (string[count] != 0)
                count++;

        return count;
    }

    u64 CStr8Copy(c8* destination, const c8* source, u64 capacity)
    {
        if (capacity == 0)
            return 0;

        u64 size = CStr8Size(source);
        if (size > capacity - 1)
            size = UTF8TrimPartial(source, capacity - 1);

        if (size != 0)
            memcpy(destination, source, size);
        destination[size] = 0;
        return size;
    }

    u64 CStr16Copy(c16* destination, const c16* source, u64 capacity)
    {
        if (capacity == 0)
            return 0;

        u64 count = CStr16Count(source);
        if (count > capacity - 1)
        {
            count = capacity - 1;
            if (count > 0 && source[count - 1] >= 0xD800 && source[count - 1] <= 0xDBFF)
                count--; // Not half a surrogate pair
        }

        if (count != 0)
            memcpy(destination, source, count * sizeof(c16));
        destination[count] = 0;
        return count;
    }

    u64 CStr8Format(c8* destination, u64 capacity, const c8* format, ...)
    {
        if (capacity == 0)
            return 0;

        va_list arguments;
        va_start(arguments, format);
        i32 length = FormatList(destination, capacity, format, arguments);
        va_end(arguments);

        if (length < 0)
        {
            destination[0] = 0;
            TOOL_FAIL("Could not format \"%s\".", format);
            return 0;
        }

        if ((u64)length < capacity)
            return (u64)length;

        // Cut short, so drop a character the cut split
        u64 written = UTF8TrimPartial(destination, capacity - 1);
        destination[written] = 0;
        return written;
    }



    //- String builder

    // Whether 'addedBytes' more content, plus a terminator of 'terminatorBytes', fits the builder
    static b8 StringBuilderFits(StringBuilder* builder, u64 addedBytes, u64 terminatorBytes)
    {
        if (builder->size + addedBytes + terminatorBytes > builder->capacity)
        {
            return TOOL_FAIL("String does not fit the String Builder. (%llu + %llu bytes, %llu available)",
                             (unsigned long long)builder->size, (unsigned long long)addedBytes,
                             (unsigned long long)(builder->capacity - terminatorBytes));
        }

        return true;
    }

    static b8 StringBuilderAddUTF8(StringBuilder* builder, const c8* string, u64 size)
    {
        switch (builder->type)
        {
            case StringTypeUTF8:
            {
                if (!StringBuilderFits(builder, size, 1))
                    return false;

                if (size != 0)
                    memcpy(builder->str8 + builder->size, string, size);
                builder->size += size;
                builder->str8[builder->size] = 0;
                return true;
            }

            case StringTypeUTF16:
            {
                u64 count = UTF8ToUTF16(string, size, nullptr, 0);
                if (!StringBuilderFits(builder, count * 2, 2))
                    return false;

                UTF8ToUTF16(string, size, builder->str16 + builder->size / 2, count);
                builder->size += count * 2;
                builder->str16[builder->size / 2] = 0;
                return true;
            }

            default:
                return false;
        }
    }

    static b8 StringBuilderAddUTF16(StringBuilder* builder, const c16* string, u64 count)
    {
        switch (builder->type)
        {
            case StringTypeUTF8:
            {
                u64 size = UTF16ToUTF8(string, count, nullptr, 0);
                if (!StringBuilderFits(builder, size, 1))
                    return false;

                UTF16ToUTF8(string, count, builder->str8 + builder->size, size);
                builder->size += size;
                builder->str8[builder->size] = 0;
                return true;
            }

            case StringTypeUTF16:
            {
                if (!StringBuilderFits(builder, count * 2, 2))
                    return false;

                if (count != 0)
                    memcpy(builder->str16 + builder->size / 2, string, count * 2);
                builder->size += count * 2;
                builder->str16[builder->size / 2] = 0;
                return true;
            }

            default:
                return false;
        }
    }

    b8 StringBuilderInit(StringBuilder* builder, u64 capacity, StringType type)
    {
        // Room for the widest terminator, so an empty builder is always a valid string
        TOOL_ASSERT(capacity >= 2, "A String Builder needs a capacity of at least 2 bytes. (%llu)", (unsigned long long)capacity);

        *builder = {};

        builder->str8 = (c8*)ClassicAlloc(capacity);
        if (builder->str8 == nullptr)
            return false;

        builder->type = type;
        builder->capacity = capacity;
        builder->str16[0] = 0;
        return true;
    }

    void StringBuilderDestroy(StringBuilder* builder)
    {
        ClassicDealloc(builder->str8);
        *builder = {};
    }

    void StringBuilderReset(StringBuilder* builder)
    {
        builder->size = 0;
        builder->str16[0] = 0;
    }

    b8 StringBuilderAdd(StringBuilder* builder, s8 string)
    {
        return StringBuilderAddUTF8(builder, string.str, string.size);
    }

    b8 StringBuilderAdd(StringBuilder* builder, s16 string)
    {
        return StringBuilderAddUTF16(builder, string.str, string.size);
    }

    b8 StringBuilderAdd(StringBuilder* builder, const c8* cstr)
    {
        return StringBuilderAddUTF8(builder, cstr, CStr8Size(cstr));
    }

    b8 StringBuilderAdd(StringBuilder* builder, const c16* cstr)
    {
        return StringBuilderAddUTF16(builder, cstr, CStr16Count(cstr));
    }
}
