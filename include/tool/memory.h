#ifndef _TOOL_MEMORY_H
#define _TOOL_MEMORY_H

#include "basics.h"



//~ Definitions

// Arena: chunk payload size when none is given at init
#define TOOL_ARENA_DEFAULT_CHUNK_SIZE (64ull * 1024)

// Arena: alignment of every allocation that does not ask for its own
#define TOOL_ARENA_ALIGNMENT 16ull

// Arena: byte written over released memory in debug builds (TOOL_DEBUG_ASSERTS), so a stale pointer reads
// back 0xDADADADA... - a recognizable pattern, and a non-canonical address when dereferenced as a pointer.
#define TOOL_ARENA_POISON 0xDA

// Contiguous arena: commit granularity
#define TOOL_CONTIGUOUS_ARENA_COMMIT_SIZE 1024
#define TOOL_CONTIGUOUS_ARENA_MAX_INCREMENT_SIZE 64 * 1024 * 1024



namespace Tool
{
    //- Struct definitions

    //~ Allocator

    typedef void* (*AllocateFunction)(u64 size, void* data);
    typedef void (*DeallocateFunction)(void* target, void* data);

    struct MemoryAllocator
    {
        AllocateFunction allocate;
        DeallocateFunction deallocate;
        void* data;
    };

    //~ Arena

    // One allocation from an arena's source. The payload follows the header.
    struct ArenaChunk
    {
        ArenaChunk* next;
        u64 capacity; // Payload bytes
        u64 used;     // Payload bytes handed out, alignment padding included
        u64 sequence; // Dedicated chunks only: when it was handed out, in the arena's sequence
    };

    // Record left in the arena by ArenaPush. Holds the position to restore on the matching ArenaPop.
    struct ArenaFrame
    {
        ArenaFrame* previous;
        ArenaChunk* chunk;
        u64 chunkUsed;
        u64 used;
        u64 sequence;
    };

    // Chunked bump allocator. Grows by appending chunks from its source, never by resizing or remapping, so
    // it works on every platform - including linear-memory targets with no reserve/commit. Consecutive
    // allocations are not guaranteed to be adjacent: never treat an arena as one contiguous range.
    struct Arena
    {
        MemoryAllocator source;
        u64 chunkSize; // Payload capacity of a regular chunk
        const c8* name; // Optional, for diagnostics

        ArenaChunk* first;   // Regular chunks in allocation order. Those after 'current' are spares.
        ArenaChunk* current; // Receives allocations that fit a regular chunk

        ArenaChunk* dedicated;      // Chunks sized to one oversized allocation, newest first
        ArenaChunk* dedicatedSpare; // Dedicated chunks released by a reset or pop, kept for reuse
        ArenaChunk* pending;        // Dedicated chunk held between ArenaAllocBegin and ArenaAllocEnd

        ArenaFrame* frame; // Innermost ArenaPush, or null
        u64 sequence;      // Last sequence number stamped on a dedicated chunk. Orders them against pushes.

        // Stats
        u64 used;       // Bytes handed out, alignment padding included
        u64 peak;       // Highest 'used' since init
        u64 owned;      // Bytes held from the source, chunk headers and spares included
        u32 chunkCount; // Chunks held from the source, spares included
    };

#ifdef TOOL_VIRTUAL_MEMORY

    //~ Memory region

    struct MemoryRegion
    {
        void* start = nullptr;
        u64 reserved;
        u64 committed;
    };

    //~ Contiguous arena

    struct ContiguousArenaFrame
    {
        void* start = nullptr;
        u64 size;
    };

    // Reserve/commit arena: one contiguous address range that commits as it grows.
    struct ContiguousArena
    {
        MemoryRegion region;

        u64 size;

        void* startCurrent = nullptr;
        u64 sizeCurrent;
    };

#endif // TOOL_VIRTUAL_MEMORY

#ifdef TOOL_MIRRORED_MEMORY

    //~ Memory loop

    // Handle to anonymous memory, without inherent address space.
    // Represents a file mapping on Windows, backed by the page file.
    typedef void* AnonymousMemory;

    struct MemoryLoop
    {
        AnonymousMemory memory;
        void* start = nullptr;
        u64 committed;
        u64 mirrored;
    };

    //~ Magic circular buffer

    // Ring buffer over a memory loop: the pages are mapped twice back to back, so an allocation that
    // crosses the end of the ring stays contiguous.
    struct MagicCircular
    {
        MemoryLoop loop;
        u64 start;
        u64 size;
    };

#endif // TOOL_MIRRORED_MEMORY



    //- Core memory helper functions

    //~ Heap functionality

    // Plain malloc/free/realloc. Returns null on failure.
    void* ClassicAlloc(u64 size);
    void* ClassicAlloc(u64 count, u64 size);
    template<typename T> inline T* ClassicAlloc() { return (T*)ClassicAlloc(sizeof(T)); }

    void ClassicDealloc(void* start);

    // Returns false on failure, leaving the original allocation in place.
    b8 ClassicRealloc(void** target, u64 currentSize, u64 newSize);
    b8 ClassicRealloc(void** target, u64 currentCount, u64 newCount, u64 size);

#ifdef TOOL_VIRTUAL_MEMORY

    //~ Page functionality

    // Whole pages straight from the OS, rounded up to its allocation granularity. Returns null on failure.
    void* PageAlloc(u64 size);

    // 'size' must be the size passed to PageAlloc.
    void PageDealloc(void* start, u64 size);

    //~ Memory region

    // Initializes/reserves uninitialized region. Returns false if the address space could not be reserved.
    b8 RegionReserve(MemoryRegion* region, u64 size);
    b8 RegionReserve(MemoryRegion* region, u64 count, u64 size);

    // Commits reserved memory pages. Cannot commit beyond reserved range. Returns false if the pages could not be committed.
    b8 RegionCommit(MemoryRegion* region, u64 newSize);
    b8 RegionCommit(MemoryRegion* region, u64 newCount, u64 size);

    // De-commits committed memory pages, reverting them to "reserved".
    void RegionRevert(MemoryRegion* region, u64 newSize);
    void RegionRevert(MemoryRegion* region, u64 newCount, u64 size);

    // Deallocates region, returning it to an uninitialized state.
    void RegionDealloc(MemoryRegion* region);

#endif // TOOL_VIRTUAL_MEMORY

#ifdef TOOL_MIRRORED_MEMORY

    //~ Memory loop

    // Initializes/allocates uninitialized memory loop. Returns false on failure.
    b8 LoopAlloc(MemoryLoop* loop, u64 minCommittedSize, u64 minMirroredSize);

    // Deallocates region, returning it to an uninitialized state.
    void LoopDealloc(MemoryLoop* loop);

    bool LoopIsInitialized(const MemoryLoop* loop);

#endif // TOOL_MIRRORED_MEMORY



    //- Higher level memory helper functions

    //~ Arena

    // Initializes an empty arena. No memory is taken from 'source' until the first allocation; after that it
    // is taken one chunk at a time, each 'chunkSize' payload bytes. 'name' is kept by pointer.
    void ArenaInit(Arena* arena, MemoryAllocator source, u64 chunkSize = TOOL_ARENA_DEFAULT_CHUNK_SIZE, const c8* name = nullptr);

    // Initializes a child arena whose chunks are allocations in 'parent'. Resetting or popping the parent
    // past those allocations reclaims the child wholesale, with no bookkeeping on the child: the child is
    // dead from then on, and must not be used again without re-initializing it.
    void ArenaInitChild(Arena* child, Arena* parent, u64 chunkSize = TOOL_ARENA_DEFAULT_CHUNK_SIZE, const c8* name = nullptr);

    // Allocates within the arena, aligned to TOOL_ARENA_ALIGNMENT. An allocation never straddles chunks; one
    // larger than a chunk gets a dedicated chunk of its own. Returns null if the source is exhausted.
    void* ArenaAlloc(Arena* arena, u64 size);
    void* ArenaAlloc(Arena* arena, u64 count, u64 size);

    // As ArenaAlloc, with an explicit alignment. 'alignment' must be a power of two.
    void* ArenaAllocAligned(Arena* arena, u64 size, u64 alignment);

    template<typename T> inline T* ArenaAlloc(Arena* arena) { return (T*)ArenaAllocAligned(arena, sizeof(T), alignof(T)); }

    // Allocates space in two steps, retrieving the location first, then committing the space.
    // No safety features synchronize mulitple simultaneous allocations, which has to be external.
    // Begin guarantees 'reservedSize' contiguous bytes at the returned location, and returns null if the source
    // is exhausted. End commits 'actualSize' <= 'reservedSize' of them, and returns the same location.
    void* ArenaAllocBegin(Arena* arena, u64 reservedSize);
    void* ArenaAllocEnd(Arena* arena, u64 actualSize);

    // Places new object onto the arena.
    // WARNING: objects created this way will NOT automatically destruct when popping or resetting the arena!
    template<typename T, class... Args> inline T* ArenaPlace(Arena* arena, Args&&... args) { return new ((T*)Tool::ArenaAllocAligned(arena, sizeof(T), alignof(T))) T(static_cast<Args&&>(args)...); }

    // Saves the current position. The matching ArenaPop releases everything allocated since, child arenas
    // included, and keeps the chunks for reuse.
    void ArenaPush(Arena* arena);

    // Restores the position saved by the innermost ArenaPush. Must be paired with one.
    void ArenaPop(Arena* arena);

    // Releases every allocation, child arenas included, and drops every push. All chunks are kept for reuse,
    // so a reset/refill cycle of the same shape makes no source calls.
    void ArenaReset(Arena* arena);

    // Returns spare chunks to the source: every regular spare beyond 'keepSpareCount', and every dedicated
    // spare. Does nothing for a source without deallocate.
    void ArenaTrim(Arena* arena, u32 keepSpareCount = 0);

    // Returns every chunk to the source (a no-op for a source without deallocate) and clears the arena.
    void ArenaDeInit(Arena* arena);

    // The arena's name, or a placeholder if it has none. Never null.
    const c8* ArenaName(const Arena* arena);

#ifdef TOOL_VIRTUAL_MEMORY

    //~ Contiguous arena

    // Initializes and reserves memory arena. The allocated arena size may never exceed 'reservedSize'.
    // Returns false if the memory could not be reserved.
    b8 ContiguousArenaInit(ContiguousArena* arena, u64 reservedSize);

    // Allocates space within the current arena frame. Returns null if the arena is exhausted or cannot commit.
    void* ContiguousArenaAlloc(ContiguousArena* arena, u64 size);
    void* ContiguousArenaAlloc(ContiguousArena* arena, u64 count, u64 size);
    template<typename T> inline T* ContiguousArenaAlloc(ContiguousArena* arena) { return (T*)ContiguousArenaAlloc(arena, sizeof(T)); }

    // Allocates space in two steps, retrieving the location first, then committing the space.
    // No safety features synchronize mulitple simultaneous allocations, which has to be external.
    // Begin returns null if the arena is exhausted or cannot commit.
    void* ContiguousArenaAllocBegin(ContiguousArena* arena, u64 reservedSize);
    void* ContiguousArenaAllocEnd(ContiguousArena* arena, u64 actualSize); // Actual size should always be <= reserved size

    // Places new object onto the current arena frame
    // WARNING: objects created this way will NOT automatically destruct when popping the arena frame!
    template<typename T, class... Args> inline T* ContiguousArenaPlace(ContiguousArena* arena, Args&&... args) { return new ((T*)Tool::ContiguousArenaAlloc(arena, sizeof(T))) T(static_cast<Args&&>(args)...); }

    // Pushes a new arena frame.
    void ContiguousArenaPush(ContiguousArena* arena);

    // Pops the current arena frame. Must not be called in frame 0.
    void ContiguousArenaPop(ContiguousArena* arena);

    // De-initializes and frees memory arena.
    void ContiguousArenaDeInit(ContiguousArena* arena);

#endif // TOOL_VIRTUAL_MEMORY

#ifdef TOOL_MIRRORED_MEMORY

    //~ Magic circular buffer

    // Initialize and allocate a new circular buffer. The actual size and overflow region might be larger than requested.
    // Returns false on failure.
    b8 MagicCircularInit(MagicCircular* circular, u64 requestedSize, u64 requestedOverflowSize);

    // Allocate space within the circular buffer. Nullptr indicates insufficient space.
    void* MagicCircularAlloc(MagicCircular* circular, u64 size);
    void* MagicCircularAlloc(MagicCircular* circular, u64 count, u64 size);
    template<typename T> inline T* MagicCircularAlloc(MagicCircular* circular) { return (T*)MagicCircularAlloc(circular, sizeof(T)); }

    // Allocates space in two steps, similarly to the same Arena feature.
    void* MagicCircularAllocBegin(MagicCircular* circular, u64 reservedSize);
    void* MagicCircularAllocEnd(MagicCircular* circular, u64 actualSize);

    // Places new object onto the circular buffer.
    // WARNING: objects created this way will NOT automatically destruct when popping the arena frame!
    template<typename T, class... Args> inline T* MagicCircularPlace(MagicCircular* circular, Args&&... args) { return new ((T*)Tool::MagicCircularAlloc(circular, sizeof(T))) T(static_cast<Args&&>(args)...); }

    // Get a reference to the current writing location (bookmark).
    // Can be used to then get a data pointer, or deallocate everything prior to the bookmark.
    u64 MagicCircularGetBookmark(const MagicCircular* circular);
    void* MagicCircularGetDataAt(MagicCircular* circular, u64 bookmark);
    void MagicCircularPopToBookmark(MagicCircular* circular, u64 bookmark);

    // Deinitialize the circular buffer.
    void MagicCircularDeInit(MagicCircular* circular);

#endif // TOOL_MIRRORED_MEMORY

    //~ Allocators

    // Produces an allocator
    MemoryAllocator Allocator();             // Heap (malloc/free)
    MemoryAllocator Allocator(Arena* arena); // Arena, with no deallocate

#ifdef TOOL_VIRTUAL_MEMORY
    MemoryAllocator Allocator(ContiguousArena* arena); // Contiguous arena, with no deallocate
#endif

#ifdef TOOL_MIRRORED_MEMORY
    MemoryAllocator Allocator(MagicCircular* circular); // Magic circular buffer, with no deallocate
#endif

    // Allocates space using the given allocator method
    void* AllocatorAlloc(MemoryAllocator allocator, u64 size);
    void* AllocatorAlloc(MemoryAllocator allocator, u64 count, u64 size);
    template<typename T> inline T* AllocatorAlloc(MemoryAllocator allocator) { return (T*)AllocatorAlloc(allocator, sizeof(T)); }

    // Deallocates previously allocated data. This might (intentionally) not do anything for certain allocators.
    void AllocatorDealloc(MemoryAllocator allocator, void* target);

}



#endif //_MEMORY_H
