#include "tool.h"
#include "test.h"
#include <stdio.h>

using namespace Tool;



//~ Hashes

static_assert(ConstHashFnv1a("") == 0xcbf29ce484222325ull);
static_assert(ConstHashFnv1a("a") == 0xaf63dc4c8601ec8cull);
static_assert(ConstHashFnv1a("foobar") == 0x85944171f73967e8ull);

// Reference outputs of upstream rapidhash V3.
static_assert(ConstHashRapid("") == 0x0338dc4be2cecdaeull);
static_assert(ConstHashRapid("abc") == 0xcb475beafa9c0da2ull);
static_assert(ConstHashRapid("hello world") == 0x2f27cb27d5240940ull);
static_assert(ConstHashRapid("forty bytes of text to hit mid branches.") == 0xbeb35d5906591711ull);

static void TestHashes()
{
    // Compile-time and runtime paths agree, bulk loop included.
    static constexpr c8 longText[] = "A string over one hundred and twelve bytes long, so that the hash runs its bulk loop at least once, "
                                 "and then handles the remaining tail as well.";
    constexpr u64 constLong = ConstHashRapid(longText);
    volatile u64 size = sizeof(longText) - 1;
    TOOL_ASSERT(HashRapid(longText, size) == constLong);

    // Integer overloads hash the value's little-endian bytes.
    for (u64 i = 0; i < 1000; i++)
    {
        u64 value = i * 0x9e3779b97f4a7c15ull;
        TOOL_ASSERT(HashRapid(value) == HashRapid(&value, 8));
        TOOL_ASSERT(HashFnv1a(value) == HashFnv1a(&value, 8));
    }
}



//~ Integer keys

static void TestIntegerKeys()
{
    HashMap<HashFunctionNone> map = {};
    HashMapInit<u64>(&map, Allocator(), HashKeyTypeU64);
    TOOL_ASSERT(map.core.hashes == nullptr);

    // Key 0 hashes to 0, the empty marker.
    for (u64 key = 0; key < 1000; key++)
        TOOL_ASSERT(HashMapInsert(&map, key, key * 3) != nullptr);

    TOOL_ASSERT(map.core.count == 1000);
    TOOL_ASSERT(map.core.capacity >= 1000 * 4 / 3);

    for (u64 key = 0; key < 1000; key++)
    {
        u64* value = HashMapFind<u64>(&map, key);
        TOOL_ASSERT(value != nullptr && *value == key * 3);
    }
    TOOL_ASSERT(HashMapFind<u64>(&map, 1000) == nullptr);

    // Overwrite keeps the count.
    HashMapInsert(&map, 7ull, 70ull);
    TOOL_ASSERT(*HashMapFind<u64>(&map, 7) == 70 && map.core.count == 1000);

    for (u64 key = 0; key < 1000; key += 2)
        TOOL_ASSERT(HashMapRemove(&map, key));
    TOOL_ASSERT(!HashMapRemove(&map, 0));
    TOOL_ASSERT(map.core.count == 500);

    for (u64 key = 0; key < 1000; key++)
        TOOL_ASSERT((HashMapFind<u64>(&map, key) != nullptr) == (key % 2 == 1));

    // Iteration visits each entry once.
    u64 visited = 0;
    u64 key;
    u64* value;
    for (u64 cursor = 0; HashMapNext(&map, &cursor, &key, &value);)
    {
        TOOL_ASSERT(key % 2 == 1);
        visited++;
    }
    TOOL_ASSERT(visited == 500);

    HashMapClear(&map);
    TOOL_ASSERT(map.core.count == 0 && HashMapFind<u64>(&map, 1) == nullptr);

    HashMapDeInit(&map);
    TOOL_ASSERT(map.core.hashes == nullptr);

    // Negative i32 keys widen consistently.
    HashMap<HashFunctionRapid> signedMap = {};
    HashMapInit<i32>(&signedMap, Allocator(), HashKeyTypeU64, 64);
    for (i32 id = -1; id > -50; id--)
        HashMapInsert(&signedMap, id, id);
    for (i32 id = -1; id > -50; id--)
        TOOL_ASSERT(*HashMapFind<i32>(&signedMap, id) == id);
    HashMapDeInit(&signedMap);
}

// Random inserts and removes against a flat reference, over keys that share their low bits.
static void TestChurn()
{
    const u64 keyCount = 512;
    u64 reference[keyCount] = {}; // 0 = absent, else value
    u64 present = 0;

    HashMap<HashFunctionNone> map = {};
    HashMapInit<u64>(&map, Allocator(), HashKeyTypeU64);

    u64 random = 12345;
    for (u32 step = 0; step < 200000; step++)
    {
        random = random * 6364136223846793005ull + 1442695040888963407ull;
        u64 index = (random >> 33) % keyCount;
        u64 key = index << 32;

        if ((random >> 20) & 1)
        {
            u64 value = (random >> 40) | 1;
            HashMapInsert(&map, key, value);
            present += reference[index] == 0;
            reference[index] = value;
        }
        else
        {
            b8 removed = HashMapRemove(&map, key);
            TOOL_ASSERT(removed == (reference[index] != 0));
            present -= removed;
            reference[index] = 0;
        }

        if (step % 1000 == 0)
        {
            TOOL_ASSERT(map.core.count == present);
            for (u64 i = 0; i < keyCount; i++)
            {
                u64* value = HashMapFind<u64>(&map, i << 32);
                TOOL_ASSERT(reference[i] == 0 ? value == nullptr : (value != nullptr && *value == reference[i]));
            }
        }
    }

    HashMapDeInit(&map);
}



//~ String keys

static void TestStringKeys()
{
    HashMap<HashFunctionFnv1a> map = {};
    HashMapInit<u32>(&map, Allocator(), HashKeyTypeS8);

    // One reused buffer: the map must copy the keys.
    c8 buffer[32];
    for (u32 i = 0; i < 300; i++)
    {
        snprintf(buffer, sizeof(buffer), "key%u", i);
        TOOL_ASSERT(HashMapInsert(&map, S8(buffer), i) != nullptr);
    }
    TOOL_ASSERT(map.core.count == 300);

    for (u32 i = 0; i < 300; i++)
    {
        snprintf(buffer, sizeof(buffer), "key%u", i);
        u32* value = HashMapFind<u32>(&map, S8(buffer));
        TOOL_ASSERT(value != nullptr && *value == i);
    }
    TOOL_ASSERT(HashMapFind<u32>(&map, S8("key")) == nullptr);
    TOOL_ASSERT(HashMapFind<u32>(&map, S8("")) == nullptr);

    // Precomputed hash, from the map's own type.
    u32* hashed = HashMapFind<u32>(&map, S8("key42"), HashMapConstHash<decltype(map)>("key42"));
    TOOL_ASSERT(hashed != nullptr && *hashed == 42);

    // Copied keys are null terminated.
    s8 key;
    u32* value;
    for (u64 cursor = 0; HashMapNext(&map, &cursor, &key, &value);)
        TOOL_ASSERT(key.str[key.size] == '\0' && key.str != buffer);

    TOOL_ASSERT(HashMapRemove(&map, S8("key7")));
    TOOL_ASSERT(HashMapFind<u32>(&map, S8("key7")) == nullptr);
    TOOL_ASSERT(*HashMapFind<u32>(&map, S8("key8")) == 8);

    HashMapInsert(&map, S8(""), 999u);
    TOOL_ASSERT(*HashMapFind<u32>(&map, S8("")) == 999);

    HashMapDeInit(&map);

    // A map with no hash function takes the caller's hash.
    HashMap<HashFunctionNone> manual = {};
    HashMapInit<u32>(&manual, Allocator(), HashKeyTypeS8);
    HashMapInsert(&manual, S8("alpha"), 1, 10u);
    HashMapInsert(&manual, S8("beta"), 1, 20u); // Equal hashes, told apart by key
    TOOL_ASSERT(*HashMapFind<u32>(&manual, S8("alpha"), 1) == 10);
    TOOL_ASSERT(*HashMapFind<u32>(&manual, S8("beta"), 1) == 20);
    TOOL_ASSERT(HashMapRemove(&manual, S8("alpha"), 1));
    TOOL_ASSERT(*HashMapFind<u32>(&manual, S8("beta"), 1) == 20);
    HashMapDeInit(&manual);
}



//~ Arena source

static void TestArenaSource()
{
    Arena arena = {};
    ArenaInit(&arena, Allocator(), 4096, "Hash map test");

    HashMap<HashFunctionRapid> map = {};
    HashMapInit<u64>(&map, Allocator(&arena), HashKeyTypeS8);

    c8 buffer[32];
    for (u64 i = 0; i < 2000; i++)
    {
        snprintf(buffer, sizeof(buffer), "asset/path/%llu.png", i);
        TOOL_ASSERT(HashMapInsert(&map, S8(buffer), i) != nullptr);
    }
    for (u64 i = 0; i < 2000; i += 3)
    {
        snprintf(buffer, sizeof(buffer), "asset/path/%llu.png", i);
        TOOL_ASSERT(HashMapRemove(&map, S8(buffer)));
    }
    for (u64 i = 0; i < 2000; i++)
    {
        snprintf(buffer, sizeof(buffer), "asset/path/%llu.png", i);
        u64* value = HashMapFind<u64>(&map, S8(buffer));
        TOOL_ASSERT((i % 3 == 0) ? value == nullptr : (value != nullptr && *value == i));
    }

    // The arena reclaims the map wholesale.
    ArenaReset(&arena);
    ArenaDeInit(&arena);
}



void TestHashMap()
{
    TestHashes();
    TestIntegerKeys();
    TestChurn();
    TestStringKeys();
    TestArenaSource();
    printf("Hash map: ok\n");
}
