#include "hashmap.h"
#include "error.h"

#include <string.h>



//- Static helper functions

namespace Tool
{
    //~ Slots

    // 2^64 / golden ratio. Multiplying by it moves every input bit into the high bits that pick the home slot.
    static const u64 hashMapFibonacci = 0x9e3779b97f4a7c15ull;

    // A stored hash of 0 marks an empty slot, so a real hash of 0 is stored as 1. Keys still compare in full.
    static u64 HashMapStoredHash(u64 hash)
    {
        return hash + (hash == 0);
    }

    static u64 HashMapHome(const HashMapCore* map, u64 storedHash)
    {
        return (storedHash * hashMapFibonacci) >> map->shift;
    }

    static u64 HashMapKeySize(HashKeyType keyType)
    {
        return (keyType == HashKeyTypeU64) ? sizeof(u64) : sizeof(s8);
    }

    static void* HashMapSlotKey(const HashMapCore* map, u64 slot)
    {
        return map->keys + slot * HashMapKeySize(map->keyType);
    }

    static void* HashMapSlotValue(const HashMapCore* map, u64 slot)
    {
        return map->values + slot * map->valueSize;
    }

    // Smallest slot count holding 'entries' within the 3/4 load limit.
    static u64 HashMapCapacityFor(u64 entries)
    {
        u64 capacity = TOOL_HASH_MAP_MIN_CAPACITY;
        while (entries > capacity - capacity / 4)
            capacity *= 2;
        return capacity;
    }

    static u32 HashMapShiftFor(u64 capacity)
    {
        u32 log2 = 0;
        while (((u64)1 << log2) < capacity)
            log2++;
        return 64 - log2;
    }

    //~ Keys

    static b8 HashMapKeyEquals(const HashMapCore* map, u64 slot, const void* key)
    {
        if (map->keyType == HashKeyTypeU64)
            return *(const u64*)HashMapSlotKey(map, slot) == *(const u64*)key;

        const s8* a = (const s8*)HashMapSlotKey(map, slot);
        const s8* b = (const s8*)key;
        return a->size == b->size && (a->size == 0 || memcmp(a->str, b->str, a->size) == 0);
    }

    static void HashMapFreeKeys(HashMapCore* map)
    {
        if (map->keyType != HashKeyTypeS8 || map->hashes == nullptr || map->allocator.deallocate == nullptr)
            return;

        for (u64 slot = 0; slot < map->capacity; slot++)
        {
            if (map->hashes[slot] != 0)
                AllocatorDealloc(map->allocator, ((s8*)HashMapSlotKey(map, slot))->str);
        }
    }

    //~ Probing

    // Finds the slot holding 'key', or the empty slot ending its probe sequence. Returns whether the key was found.
    static b8 HashMapLocate(const HashMapCore* map, const void* key, u64 storedHash, u64* outSlot)
    {
        u64 mask = map->capacity - 1;
        u64 slot = HashMapHome(map, storedHash);

        while (map->hashes[slot] != 0)
        {
            if (map->hashes[slot] == storedHash && HashMapKeyEquals(map, slot, key))
            {
                *outSlot = slot;
                return true;
            }
            slot = (slot + 1) & mask;
        }

        *outSlot = slot;
        return false;
    }

    // Moves the table to 'capacity' slots, releasing the old one to the allocator.
    static b8 HashMapResize(HashMapCore* map, u64 capacity)
    {
        u64 keySize = HashMapKeySize(map->keyType);
        u64 hashesSize = capacity * sizeof(u64);
        u64 keysSize = capacity * keySize;

        // One block: hashes, keys, values. Every section starts at a multiple of 64 bytes, as capacity >= 8.
        u8* block = (u8*)AllocatorAlloc(map->allocator, hashesSize + keysSize + capacity * map->valueSize);
        if (block == nullptr)
            return TOOL_FAIL("Hash map could not allocate %llu slots.", capacity);

        HashMapCore old = *map;

        map->capacity = capacity;
        map->shift = HashMapShiftFor(capacity);
        map->hashes = (u64*)block;
        map->keys = block + hashesSize;
        map->values = block + hashesSize + keysSize;
        memset(map->hashes, 0, hashesSize);

        if (old.hashes == nullptr)
            return true;

        u64 mask = capacity - 1;
        for (u64 oldSlot = 0; oldSlot < old.capacity; oldSlot++)
        {
            u64 storedHash = old.hashes[oldSlot];
            if (storedHash == 0)
                continue;

            u64 slot = HashMapHome(map, storedHash);
            while (map->hashes[slot] != 0)
                slot = (slot + 1) & mask;

            map->hashes[slot] = storedHash;
            memcpy(HashMapSlotKey(map, slot), HashMapSlotKey(&old, oldSlot), keySize);
            memcpy(HashMapSlotValue(map, slot), HashMapSlotValue(&old, oldSlot), map->valueSize);
        }

        AllocatorDealloc(map->allocator, old.hashes);
        return true;
    }

    //~ Operations

    static void* HashMapFindAny(HashMapCore* map, const void* key, u64 hash)
    {
        if (map->hashes == nullptr)
            return nullptr;

        u64 slot;
        if (!HashMapLocate(map, key, HashMapStoredHash(hash), &slot))
            return nullptr;

        return HashMapSlotValue(map, slot);
    }

    static void* HashMapInsertAny(HashMapCore* map, const void* key, u64 hash, const void* value)
    {
        u64 storedHash = HashMapStoredHash(hash);
        u64 slot;

        b8 found = false;
        if (map->hashes != nullptr)
            found = HashMapLocate(map, key, storedHash, &slot);

        if (!found)
        {
            // Grow first, so the slot is found in the table the entry ends up in.
            if (map->hashes == nullptr || map->count + 1 > map->capacity - map->capacity / 4)
            {
                u64 capacity = (map->hashes == nullptr) ? map->capacity : map->capacity * 2;
                if (!HashMapResize(map, capacity))
                    return nullptr;

                HashMapLocate(map, key, storedHash, &slot);
            }

            if (map->keyType == HashKeyTypeU64)
            {
                *(u64*)HashMapSlotKey(map, slot) = *(const u64*)key;
            }
            else
            {
                const s8* source = (const s8*)key;
                c8* copy = (c8*)AllocatorAlloc(map->allocator, source->size + 1);
                if (copy == nullptr)
                {
                    TOOL_FAIL("Hash map could not copy a %llu byte key.", source->size);
                    return nullptr;
                }

                if (source->size > 0)
                    memcpy(copy, source->str, source->size);
                copy[source->size] = '\0';

                *(s8*)HashMapSlotKey(map, slot) = { copy, source->size };
            }

            map->hashes[slot] = storedHash;
            map->count++;
        }

        void* stored = HashMapSlotValue(map, slot);
        if (value != nullptr)
            memcpy(stored, value, map->valueSize);
        else
            memset(stored, 0, map->valueSize);

        return stored;
    }

    static b8 HashMapRemoveAny(HashMapCore* map, const void* key, u64 hash)
    {
        if (map->hashes == nullptr)
            return false;

        u64 hole;
        if (!HashMapLocate(map, key, HashMapStoredHash(hash), &hole))
            return false;

        if (map->keyType == HashKeyTypeS8)
            AllocatorDealloc(map->allocator, ((s8*)HashMapSlotKey(map, hole))->str);

        // Backward shift: pull each following entry of the run into the hole when the hole lies between the
        // entry's home and its slot, so every probe sequence stays unbroken without tombstones.
        u64 keySize = HashMapKeySize(map->keyType);
        u64 mask = map->capacity - 1;
        u64 next = (hole + 1) & mask;

        while (map->hashes[next] != 0)
        {
            u64 home = HashMapHome(map, map->hashes[next]);
            if (((next - home) & mask) >= ((next - hole) & mask))
            {
                map->hashes[hole] = map->hashes[next];
                memcpy(HashMapSlotKey(map, hole), HashMapSlotKey(map, next), keySize);
                memcpy(HashMapSlotValue(map, hole), HashMapSlotValue(map, next), map->valueSize);
                hole = next;
            }
            next = (next + 1) & mask;
        }

        map->hashes[hole] = 0;
        map->count--;
        return true;
    }
}



namespace Tool
{
    //- Core

    void HashMapCoreInit(HashMapCore* map, MemoryAllocator allocator, HashKeyType keyType, u64 valueSize, u64 capacity)
    {
        *map = {};
        map->allocator = allocator;
        map->keyType = keyType;
        map->valueSize = valueSize;
        map->capacity = HashMapCapacityFor(capacity);
        map->shift = HashMapShiftFor(map->capacity);
    }

    void* HashMapCoreFind(HashMapCore* map, u64 key, u64 hash)
    {
        TOOL_DEBUG_ASSERT(map->keyType == HashKeyTypeU64, "Integer key used on a map of string keys.");
        return HashMapFindAny(map, &key, hash);
    }

    void* HashMapCoreFind(HashMapCore* map, s8 key, u64 hash)
    {
        TOOL_DEBUG_ASSERT(map->keyType == HashKeyTypeS8, "String key used on a map of integer keys.");
        return HashMapFindAny(map, &key, hash);
    }

    void* HashMapCoreInsert(HashMapCore* map, u64 key, u64 hash, const void* value)
    {
        TOOL_DEBUG_ASSERT(map->keyType == HashKeyTypeU64, "Integer key used on a map of string keys.");
        return HashMapInsertAny(map, &key, hash, value);
    }

    void* HashMapCoreInsert(HashMapCore* map, s8 key, u64 hash, const void* value)
    {
        TOOL_DEBUG_ASSERT(map->keyType == HashKeyTypeS8, "String key used on a map of integer keys.");
        return HashMapInsertAny(map, &key, hash, value);
    }

    b8 HashMapCoreRemove(HashMapCore* map, u64 key, u64 hash)
    {
        TOOL_DEBUG_ASSERT(map->keyType == HashKeyTypeU64, "Integer key used on a map of string keys.");
        return HashMapRemoveAny(map, &key, hash);
    }

    b8 HashMapCoreRemove(HashMapCore* map, s8 key, u64 hash)
    {
        TOOL_DEBUG_ASSERT(map->keyType == HashKeyTypeS8, "String key used on a map of integer keys.");
        return HashMapRemoveAny(map, &key, hash);
    }

    b8 HashMapCoreNext(HashMapCore* map, u64* cursor, void** outKey, void** outValue)
    {
        if (map->hashes == nullptr)
            return false;

        for (u64 slot = *cursor; slot < map->capacity; slot++)
        {
            if (map->hashes[slot] == 0)
                continue;

            if (outKey != nullptr)
                *outKey = HashMapSlotKey(map, slot);
            if (outValue != nullptr)
                *outValue = HashMapSlotValue(map, slot);

            *cursor = slot + 1;
            return true;
        }

        *cursor = map->capacity;
        return false;
    }

    void HashMapCoreClear(HashMapCore* map)
    {
        if (map->hashes == nullptr)
            return;

        HashMapFreeKeys(map);
        memset(map->hashes, 0, map->capacity * sizeof(u64));
        map->count = 0;
    }

    void HashMapCoreDeInit(HashMapCore* map)
    {
        HashMapFreeKeys(map);
        if (map->hashes != nullptr)
            AllocatorDealloc(map->allocator, map->hashes);

        *map = {};
    }
}
