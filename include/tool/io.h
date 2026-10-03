#ifndef _TOOL_IO_H
#define _TOOL_IO_H

#include "basics.h"
#include "memory.h"
#include "temporal.h"



//~ File IO
//
// Paths are UTF-8 with '/' separators on every platform, and relative ones start from the working directory. On
// Windows, '\' works too, and long paths are handled internally. Functions that can fail return false and record
// why in the thread's error record (error.h).
//
// On the web, paths lead into Emscripten's virtual file system: preloaded files are there before main runs, and
// files written elsewhere last until the page closes.



namespace Tool
{
	//- Types

	// An open file. 0 for none.
	typedef u64 File;

	// The position starts at 0, or at the end of the file for the Append modes. Every mode but Read can also read.
	enum OpenMode
	{
		OpenModeRead,              // Read only. Fails if the file doesn't exist
		OpenModeNew,               // Fails if the file exists
		OpenModeNewOrAppend,       // Creates the file, or opens it at the end if it exists
		OpenModeNewOrOverwrite,    // Creates the file, or empties it if it exists
		OpenModeAppendExisting,    // Fails if the file doesn't exist. Opens it at the end
		OpenModeOverwriteExisting, // Fails if the file doesn't exist. Empties it
		OpenModeEditExisting,      // Fails if the file doesn't exist. Keeps its contents
	};

	enum OpenFlags
	{
		OpenFlagsUnbuffered = 1 << 0, // Skips the system's cache. Reads and writes must then be aligned to sectors. Ignored on the web
		OpenFlagsShareRead = 1 << 2,  // Lets others open the file to read while it's open. Only Windows locks files
		OpenFlagsShareWrite = 1 << 3, // Lets others open the file to write while it's open. Only Windows locks files
	};

	struct alignas(8) PathInfo
	{
		u64 size;                 // Bytes. 0 for directories
		SystemTimepoint modified; // Last write, UTC
		b8 directory;
	};

	// Lists a directory's entries, one per DirectoryNext, without '.' and '..', in no particular order.
	struct alignas(8) DirectoryIterator
	{
		u64 handle; // Internal
		u32 state;  // Internal

		// The current entry
		PathInfo info;
		c8 name[1024]; // The entry's name alone, UTF-8
	};

	static_assert(sizeof(PathInfo) == 24 && alignof(PathInfo) == 8, "PathInfo layout");
	static_assert(sizeof(DirectoryIterator) == 1064 && alignof(DirectoryIterator) == 8, "DirectoryIterator layout");



	//- Functions

	//~ Open files

	b8 FileOpen(File* outFile, const c8* path, OpenMode mode, u32 flags = 0);
	void FileClose(File file);

	u64 FileSize(File file);
	u64 FilePosition(File file);
	b8 FileSeek(File file, u64 position); // From the start. Seek to FileSize for the end

	// Reads until 'size' bytes or the end of the file. 'outRead' receives the bytes read, short only at the end.
	b8 FileRead(File file, void* destination, u64 size, u64* outRead = nullptr);
	b8 FileWrite(File file, const void* source, u64 size); // Writes all of it, or fails
	b8 FileFlush(File file);                                // Waits until what's written is on the disk

	//~ Whole files

	// Reads a file into one allocation from 'allocator', with a zero byte after the contents for use as text.
	b8 FileReadAll(const c8* path, void** outData, u64* outSize, MemoryAllocator allocator);

	// Writes a file in full. It goes to a temporary file first, which then replaces the file, so a crash or power
	// loss leaves either the old contents or the new, never part of either.
	b8 FileWriteAll(const c8* path, const void* data, u64 size);

	//~ Paths

	b8 PathInfoGet(const c8* path, PathInfo* outInfo); // False if nothing is there
	b8 FileExists(const c8* path);                     // A file, not a directory
	b8 DirectoryExists(const c8* path);

	b8 FileDelete(const c8* path);
	b8 FileMove(const c8* from, const c8* to); // Files or directories, on the same drive. Replaces a file at 'to' in one step

	b8 DirectoryCreate(const c8* path); // Creates missing parents too. True if it already exists
	b8 DirectoryDelete(const c8* path); // Empty directories only

	// The directory holding the running executable, without a trailing '/'. "/" on the web.
	b8 PathExecutableDirectory(c8* outPath, u32 capacity);

	//~ Directory listing
	// DirectoryIterator iterator;
	// if (DirectoryOpen(&iterator, "assets"))
	// {
	//     while (DirectoryNext(&iterator))
	//         Use(iterator.name, iterator.info);
	//     DirectoryClose(&iterator);
	// }

	b8 DirectoryOpen(DirectoryIterator* iterator, const c8* path);
	b8 DirectoryNext(DirectoryIterator* iterator); // Steps to the next entry. False when there are no more
	void DirectoryClose(DirectoryIterator* iterator);
} //namespace Tool

#endif //_TOOL_IO_H
