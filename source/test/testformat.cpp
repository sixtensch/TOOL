#include "tool.h"
#include "tool/format.h"
#include "test.h"

#include <format>
#include <stdio.h>
#include <string.h>
#include <string>

using namespace Tool;



//- Helpers

static u32 formatFailures = 0;

static void Report(const c8* format, const c8* what, const c8* expected, const c8* actual)
{
	if (formatFailures < 40)
		printf("Format mismatch for \"%s\" (%s): std \"%s\", TOOL \"%s\"\n", format, what, expected, actual);
	formatFailures++;
}

// Renders through the runtime path and compares with std::vformat, validity included.
template<typename... A>
static void Compare(const c8* format, const A&... values)
{
	std::string expected;
	b8 expectedValid = true;
	try
	{
		expected = std::vformat(format, std::make_format_args(values...));
	}
	catch (...) // format_error, or bad_alloc for huge widths in libstdc++
	{
		expectedValid = false;
	}

	const FormatArg args[sizeof...(A) + 1] = { FormatArgMake(values)..., FormatArg{} };
	static c8 buffer[8192];
	FormatWriter writer = { buffer, sizeof(buffer) - 1, 0 };
	FormatError error = FormatRender(&writer, S8(format), args, (u32)sizeof...(A));
	buffer[writer.size < writer.capacity ? writer.size : writer.capacity] = 0;
	b8 valid = error == FormatErrorNone;

	if (valid != expectedValid)
		Report(format, "validity", expectedValid ? expected.c_str() : "<error>", valid ? buffer : FormatErrorName(error));
	else if (valid && (expected.size() != writer.size || memcmp(expected.data(), buffer, writer.size) != 0))
		Report(format, "text", expected.c_str(), buffer);
}

template<typename... A>
constexpr FormatError Validate(const c8* format)
{
	const FormatType types[sizeof...(A) + 1] = { FormatTypeOf<A>..., FormatTypeNone };
	FormatValidator validator = { types };
	return FormatWalk(S8(format), (u32)sizeof...(A), &validator);
}

static b8 Is(s8 string, const c8* expected)
{
	return S8Equal(string, S8(expected));
}



//- Compile-time checks

static_assert(Validate<i32>("{}") == FormatErrorNone);
static_assert(Validate<i32>("{:x}") == FormatErrorNone);
static_assert(Validate<i32>("{:f}") == FormatErrorSpecType);
static_assert(Validate<i32>("{:.2}") == FormatErrorSpecType);
static_assert(Validate<f64>("{:.2f}") == FormatErrorNone);
static_assert(Validate<const c8*>("{:>8.3}") == FormatErrorNone);
static_assert(Validate<const c8*>("{:+}") == FormatErrorSpecType);
static_assert(Validate<i32, i32>("{1} {0}") == FormatErrorNone);
static_assert(Validate<i32, i32>("{} {1}") == FormatErrorMixedIndexing);
static_assert(Validate<i32>("{} {}") == FormatErrorArgumentRange);
static_assert(Validate<i32>("{") == FormatErrorUnmatchedOpen);
static_assert(Validate<i32>("{}}") == FormatErrorUnmatchedClose);
static_assert(Validate<f64, i32>("{:{}}") == FormatErrorNone);
static_assert(Validate<f64, f64>("{:{}}") == FormatErrorDynamicType);
static_assert(Validate<f64>("{:L}") == FormatErrorUnsupported);
static_assert(Validate<>("{{}}") == FormatErrorNone);
static_assert(FormatTypeOf<c8[4]> == FormatTypeString && FormatTypeOf<i32*> == FormatTypePointer);
static_assert(FormatTypeOf<FormatError> == FormatTypeI64 && FormatTypeOf<v2> == FormatTypeCustom);



//- Against std::format

template<typename T>
static void CompareSpecs(const T& value)
{
	static const c8* aligns[] = { "", "<", ">", "^", "*^" };
	static const c8* signs[] = { "", "+", "-", " " };
	static const c8* alternates[] = { "", "#" };
	static const c8* zeros[] = { "", "0" };
	static const c8* widths[] = { "", "7" };
	static const c8* precisions[] = { "", ".0", ".3", ".12" };
	static const c8* types[] = { "", "a", "A", "b", "B", "c", "d", "e", "E", "f", "F", "g", "G", "o", "p", "P", "s", "x", "X" };

	c8 format[64];
	for (const c8* align : aligns)
		for (const c8* sign : signs)
			for (const c8* alternate : alternates)
				for (const c8* zero : zeros)
					for (const c8* width : widths)
						for (const c8* precision : precisions)
							for (const c8* type : types)
							{
#if defined(__GLIBCXX__)
								// libstdc++ writes nothing for '#' with fixed notation of huge values
								if constexpr (FormatTypeOf<T> == FormatTypeF64)
								{
									if (alternate[0] != 0 && (type[0] == 'f' || type[0] == 'F') && (value > 1e200 || value < -1e200))
										continue;
								}

								// libstdc++ pads pointers with zeros despite an alignment, which should override '0'
								if constexpr (FormatTypeOf<T> == FormatTypePointer)
								{
									if (zero[0] != 0 && align[0] != 0)
										continue;
								}
#endif

								// Sign, '#' and '0' with 'c' are errors in MSVC and TOOL, but accepted by libstdc++
								if (type[0] == 'c' && (sign[0] != 0 || alternate[0] != 0 || zero[0] != 0))
									continue;

								// Integers as 'c' are code points here and chars in std, which only agree on ASCII
								if constexpr (FormatTypeOf<T> == FormatTypeI64 || FormatTypeOf<T> == FormatTypeU64)
								{
									if (type[0] == 'c' && !(value >= 0 && value < 128))
										continue;
								}

								snprintf(format, sizeof(format), "{:%s%s%s%s%s%s%s}", align, sign, alternate, zero, width, precision, type);
								Compare(format, value);
							}
}

static void TestAgainstStd()
{
	i32 integers32[] = { 0, 42, -42, 65, I32_MIN, I32_MAX };
	for (i32 value : integers32)
		CompareSpecs(value);

	i64 integers64[] = { I64_MIN, I64_MAX, -1 };
	for (i64 value : integers64)
		CompareSpecs(value);

	u64 unsigned64 = U64_MAX;
	CompareSpecs(unsigned64);
	u8 byte = 200;
	CompareSpecs(byte);
	i8 small = -5;
	CompareSpecs(small);

	b8 booleans[] = { true, false };
	for (b8 value : booleans)
		CompareSpecs(value);

	c8 character = 'x';
	CompareSpecs(character);

	f64 doubles[] = { 0.0, -0.0, 1.0, 0.1, 2.5, 0.125, 123.456, 1e16, 1e20, 1e-7, 1.5e300, 5e-324, 9.9999999, 1e300 * 1e300, -(1e300 * 1e300) };
	for (f64 value : doubles)
		CompareSpecs(value);

	f64 nan = 0.0;
	nan = nan / nan;
	CompareSpecs(nan);

	f32 floats[] = { 0.1f, 3.14159f, 1e10f, -2.5f, 1e-40f, 16777216.0f };
	for (f32 value : floats)
		CompareSpecs(value);

	const c8* strings[] = { "", "abc", "h\xC3\xA9llo", "a longer string, with spaces" };
	for (const c8* value : strings)
		CompareSpecs(value);

	const void* pointers[] = { nullptr, (const void*)0x1234abcdull };
	for (const void* value : pointers)
		CompareSpecs(value);

	// Whole formats: ids, braces, nested widths and precisions
	i32 a = 42;
	i32 b = 7;
	f64 c = 3.14159;
	const c8* formats[] = {
		"", "plain", "{{}}", "{{{}}}", "}", "{", "{}", "{} {}", "{0} {0}", "{1} {0}", "{0} {}", "{} {1}", "{3}", "{01}",
		"{:{}}", "{0:{1}}", "{2:.{1}f}", "{2:{0}.{1}}", "{2:{}}", "{:{}}{}", "{:}", "{0:}", "{ }", "{:x", "{:{}", "{a}",
		"{:%}", "{:.}", "{:00}", "{:<<5}", "{:{<5}", "{:\xC3\xA9^9}", "{:?}", "{2:#.3g}", "{2:#g}", "{2:#}", "{2:#.3}",
	};
	for (const c8* format : formats)
		Compare(format, a, b, c);

	i32 negative = -3;
	Compare("{0:{1}}", a, negative);
	Compare("{0:.{1}f}", c, negative);
	u64 huge = U64_MAX;
	Compare("{0:{1}}", a, huge);
}



//- TOOL behavior

struct Coin
{
	i32 value;
};

static b8 CoinFormat(FormatWriter* writer, const void* value, s8 spec)
{
	FormatWrite(writer, S8("Coin "));
	return FormatWriteArg(writer, FormatArgMake(((const Coin*)value)->value), spec);
}

inline FormatArg FormatArgMake(const Coin& coin)
{
	return FormatArgCustom(coin, &CoinFormat);
}

static void TestTool()
{
	s8 formatted = S8Format(Allocator(), "{}-{}-{:.2f}-{}", 42, "x", 1.5, U64_MAX);
	TOOL_ASSERT(Is(formatted, "42-x-1.50-18446744073709551615") && formatted.str[formatted.size] == 0);
	AllocatorDealloc(Allocator(), formatted.str);

	// Past the stack buffer, so rendered again into the allocation
	s8 wide = S8Format(Allocator(), "{:>3000}", 7);
	TOOL_ASSERT(wide.size == 3000 && wide.str[2999] == '7' && wide.str[0] == ' ' && wide.str[3000] == 0);
	AllocatorDealloc(Allocator(), wide.str);

	s8 empty = S8Format(Allocator(), "");
	TOOL_ASSERT(empty.size == 0 && empty.str != nullptr && empty.str[0] == 0);
	AllocatorDealloc(Allocator(), empty.str);

	c8 buffer[64];
	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "{}={}", "x", 42) == 4 && strcmp(buffer, "x=42") == 0);
	TOOL_ASSERT(CStr8Format(buffer, 4, "{}", "abcdef") == 3 && strcmp(buffer, "abc") == 0);
	TOOL_ASSERT(CStr8Format(buffer, 4, "{}", "a\xC3\xA9\xE2\x82\xAC") == 3 && strcmp(buffer, "a\xC3\xA9") == 0);
	TOOL_ASSERT(CStr8Format(buffer, 3, "{}", "a\xC3\xA9") == 1 && strcmp(buffer, "a") == 0);
	TOOL_ASSERT(CStr8Format(buffer, 0, "{}", 1) == 0);

	s8 view = S8("view, not terminated");
	view.size = 4;
	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "[{:>6}]", view) == 8 && strcmp(buffer, "[  view]") == 0);

	// Enums, code points and fills beyond ASCII
	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "{}", FormatErrorSpec) == 1 && strcmp(buffer, "6") == 0);
	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "{:c}", 0xE9) == 2 && strcmp(buffer, "\xC3\xA9") == 0);
	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "{:\xE2\x82\xAC^5}", 1) == 13 && strcmp(buffer, "\xE2\x82\xAC\xE2\x82\xAC" "1" "\xE2\x82\xAC\xE2\x82\xAC") == 0);

	// TOOL types apply the spec per component
	v2 vector = { 1.0f, 2.5f };
	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "{:.2f}", vector) > 0 && strcmp(buffer, "(1.00, 2.50)") == 0);
	p3 point = { 1, -2, 3 };
	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "{}", point) > 0 && strcmp(buffer, "(1, -2, 3)") == 0);
	Color4U color = { 255, 0, 128, 255 };
	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "{:02x}", color) > 0 && strcmp(buffer, "(ff, 00, 80, ff)") == 0);
	q4 rotation = Q4Identity();
	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "{}", rotation) > 0 && strcmp(buffer, "(0, 0, 0, 1)") == 0);

	// Custom types
	Coin coin = { 5 };
	TOOL_ASSERT(CStr8Format(buffer, sizeof(buffer), "{:03}!", coin) > 0 && strcmp(buffer, "Coin 005!") == 0);

	// Runtime formats report errors and render up to them
	FormatArg args[] = { FormatArgMake(1), FormatArgMake(2.0) };
	TOOL_ASSERT(CStr8FormatArgs(buffer, sizeof(buffer), S8("a{}b{:d}c"), args, 2) == 3 && strcmp(buffer, "a1b") == 0);
	TOOL_ASSERT(ErrorLast()->kind == ErrorKindMessage);
	ErrorClear();
}

static void TestCaptures()
{
	c8 transient[16];
	memcpy(transient, "temporary", 10);
	Coin coin = { 9 };
	v2 vector = { 0.5f, -1.0f };

	u64 size = FormatCaptureSize(transient, 42, coin, vector, 2.5f);
	TOOL_ASSERT(size % TOOL_FORMAT_CAPTURE_ALIGNMENT == 0);

	alignas(16) u8 block[512];
	TOOL_ASSERT(size <= sizeof(block));
	FormatCapture* capture = FormatCaptureWrite(block, "{} {:x} {} {:.1f} {}", transient, 42, coin, vector, 2.5f);
	TOOL_ASSERT(capture->size == size && capture->count == 5);

	// The capture holds copies, and moves as plain bytes
	memcpy(transient, "overwritten", 12);
	coin.value = 0;
	alignas(16) u8 moved[512];
	memcpy(moved, block, size);
	memset(block, 0xCD, sizeof(block));

	s8 rendered = S8FormatCapture(Allocator(), (const FormatCapture*)moved);
	TOOL_ASSERT(Is(rendered, "temporary 2a Coin 9 (0.5, -1.0) 2.5"));
	AllocatorDealloc(Allocator(), rendered.str);

	c8 buffer[16];
	TOOL_ASSERT(CStr8FormatCapture(buffer, sizeof(buffer), (const FormatCapture*)moved) == 15);

	FormatCapture* none = FormatCaptureWrite(block, "no arguments");
	TOOL_ASSERT(none->size == FormatCaptureSize() && CStr8FormatCapture(buffer, sizeof(buffer), none) == 12);
}



//- Speed

static void TestSpeed()
{
	const u32 iterations = 200000;
	c8 buffer[256];
	u64 sink = 0;

	Timepoint start = TimepointNow();
	for (u32 i = 0; i < iterations; i++)
		sink += CStr8Format(buffer, sizeof(buffer), "Frame {} took {:.3f} ms ({} draws, {})", i, i * 0.001, i & 1023, "ok");
	f64 tool = SecondsSince<f64>(start);

	start = TimepointNow();
	for (u32 i = 0; i < iterations; i++)
		sink += (u64)(std::format_to_n(buffer, sizeof(buffer) - 1, "Frame {} took {:.3f} ms ({} draws, {})", i, i * 0.001, i & 1023, "ok").size);
	f64 standard = SecondsSince<f64>(start);

	start = TimepointNow();
	for (u32 i = 0; i < iterations; i++)
		sink += CStr8FormatPrintf(buffer, sizeof(buffer), "Frame %u took %.3f ms (%u draws, %s)", i, i * 0.001, i & 1023, "ok");
	f64 printfTime = SecondsSince<f64>(start);

	alignas(16) u8 block[256];
	start = TimepointNow();
	for (u32 i = 0; i < iterations; i++)
		sink += FormatCaptureWrite(block, "Frame {} took {:.3f} ms ({} draws, {})", i, i * 0.001, i & 1023, "ok")->size;
	f64 capture = SecondsSince<f64>(start);

	f64 scale = 1e9 / iterations;
	printf("Format speed, ns per call: TOOL %.0f, std::format %.0f, printf %.0f, capture %.0f (%llu)\n",
		tool * scale, standard * scale, printfTime * scale, capture * scale, (unsigned long long)(sink & 1));
}



void TestFormat()
{
	TestTool();
	TestCaptures();
	TestAgainstStd();
	printf("Format: %u mismatches with std::format\n", formatFailures);
	TestSpeed();
}
