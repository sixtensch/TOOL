#include "json.h"
#include "error.h"

#include <string.h>

namespace Tool
{
	// Room for 'size' bytes and a terminator, which is set
	static s8 JsonTextAllocate(u64 size, MemoryAllocator allocator)
	{
		s8 text = {};
		text.str = (c8*)allocator.allocate(size + 1, allocator.data);
		if (text.str != nullptr)
		{
			text.str[size] = '\0';
			text.size = size;
		}
		return text;
	}

	//~ Reading

	struct JsonReader
	{
		const c8* start;
		const c8* at;
		const c8* end;
		MemoryAllocator allocator;
	};

	static b8 JsonFail(JsonReader* reader, const c8* reason)
	{
		return TOOL_FAIL("JSON: %s at byte %llu", reason, (unsigned long long)(reader->at - reader->start));
	}

	static void JsonSkipSpace(JsonReader* reader)
	{
		while (reader->at < reader->end && (*reader->at == ' ' || *reader->at == '\t' || *reader->at == '\n' || *reader->at == '\r'))
			reader->at++;
	}

	static b8 JsonDigit(const JsonReader* reader)
	{
		return reader->at < reader->end && *reader->at >= '0' && *reader->at <= '9';
	}

	static b8 JsonLiteral(JsonReader* reader, const c8* literal, u64 size)
	{
		if ((u64)(reader->end - reader->at) < size || memcmp(reader->at, literal, size) != 0)
			return JsonFail(reader, "Unknown value");

		reader->at += size;
		return true;
	}

	static b8 JsonHex4(JsonReader* reader, u32* outValue)
	{
		if (reader->end - reader->at < 4)
			return JsonFail(reader, "Unfinished \\u escape");

		u32 value = 0;
		for (u32 i = 0; i < 4; i++)
		{
			c8 c = *reader->at;
			value <<= 4;
			if (c >= '0' && c <= '9')
				value |= (u32)(c - '0');
			else if (c >= 'a' && c <= 'f')
				value |= (u32)(c - 'a' + 10);
			else if (c >= 'A' && c <= 'F')
				value |= (u32)(c - 'A' + 10);
			else
				return JsonFail(reader, "Malformed \\u escape");
			reader->at++;
		}

		*outValue = value;
		return true;
	}

	static u64 JsonEncodeUtf8(u32 code, c8* out)
	{
		if (code < 0x80)
		{
			out[0] = (c8)code;
			return 1;
		}
		if (code < 0x800)
		{
			out[0] = (c8)(0xC0 | (code >> 6));
			out[1] = (c8)(0x80 | (code & 0x3F));
			return 2;
		}
		if (code < 0x10000)
		{
			out[0] = (c8)(0xE0 | (code >> 12));
			out[1] = (c8)(0x80 | ((code >> 6) & 0x3F));
			out[2] = (c8)(0x80 | (code & 0x3F));
			return 3;
		}
		out[0] = (c8)(0xF0 | (code >> 18));
		out[1] = (c8)(0x80 | ((code >> 12) & 0x3F));
		out[2] = (c8)(0x80 | ((code >> 6) & 0x3F));
		out[3] = (c8)(0x80 | (code & 0x3F));
		return 4;
	}

	// At the opening quote. Escapes only ever shrink, so the raw size bounds the decoded one.
	static b8 JsonReadString(JsonReader* reader, s8* outText)
	{
		reader->at++;

		const c8* close = reader->at;
		while (close < reader->end && *close != '"')
			close += (*close == '\\') ? 2 : 1;
		if (close >= reader->end)
			return JsonFail(reader, "Unfinished string");

		s8 text = JsonTextAllocate((u64)(close - reader->at), reader->allocator);
		if (text.str == nullptr)
			return JsonFail(reader, "Out of memory");

		u64 size = 0;
		while (reader->at < close)
		{
			c8 c = *reader->at;
			if ((u8)c < 0x20)
				return JsonFail(reader, "Control character in a string");

			if (c != '\\')
			{
				text.str[size++] = c;
				reader->at++;
				continue;
			}

			reader->at++;
			c8 escape = *reader->at++;
			switch (escape)
			{
			case '"':
			case '\\':
			case '/':
				text.str[size++] = escape;
				break;
			case 'b':
				text.str[size++] = '\b';
				break;
			case 'f':
				text.str[size++] = '\f';
				break;
			case 'n':
				text.str[size++] = '\n';
				break;
			case 'r':
				text.str[size++] = '\r';
				break;
			case 't':
				text.str[size++] = '\t';
				break;
			case 'u':
			{
				u32 code;
				if (!JsonHex4(reader, &code))
					return false;

				if (code >= 0xD800 && code < 0xDC00)
				{
					u32 low;
					if (close - reader->at < 6 || reader->at[0] != '\\' || reader->at[1] != 'u')
						return JsonFail(reader, "Lone surrogate in a string");
					reader->at += 2;
					if (!JsonHex4(reader, &low))
						return false;
					if (low < 0xDC00 || low >= 0xE000)
						return JsonFail(reader, "Lone surrogate in a string");
					code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
				}
				else if (code >= 0xDC00 && code < 0xE000)
				{
					return JsonFail(reader, "Lone surrogate in a string");
				}

				size += JsonEncodeUtf8(code, text.str + size);
				break;
			}
			default:
				reader->at--;
				return JsonFail(reader, "Unknown escape in a string");
			}
		}

		text.str[size] = '\0';
		text.size = size;

		reader->at = close + 1;
		*outText = text;
		return true;
	}

	static b8 JsonReadNumber(JsonReader* reader, JsonValue* value)
	{
		const c8* start = reader->at;

		if (reader->at < reader->end && *reader->at == '-')
			reader->at++;

		if (!JsonDigit(reader))
			return JsonFail(reader, "Unknown value");
		if (*reader->at == '0')
			reader->at++;
		else
			while (JsonDigit(reader))
				reader->at++;

		if (reader->at < reader->end && *reader->at == '.')
		{
			reader->at++;
			if (!JsonDigit(reader))
				return JsonFail(reader, "Malformed number");
			while (JsonDigit(reader))
				reader->at++;
		}

		if (reader->at < reader->end && (*reader->at == 'e' || *reader->at == 'E'))
		{
			reader->at++;
			if (reader->at < reader->end && (*reader->at == '+' || *reader->at == '-'))
				reader->at++;
			if (!JsonDigit(reader))
				return JsonFail(reader, "Malformed number");
			while (JsonDigit(reader))
				reader->at++;
		}

		s8 written;
		written.str = (c8*)start;
		written.size = (u64)(reader->at - start);

		value->type = JsonTypeNumber;
		value->text = S8Copy(written, reader->allocator);
		if (value->text.str == nullptr)
			return JsonFail(reader, "Out of memory");
		return S8ParseF64(value->text, &value->number) ? true : JsonFail(reader, "Malformed number");
	}

	static JsonValue* JsonValueNew(JsonReader* reader)
	{
		JsonValue* value = (JsonValue*)reader->allocator.allocate(sizeof(JsonValue), reader->allocator.data);
		if (value == nullptr)
		{
			JsonFail(reader, "Out of memory");
			return nullptr;
		}

		memset(value, 0, sizeof(JsonValue));
		return value;
	}

	static b8 JsonReadValue(JsonReader* reader, JsonValue* value, u32 depth);

	// At the opening bracket
	static b8 JsonReadChildren(JsonReader* reader, JsonValue* value, u32 depth, c8 close)
	{
		if (depth >= TOOL_JSON_DEPTH_MAX)
			return JsonFail(reader, "Nested too deeply");

		b8 object = close == '}';
		reader->at++;

		JsonSkipSpace(reader);
		if (reader->at < reader->end && *reader->at == close)
		{
			reader->at++;
			return true;
		}

		JsonValue** link = &value->first;
		while (true)
		{
			JsonValue* child = JsonValueNew(reader);
			if (child == nullptr)
				return false;

			if (object)
			{
				JsonSkipSpace(reader);
				if (reader->at >= reader->end || *reader->at != '"')
					return JsonFail(reader, "Expected a key");
				if (!JsonReadString(reader, &child->key))
					return false;

				JsonSkipSpace(reader);
				if (reader->at >= reader->end || *reader->at != ':')
					return JsonFail(reader, "Expected ':'");
				reader->at++;
			}

			if (!JsonReadValue(reader, child, depth + 1))
				return false;

			*link = child;
			link = &child->next;
			value->count++;

			JsonSkipSpace(reader);
			if (reader->at >= reader->end)
				return JsonFail(reader, "Unfinished array or object");

			c8 c = *reader->at++;
			if (c == close)
				return true;
			if (c != ',')
			{
				reader->at--;
				return JsonFail(reader, "Expected ',' or a closing bracket");
			}
		}
	}

	static b8 JsonReadValue(JsonReader* reader, JsonValue* value, u32 depth)
	{
		JsonSkipSpace(reader);
		if (reader->at >= reader->end)
			return JsonFail(reader, "Expected a value");

		switch (*reader->at)
		{
		case 'n':
			value->type = JsonTypeNull;
			return JsonLiteral(reader, "null", 4);
		case 't':
			value->type = JsonTypeBool;
			value->boolean = true;
			return JsonLiteral(reader, "true", 4);
		case 'f':
			value->type = JsonTypeBool;
			return JsonLiteral(reader, "false", 5);
		case '"':
			value->type = JsonTypeString;
			return JsonReadString(reader, &value->text);
		case '[':
			value->type = JsonTypeArray;
			return JsonReadChildren(reader, value, depth, ']');
		case '{':
			value->type = JsonTypeObject;
			return JsonReadChildren(reader, value, depth, '}');
		default:
			return JsonReadNumber(reader, value);
		}
	}

	//~ Interface

	const JsonValue* JsonParse(s8 text, MemoryAllocator allocator)
	{
		JsonReader reader = {};
		reader.start = text.str;
		reader.at = text.str;
		reader.end = text.str + text.size;
		reader.allocator = allocator;

		JsonValue* root = JsonValueNew(&reader);
		if (root == nullptr || !JsonReadValue(&reader, root, 0))
			return nullptr;

		JsonSkipSpace(&reader);
		if (reader.at != reader.end)
		{
			JsonFail(&reader, "Text after the value");
			return nullptr;
		}

		return root;
	}

	const JsonValue* JsonFind(const JsonValue* object, s8 key)
	{
		if (object == nullptr || object->type != JsonTypeObject)
			return nullptr;

		for (const JsonValue* member = object->first; member != nullptr; member = member->next)
		{
			if (S8Equal(member->key, key))
				return member;
		}
		return nullptr;
	}

	s8 JsonEscape(s8 text, MemoryAllocator allocator)
	{
		u64 size = 0;
		for (u64 i = 0; i < text.size; i++)
		{
			u8 c = (u8)text.str[i];
			if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t')
				size += 2;
			else if (c < 0x20)
				size += 6;
			else
				size += 1;
		}

		s8 escaped = JsonTextAllocate(size, allocator);
		if (escaped.str == nullptr)
			return escaped;

		static const c8 hex[] = "0123456789abcdef";

		c8* out = escaped.str;
		for (u64 i = 0; i < text.size; i++)
		{
			u8 c = (u8)text.str[i];
			switch (c)
			{
			case '"':
				*out++ = '\\';
				*out++ = '"';
				break;
			case '\\':
				*out++ = '\\';
				*out++ = '\\';
				break;
			case '\n':
				*out++ = '\\';
				*out++ = 'n';
				break;
			case '\r':
				*out++ = '\\';
				*out++ = 'r';
				break;
			case '\t':
				*out++ = '\\';
				*out++ = 't';
				break;
			default:
				if (c < 0x20)
				{
					memcpy(out, "\\u00", 4);
					out[4] = hex[c >> 4];
					out[5] = hex[c & 0xF];
					out += 6;
				}
				else
				{
					*out++ = (c8)c;
				}
				break;
			}
		}

		return escaped;
	}
}
