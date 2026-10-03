#include "tool.h"
#include "test.h"
#include <locale.h>
#include <stdio.h>
#include <string.h>

using namespace Tool;



static b8 Is(s8 string, const c8* expected)
{
	return S8Equal(string, S8(expected));
}

static_assert(S8("abc").size == 3 && S8("").size == 0, "S8 sizes literals at compile time");

static void TestCharacters()
{
	const c8* spaces = " \t\n\v\f\r";
	for (const c8* c = spaces; *c != 0; c++)
		TOOL_ASSERT(C8IsSpace(*c));
	TOOL_ASSERT(!C8IsSpace('a') && !C8IsSpace(0) && !C8IsSpace((c8)0xA0));

	TOOL_ASSERT(C8IsDigit('0') && C8IsDigit('9') && !C8IsDigit('a') && !C8IsDigit('/'));
	TOOL_ASSERT(C8IsHexDigit('f') && C8IsHexDigit('F') && !C8IsHexDigit('g'));
	TOOL_ASSERT(C8IsAlpha('z') && C8IsAlpha('A') && !C8IsAlpha('1') && !C8IsAlpha((c8)0xC3));
	TOOL_ASSERT(C8IsAlphaNumeric('7') && !C8IsAlphaNumeric('_'));
	TOOL_ASSERT(C8ToLower('Q') == 'q' && C8ToLower('q') == 'q' && C8ToLower('@') == '@' && C8ToLower((c8)0xC3) == (c8)0xC3);
	TOOL_ASSERT(C8ToUpper('q') == 'Q' && C8ToUpper('[') == '[');
}

static void TestMaking()
{
	TOOL_ASSERT(S8(nullptr).size == 0);

	s8 copy = S8Copy(S8("copy"), Allocator());
	TOOL_ASSERT(Is(copy, "copy") && copy.str[4] == 0);
	s8 joined = S8Concat(S8("ab"), S8("cd"), Allocator());
	TOOL_ASSERT(Is(joined, "abcd") && joined.str[4] == 0);

	s8 formatted = S8Format(Allocator(), "%d-%s-%.2f-%llu", 42, "x", 1.5, 18446744073709551615ull);
	TOOL_ASSERT(Is(formatted, "42-x-1.50-18446744073709551615") && formatted.str[formatted.size] == 0);
	s8 wide = S8Format(Allocator(), "%0300d", 7);
	TOOL_ASSERT(wide.size == 300 && wide.str[299] == '7' && wide.str[0] == '0');
	s8 empty = S8Format(Allocator(), "%s", "");
	TOOL_ASSERT(empty.size == 0 && empty.str != nullptr && empty.str[0] == 0);

	AllocatorDealloc(Allocator(), copy.str);
	AllocatorDealloc(Allocator(), joined.str);
	AllocatorDealloc(Allocator(), formatted.str);
	AllocatorDealloc(Allocator(), wide.str);
	AllocatorDealloc(Allocator(), empty.str);
}

static void TestSearching()
{
	TOOL_ASSERT(S8Equal(S8(""), S8("")) && !S8Equal(S8("a"), S8("ab")));
	TOOL_ASSERT(S8EqualIgnoreCase(S8("HeLLo"), S8("hello")) && !S8EqualIgnoreCase(S8("hello"), S8("hellp")));
	TOOL_ASSERT(S8Compare(S8("abc"), S8("abd")) < 0 && S8Compare(S8("abd"), S8("abc")) > 0);
	TOOL_ASSERT(S8Compare(S8("ab"), S8("abc")) < 0 && S8Compare(S8("abc"), S8("ab")) > 0 && S8Compare(S8("x"), S8("x")) == 0);
	TOOL_ASSERT(S8Compare(S8("\xC3"), S8("a")) > 0); // Bytes compare unsigned

	s8 text = S8("hello world");
	TOOL_ASSERT(S8StartsWith(text, S8("hello")) && !S8StartsWith(text, S8("world")) && S8StartsWith(text, S8("")));
	TOOL_ASSERT(S8EndsWith(text, S8("world")) && !S8EndsWith(text, S8("hello")) && !S8EndsWith(S8("d"), text));
	TOOL_ASSERT(S8Find(text, S8("o")) == 4 && S8Find(text, S8("o"), 5) == 7 && S8Find(text, S8("o"), 8) == S8_NONE);
	TOOL_ASSERT(S8Find(text, S8("xyz")) == S8_NONE && S8Find(text, S8("world")) == 6 && S8Find(text, S8("world!")) == S8_NONE);
	TOOL_ASSERT(S8Find(text, S8(""), 3) == 3 && S8Find(text, S8(""), 99) == S8_NONE);
	TOOL_ASSERT(S8FindLast(text, S8("o")) == 7 && S8FindLast(text, S8("h")) == 0 && S8FindLast(text, S8("q")) == S8_NONE);
	TOOL_ASSERT(S8FindLast(text, S8("")) == text.size);
}

static void TestSlicing()
{
	s8 text = S8("hello world");
	TOOL_ASSERT(Is(S8Slice(text, 6, 11), "world") && Is(S8Slice(text, 6, 99), "world") && S8Slice(text, 9, 3).size == 0);
	TOOL_ASSERT(Is(S8Trim(S8("  \t hi there \n")), "hi there") && S8Trim(S8(" \t\n ")).size == 0);
	TOOL_ASSERT(Is(S8TrimStart(S8("  x ")), "x ") && Is(S8TrimEnd(S8("  x ")), "  x"));

	s8 before;
	s8 after;
	TOOL_ASSERT(S8Cut(S8("key=value=x"), S8("="), &before, &after) && Is(before, "key") && Is(after, "value=x"));
	TOOL_ASSERT(!S8Cut(S8("nothing"), S8("="), &before, &after) && Is(before, "nothing") && after.size == 0);
	TOOL_ASSERT(S8Cut(S8("a::b"), S8("::"), nullptr, &after) && Is(after, "b"));

	// A whole list, empty items included.
	const c8* expected[] = { "a", "b", "", "c" };
	s8 rest = S8("a,b,,c");
	u32 count = 0;
	b8 more = true;
	while (more)
	{
		s8 item;
		more = S8Cut(rest, S8(","), &item, &rest);
		TOOL_ASSERT(count < 4 && Is(item, expected[count]));
		count++;
	}
	TOOL_ASSERT(count == 4);

	s8 lower = S8ToLower(S8("Hello \xC3\x9C!"), Allocator());
	s8 upper = S8ToUpper(S8("Hello \xC3\xBC!"), Allocator());
	TOOL_ASSERT(Is(lower, "hello \xC3\x9C!") && Is(upper, "HELLO \xC3\xBC!") && lower.str[lower.size] == 0);
	AllocatorDealloc(Allocator(), lower.str);
	AllocatorDealloc(Allocator(), upper.str);
}

static void TestNumbers()
{
	u64 u = 0;
	TOOL_ASSERT(S8ParseU64(S8("0"), &u) && u == 0);
	TOOL_ASSERT(S8ParseU64(S8("18446744073709551615"), &u) && u == U64_MAX);
	TOOL_ASSERT(S8ParseU64(S8("0xFFffFFffFFffFFff"), &u) && u == U64_MAX);
	TOOL_ASSERT(S8ParseU64(S8("0x1F"), &u) && u == 31);
	const c8* badUnsigned[] = { "18446744073709551616", "0x10000000000000000", "0x", "", " 1", "1 ", "-1", "+1", "0x1g", "12a", "1.0" };
	for (const c8* bad : badUnsigned)
		TOOL_ASSERT(!S8ParseU64(S8(bad), &u), "Parsed %s", bad);

	i64 i = 0;
	TOOL_ASSERT(S8ParseI64(S8("-9223372036854775808"), &i) && i == I64_MIN);
	TOOL_ASSERT(S8ParseI64(S8("9223372036854775807"), &i) && i == I64_MAX);
	TOOL_ASSERT(S8ParseI64(S8("+5"), &i) && i == 5 && S8ParseI64(S8("-0x10"), &i) && i == -16);
	const c8* badSigned[] = { "9223372036854775808", "-9223372036854775809", "-", "+", "--1", "+-1", "" };
	for (const c8* bad : badSigned)
		TOOL_ASSERT(!S8ParseI64(S8(bad), &i), "Parsed %s", bad);

	f64 f = 0.0;
	TOOL_ASSERT(S8ParseF64(S8("1.5"), &f) && f == 1.5);
	TOOL_ASSERT(S8ParseF64(S8("-2.5e3"), &f) && f == -2500.0);
	TOOL_ASSERT(S8ParseF64(S8("0.1"), &f) && f == 0.1);
	TOOL_ASSERT(S8ParseF64(S8("1e-400"), &f) && f >= 0.0 && f < 1e-300);
	TOOL_ASSERT(S8ParseF64(S8("inf"), &f) && f > 1e308);
	const c8* badFloats[] = { "1e400", "-1e400", "1.5x", " 1", "1 ", "", "1,5", "." };
	for (const c8* bad : badFloats)
		TOOL_ASSERT(!S8ParseF64(S8(bad), &f), "Parsed %s", bad);

	// A slice of a longer string parses alone.
	TOOL_ASSERT(S8ParseU64(S8Slice(S8("12345"), 1, 3), &u) && u == 23);
	TOOL_ASSERT(S8ParseF64(S8Slice(S8("2.75 rest"), 0, 4), &f) && f == 2.75);
}

// Formatting and parsing keep '.' when the C library's locale uses ','.
static void TestLocale()
{
	const c8* names[] = { "de-DE", "de_DE.UTF-8", "de_DE.utf8", "sv_SE.UTF-8", "German", "C.UTF-8" };
	const c8* chosen = nullptr;
	for (const c8* name : names)
	{
		if (setlocale(LC_ALL, name) != nullptr)
		{
			c8 probe[16];
			snprintf(probe, sizeof(probe), "%.1f", 1.5);
			if (strcmp(probe, "1,5") == 0)
			{
				chosen = name;
				break;
			}
		}
	}

	if (chosen != nullptr)
	{
		s8 formatted = S8Format(Allocator(), "%.1f", 1.5);
		f64 value = 0.0;
		b8 parsed = S8ParseF64(S8("2.5"), &value);
		b8 comma = S8ParseF64(S8("2,5"), &value);
		setlocale(LC_ALL, "C");
		TOOL_ASSERT(Is(formatted, "1.5") && parsed && !comma);
		AllocatorDealloc(Allocator(), formatted.str);
	}
	else
	{
		setlocale(LC_ALL, "C");
		printf("Text: no comma locale here, locale independence untested\n");
	}
}

static void TestCodepoints()
{
	s8 text = S8("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80"); // a, e acute, euro sign, grinning face
	u32 expected[] = { 'a', 0xE9, 0x20AC, 0x1F600 };
	u64 ends[] = { 1, 3, 6, 10 };
	u64 index = 0;
	for (u32 i = 0; i < 4; i++)
	{
		TOOL_ASSERT(S8DecodeCodepoint(text, &index) == expected[i] && index == ends[i]);

		c8 bytes[4];
		u32 length = CodepointEncodeUTF8(expected[i], bytes);
		TOOL_ASSERT(length == ends[i] - (i == 0 ? 0 : ends[i - 1]) && memcmp(bytes, text.str + ends[i] - length, length) == 0);
	}
	TOOL_ASSERT(S8CodepointCount(text) == 4 && S8IsValidUTF8(text));

	// Surrogates and values past Unicode encode as U+FFFD.
	c8 bytes[4];
	TOOL_ASSERT(CodepointEncodeUTF8(0xD800, bytes) == 3 && memcmp(bytes, "\xEF\xBF\xBD", 3) == 0);
	TOOL_ASSERT(CodepointEncodeUTF8(0x110000, bytes) == 3 && memcmp(bytes, "\xEF\xBF\xBD", 3) == 0);

	// Invalid sequences decode to U+FFFD and move on: overlong, surrogate, past Unicode, cut off, stray.
	const c8* invalid[] = { "\xC0\x80", "\xED\xA0\x80", "\xF4\x90\x80\x80", "\xE2\x82", "\x80", "\xFF" };
	for (const c8* bad : invalid)
	{
		s8 string = S8(bad);
		index = 0;
		TOOL_ASSERT(S8DecodeCodepoint(string, &index) == 0xFFFD && index > 0 && !S8IsValidUTF8(string));
	}

	index = 0;
	s8 cut = S8("\xE2\x82" "a");
	TOOL_ASSERT(S8DecodeCodepoint(cut, &index) == 0xFFFD && index == 2 && S8DecodeCodepoint(cut, &index) == 'a');
	TOOL_ASSERT(S8CodepointCount(S8("a\x80" "b")) == 3);
	TOOL_ASSERT(S8IsValidUTF8(S8("\xEF\xBF\xBD"))); // A U+FFFD in the text is valid
}

static void TestUTF16()
{
	s8 text = S8("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
	s16 wide = S16FromS8(text, Allocator());
	c16 expected[] = { 0x61, 0xE9, 0x20AC, 0xD83D, 0xDE00 };
	TOOL_ASSERT(wide.size == 5 && memcmp(wide.str, expected, sizeof(expected)) == 0 && wide.str[5] == 0);

	s8 back = S8FromS16(wide, Allocator());
	TOOL_ASSERT(S8Equal(back, text) && back.str[back.size] == 0);
	AllocatorDealloc(Allocator(), wide.str);
	AllocatorDealloc(Allocator(), back.str);

	// Unpaired surrogates become U+FFFD.
	c16 broken[] = { 0xD800, 'a', 0xDC00 };
	s8 repaired = S8FromS16({ broken, 3 }, Allocator());
	TOOL_ASSERT(Is(repaired, "\xEF\xBF\xBD" "a" "\xEF\xBF\xBD"));
	AllocatorDealloc(Allocator(), repaired.str);

	// Buffers stop before a character that doesn't fit, and always terminate.
	c16 units[8];
	TOOL_ASSERT(S8ToUTF16(S8("a\xF0\x9F\x98\x80"), units, 3) == 1 && units[0] == 'a' && units[1] == 0);
	TOOL_ASSERT(S8ToUTF16(S8("a\xF0\x9F\x98\x80"), units, 4) == 3 && units[3] == 0);
	TOOL_ASSERT(S8ToUTF16(S8("abc"), units, 0) == 0);

	c8 narrow[8];
	c16 accents[] = { 0xE9, 0x20AC };
	TOOL_ASSERT(S16ToUTF8({ accents, 2 }, narrow, 4) == 2 && memcmp(narrow, "\xC3\xA9", 3) == 0);
	TOOL_ASSERT(S16ToUTF8({ accents, 2 }, narrow, 6) == 5 && narrow[5] == 0);
}

static void TestCStrings()
{
	TOOL_ASSERT(CStr8Size(nullptr) == 0 && CStr8Size("abc") == 3 && CStr16Count(UTF16("abc")) == 3 && CStr16Count(nullptr) == 0);

	// No cap on length.
	u64 longSize = 100000;
	c8* longString = (c8*)AllocatorAlloc(Allocator(), longSize + 1);
	memset(longString, 'x', longSize);
	longString[longSize] = 0;
	TOOL_ASSERT(CStr8Size(longString) == longSize);
	AllocatorDealloc(Allocator(), longString);

	c8 buffer[8];
	TOOL_ASSERT(CStr8Copy(buffer, "abc", sizeof(buffer)) == 3 && strcmp(buffer, "abc") == 0);
	TOOL_ASSERT(CStr8Copy(buffer, "abcdef", 4) == 3 && strcmp(buffer, "abc") == 0);
	TOOL_ASSERT(CStr8Copy(buffer, "a\xC3\xA9", 3) == 1 && strcmp(buffer, "a") == 0); // The e acute would be cut
	TOOL_ASSERT(CStr8Copy(buffer, "a\xC3\xA9", 4) == 3 && strcmp(buffer, "a\xC3\xA9") == 0);
	buffer[0] = 'z';
	TOOL_ASSERT(CStr8Copy(buffer, "abc", 0) == 0 && buffer[0] == 'z');

	c16 wide[4];
	const c16 emoji[] = { 'a', 0xD83D, 0xDE00, 0 };
	TOOL_ASSERT(CStr16Copy(wide, emoji, 3) == 1 && wide[1] == 0); // Not half a pair
	TOOL_ASSERT(CStr16Copy(wide, emoji, 4) == 3 && wide[3] == 0);

	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "%s=%d", "x", 42) == 4 && strcmp(buffer, "x=42") == 0);
	TOOL_ASSERT(CStr8Format(buffer, 4, "%s", "abcdef") == 3 && strcmp(buffer, "abc") == 0);
	TOOL_ASSERT(CStr8Format(buffer, 4, "%s", "a\xC3\xA9\xE2\x82\xAC") == 3 && strcmp(buffer, "a\xC3\xA9") == 0);
	TOOL_ASSERT(CStr8Format(buffer, 3, "%s", "a\xC3\xA9") == 1 && strcmp(buffer, "a") == 0);
	TOOL_ASSERT(CStr8Format(buffer, 0, "%d", 1) == 0);

	// Appending formatted pieces never runs past the buffer.
	c8 line[16];
	u64 at = 0;
	for (u32 i = 0; i < 10; i++)
		at += CStr8Format(line + at, sizeof(line) - at, "[%u]", i);
	TOOL_ASSERT(at < sizeof(line) && strcmp(line, "[0][1][2][3][4]") == 0);
}

static void TestBuilder()
{
	StringBuilder builder;
	TOOL_ASSERT(StringBuilderInit(&builder, 32, StringTypeUTF16));
	const c16 eAcute[] = { 0xE9, 0 };
	TOOL_ASSERT(StringBuilderAdd(&builder, S8("ab")) && StringBuilderAdd(&builder, eAcute) &&StringBuilderAdd(&builder, "\xF0\x9F\x98\x80"));
	c16 expected[] = { 'a', 'b', 0xE9, 0xD83D, 0xDE00, 0 };
	TOOL_ASSERT(builder.size == 10 && memcmp(builder.str16, expected, sizeof(expected)) == 0, "size %llu, units %04x %04x %04x %04x %04x",
		(unsigned long long)builder.size, builder.str16[0], builder.str16[1], builder.str16[2], builder.str16[3], builder.str16[4]);
	StringBuilderDestroy(&builder);

	TOOL_ASSERT(StringBuilderInit(&builder, 8, StringTypeUTF8));
	c16 accent[] = { 0xE9 };
	TOOL_ASSERT(StringBuilderAdd(&builder, s16 { accent, 1 }) && StringBuilderAdd(&builder, "xyz"));
	TOOL_ASSERT(strcmp(builder.str8, "\xC3\xA9xyz") == 0);
	TOOL_ASSERT(!StringBuilderAdd(&builder, "abc") && strcmp(builder.str8, "\xC3\xA9xyz") == 0); // Leaves it unchanged
	StringBuilderReset(&builder);
	TOOL_ASSERT(builder.size == 0 && builder.str8[0] == 0);
	StringBuilderDestroy(&builder);
}



void TestText()
{
	TestCharacters();
	TestMaking();
	TestSearching();
	TestSlicing();
	TestNumbers();
	TestLocale();
	TestCodepoints();
	TestUTF16();
	TestCStrings();
	TestBuilder();
	printf("Text: passed\n");
}
