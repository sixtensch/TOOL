#ifndef _TOOL_MEMORY_H
#define _TOOL_MEMORY_H

#include "basics.h"



//~ Definitions

#define TOOL_ARENA_COMMIT_SIZE 1024
#define TOOL_ARENA_MAX_INCREMENT_SIZE 64 * 1024 * 1024



namespace Tool
{
    //- Struct definitions
    
    //~ Memory region
    
    struct MemoryRegion
    {
        void* start = nullptr;
        u64 reserved;
        u64 committed;
    };
    
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
    
    //~ Arena
    
    struct ArenaFrame
    {
        void* start = nullptr;
        u64 size;
    };
    
    struct Arena
    {
        MemoryRegion region;
        
        u64 size;
        
        void* startCurrent = nullptr;
        u64 sizeCurrent;
    };
    
    //~ Circular buffer
    
    struct Circular
    {
        MemoryLoop loop;
        u64 start;
        u64 size;
    };
    
    //~ Allocator
    
    typedef void* (*AllocateFunction)(u64 size, void* data);
    typedef void (*DeallocateFunction)(void* target, void* data);
    
    struct MemoryAllocator
    {
        AllocateFunction allocate;
        DeallocateFunction deallocate;
        void* data;
    };
    
    
    
    //- Core memory helper functions
    
    //~ Heap functionality
    
    void* ClassicAlloc(u64 size); 
    void* ClassicAlloc(u64 count, u64 size);
    template<typename T> inline T* ClassicAlloc() { return (T*)ClassicAlloc(sizeof(T)); }
    
    void ClassicDealloc(void* start);
    
    void ClassicRealloc(void** target, u64 currentSize, u64 newSize);
    void ClassicRealloc(void** target, u64 currentCount, u64 newCount, u64 size);
    
    //~ Memory region
    
    // Initializes/reserves uninitialized region.
    void RegionReserve(MemoryRegion* region, u64 size);
    void RegionReserve(MemoryRegion* region, u64 count, u64 size);
    
    // Commits reserved memory pages. Cannot commit beyond reserved range.
    void RegionCommit(MemoryRegion* region, u64 newSize);    
    void RegionCommit(MemoryRegion* region, u64 newCount, u64 size);    
    
    // De-commits committed memory pages, reverting them to "reserved".
    void RegionRevert(MemoryRegion* region, u64 newSize); 
    void RegionRevert(MemoryRegion* region, u64 newCount, u64 size);
    
    // Deallocates region, returning it to an uninitialized state.
    void RegionDealloc(MemoryRegion* region);
    
    //~ Memory loop
    
    // Initializes/allocates uninitialized memory loop.
    void LoopAlloc(MemoryLoop* loop, u64 minCommittedSize, u64 minMirroredSize);
    
    // Deallocates region, returning it to an uninitialized state.
    void LoopDealloc(MemoryLoop* loop);
    
    bool LoopIsInitialized(const MemoryLoop* loop);
    
    
    
    //- Higher level memory helper functions
    
    //~ Arena
    
    // Initializes and reserves memory arena. The allocated arena size may never exceed 'reservedSize'.
    void ArenaInit(Arena* arena, u64 reservedSize);
    
    // Allocates space within the current arena frame.
    void* ArenaAlloc(Arena* arena, u64 size);
    void* ArenaAlloc(Arena* arena, u64 count, u64 size);
    template<typename T> inline T* ArenaAlloc(Arena* arena) { return (T*)ArenaAlloc(arena, sizeof(T)); }
    
    // Allocates space in two steps, retrieving the location first, then committing the space.
    // No safety features synchronize mulitple simultaneous allocations, which has to be external.
    void* ArenaAllocBegin(Arena* arena, u64 reservedSize);
    void* ArenaAllocEnd(Arena* arena, u64 actualSize); // Actual size should always be <= reserved size
    
    // Places new object onto the current arena frame
    // WARNING: objects created this way will NOT automatically destruct when popping the arena frame!
    template<typename T, class... Args> inline T* ArenaPlace(Arena* arena, Args&&... args) { return new ((T*)Tool::ArenaAlloc(arena, sizeof(T))) T((Args&&...)args...); }
    
    // Pushes a new arena frame.
    void ArenaPush(Arena* arena);
    
    // Pops the current arena frame. Throws an exception if currently in frame 0.
    void ArenaPop(Arena* arena);
    
    // De-initializes and frees memory arena.
    void ArenaDeInit(Arena* arena);
    
    //~ Circular buffer
    
    // Initialize and allocate a new circular buffer. The actual size and overflow region might be larger than requested.
    void CircularInit(Circular* circular, u64 requestedSize, u64 requestedOverflowSize);
    
    // Allocate space within the circular buffer. Nullptr indicates insufficient space.
    void* CircularAlloc(Circular* circular, u64 size);
    void* CircularAlloc(Circular* circular, u64 count, u64 size);
    template<typename T> inline T* CircularAlloc(Circular* circular) { return (T*)CircularAlloc(circular, sizeof(T)); }
    
    // Allocates space in two steps, similarly to the same Arena feature.
    void* CircularAllocBegin(Circular* circular, u64 reservedSize);
    void* CircularAllocEnd(Circular* circular, u64 actualSize);
    
    // Places new object onto the circular buffer.
    // WARNING: objects created this way will NOT automatically destruct when popping the arena frame!
    template<typename T, class... Args> inline T* CircularPlace(Circular* circular, Args&&... args) { return new ((T*)Tool::CircularAlloc(circular, sizeof(T))) T((Args&&...)args...); }
    
    // Get a reference to the current writing location (bookmark).
    // Can be used to then get a data pointer, or deallocate everything prior to the bookmark.
    u64 CircularGetBookmark(const Circular* circular);
    void* CircularGetDataAt(Circular* circular, u64 bookmark);
    void CircularPopToBookmark(Circular* circular, u64 bookmark);
    
    // Deinitialize the circular buffer.
    void CircularDeInit(Circular* circular);
    
    //~ Allocators
    
    // Produces an allocator
    MemoryAllocator Allocator();                   // Heap
    MemoryAllocator Allocator(Arena* arena);       // Arena
    MemoryAllocator Allocator(Circular* circular); // Circular buffer
    
    // Allocates space using the given allocator method
    void* AllocatorAlloc(MemoryAllocator allocator, u64 size);
    void* AllocatorAlloc(MemoryAllocator allocator, u64 count, u64 size);
    template<typename T> inline T* AllocatorAlloc(MemoryAllocator allocator) { return (T*)AllocatorAlloc(allocator, sizeof(T)); }
    
    // Deallocates previously allocated data. This might (intentionally) not do anything for certain allocators. 
    void AllocatorDealloc(MemoryAllocator allocator, void* target);
    
}



#endif //_MEMORY_H
