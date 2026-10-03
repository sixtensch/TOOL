#ifndef _TOOL_COLOR_H
#define _TOOL_COLOR_H

#include "basics.h"
#include "mathematics.h"
#include "vector.h"



//~ Colors
//
// Byte colors (c3u, c4u) hold 0 to 255 per channel, and their memory is plain RGBA8. Float colors (c3f, c4f) hold
// 0 to 1, though values outside that range pass through everything except the conversion to bytes, which clamps.
//
// Nothing here tracks whether a color is sRGB or linear: convert explicitly where it matters. HSV and HSL are v3s
// of hue, saturation and value or lightness, each 0 to 1, with hue in turns (0.5 is cyan).



namespace Tool
{
	//- Types

	//~ Byte colors

	struct Color3U
	{
		u8 r;
		u8 g;
		u8 b;
	};

	struct Color4U
	{
		union
		{
			struct
			{
				u8 r;
				u8 g;
				u8 b;
				u8 a;
			};

			Color3U c3;
		};
	};

	//~ Floating-point colors

	struct Color3F
	{
		union
		{
			struct
			{
				f32 r;
				f32 g;
				f32 b;
			};

			v3 rgb;
		};
	};

	struct Color4F
	{
		union
		{
			struct
			{
				f32 r;
				f32 g;
				f32 b;
				f32 a;
			};

			Color3F c3;

			v3 rgb;
			v4 rgba;
		};
	};

	//~ Acronyms

	typedef Color3U c3u;
	typedef Color4U c4u;

	typedef Color3F c3f;
	typedef Color4F c4f;



	//- Functions

	//~ Hex
	// 0xRRGGBB and 0xRRGGBBAA, as written in code and color pickers.

	inline c3u C3UFromHex(u32 rgb);
	inline c4u C4UFromHex(u32 rgba);
	inline u32 C3UToHex(c3u color);
	inline u32 C4UToHex(c4u color);

	//~ Bytes and floats
	// Bytes map 0 to 255 onto 0 to 1. Floats round to the nearest byte, clamped.

	inline c3f C3FFromC3U(c3u color);
	inline c4f C4FFromC4U(c4u color);
	inline c3u C3UFromC3F(c3f color);
	inline c4u C4UFromC4F(c4f color);

	//~ sRGB and linear
	// The exact sRGB curve, per color channel. Alpha is linear in both and passes through.

	inline c3f C3FSRGBToLinear(c3f color);
	inline c4f C4FSRGBToLinear(c4f color);
	inline c3f C3FLinearToSRGB(c3f color);
	inline c4f C4FLinearToSRGB(c4f color);

	//~ Blending

	inline c3f C3FLerp(c3f a, c3f b, f32 t); // a at 0, b at 1, per channel
	inline c4f C4FLerp(c4f a, c4f b, f32 t);

	inline c3f C3FMultiply(c3f a, c3f b); // Per channel, as a tint or light
	inline c4f C4FMultiply(c4f a, c4f b);

	inline c4f C4FPremultiply(c4f color); // Color channels times alpha

	//~ HSV and HSL
	// Hue wraps, so any value works going in. Gray has hue 0.

	inline c3f C3FFromHSV(v3 hsv);
	inline v3 C3FToHSV(c3f color);
	inline c3f C3FFromHSL(v3 hsl);
	inline v3 C3FToHSL(c3f color);
} //namespace Tool



//- Implementation

namespace Tool
{
	//~ Hex

	inline c3u C3UFromHex(u32 rgb)
	{
		return { (u8)(rgb >> 16), (u8)(rgb >> 8), (u8)rgb };
	}

	inline c4u C4UFromHex(u32 rgba)
	{
		c4u color;
		color.r = (u8)(rgba >> 24);
		color.g = (u8)(rgba >> 16);
		color.b = (u8)(rgba >> 8);
		color.a = (u8)rgba;
		return color;
	}

	inline u32 C3UToHex(c3u color)
	{
		return ((u32)color.r << 16) | ((u32)color.g << 8) | color.b;
	}

	inline u32 C4UToHex(c4u color)
	{
		return ((u32)color.r << 24) | ((u32)color.g << 16) | ((u32)color.b << 8) | color.a;
	}

	//~ Bytes and floats

	inline f32 ColorChannelFromByte(u8 channel)
	{
		return (f32)channel * (1.0f / 255.0f);
	}

	inline u8 ColorChannelToByte(f32 channel)
	{
		return (u8)(F32Saturate(channel) * 255.0f + 0.5f);
	}

	inline c3f C3FFromC3U(c3u color)
	{
		c3f result;
		result.r = ColorChannelFromByte(color.r);
		result.g = ColorChannelFromByte(color.g);
		result.b = ColorChannelFromByte(color.b);
		return result;
	}

	inline c4f C4FFromC4U(c4u color)
	{
		c4f result;
		result.c3 = C3FFromC3U(color.c3);
		result.a = ColorChannelFromByte(color.a);
		return result;
	}

	inline c3u C3UFromC3F(c3f color)
	{
		return { ColorChannelToByte(color.r), ColorChannelToByte(color.g), ColorChannelToByte(color.b) };
	}

	inline c4u C4UFromC4F(c4f color)
	{
		c4u result;
		result.c3 = C3UFromC3F(color.c3);
		result.a = ColorChannelToByte(color.a);
		return result;
	}

	//~ sRGB and linear

	inline f32 ColorChannelSRGBToLinear(f32 channel)
	{
		return channel <= 0.04045f ? channel * (1.0f / 12.92f) : F32Pow((channel + 0.055f) * (1.0f / 1.055f), 2.4f);
	}

	inline f32 ColorChannelLinearToSRGB(f32 channel)
	{
		return channel <= 0.0031308f ? channel * 12.92f : 1.055f * F32Pow(channel, 1.0f / 2.4f) - 0.055f;
	}

	inline c3f C3FSRGBToLinear(c3f color)
	{
		c3f result;
		result.r = ColorChannelSRGBToLinear(color.r);
		result.g = ColorChannelSRGBToLinear(color.g);
		result.b = ColorChannelSRGBToLinear(color.b);
		return result;
	}

	inline c4f C4FSRGBToLinear(c4f color)
	{
		color.c3 = C3FSRGBToLinear(color.c3);
		return color;
	}

	inline c3f C3FLinearToSRGB(c3f color)
	{
		c3f result;
		result.r = ColorChannelLinearToSRGB(color.r);
		result.g = ColorChannelLinearToSRGB(color.g);
		result.b = ColorChannelLinearToSRGB(color.b);
		return result;
	}

	inline c4f C4FLinearToSRGB(c4f color)
	{
		color.c3 = C3FLinearToSRGB(color.c3);
		return color;
	}

	//~ Blending

	inline c3f C3FLerp(c3f a, c3f b, f32 t)
	{
		c3f result;
		result.r = F32Lerp(a.r, b.r, t);
		result.g = F32Lerp(a.g, b.g, t);
		result.b = F32Lerp(a.b, b.b, t);
		return result;
	}

	inline c4f C4FLerp(c4f a, c4f b, f32 t)
	{
		c4f result;
		result.c3 = C3FLerp(a.c3, b.c3, t);
		result.a = F32Lerp(a.a, b.a, t);
		return result;
	}

	inline c3f C3FMultiply(c3f a, c3f b)
	{
		c3f result;
		result.r = a.r * b.r;
		result.g = a.g * b.g;
		result.b = a.b * b.b;
		return result;
	}

	inline c4f C4FMultiply(c4f a, c4f b)
	{
		c4f result;
		result.c3 = C3FMultiply(a.c3, b.c3);
		result.a = a.a * b.a;
		return result;
	}

	inline c4f C4FPremultiply(c4f color)
	{
		color.r *= color.a;
		color.g *= color.a;
		color.b *= color.a;
		return color;
	}

	//~ HSV and HSL

	// Hue in turns from the largest channel and the spread between largest and smallest.
	inline f32 ColorHue(c3f color, f32 largest, f32 spread)
	{
		if (spread <= 0.0f)
			return 0.0f;

		f32 sixths;
		if (largest == color.r)
			sixths = (color.g - color.b) / spread;
		else if (largest == color.g)
			sixths = (color.b - color.r) / spread + 2.0f;
		else
			sixths = (color.r - color.g) / spread + 4.0f;

		return F32Wrap(sixths * (1.0f / 6.0f), 0.0f, 1.0f);
	}

	// One channel of an HSV or HSL color: 'offset' is 5 for red, 3 for green and 1 for blue, in sixths of a turn.
	inline f32 ColorChannelFromHSV(v3 hsv, f32 offset)
	{
		f32 k = F32Wrap(offset + hsv.x * 6.0f, 0.0f, 6.0f);
		return hsv.z - hsv.z * hsv.y * F32Saturate(F32Min(k, 4.0f - k));
	}

	inline c3f C3FFromHSV(v3 hsv)
	{
		c3f result;
		result.r = ColorChannelFromHSV(hsv, 5.0f);
		result.g = ColorChannelFromHSV(hsv, 3.0f);
		result.b = ColorChannelFromHSV(hsv, 1.0f);
		return result;
	}

	inline v3 C3FToHSV(c3f color)
	{
		f32 largest = F32Max(color.r, F32Max(color.g, color.b));
		f32 smallest = F32Min(color.r, F32Min(color.g, color.b));
		f32 spread = largest - smallest;
		f32 saturation = largest > 0.0f ? spread / largest : 0.0f;
		return v3 { ColorHue(color, largest, spread), saturation, largest };
	}

	// HSL is HSV with value and saturation measured against lightness instead of the largest channel.
	inline c3f C3FFromHSL(v3 hsl)
	{
		f32 value = hsl.z + hsl.y * F32Min(hsl.z, 1.0f - hsl.z);
		f32 saturation = value > 0.0f ? 2.0f * (1.0f - hsl.z / value) : 0.0f;
		return C3FFromHSV(v3 { hsl.x, saturation, value });
	}

	inline v3 C3FToHSL(c3f color)
	{
		f32 largest = F32Max(color.r, F32Max(color.g, color.b));
		f32 smallest = F32Min(color.r, F32Min(color.g, color.b));
		f32 spread = largest - smallest;
		f32 lightness = (largest + smallest) * 0.5f;
		f32 range = 1.0f - F32Abs(2.0f * lightness - 1.0f);
		f32 saturation = range > 0.0f ? spread / range : 0.0f;
		return v3 { ColorHue(color, largest, spread), saturation, lightness };
	}
} //namespace Tool



//~ Acronym usings

#ifndef TOOL_NO_ACRONYMS

using Tool::c3u;
using Tool::c4u;

using Tool::c3f;
using Tool::c4f;

#endif



#endif //_TOOL_COLOR_H
