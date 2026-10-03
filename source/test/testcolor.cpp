#include "tool.h"
#include "test.h"
#include <stdio.h>

using namespace Tool;



static b8 Near(f32 a, f32 b, f32 tolerance = 1e-5f)
{
	return F32Abs(a - b) <= tolerance;
}

static b8 Near(c3f a, c3f b, f32 tolerance = 1e-5f)
{
	return Near(a.r, b.r, tolerance) && Near(a.g, b.g, tolerance) && Near(a.b, b.b, tolerance);
}

static b8 Near(v3 a, v3 b, f32 tolerance = 1e-5f)
{
	return Near(a.x, b.x, tolerance) && Near(a.y, b.y, tolerance) && Near(a.z, b.z, tolerance);
}

static c3f Rgb(f32 r, f32 g, f32 b)
{
	c3f color;
	color.r = r;
	color.g = g;
	color.b = b;
	return color;
}

static void TestBytes()
{
	c4u orange = C4UFromHex(0xff8000c0);
	TOOL_ASSERT(orange.r == 0xff && orange.g == 0x80 && orange.b == 0x00 && orange.a == 0xc0);
	TOOL_ASSERT(C4UToHex(orange) == 0xff8000c0 && C3UToHex(C3UFromHex(0x123456)) == 0x123456);

	// Every byte survives a round trip through floats, and floats clamp and round on the way back.
	for (u32 i = 0; i < 256; i++)
	{
		c4u color = C4UFromHex(i * 0x01010101u);
		c4u back = C4UFromC4F(C4FFromC4U(color));
		TOOL_ASSERT(C4UToHex(back) == C4UToHex(color));
	}
	c3u clamped = C3UFromC3F(Rgb(-0.5f, 1.5f, 0.5f));
	TOOL_ASSERT(clamped.r == 0 && clamped.g == 255 && clamped.b == 128);
}

static void TestTransfer()
{
	// Reference points of the sRGB curve, and round trips at both ends of its linear and power segments.
	TOOL_ASSERT(Near(C3FSRGBToLinear(Rgb(0.0f, 1.0f, 0.5f)), Rgb(0.0f, 1.0f, 0.214041f)));
	TOOL_ASSERT(Near(C3FLinearToSRGB(Rgb(0.0f, 1.0f, 0.214041f)), Rgb(0.0f, 1.0f, 0.5f)));
	for (f32 x = 0.0f; x <= 1.0f; x += 0.03125f)
		TOOL_ASSERT(Near(C3FLinearToSRGB(C3FSRGBToLinear(Rgb(x, x * 0.01f, x * 0.5f))), Rgb(x, x * 0.01f, x * 0.5f), 1e-5f));

	c4f translucent;
	translucent.c3 = Rgb(0.5f, 0.5f, 0.5f);
	translucent.a = 0.25f;
	TOOL_ASSERT(C4FSRGBToLinear(translucent).a == 0.25f && C4FLinearToSRGB(translucent).a == 0.25f);
}

static void TestBlending()
{
	c4f a;
	a.c3 = Rgb(0.0f, 0.5f, 1.0f);
	a.a = 1.0f;
	c4f b;
	b.c3 = Rgb(1.0f, 0.5f, 0.0f);
	b.a = 0.5f;

	c4f half = C4FLerp(a, b, 0.5f);
	TOOL_ASSERT(Near(half.c3, Rgb(0.5f, 0.5f, 0.5f)) && half.a == 0.75f);

	c4f product = C4FMultiply(a, b);
	TOOL_ASSERT(Near(product.c3, Rgb(0.0f, 0.25f, 0.0f)) && product.a == 0.5f);

	c4f premultiplied = C4FPremultiply(b);
	TOOL_ASSERT(Near(premultiplied.c3, Rgb(0.5f, 0.25f, 0.0f)) && premultiplied.a == 0.5f);
}

static void TestHue()
{
	// Primaries and secondaries sit a sixth of a turn apart, at full saturation and value.
	TOOL_ASSERT(Near(C3FToHSV(Rgb(1.0f, 0.0f, 0.0f)), v3 { 0.0f, 1.0f, 1.0f }));
	TOOL_ASSERT(Near(C3FToHSV(Rgb(1.0f, 1.0f, 0.0f)), v3 { 1.0f / 6.0f, 1.0f, 1.0f }));
	TOOL_ASSERT(Near(C3FToHSV(Rgb(0.0f, 1.0f, 1.0f)), v3 { 0.5f, 1.0f, 1.0f }));
	TOOL_ASSERT(Near(C3FToHSV(Rgb(1.0f, 0.0f, 1.0f)), v3 { 5.0f / 6.0f, 1.0f, 1.0f }));
	TOOL_ASSERT(Near(C3FFromHSV(v3 { 2.0f / 3.0f, 1.0f, 1.0f }), Rgb(0.0f, 0.0f, 1.0f)));
	TOOL_ASSERT(Near(C3FFromHSV(v3 { -1.0f / 3.0f, 1.0f, 1.0f }), Rgb(0.0f, 0.0f, 1.0f)));

	// Gray has hue and saturation 0. Mid gray is lightness 0.5, and a full color is lightness 0.5 too.
	TOOL_ASSERT(Near(C3FToHSV(Rgb(0.25f, 0.25f, 0.25f)), v3 { 0.0f, 0.0f, 0.25f }));
	TOOL_ASSERT(Near(C3FToHSL(Rgb(0.5f, 0.5f, 0.5f)), v3 { 0.0f, 0.0f, 0.5f }));
	TOOL_ASSERT(Near(C3FToHSL(Rgb(0.0f, 1.0f, 0.0f)), v3 { 1.0f / 3.0f, 1.0f, 0.5f }));
	TOOL_ASSERT(Near(C3FFromHSL(v3 { 0.0f, 1.0f, 0.75f }), Rgb(1.0f, 0.5f, 0.5f)));
	TOOL_ASSERT(Near(C3FFromHSL(v3 { 0.0f, 0.0f, 1.0f }), Rgb(1.0f, 1.0f, 1.0f)));

	// Round trips over a grid of colors.
	for (u32 r = 0; r <= 4; r++)
		for (u32 g = 0; g <= 4; g++)
			for (u32 b = 0; b <= 4; b++)
			{
				c3f color = Rgb(r * 0.25f, g * 0.25f, b * 0.25f);
				TOOL_ASSERT(Near(C3FFromHSV(C3FToHSV(color)), color, 1e-5f));
				TOOL_ASSERT(Near(C3FFromHSL(C3FToHSL(color)), color, 1e-5f));
			}
}

static void TestClockDuration()
{
	// SystemTimepoints are 100 ns ticks.
	u64 ms = 10000;
	ClockDuration duration = ClockDurationFromTo(1000, 1000 + ((((3ull * 24 + 5) * 60 + 7) * 60 + 9) * 1000 + 11) * ms + 9999);
	TOOL_ASSERT(duration.days == 3 && duration.hours == 5 && duration.minutes == 7 && duration.seconds == 9 && duration.milliseconds == 11);

	ClockDuration none = ClockDurationFromTo(42, 42);
	TOOL_ASSERT(none.days == 0 && none.hours == 0 && none.minutes == 0 && none.seconds == 0 && none.milliseconds == 0);

	// Clock times number weekdays from Monday.
	ClockTime monday = ClockTimeFromSystemTimepoint(133497504000000000ull); // 2024-01-15 00:00 UTC
	TOOL_ASSERT(monday.year == 2024 && monday.month == 1 && monday.day == 15 && monday.weekday == Monday);
	ClockTime sunday = ClockTimeFromSystemTimepoint(133497504000000000ull - 864000000000ull);
	TOOL_ASSERT(sunday.day == 14 && sunday.weekday == Sunday);
}



void TestColor()
{
	TestBytes();
	TestTransfer();
	TestBlending();
	TestHue();
	TestClockDuration();
	printf("Color and clock: passed\n");
}
