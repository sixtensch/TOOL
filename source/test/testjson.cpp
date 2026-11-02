#include "tool.h"
#include "test.h"
#include <stdio.h>

using namespace Tool;

static void TestJsonValues(MemoryAllocator allocator)
{
	const JsonValue* root = JsonParse(S8(" {\"a\": null, \"b\": [true, false], \"c\": -1.5e2, \"d\": \"x\", \"e\": {}, \"f\": [], \"a\": 7} "), allocator);
	TOOL_ASSERT(root != nullptr && root->type == JsonTypeObject && root->count == 7);

	TOOL_ASSERT(JsonFind(root, S8("a"))->type == JsonTypeNull); // The first of a repeated key

	const JsonValue* b = JsonFind(root, S8("b"));
	TOOL_ASSERT(b->type == JsonTypeArray && b->count == 2);
	TOOL_ASSERT(b->first->type == JsonTypeBool && b->first->boolean);
	TOOL_ASSERT(b->first->next->type == JsonTypeBool && !b->first->next->boolean);
	TOOL_ASSERT(b->first->next->next == nullptr);

	const JsonValue* c = JsonFind(root, S8("c"));
	TOOL_ASSERT(c->type == JsonTypeNumber && c->number == -150.0 && S8Equal(c->text, S8("-1.5e2")));

	TOOL_ASSERT(S8Equal(JsonFind(root, S8("d"))->text, S8("x")));
	TOOL_ASSERT(JsonFind(root, S8("e"))->type == JsonTypeObject && JsonFind(root, S8("e"))->first == nullptr);
	TOOL_ASSERT(JsonFind(root, S8("f"))->type == JsonTypeArray && JsonFind(root, S8("f"))->count == 0);

	TOOL_ASSERT(JsonFind(root, S8("missing")) == nullptr);
	TOOL_ASSERT(JsonFind(b, S8("a")) == nullptr);
	TOOL_ASSERT(JsonFind(nullptr, S8("a")) == nullptr);

	// A number too long for an f64 stays exact as text
	const JsonValue* big = JsonParse(S8("12345678901234567890"), allocator);
	TOOL_ASSERT(big != nullptr && S8Equal(big->text, S8("12345678901234567890")));
}

static void TestJsonStrings(MemoryAllocator allocator)
{
	const JsonValue* escapes = JsonParse(S8("\"\\\"\\\\\\/\\b\\f\\n\\r\\t\\u0041\\u00E5\\u20AC\\uD83D\\uDE00\""), allocator);
	TOOL_ASSERT(escapes != nullptr);
	TOOL_ASSERT(S8Equal(escapes->text, S8("\"\\/\b\f\n\r\tA\xC3\xA5\xE2\x82\xAC\xF0\x9F\x98\x80")));
	TOOL_ASSERT(escapes->text.str[escapes->text.size] == 0);

	const JsonValue* empty = JsonParse(S8("\"\""), allocator);
	TOOL_ASSERT(empty != nullptr && empty->type == JsonTypeString && empty->text.size == 0);

	TOOL_ASSERT(S8Equal(JsonParse(S8("\"\xC3\xA5\""), allocator)->text, S8("\xC3\xA5")));

	// What's escaped reads back the same
	s8 text = S8("quote \" slash \\ line \n tab \t cr \r low \x01 utf8 \xC3\xA5");
	s8 escaped = JsonEscape(text, allocator);
	TOOL_ASSERT(S8Equal(escaped, S8("quote \\\" slash \\\\ line \\n tab \\t cr \\r low \\u0001 utf8 \xC3\xA5")));
	TOOL_ASSERT(escaped.str[escaped.size] == 0);

	s8 quoted = S8Concat(S8Concat(S8("\""), escaped, allocator), S8("\""), allocator);
	const JsonValue* back = JsonParse(quoted, allocator);
	TOOL_ASSERT(back != nullptr && S8Equal(back->text, text));

	TOOL_ASSERT(JsonEscape({}, allocator).size == 0);
}

static void TestJsonRefused(MemoryAllocator allocator)
{
	const c8* bad[] = { "", " ", "nul", "truex", "01", "1.", ".5", "1e", "+1", "-", "\"", "\"a", "\"\\", "\"\\x\"", "\"\\u12\"", "\"\\u12G4\"",
		"\"\\uD83D\"", "\"\\uD83Dx\"", "\"\\uD83D\\u0041\"", "\"\\uDE00\"", "\"a\nb\"", "[", "[1", "[1,", "[1,]", "[,1]", "[1 2]", "{", "{\"a\"",
		"{\"a\":", "{\"a\":1", "{\"a\":1,}", "{a:1}", "{1:1}", "1 2", "[] []", "NaN", "Infinity" };
	for (const c8* text : bad)
	{
		TOOL_ASSERT(JsonParse(S8(text), allocator) == nullptr, "%s", text);
		ErrorClear();
	}

	// Nesting stops at the limit rather than at the stack's
	c8 deep[TOOL_JSON_DEPTH_MAX * 2 + 2];
	for (u32 i = 0; i < TOOL_JSON_DEPTH_MAX; i++)
	{
		deep[i] = '[';
		deep[TOOL_JSON_DEPTH_MAX * 2 - 1 - i] = ']';
	}
	s8 fits;
	fits.str = deep;
	fits.size = TOOL_JSON_DEPTH_MAX * 2;
	TOOL_ASSERT(JsonParse(fits, allocator) != nullptr);

	c8 deeper[TOOL_JSON_DEPTH_MAX * 2 + 4];
	for (u32 i = 0; i < TOOL_JSON_DEPTH_MAX + 1; i++)
	{
		deeper[i] = '[';
		deeper[(TOOL_JSON_DEPTH_MAX + 1) * 2 - 1 - i] = ']';
	}
	s8 tooDeep;
	tooDeep.str = deeper;
	tooDeep.size = (TOOL_JSON_DEPTH_MAX + 1) * 2;
	TOOL_ASSERT(JsonParse(tooDeep, allocator) == nullptr);
	ErrorClear();
}

void TestJson()
{
	Arena arena = {};
	ArenaInit(&arena, Allocator());
	MemoryAllocator allocator = Allocator(&arena);

	TestJsonValues(allocator);
	TestJsonStrings(allocator);
	TestJsonRefused(allocator);

	ArenaDeInit(&arena);

	printf("Json: passed\n");
}
