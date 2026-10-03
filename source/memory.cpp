#include "memory.h"
#include "error.h"
#include "mathematics.h"

#include <stdlib.h>
#include <string.h>

#ifdef TOOL_WINDOWS
#include <Windows.h>
#endif

#if defined(TOOL_UNIX) && defined(TOOL_VIRTUAL_MEMORY)
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#endif



//- Static helper functions

//~ Windows static helper functions

#ifdef TOOL_WINDOWS

// Get DWORD parts
static DWORD GetLowDWORD(u64 whole) { return (DWORD)(whole & 0xffffffff); }
static DWORD GetHighDWORD(u64 whole) { return (DWORD)(whole >> 0x20); }

// Round a given size up to the nearest multiple of the granularity, which itself is a power of 2
static u64 RoundToGranularity(u64 size, u64 granularity)
{
    return (size + granularity - 1) & ~(granularity - 1);
}

#endif



namespace Tool
{
    //- Classic allocation
    
    //~ Classic allocation general implementation
    
    void* ClassicAlloc(u64 size)
    {
        void* result = malloc(size);
        
        if (result == nullptr)
        {
            TOOL_FAIL("Out of memory allocating %llu bytes.", size);
        }
        
        return result;
    }
    
    void* ClassicAlloc(u64 count, u64 size)
    {
        return ClassicAlloc(count * size);
    }
    
    void ClassicDealloc(void* start)
    {
        free(start);
    }
    
    b8 ClassicRealloc(void** target, u64 currentSize, u64 newSize)
    {
        void* result = realloc(*target, newSize);
        
        if (result == nullptr && newSize > 0)
        {
            return TOOL_FAIL("Out of memory reallocating %llu bytes to %llu.", currentSize, newSize);
        }
        
        *target = result;
        
        return true;
    }
    
    b8 ClassicRealloc(void** target, u64 currentCount, u64 newCount, u64 size)
    {
        return ClassicRealloc(target, currentCount * size, newCount * size);
    }
    
    
    
#ifdef TOOL_VIRTUAL_MEMORY
    
    //- Page allocation
    
    //~ Page allocation Windows implementation
    
#ifdef TOOL_WINDOWS
    
    void* PageAlloc(u64 size)
    {
        void* result = (void*)VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        
        if (result == nullptr)
        {
            TOOL_FAIL_WINDOWS();
        }
        
        return result;
    }
    
    void PageDealloc(void* start, u64 size)
    {
        (void)size;
        
        b32 result = VirtualFree(start, 0, MEM_RELEASE);
        TOOL_ASSERT(result, "Could not free page allocation (Windows error %lu)", GetLastError());
    }
    
#endif // TOOL_WINDOWS
    
    //~ Page allocation Unix implementation
    
#ifdef TOOL_UNIX
    
    void* PageAlloc(u64 size)
    {
        void* result = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (result == MAP_FAILED)
        {
            TOOL_FAIL_ERRNO();
            return nullptr;
        }
        
        return result;
    }
    
    void PageDealloc(void* start, u64 size)
    {
        i32 result = munmap(start, size);
        TOOL_ASSERT(result == 0, "Could not free page allocation (errno %i)", errno);
    }
    
#endif // TOOL_UNIX
    
    
    
    //- Memory region
    
    //~ Memory region Windows implementation
    
#ifdef TOOL_WINDOWS
    
    b8 RegionReserve(MemoryRegion* region, u64 size)
    {
        TOOL_ASSERT(region->start == nullptr, "Cannot reserve a Region which is already initialized.");
        
        region->start = (void*)VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);
        
        if (region->start == nullptr)
        {
            return TOOL_FAIL_WINDOWS();
        }
        
        region->reserved = size;
        region->committed = 0;
        
        return true;
    }
    
    b8 RegionCommit(MemoryRegion* region, u64 newSize)
    {
        TOOL_ASSERT(newSize <= region->reserved,
                    "Cannot commit more memory to a Region than is reserved. (%llu > %llu)", newSize, region->reserved);
        
        void* result = VirtualAlloc(region->start, newSize, MEM_COMMIT, PAGE_READWRITE);
        
        if (result == nullptr)
        {
            return TOOL_FAIL_WINDOWS();
        }
        
        region->committed = newSize;
        
        return true;
    }
    
    void RegionRevert(MemoryRegion* region, u64 newSize)
    {
        if (newSize >= region->committed)
        {
            return;
        }
        
        b32 result = VirtualFree((u8*)region->start + newSize, region->committed - newSize, MEM_DECOMMIT);
        TOOL_ASSERT(result, "Could not decommit Region memory (Windows error %lu)", GetLastError());
        
        region->committed = newSize;
    }
    
    void RegionDealloc(MemoryRegion* region)
    {
        if (region->start == nullptr)
        {
            return;
        }
        
        b32 result = VirtualFree(region->start, 0, MEM_RELEASE);
        TOOL_ASSERT(result, "Could not release Region memory (Windows error %lu)", GetLastError());
        
        region->start = nullptr;
        region->reserved = 0;
        region->committed = 0;
    }
    
#endif // TOOL_WINDOWS
    
    //~ Memory region Unix implementation
    
#ifdef TOOL_UNIX
    
    // On Unix, mmap does not actually allocate the memory. Physical pages will only be assigned when the memory is acted upon, such as by writing.
    
    // Reservation will mmap the entire region, and *protect* virtual pages beyond the boundary from being interacted with.
    
    static b8 UnixRegionProtect(void* memory, u64 accessible, u64 total)
    {
        static u64 pageSize = (u64)getpagesize();
        
        u64 accessibleAligned = pageSize * ((accessible - 1) / pageSize + 1);
        
        void* startAccessible = memory;
        void* startInaccessible = (void*)((char*)memory + accessibleAligned);
        
        i32 result = 0; 
        result |= mprotect(startAccessible, accessibleAligned, PROT_READ | PROT_WRITE);
        
        if (total > accessibleAligned)
        {
            result |= mprotect(startInaccessible, total - accessibleAligned, PROT_NONE);
        }
        
        if (result < 0)
        {
            return TOOL_FAIL_ERRNO();
        }
        
        return true;
    }
    
    b8 RegionReserve(MemoryRegion* region, u64 size)
    {
        TOOL_ASSERT(region->start == nullptr, "Cannot reserve a Region which is already initialized.");
        
        // Initialize the memory with no access rights
        region->start = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (region->start == MAP_FAILED)
        {
            region->start = nullptr;
            return TOOL_FAIL_ERRNO();
        }
        
        region->reserved = size;
        region->committed = 0;
        
        return true;
    }
    
    b8 RegionCommit(MemoryRegion* region, u64 newSize)
    {
        TOOL_ASSERT(newSize <= region->reserved,
                    "Cannot commit more memory to a Region than is reserved. (%llu > %llu)", newSize, region->reserved);
        
        if (!UnixRegionProtect(region->start, newSize, region->reserved))
        {
            return false;
        }
        
        region->committed = newSize;
        
        return true;
    }
    
    void RegionRevert(MemoryRegion* region, u64 newSize)
    {
        if (newSize >= region->committed)
        {
            return;
        }
        
        b8 reverted = UnixRegionProtect(region->start, newSize, region->reserved);
        TOOL_ASSERT(reverted, "Could not protect reverted Region memory: %s", ErrorMessage());
        
        region->committed = newSize;
    }
    
    void RegionDealloc(MemoryRegion* region)
    {
        if (region->start == nullptr)
        {
            return;
        }
        
        i32 result = munmap(region->start, region->reserved);
        TOOL_ASSERT(result == 0, "Could not release Region memory (errno %i)", errno);
        
        region->start = nullptr;
        region->reserved = 0;
        region->committed = 0;
    }
    
#endif
    
    //~ Memory region general functions
    
    b8 RegionReserve(MemoryRegion* region, u64 count, u64 size)
    {
        return RegionReserve(region, count * size);
    }
    
    b8 RegionCommit(MemoryRegion* region, u64 newCount, u64 size)
    {
        return RegionCommit(region, newCount * size);
    }
    
    void RegionRevert(MemoryRegion* region, u64 newCount, u64 size)
    {
        RegionRevert(region, newCount * size);
    }

#endif // TOOL_VIRTUAL_MEMORY
    
    
    
#ifdef TOOL_MIRRORED_MEMORY
    
    //- Memory loop
    
    //~ Memory loop Windows implementation
    
#ifdef TOOL_WINDOWS
    
    //~ Definitions and function types for runtime loading
    
#ifndef MEM_PRESERVE_PLACEHOLDER
    struct MEM_EXTENDED_PARAMETER;
#define MEM_PRESERVE_PLACEHOLDER 0x2
#define MEM_REPLACE_PLACEHOLDER 0x4000
#define MEM_RESERVE_PLACEHOLDER 0x40000
#define MemExtendedParameterAddressRequirements 0x1
#endif
    
    // Function type definitions mirroring those in memoryapi.h (on newer SDKs)
    
    typedef PVOID (WINAPI* VirtualAlloc2Function)(HANDLE Process,
                                           PVOID BaseAddress,
                                           SIZE_T Size,
                                           ULONG AllocationType,
                                           ULONG PageProtection,
                                           MEM_EXTENDED_PARAMETER* ExtendedParameters,
                                           ULONG ParameterCount);
    
    typedef PVOID (WINAPI* MapViewOfFile3Function)(HANDLE FileMapping,
                                            HANDLE Process,
                                            PVOID BaseAddress,
                                            ULONG64 Offset,
                                            SIZE_T ViewSize,
                                            ULONG AllocationType,
                                            ULONG PageProtection,
                                            MEM_EXTENDED_PARAMETER* ExtendedParameters,
                                            ULONG ParameterCount);
    
    // Initializes/reserves uninitialized memory loop.
    b8 LoopAlloc(MemoryLoop* loop, u64 minCommittedSize, u64 minMirroredSize)
    {
        TOOL_ASSERT(!LoopIsInitialized(loop), "Cannot initialize a Memory Loop which is already initialized.");
        
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        
        u64 granularity = (u64)systemInfo.dwAllocationGranularity; // Most likely 64k
        u64 committedSize = RoundToGranularity(minCommittedSize, granularity);
        u64 mirroredSize = RoundToGranularity(minMirroredSize, granularity);
        u64 totalSize = committedSize + mirroredSize;
        
        // This does not represent an actual file, but rather an anonymous memory allocation backed by the system paging file.
        HANDLE fileMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, 
                                                PAGE_READWRITE, 
                                                GetHighDWORD(committedSize), 
                                                GetLowDWORD(committedSize),
                                                nullptr);
        
        if (fileMapping == NULL || fileMapping == INVALID_HANDLE_VALUE)
        {
            return TOOL_FAIL_WINDOWS();
        }
        
        // Try to load the modern Windows runtime functions from the kernelbase system dll. Performance shouldn't be a big issue.
        HMODULE kernel = LoadLibraryA("kernelbase.dll");
        
        if (kernel == NULL)
        {
            return TOOL_FAIL_WINDOWS();
        }
        
        VirtualAlloc2Function virtualAlloc2 = (VirtualAlloc2Function)(void*)GetProcAddress(kernel, "VirtualAlloc2");
        MapViewOfFile3Function mapViewOfFile3 = (MapViewOfFile3Function)(void*)GetProcAddress(kernel, "MapViewOfFile3");
        
        char* start = nullptr;
        if (virtualAlloc2 && mapViewOfFile3) // If they exist, then use them
        {
            // First, reserve the whole range, both committed and mirrored, in the virtual address space
            start = (char*)virtualAlloc2(0, 0,
                                         totalSize,
                                         MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,	// Reserve the space, and designate it as a placeholder
                                         PAGE_NOACCESS,							// The pages cannot be accessed in this state
                                         0, 0);
            
            if (start == NULL)
            {
                return TOOL_FAIL_WINDOWS();
            }
            
            // Then, remap the reserved range to the memory allocation in chunks.
            u64 currentOffset = 0;
            while (currentOffset < totalSize)
            {
                u64 currentChunkSize = min(totalSize - currentOffset, committedSize);
                bool lastIteration = currentOffset + currentChunkSize >= totalSize;
                
                // First, split off a leading chunk of the original reservation, and preserve it as a placeholder.
                // This prevents other user mode applications from theoretically consuming the addresses between this operation and the next, which is the advantage that the newer function versions provide.
                if (!lastIteration)
                {
                    bool freed = VirtualFree(start + currentOffset,
                                             currentChunkSize,
                                             MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
                    
                    if (!freed)
                    {
                        return TOOL_FAIL_WINDOWS();
                    }
                }
                
                // Then, map the freed address space range to the anonymous memory created earlier.
                PVOID remapping = mapViewOfFile3(fileMapping, 0,
                                                 start + currentOffset, 
                                                 0,							// The offset into the file mapping is always 0
                                                 currentChunkSize,
                                                 MEM_REPLACE_PLACEHOLDER,	// Replace the placeholder with the new mapping
                                                 PAGE_READWRITE,				// These pages can now be accessed as normal
                                                 0, 0);
                
                if (remapping == NULL)
                {
                    return TOOL_FAIL_WINDOWS();
                }
                
                currentOffset += currentChunkSize;
            }
        }
        else // ... if not, we will have to use the older method
        {
            // This method is volatile, having a small chance of failure.
            const int maxAttempts = 32;
            
            bool success = false;
            
            for (int attempt = 0; attempt < maxAttempts && !success; attempt++)
            {
                // Reserve the whole range. This API does not support placeholders.
                start = (char*)VirtualAlloc(0, totalSize, MEM_RESERVE, PAGE_NOACCESS);
                
                if (start == NULL)
                {
                    return TOOL_FAIL_WINDOWS();
                }
                
                // Free the reservation. This will ensure that a continuous block of virtual address space is available, but it does not prevent the OS from mapping other allocations there.
                VirtualFree(start, 0, MEM_RELEASE);
                
                bool couldRemap = true;
                
                u64 currentOffset = 0;
                while (currentOffset < totalSize)
                {
                    u64 currentChunkSize = min(totalSize - currentOffset, committedSize);
                    
                    // Map the address space range to the anonymous memory created earlier, and hope that it is still available.
                    LPVOID remapping = MapViewOfFileEx(fileMapping,
                                                       FILE_MAP_ALL_ACCESS,
                                                       0, 0,
                                                       currentChunkSize,
                                                       start + currentOffset);
                    
                    if (remapping == NULL)
                    {
                        couldRemap = false;
                        break;
                    }
                    
                    currentOffset += currentChunkSize;
                }
                
                // Unmap the partial mapping and try again if not successful
                if (!couldRemap)
                {
                    currentOffset = 0;
                    while (currentOffset < totalSize)
                    {
                        u64 currentChunkSize = min(totalSize - currentOffset, loop->committed);
                        
                        // Unmap each mapped section if possible
                        UnmapViewOfFile(start + currentChunkSize);
                        
                        currentOffset += currentChunkSize;
                    }
                }
                else
                {
                    success = true;
                }
            }
            
            if (!success)
            {
                return TOOL_FAIL("Could not allocate Memory Loop using legacy method, maximum number of failed attempts reached. "
                                 "Consider upgrading to a newer Windows runtime. (%i attempts)",
                                 maxAttempts);
            }
        }
        
        // Set the members
        loop->memory = (AnonymousMemory)fileMapping;
        loop->start = (void*)start;
        loop->committed = committedSize;
        loop->mirrored = mirroredSize;
        
        return true;
    }
    
    // Deallocates region, returning it to an uninitialized state.
    void LoopDealloc(MemoryLoop* loop)
    {
        if (!LoopIsInitialized(loop))
            return;
        
        char* start = (char*)loop->start;
        u64 totalSize = loop->committed + loop->mirrored;
        u64 currentOffset = 0;
        while (currentOffset < totalSize)
        {
            u64 currentChunkSize = min(totalSize - currentOffset, loop->committed);
            
            // Unmap each mapped section of virtual address space
            UnmapViewOfFile(start + currentChunkSize);
            
            currentOffset += currentChunkSize;
        }
        
        // Close the file mapping, releasing the anonymous memory
        CloseHandle((HANDLE)loop->memory);
        
        loop->memory = nullptr;
        loop->start = nullptr;
        loop->committed = 0;
        loop->mirrored = 0;
    }
    
#endif
    
    //~ Memory loop general implementation
    
    bool LoopIsInitialized(const MemoryLoop* loop)
    {
        return loop->start != nullptr;
    }
    
#endif // TOOL_MIRRORED_MEMORY
    
    
    
    //- Arena
    //
    // Regular chunks form one list in allocation order: 'first' up to 'current' hold allocations, the rest
    // are spares with nothing in them. An allocation too large for a regular chunk gets a dedicated chunk,
    // kept on its own list so the regular chunk it interrupted keeps packing. Dedicated chunks are stamped
    // from a counter that pushes also record, which is how a pop tells which ones came after its push.

    //~ Arena static helpers

    // Payload offset within a chunk. The header is padded so the payload starts on a pointer-aligned boundary;
    // allocations align themselves by address, so the source's own alignment does not matter.
    static const u64 arenaChunkHeaderSize = (sizeof(ArenaChunk) + 15ull) & ~15ull;

    static u8* ArenaChunkPayload(ArenaChunk* chunk)
    {
        return (u8*)chunk + arenaChunkHeaderSize;
    }

    static u64 ArenaPadding(const u8* address, u64 alignment)
    {
        u64 misalignment = (u64)address & (alignment - 1);
        return (misalignment == 0) ? 0 : alignment - misalignment;
    }

    static void ArenaPoison(u8* start, u64 size)
    {
#ifdef TOOL_DEBUG_ASSERTS
        memset(start, TOOL_ARENA_POISON, size);
#else
        (void)start;
        (void)size;
#endif
    }

    static void ArenaCountUsed(Arena* arena, u64 size)
    {
        arena->used += size;

        if (arena->used > arena->peak)
        {
            arena->peak = arena->used;
        }
    }

    static ArenaChunk* ArenaChunkCreate(Arena* arena, u64 capacity)
    {
        ArenaChunk* chunk = (ArenaChunk*)AllocatorAlloc(arena->source, arenaChunkHeaderSize + capacity);

        if (chunk == nullptr)
        {
            TOOL_FAIL("Arena '%s' could not get a %llu byte chunk from its source.", ArenaName(arena), capacity);
            return nullptr;
        }

        *chunk = {};
        chunk->capacity = capacity;

        arena->chunkCount++;
        arena->owned += arenaChunkHeaderSize + capacity;

        return chunk;
    }

    static void ArenaChunkDestroy(Arena* arena, ArenaChunk* chunk)
    {
        arena->chunkCount--;
        arena->owned -= arenaChunkHeaderSize + chunk->capacity;

        AllocatorDealloc(arena->source, chunk);
    }

    static void ArenaChunkDestroyList(Arena* arena, ArenaChunk* chunk)
    {
        while (chunk != nullptr)
        {
            ArenaChunk* next = chunk->next;
            ArenaChunkDestroy(arena, chunk);
            chunk = next;
        }
    }

    // True when 'size' at 'alignment' fits a fresh regular chunk whatever the payload's own alignment.
    static b8 ArenaFitsRegular(const Arena* arena, u64 size, u64 alignment)
    {
        return size + alignment - 1 <= arena->chunkSize;
    }

    // Makes 'current' a regular chunk with room for 'size' at 'alignment', moving on to the next spare or a
    // new chunk if it has none. Returns false if the source is exhausted.
    static b8 ArenaMakeRoom(Arena* arena, u64 size, u64 alignment)
    {
        ArenaChunk* current = arena->current;

        if (current != nullptr)
        {
            u8* head = ArenaChunkPayload(current) + current->used;
            if (current->used + ArenaPadding(head, alignment) + size <= current->capacity)
            {
                return true;
            }
        }

        ArenaChunk* next = (current == nullptr) ? arena->first : current->next;

        if (next == nullptr)
        {
            next = ArenaChunkCreate(arena, arena->chunkSize);
            if (next == nullptr)
            {
                return false;
            }

            if (current == nullptr)
            {
                arena->first = next;
            }
            else
            {
                current->next = next;
            }
        }

        arena->current = next;

        return true;
    }

    // A dedicated chunk with room for 'size' at 'alignment', reusing a spare if one is large enough. Not yet
    // linked into the in-use list.
    static ArenaChunk* ArenaTakeDedicated(Arena* arena, u64 size, u64 alignment)
    {
        u64 capacity = size + alignment - 1;

        ArenaChunk** link = &arena->dedicatedSpare;
        while (*link != nullptr)
        {
            ArenaChunk* spare = *link;

            if (spare->capacity >= capacity)
            {
                *link = spare->next;
                spare->next = nullptr;
                spare->used = 0;
                return spare;
            }

            link = &spare->next;
        }

        return ArenaChunkCreate(arena, capacity);
    }

    // Links a dedicated chunk into the in-use list, stamped with the next sequence number.
    static void ArenaLinkDedicated(Arena* arena, ArenaChunk* chunk)
    {
        chunk->sequence = ++arena->sequence;

        chunk->next = arena->dedicated;
        arena->dedicated = chunk;

        ArenaCountUsed(arena, chunk->used);
    }

    static void ArenaSpareDedicated(Arena* arena, ArenaChunk* chunk)
    {
        ArenaPoison(ArenaChunkPayload(chunk), chunk->used);
        chunk->used = 0;

        chunk->next = arena->dedicatedSpare;
        arena->dedicatedSpare = chunk;
    }

    static void ArenaReleasePending(Arena* arena)
    {
        if (arena->pending != nullptr)
        {
            ArenaSpareDedicated(arena, arena->pending);
            arena->pending = nullptr;
        }
    }

    // Rewinds to a regular position: 'chunk' (null for the very start) with 'chunkUsed' bytes in it. Every
    // regular chunk after it becomes a spare, and every dedicated chunk stamped after 'sequence' is released.
    static void ArenaRewind(Arena* arena, ArenaChunk* chunk, u64 chunkUsed, u64 used, u64 sequence)
    {
        ArenaReleasePending(arena);

        // Dedicated chunks are newest first, so the ones to release are a prefix of the list
        while (arena->dedicated != nullptr && arena->dedicated->sequence > sequence)
        {
            ArenaChunk* dedicated = arena->dedicated;

            arena->dedicated = dedicated->next;
            ArenaSpareDedicated(arena, dedicated);
        }

        if (arena->current != nullptr)
        {
            ArenaChunk* restored = (chunk == nullptr) ? arena->first : chunk;

            // Every chunk after the restored one, up to the old current, is emptied
            if (restored != arena->current)
            {
                for (ArenaChunk* emptied = restored->next; emptied != nullptr; emptied = emptied->next)
                {
                    ArenaPoison(ArenaChunkPayload(emptied), emptied->used);
                    emptied->used = 0;

                    if (emptied == arena->current)
                    {
                        break;
                    }
                }
            }

            TOOL_ASSERT(chunkUsed <= restored->used, "Arena '%s' cannot rewind forwards.", ArenaName(arena));

            ArenaPoison(ArenaChunkPayload(restored) + chunkUsed, restored->used - chunkUsed);
            restored->used = chunkUsed;

            arena->current = restored;
        }

        arena->used = used;
    }

    //~ Arena implementation

    void ArenaInit(Arena* arena, MemoryAllocator source, u64 chunkSize, const c8* name)
    {
        TOOL_ASSERT(source.allocate != nullptr, "An Arena needs a source that can allocate.");
        TOOL_ASSERT(chunkSize > 0, "An Arena needs a chunk size above zero.");

        *arena = {};
        arena->source = source;
        arena->chunkSize = chunkSize;
        arena->name = name;
    }

    void ArenaInitChild(Arena* child, Arena* parent, u64 chunkSize, const c8* name)
    {
        TOOL_ASSERT(child != parent, "An Arena cannot be its own child.");

        ArenaInit(child, Allocator(parent), chunkSize, name);
    }

    void* ArenaAllocAligned(Arena* arena, u64 size, u64 alignment)
    {
        TOOL_ASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0,
                    "Arena alignment must be a power of two. (%llu)", alignment);

        if (!ArenaFitsRegular(arena, size, alignment))
        {
            ArenaReleasePending(arena);

            ArenaChunk* chunk = ArenaTakeDedicated(arena, size, alignment);
            if (chunk == nullptr)
            {
                return nullptr;
            }

            u8* payload = ArenaChunkPayload(chunk);
            u64 padding = ArenaPadding(payload, alignment);

            chunk->used = padding + size;
            ArenaLinkDedicated(arena, chunk);

            return payload + padding;
        }

        if (!ArenaMakeRoom(arena, size, alignment))
        {
            return nullptr;
        }

        ArenaChunk* current = arena->current;
        u8* head = ArenaChunkPayload(current) + current->used;
        u64 padding = ArenaPadding(head, alignment);

        current->used += padding + size;
        ArenaCountUsed(arena, padding + size);

        return head + padding;
    }

    void* ArenaAlloc(Arena* arena, u64 size)
    {
        return ArenaAllocAligned(arena, size, TOOL_ARENA_ALIGNMENT);
    }

    void* ArenaAlloc(Arena* arena, u64 count, u64 size)
    {
        return ArenaAllocAligned(arena, count * size, TOOL_ARENA_ALIGNMENT);
    }

    void* ArenaAllocBegin(Arena* arena, u64 reservedSize)
    {
        ArenaReleasePending(arena);

        if (!ArenaFitsRegular(arena, reservedSize, TOOL_ARENA_ALIGNMENT))
        {
            arena->pending = ArenaTakeDedicated(arena, reservedSize, TOOL_ARENA_ALIGNMENT);
            if (arena->pending == nullptr)
            {
                return nullptr;
            }

            u8* payload = ArenaChunkPayload(arena->pending);
            return payload + ArenaPadding(payload, TOOL_ARENA_ALIGNMENT);
        }

        if (!ArenaMakeRoom(arena, reservedSize, TOOL_ARENA_ALIGNMENT))
        {
            return nullptr;
        }

        u8* head = ArenaChunkPayload(arena->current) + arena->current->used;
        return head + ArenaPadding(head, TOOL_ARENA_ALIGNMENT);
    }

    void* ArenaAllocEnd(Arena* arena, u64 actualSize)
    {
        if (arena->pending != nullptr)
        {
            ArenaChunk* chunk = arena->pending;
            arena->pending = nullptr;

            u8* payload = ArenaChunkPayload(chunk);
            u64 padding = ArenaPadding(payload, TOOL_ARENA_ALIGNMENT);

            TOOL_ASSERT(padding + actualSize <= chunk->capacity,
                        "Arena '%s' allocation ended past its reservation. (%llu)", ArenaName(arena), actualSize);

            chunk->used = padding + actualSize;
            ArenaLinkDedicated(arena, chunk);

            return payload + padding;
        }

        ArenaChunk* current = arena->current;
        TOOL_ASSERT(current != nullptr, "Arena '%s' allocation ended without a matching begin.", ArenaName(arena));

        u8* head = ArenaChunkPayload(current) + current->used;
        u64 padding = ArenaPadding(head, TOOL_ARENA_ALIGNMENT);

        TOOL_ASSERT(current->used + padding + actualSize <= current->capacity,
                    "Arena '%s' allocation ended past its reservation. (%llu)", ArenaName(arena), actualSize);

        current->used += padding + actualSize;
        ArenaCountUsed(arena, padding + actualSize);

        return head + padding;
    }

    void ArenaPush(Arena* arena)
    {
        ArenaFrame mark = {};
        mark.previous = arena->frame;
        mark.chunk = arena->current;
        mark.chunkUsed = (arena->current == nullptr) ? 0 : arena->current->used;
        mark.used = arena->used;
        mark.sequence = arena->sequence;

        ArenaFrame* frame = ArenaAlloc<ArenaFrame>(arena);
        TOOL_ASSERT(frame != nullptr, "Could not push an Arena frame onto '%s': %s", ArenaName(arena), ErrorMessage());

        *frame = mark;
        arena->frame = frame;
    }

    void ArenaPop(Arena* arena)
    {
        TOOL_ASSERT(arena->frame != nullptr, "Arena '%s' popped without a matching push.", ArenaName(arena));

        // Read out before the rewind poisons the frame record itself
        ArenaFrame mark = *arena->frame;

        ArenaRewind(arena, mark.chunk, mark.chunkUsed, mark.used, mark.sequence);
        arena->frame = mark.previous;
    }

    void ArenaReset(Arena* arena)
    {
        ArenaRewind(arena, nullptr, 0, 0, 0);
        arena->frame = nullptr;
    }

    void ArenaTrim(Arena* arena, u32 keepSpareCount)
    {
        if (arena->source.deallocate == nullptr)
        {
            return;
        }

        ArenaChunkDestroyList(arena, arena->dedicatedSpare);
        arena->dedicatedSpare = nullptr;

        // Regular spares follow 'current', or make up the whole list if nothing was ever allocated
        ArenaChunk** link = (arena->current == nullptr) ? &arena->first : &arena->current->next;
        for (u32 kept = 0; *link != nullptr && kept < keepSpareCount; kept++)
        {
            link = &(*link)->next;
        }

        ArenaChunkDestroyList(arena, *link);
        *link = nullptr;
    }

    void ArenaDeInit(Arena* arena)
    {
        ArenaChunkDestroyList(arena, arena->first);
        ArenaChunkDestroyList(arena, arena->dedicated);
        ArenaChunkDestroyList(arena, arena->dedicatedSpare);
        ArenaChunkDestroyList(arena, arena->pending);

        *arena = {};
    }

    const c8* ArenaName(const Arena* arena)
    {
        return (arena->name == nullptr) ? "(unnamed)" : arena->name;
    }



#ifdef TOOL_VIRTUAL_MEMORY
    
    //- Contiguous arena
    
    //~ Contiguous arena general implementation
    
    b8 ContiguousArenaInit(ContiguousArena* arena, u64 reservedSize)
    {
        if (!RegionReserve(&arena->region, reservedSize))
        {
            return false;
        }
        
        if (!RegionCommit(&arena->region, TOOL_CONTIGUOUS_ARENA_COMMIT_SIZE))
        {
            RegionDealloc(&arena->region);
            return false;
        }
        
        arena->size = 0;
        
        arena->startCurrent = arena->region.start;
        arena->sizeCurrent = 0;
        
        return true;
    }
    
    void* ContiguousArenaAllocBegin(ContiguousArena* arena, u64 reservedSize)
    {
        u64 newSize = arena->size + reservedSize;
        
        if (newSize > arena->region.committed)
        {
            if (newSize > arena->region.reserved)
            {
                TOOL_FAIL("Cannot allocate more memory than is reserved in the Contiguous Arena. (%llu + %llu > %llu)",
                          arena->size, reservedSize, arena->region.reserved);
                return nullptr;
            }
            
            u64 newCommittedSize = arena->region.committed;
            while (newSize > newCommittedSize)
            {
                newCommittedSize += TOOL_MIN(newCommittedSize, TOOL_CONTIGUOUS_ARENA_MAX_INCREMENT_SIZE);
            }
            
            if (!RegionCommit(&arena->region, newCommittedSize))
            {
                return nullptr;
            }
        }
        
        void* result = (u8*)arena->startCurrent + arena->sizeCurrent;
        
        return result;
    }
    
    void* ContiguousArenaAllocEnd(ContiguousArena* arena, u64 actualSize)
    {
        void* head = (u8*)arena->startCurrent + arena->sizeCurrent;
        
        arena->sizeCurrent += actualSize;
        arena->size += actualSize;
        
        return head;
    }
    
    void* ContiguousArenaAlloc(ContiguousArena* arena, u64 size)
    {
        if (ContiguousArenaAllocBegin(arena, size) == nullptr)
        {
            return nullptr;
        }
        
        return ContiguousArenaAllocEnd(arena, size);
    }
    
    void ContiguousArenaPush(ContiguousArena* arena)
    {
        ContiguousArenaFrame frame = { arena->startCurrent, arena->sizeCurrent };
        
        arena->startCurrent = (u8*)arena->startCurrent + arena->sizeCurrent;
        arena->sizeCurrent = 0;
        
        ContiguousArenaFrame* destination = (ContiguousArenaFrame*)ContiguousArenaAlloc(arena, sizeof(ContiguousArenaFrame));
        TOOL_ASSERT(destination != nullptr, "Could not push a Contiguous Arena frame: %s", ErrorMessage());
        *destination = frame;
    }
    
    void ContiguousArenaPop(ContiguousArena* arena)
    {
        ContiguousArenaFrame* frame = (ContiguousArenaFrame*)arena->startCurrent;
        
        arena->size -= arena->sizeCurrent;
        arena->startCurrent = frame->start;
        arena->sizeCurrent = frame->size;
    }
    
    void ContiguousArenaDeInit(ContiguousArena* arena)
    {
        RegionDealloc(&arena->region);
        
        arena->size = 0;
        arena->startCurrent = nullptr;
        arena->sizeCurrent = 0;
    }
    
    void* ContiguousArenaAlloc(ContiguousArena* arena, u64 count, u64 size)
    {
        return ContiguousArenaAlloc(arena, count * size);
    }
    
#endif // TOOL_VIRTUAL_MEMORY
    
    
    
#ifdef TOOL_MIRRORED_MEMORY
    
    //- Magic circular buffer
    
    //~ Magic circular buffer general implementation
    
    // Initialize and allocate a new circular buffer. The actual size and overflow region might be larger than requested.
    b8 MagicCircularInit(MagicCircular* circular, u64 requestedSize, u64 requestedOverflowSize)
    {
        if (!LoopAlloc(&circular->loop, requestedSize, requestedOverflowSize))
        {
            return false;
        }
        
        circular->start = 0;
        circular->size = 0;
        
        return true;
    }
    
    // Allocate space within the circular buffer. Nullptr indicates insufficient space.
    void* MagicCircularAlloc(MagicCircular* circular, u64 size)
    {
        if (MagicCircularAllocBegin(circular, size) == nullptr)
        {
            return nullptr;
        }
        
        return MagicCircularAllocEnd(circular, size);
    }
    
    void* MagicCircularAlloc(MagicCircular* circular, u64 count, u64 size)
    {
        return MagicCircularAlloc(circular, count * size);
    }
    
    // Allocates space in two steps, similarly to the same Arena feature.
    void* MagicCircularAllocBegin(MagicCircular* circular, u64 reservedSize)
    {
        TOOL_ASSERT(LoopIsInitialized(&circular->loop), "Cannot allocate onto a non-initialized Circular Allocator.");
        
        u64 requestedSize = circular->size + reservedSize;
        u64 capacity = circular->loop.committed;
        
        if (requestedSize > capacity)
        {
            return nullptr;
        }
        
        TOOL_ASSERT(circular->start + reservedSize <= circular->loop.committed + circular->loop.mirrored,
                    "Cannot allocate onto Circular Allocator, "
                    "requested size does not fit into the circular buffer as a continuous region. "
                    "Consider requesting higher maximum allocation size. "
                    "(Maximum size: %llu, allocation size: %llu)",
                    circular->loop.mirrored, reservedSize);
        
        // Just acquire the current circular buffer end point. Due to the looped memory mapping, reads/writes will wrap.
        u64 head = (circular->start + circular->size) % capacity;
        void* allocationLocation = ((char*)circular->loop.start) + head;
        
        return allocationLocation;
    }
    
    void* MagicCircularAllocEnd(MagicCircular* circular, u64 actualSize)
    {
        u64 capacity = circular->loop.committed;
        
        // Just acquire the current circular buffer end point. Due to the looped memory mapping, reads/writes will wrap.
        u64 head = (circular->start + circular->size) % capacity;
        void* allocationLocation = (char*)circular->loop.start + head;
        
        circular->size += actualSize;
        
        return allocationLocation;
    }
    
    // Get a reference to the current writing location (bookmark).
    // Can be used to then get a data pointer, or deallocate everything prior to the bookmark.
    u64 MagicCircularGetBookmark(const MagicCircular* circular)
    {
        u64 capacity = circular->loop.committed;
        return (circular->start + circular->size) % capacity;
    }
    
    void* MagicCircularGetDataAt(MagicCircular* circular, u64 bookmark)
    {
        return (char*)circular->loop.start + bookmark;
    }
    
    void MagicCircularPopToBookmark(MagicCircular* circular, u64 bookmark)
    {
        TOOL_ASSERT(LoopIsInitialized(&circular->loop), "Cannot pop a non-initialized Circular Allocator to bookmark.");
        
        u64 capacity = circular->loop.committed;
        
        // The offset of the bookmark from the start
        u64 offset = (bookmark + capacity * (bookmark < circular->start) - circular->start);
        
        TOOL_ASSERT(offset <= circular->size, "Bookmark is invalid (outside the allocated region).");
        
        circular->size -= offset;
        circular->start = bookmark;
    }
    
    // Deinitialize the circular buffer.
    void MagicCircularDeInit(MagicCircular* circular)
    {
        LoopDealloc(&circular->loop);
        circular->start = 0;
        circular->size = 0;
    }
    
#endif // TOOL_MIRRORED_MEMORY
    
    
    
    //- Allocators
    
    //~ Allocator triggers
    
    static void* AllocationTriggerClassic(u64 size, void* data)
    {
        return ClassicAlloc(size);
    }
    
    static void DeallocationTriggerClassic(void* target, void* data)
    {
        ClassicDealloc(target);
    }
    
    static void* AllocationTriggerArena(u64 size, void* data)
    {
        return ArenaAlloc((Arena*)data, size);
    }
    
#ifdef TOOL_VIRTUAL_MEMORY
    static void* AllocationTriggerContiguousArena(u64 size, void* data)
    {
        return ContiguousArenaAlloc((ContiguousArena*)data, size);
    }
#endif
    
#ifdef TOOL_MIRRORED_MEMORY
    static void* AllocationTriggerMagicCircular(u64 size, void* data)
    {
        return MagicCircularAlloc((MagicCircular*)data, size);
    }
#endif
    
    //~ Exposed allocator functions
    
    MemoryAllocator Allocator() // Heap
    {
        return { &AllocationTriggerClassic, &DeallocationTriggerClassic, nullptr };
    }
    
    MemoryAllocator Allocator(Arena* arena) // Arena
    {
        return { &AllocationTriggerArena, nullptr, (void*)arena };
    }
    
#ifdef TOOL_VIRTUAL_MEMORY
    MemoryAllocator Allocator(ContiguousArena* arena) // Contiguous arena
    {
        return { &AllocationTriggerContiguousArena, nullptr, (void*)arena };
    }
#endif
    
#ifdef TOOL_MIRRORED_MEMORY
    MemoryAllocator Allocator(MagicCircular* circular) // Magic circular buffer
    {
        return { &AllocationTriggerMagicCircular, nullptr, (void*)circular };
    }
#endif
    
    void* AllocatorAlloc(MemoryAllocator allocator, u64 size)
    {
        return allocator.allocate(size, allocator.data);
    }
    
    void* AllocatorAlloc(MemoryAllocator allocator, u64 count, u64 size)
    {
        return AllocatorAlloc(allocator, count * size);
    }
    
    void AllocatorDealloc(MemoryAllocator allocator, void* target)
    {
        if (allocator.deallocate != nullptr)
        {
            return allocator.deallocate(target, allocator.data);
        }
    }
}

