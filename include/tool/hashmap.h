#ifndef _TOOL_HASHMAP_H
#define _TOOL_HASHMAP_H

#include "basics.h"
#include "error.h"
#include "hash.h"
#include "memory.h"
#include "text.h"



//~ Hash map
//
// Open addressing with linear probing and backward-shift removal (no tombstones). Power-of-two slot count, kept
// at most 3/4 full. The home slot comes from the high bits of the hash times a Fibonacci constant, so identity
// hashes of counters or aligned values spread as well as real hashes do.
//
// Values are fixed-size byte blocks copied in and out. Keys are integers (any width, widened to u64) or s8
// strings, chosen at init. String keys are copied into the map's allocator, null terminated.
//
// Memory comes from the allocator given at init, and nothing is taken until the first insert. Growing doubles
// the slot count and releases the old table to the allocator. An arena source has no deallocate, so outgrown
// tables and removed string keys stay in the arena until it resets: fine for lifetimes that reset, a slow leak
// for ones that do not. Size the map at init to avoid growth there.
//
// No pointer stability: a value pointer is valid until the next insert or remove.



//~ Definitions

// Smallest slot count of an allocated table
#define TOOL_HASH_MAP_MIN_CAPACITY 8ull



namespace Tool
{
    //- Type definitions

    // How a map turns keys into hashes. Fixed per map by its type, so a lookup can never hash differently than the
    // insert did.
    enum HashFunction
    {
        HashFunctionNone,  // No hashing: an integer key is its own hash, a string key needs its hash passed in
        HashFunctionFnv1a, // HashFnv1a over the key
        HashFunctionRapid, // HashRapid over the key
    };

    enum HashKeyType
    {
        HashKeyTypeU64, // Integers, compared by value
        HashKeyTypeS8,  // Strings, compared by content
    };

    // Untyped map state. Use it through HashMap<F>.
    struct HashMapCore
    {
        MemoryAllocator allocator;
        HashKeyType keyType;
        u64 valueSize;

        u64 capacity; // Slot count, a power of two. Set at init, allocated on the first insert.
        u32 shift;    // 64 - log2(capacity)
        u64 count;    // Entries held

        u64* hashes; // Per slot, 0 when the slot is empty. Null until the first insert.
        u8* keys;    // Per slot, a u64 or an s8
        u8* values;  // Per slot, valueSize bytes
    };

    template<HashFunction F>
    struct HashMap
    {
        static constexpr HashFunction function = F;

        HashMapCore core;
    };



    //- Core (use the HashMap accessors below)

    void HashMapCoreInit(HashMapCore* map, MemoryAllocator allocator, HashKeyType keyType, u64 valueSize, u64 capacity);

    void* HashMapCoreFind(HashMapCore* map, u64 key, u64 hash);
    void* HashMapCoreFind(HashMapCore* map, s8 key, u64 hash);

    void* HashMapCoreInsert(HashMapCore* map, u64 key, u64 hash, const void* value);
    void* HashMapCoreInsert(HashMapCore* map, s8 key, u64 hash, const void* value);

    b8 HashMapCoreRemove(HashMapCore* map, u64 key, u64 hash);
    b8 HashMapCoreRemove(HashMapCore* map, s8 key, u64 hash);

    b8 HashMapCoreNext(HashMapCore* map, u64* cursor, void** outKey, void** outValue);

    void HashMapCoreClear(HashMapCore* map);
    void HashMapCoreDeInit(HashMapCore* map);



    //- Hashing

    // The hash a map of function F gives a key.
    template<HashFunction F>
    constexpr u64 HashMapHash(u64 key)
    {
        if constexpr (F == HashFunctionFnv1a)
            return HashFnv1a(key);
        else if constexpr (F == HashFunctionRapid)
            return HashRapid(key);
        else
            return key;
    }

    template<HashFunction F>
    constexpr u64 HashMapHash(s8 key)
    {
        static_assert(F != HashFunctionNone, "A HashFunctionNone map cannot hash strings. Pass the hash in.");

        if constexpr (F == HashFunctionFnv1a)
            return HashFnv1a(key);
        else
            return HashRapid(key);
    }

    // Compile-time hash of a string literal for map type M, for the overloads that take a hash.
    // Usage: 'HashMapFind<Value>(&map, key, HashMapConstHash<decltype(map)>("name"))'.
    template<typename M>
    consteval u64 HashMapConstHash(const c8* string)
    {
        static_assert(M::function != HashFunctionNone, "A HashFunctionNone map cannot hash strings.");

        if constexpr (M::function == HashFunctionFnv1a)
            return ConstHashFnv1a(string);
        else
            return ConstHashRapid(string);
    }



    //- Accessors

    //~ Lifetime

    // Initializes an empty map of V values. 'capacity' is the entry count to hold without growing; the slots are
    // only allocated on the first insert.
    template<typename V, HashFunction F>
    inline void HashMapInit(HashMap<F>* map, MemoryAllocator allocator, HashKeyType keyType, u64 capacity = 0)
    {
        HashMapCoreInit(&map->core, allocator, keyType, sizeof(V), capacity);
    }

    // Removes every entry and keeps the table.
    template<HashFunction F>
    inline void HashMapClear(HashMap<F>* map)
    {
        HashMapCoreClear(&map->core);
    }

    // Returns the table and string keys to the allocator (a no-op for one without deallocate) and clears the map.
    template<HashFunction F>
    inline void HashMapDeInit(HashMap<F>* map)
    {
        HashMapCoreDeInit(&map->core);
    }

    //~ Find
    // Returns the key's value, or null if it has none.

    template<typename V, HashFunction F>
    inline V* HashMapFind(HashMap<F>* map, u64 key)
    {
        TOOL_DEBUG_ASSERT(sizeof(V) == map->core.valueSize, "Value type does not match the map.");
        return (V*)HashMapCoreFind(&map->core, key, HashMapHash<F>(key));
    }

    template<typename V, HashFunction F>
    inline V* HashMapFind(HashMap<F>* map, s8 key)
    {
        TOOL_DEBUG_ASSERT(sizeof(V) == map->core.valueSize, "Value type does not match the map.");
        return (V*)HashMapCoreFind(&map->core, key, HashMapHash<F>(key));
    }

    // 'hash' must be the map's hash of 'key'.
    template<typename V, HashFunction F>
    inline V* HashMapFind(HashMap<F>* map, s8 key, u64 hash)
    {
        TOOL_DEBUG_ASSERT(sizeof(V) == map->core.valueSize, "Value type does not match the map.");
        if constexpr (F != HashFunctionNone)
            TOOL_DEBUG_ASSERT(hash == HashMapHash<F>(key), "Hash does not match the map's hash of the key.");
        return (V*)HashMapCoreFind(&map->core, key, hash);
    }

    //~ Insert
    // Copies 'value' in, overwriting the value of a key already present. Returns the stored value, or null if the
    // map could not grow.

    template<typename V, HashFunction F>
    inline V* HashMapInsert(HashMap<F>* map, u64 key, const V& value)
    {
        TOOL_DEBUG_ASSERT(sizeof(V) == map->core.valueSize, "Value type does not match the map.");
        return (V*)HashMapCoreInsert(&map->core, key, HashMapHash<F>(key), &value);
    }

    template<typename V, HashFunction F>
    inline V* HashMapInsert(HashMap<F>* map, s8 key, const V& value)
    {
        TOOL_DEBUG_ASSERT(sizeof(V) == map->core.valueSize, "Value type does not match the map.");
        return (V*)HashMapCoreInsert(&map->core, key, HashMapHash<F>(key), &value);
    }

    // 'hash' must be the map's hash of 'key'.
    template<typename V, HashFunction F>
    inline V* HashMapInsert(HashMap<F>* map, s8 key, u64 hash, const V& value)
    {
        TOOL_DEBUG_ASSERT(sizeof(V) == map->core.valueSize, "Value type does not match the map.");
        if constexpr (F != HashFunctionNone)
            TOOL_DEBUG_ASSERT(hash == HashMapHash<F>(key), "Hash does not match the map's hash of the key.");
        return (V*)HashMapCoreInsert(&map->core, key, hash, &value);
    }

    //~ Remove
    // Returns false if the key was not present.

    template<HashFunction F>
    inline b8 HashMapRemove(HashMap<F>* map, u64 key)
    {
        return HashMapCoreRemove(&map->core, key, HashMapHash<F>(key));
    }

    template<HashFunction F>
    inline b8 HashMapRemove(HashMap<F>* map, s8 key)
    {
        return HashMapCoreRemove(&map->core, key, HashMapHash<F>(key));
    }

    // 'hash' must be the map's hash of 'key'.
    template<HashFunction F>
    inline b8 HashMapRemove(HashMap<F>* map, s8 key, u64 hash)
    {
        if constexpr (F != HashFunctionNone)
            TOOL_DEBUG_ASSERT(hash == HashMapHash<F>(key), "Hash does not match the map's hash of the key.");
        return HashMapCoreRemove(&map->core, key, hash);
    }

    //~ Iteration
    // Steps to the next entry from 'cursor', which starts at 0. Returns false past the last one. The map must not
    // gain or lose entries while iterating; writing through the value pointers is fine.
    // Usage: 'for (u64 cursor = 0; HashMapNext(&map, &cursor, &key, &value);)'.

    template<typename V, HashFunction F>
    inline b8 HashMapNext(HashMap<F>* map, u64* cursor, V** outValue)
    {
        TOOL_DEBUG_ASSERT(sizeof(V) == map->core.valueSize, "Value type does not match the map.");
        return HashMapCoreNext(&map->core, cursor, nullptr, (void**)outValue);
    }

    template<typename V, HashFunction F>
    inline b8 HashMapNext(HashMap<F>* map, u64* cursor, u64* outKey, V** outValue)
    {
        TOOL_DEBUG_ASSERT(sizeof(V) == map->core.valueSize, "Value type does not match the map.");
        TOOL_DEBUG_ASSERT(map->core.keyType == HashKeyTypeU64, "Key type does not match the map.");

        void* key;
        if (!HashMapCoreNext(&map->core, cursor, &key, (void**)outValue))
            return false;

        *outKey = *(u64*)key;
        return true;
    }

    template<typename V, HashFunction F>
    inline b8 HashMapNext(HashMap<F>* map, u64* cursor, s8* outKey, V** outValue)
    {
        TOOL_DEBUG_ASSERT(sizeof(V) == map->core.valueSize, "Value type does not match the map.");
        TOOL_DEBUG_ASSERT(map->core.keyType == HashKeyTypeS8, "Key type does not match the map.");

        void* key;
        if (!HashMapCoreNext(&map->core, cursor, &key, (void**)outValue))
            return false;

        *outKey = *(s8*)key;
        return true;
    }
}



#endif //_TOOL_HASHMAP_H
