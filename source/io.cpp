#include "io.h"
#include "error.h"
#include "mathematics.h"



//~ Windows
#if TOOL_WINDOWS

#include <Windows.h>

//~ Unix
#elif TOOL_UNIX

#define _FILE_OFFSET_BITS 64 // 64-bit sizes and offsets on 32-bit Linux too

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if TOOL_LINUX
#include <poll.h>
#include <sys/inotify.h>
#endif

#endif



namespace Tool
{
    //- Platform-agnostic helpers

    // Longest path handled, terminator included: in bytes, and on Windows in UTF-16 units once converted. Paths
    // convert in stack buffers of this size, so it's kept well below the systems' own limits.
    static constexpr u32 PathCapacity = 1024;

    static b8 PathIsSeparator(c8 c)
    {
        return c == '/' || c == '\\';
    }

    static b8 PathCopy(c8* destination, const c8* path, const c8* suffix = "")
    {
        u32 length = 0;
        for (const c8* c = path; *c != 0; c++)
        {
            if (length + 1 >= PathCapacity)
                return TOOL_FAIL("Path too long: %.64s...", path);
            destination[length++] = *c;
        }

        for (const c8* c = suffix; *c != 0; c++)
        {
            if (length + 1 >= PathCapacity)
                return TOOL_FAIL("Path too long: %.64s...", path);
            destination[length++] = *c;
        }

        destination[length] = 0;
        return true;
    }

    static b8 NameIsDots(const c8* name)
    {
        return name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0));
    }

    // Copies a finished absolute path out, without a trailing '/' unless it is a root ("/", "C:/")
    static b8 PathAbsoluteOut(const c8* path, c8* outPath, u32 capacity)
    {
        u32 length = 0;
        while (path[length] != 0)
            length++;

        b8 driveRoot = length == 3 && path[1] == ':';
        while (length > 1 && path[length - 1] == '/' && !driveRoot)
        {
            length--;
            driveRoot = length == 3 && path[1] == ':';
        }

        if (length + 1 > capacity)
            return TOOL_FAIL("The buffer is too small for the path.");

        for (u32 i = 0; i < length; i++)
            outPath[i] = path[i];
        outPath[length] = 0;
        return true;
    }

    // Creates one directory. True if it exists afterwards. Quiet leaves failures unrecorded.
    static b8 DirectoryMake(const c8* path, b8 quiet);

    // An index or offset that points at nothing.
    static constexpr u32 WatchNone = ~0u;

    // Bytes the system fills with changes in one go. Windows takes no more for network shares.
    static constexpr u32 WatchBufferSize = 64 * 1024;

    // What remains of 'timeout' milliseconds since 'start'.
    static u32 WatchRemaining(Timepoint start, u32 timeout)
    {
        i64 elapsed = NanosecondsSince(start) / 1000000;
        return elapsed >= (i64)timeout ? 0 : timeout - (u32)elapsed;
    }

    static void WatchOverflow(DirectoryChange* change)
    {
        change->type = DirectoryChangeTypeOverflow;
        change->directory = false;
        change->path[0] = 0;
    }



    //- Windows implementation

#if TOOL_WINDOWS

    // Windows' own form of a path: UTF-16 with '\' separators. A path too long for the classic limit becomes
    // absolute with the \\?\ prefix, which lifts the limit.
    struct WidePath
    {
        wchar_t text[PathCapacity];
    };

    static b8 WidePathConvert(WidePath* out, const c8* path, const wchar_t* suffix = nullptr)
    {
        i32 length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, out->text, PathCapacity);
        if (length == 0)
            return TOOL_FAIL_WINDOWS();
        length--; // The terminator

        for (i32 i = 0; i < length; i++)
            if (out->text[i] == L'/')
                out->text[i] = L'\\';

        if (suffix != nullptr)
        {
            while (length > 0 && out->text[length - 1] == L'\\')
                length--;

            for (const wchar_t* c = suffix; *c != 0; c++)
            {
                if (length + 1 >= (i32)PathCapacity)
                    return TOOL_FAIL("Path too long: %.64s...", path);
                out->text[length++] = *c;
            }
            out->text[length] = 0;
        }

        b8 prefixed = length >= 4 && out->text[0] == L'\\' && out->text[1] == L'\\' && out->text[2] == L'?' && out->text[3] == L'\\';
        if (prefixed)
            return true;

        // The limit applies once a relative path joins the working directory, so measure the full path. Directories
        // are limited to 12 characters less than files.
        WidePath full;
        DWORD fullLength = GetFullPathNameW(out->text, PathCapacity, full.text, nullptr);
        if (fullLength == 0)
            return TOOL_FAIL_WINDOWS();
        if (fullLength < MAX_PATH - 12)
            return true;

        // Network paths, \\server\share, take the \\?\UNC\ form
        b8 network = full.text[0] == L'\\' && full.text[1] == L'\\';
        const wchar_t* prefix = network ? L"\\\\?\\UNC" : L"\\\\?\\";
        const wchar_t* rest = network ? full.text + 1 : full.text;
        u32 prefixLength = network ? 7 : 4;

        if (fullLength >= PathCapacity || fullLength + prefixLength >= PathCapacity)
            return TOOL_FAIL("Path too long: %.64s...", path);

        u32 at = 0;
        for (const wchar_t* c = prefix; *c != 0; c++)
            out->text[at++] = *c;
        for (const wchar_t* c = rest; *c != 0; c++)
            out->text[at++] = *c;
        out->text[at] = 0;
        return true;
    }

    static HANDLE FileHandle(File file)
    {
        return (HANDLE)(UINT_PTR)file;
    }

    static SystemTimepoint TimepointFromFileTime(FILETIME time)
    {
        return ((u64)time.dwHighDateTime << 32) | time.dwLowDateTime;
    }

    //~ Open files

    b8 FileOpen(File* outFile, const c8* path, OpenMode mode, u32 flags)
    {
        *outFile = 0;

        WidePath wide;
        if (!WidePathConvert(&wide, path))
            return false;

        DWORD disposition = OPEN_EXISTING;
        b8 append = false;
        switch (mode)
        {
            case OpenModeRead:              disposition = OPEN_EXISTING; break;
            case OpenModeNew:               disposition = CREATE_NEW; break;
            case OpenModeNewOrAppend:       disposition = OPEN_ALWAYS; append = true; break;
            case OpenModeNewOrOverwrite:    disposition = CREATE_ALWAYS; break;
            case OpenModeAppendExisting:    disposition = OPEN_EXISTING; append = true; break;
            case OpenModeOverwriteExisting: disposition = TRUNCATE_EXISTING; break;
            case OpenModeEditExisting:      disposition = OPEN_EXISTING; break;
        }

        DWORD access = GENERIC_READ | (mode != OpenModeRead ? GENERIC_WRITE : 0);
        DWORD share = ((flags & OpenFlagsShareRead) ? FILE_SHARE_READ : 0) | ((flags & OpenFlagsShareWrite) ? FILE_SHARE_WRITE : 0);
        DWORD attributes = FILE_ATTRIBUTE_NORMAL | ((flags & OpenFlagsUnbuffered) ? FILE_FLAG_NO_BUFFERING : 0);

        HANDLE handle = CreateFileW(wide.text, access, share, nullptr, disposition, attributes, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
            return TOOL_FAIL_WINDOWS();

        if (append)
        {
            LARGE_INTEGER zero = {};
            if (!SetFilePointerEx(handle, zero, nullptr, FILE_END))
            {
                DWORD error = GetLastError();
                CloseHandle(handle);
                return TOOL_FAIL_WINDOWS_CODE(error);
            }
        }

        *outFile = (File)(UINT_PTR)handle;
        return true;
    }

    void FileClose(File file)
    {
        if (file != 0)
            CloseHandle(FileHandle(file));
    }

    u64 FileSize(File file)
    {
        LARGE_INTEGER size = {};
        if (!GetFileSizeEx(FileHandle(file), &size))
        {
            TOOL_FAIL_WINDOWS();
            return 0;
        }

        return (u64)size.QuadPart;
    }

    u64 FilePosition(File file)
    {
        LARGE_INTEGER zero = {};
        LARGE_INTEGER position = {};
        if (!SetFilePointerEx(FileHandle(file), zero, &position, FILE_CURRENT))
        {
            TOOL_FAIL_WINDOWS();
            return 0;
        }

        return (u64)position.QuadPart;
    }

    b8 FileSeek(File file, u64 position)
    {
        LARGE_INTEGER target = {};
        target.QuadPart = (LONGLONG)position;
        if (!SetFilePointerEx(FileHandle(file), target, nullptr, FILE_BEGIN))
            return TOOL_FAIL_WINDOWS();

        return true;
    }

    // Each call moves at most this much, as the system takes 32-bit sizes.
    static constexpr u64 TransferChunk = 1ull << 30;

    b8 FileRead(File file, void* destination, u64 size, u64* outRead)
    {
        u64 total = 0;
        b8 success = true;

        while (total < size)
        {
            DWORD read = 0;
            if (!ReadFile(FileHandle(file), (u8*)destination + total, (DWORD)U64Min(size - total, TransferChunk), &read, nullptr))
            {
                success = TOOL_FAIL_WINDOWS();
                break;
            }

            if (read == 0)
                break; // The end of the file

            total += read;
        }

        if (outRead != nullptr)
            *outRead = total;

        return success;
    }

    b8 FileWrite(File file, const void* source, u64 size)
    {
        u64 total = 0;
        while (total < size)
        {
            DWORD written = 0;
            if (!WriteFile(FileHandle(file), (const u8*)source + total, (DWORD)U64Min(size - total, TransferChunk), &written, nullptr))
                return TOOL_FAIL_WINDOWS();

            if (written == 0)
                return TOOL_FAIL("Writing to the file made no progress.");

            total += written;
        }

        return true;
    }

    b8 FileFlush(File file)
    {
        if (!FlushFileBuffers(FileHandle(file)))
            return TOOL_FAIL_WINDOWS();

        return true;
    }

    //~ Paths

    b8 PathInfoGet(const c8* path, PathInfo* outInfo)
    {
        *outInfo = {};

        WidePath wide;
        if (!WidePathConvert(&wide, path))
            return false;

        WIN32_FILE_ATTRIBUTE_DATA data = {};
        if (!GetFileAttributesExW(wide.text, GetFileExInfoStandard, &data))
        {
            DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND || error == ERROR_INVALID_NAME)
                return false;

            return TOOL_FAIL_WINDOWS_CODE(error);
        }

        outInfo->directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        outInfo->size = outInfo->directory ? 0 : ((u64)data.nFileSizeHigh << 32) | data.nFileSizeLow;
        outInfo->modified = TimepointFromFileTime(data.ftLastWriteTime);
        return true;
    }

    b8 FileDelete(const c8* path)
    {
        WidePath wide;
        if (!WidePathConvert(&wide, path))
            return false;

        if (!DeleteFileW(wide.text))
            return TOOL_FAIL_WINDOWS();

        return true;
    }

    b8 FileMove(const c8* from, const c8* to)
    {
        WidePath wideFrom;
        WidePath wideTo;
        if (!WidePathConvert(&wideFrom, from) || !WidePathConvert(&wideTo, to))
            return false;

        if (!MoveFileExW(wideFrom.text, wideTo.text, MOVEFILE_REPLACE_EXISTING))
            return TOOL_FAIL_WINDOWS();

        return true;
    }

    static b8 DirectoryMake(const c8* path, b8 quiet)
    {
        WidePath wide;
        if (!WidePathConvert(&wide, path))
            return false;

        if (CreateDirectoryW(wide.text, nullptr))
            return true;

        DWORD error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS && DirectoryExists(path))
            return true;

        return quiet ? false : TOOL_FAIL_WINDOWS_CODE(error);
    }

    b8 DirectoryDelete(const c8* path)
    {
        WidePath wide;
        if (!WidePathConvert(&wide, path))
            return false;

        if (!RemoveDirectoryW(wide.text))
            return TOOL_FAIL_WINDOWS();

        return true;
    }

    b8 PathExecutableDirectory(c8* outPath, u32 capacity)
    {
        WidePath wide;
        DWORD length = GetModuleFileNameW(nullptr, wide.text, PathCapacity);
        if (length == 0 || length >= PathCapacity)
            return TOOL_FAIL_WINDOWS();

        // Drop the \\?\ prefix, which the executable's path can carry
        const wchar_t* start = wide.text;
        if (length >= 4 && start[0] == L'\\' && start[1] == L'\\' && start[2] == L'?' && start[3] == L'\\')
            start += 4;

        if (WideCharToMultiByte(CP_UTF8, 0, start, -1, outPath, (i32)capacity, nullptr, nullptr) == 0)
            return TOOL_FAIL_WINDOWS();

        c8* lastSeparator = nullptr;
        for (c8* c = outPath; *c != 0; c++)
        {
            if (*c == '\\')
                *c = '/';
            if (*c == '/')
                lastSeparator = c;
        }

        if (lastSeparator != nullptr)
            *lastSeparator = 0;

        return true;
    }

    b8 PathAbsolute(const c8* path, c8* outPath, u32 capacity)
    {
        WidePath wide;
        if (!WidePathConvert(&wide, path))
            return false;

        WidePath full;
        DWORD length = GetFullPathNameW(wide.text, PathCapacity, full.text, nullptr);
        if (length == 0 || length >= PathCapacity)
            return TOOL_FAIL_WINDOWS();

        // Without the prefix a long path takes, and a network path's prefixed form back to its usual one
        const wchar_t* start = full.text;
        b8 prefixed = length >= 4 && start[0] == L'\\' && start[1] == L'\\' && start[2] == L'?' && start[3] == L'\\';
        b8 network = prefixed && length >= 8 && start[4] == L'U' && start[5] == L'N' && start[6] == L'C' && start[7] == L'\\';
        if (network)
        {
            start += 6;
            full.text[6] = L'\\';
        }
        else if (prefixed)
        {
            start += 4;
        }

        c8 narrow[PathCapacity];
        if (WideCharToMultiByte(CP_UTF8, 0, start, -1, narrow, sizeof(narrow), nullptr, nullptr) == 0)
            return TOOL_FAIL_WINDOWS();

        for (c8* c = narrow; *c != 0; c++)
            if (*c == '\\')
                *c = '/';

        return PathAbsoluteOut(narrow, outPath, capacity);
    }

    //~ Directory listing

    // The first entry comes with opening, so it waits for the first DirectoryNext.
    static constexpr u32 DirectoryStateReady = 0;
    static constexpr u32 DirectoryStatePending = 1;

    static void DirectoryFill(DirectoryIterator* iterator, const WIN32_FIND_DATAW* data)
    {
        if (WideCharToMultiByte(CP_UTF8, 0, data->cFileName, -1, iterator->name, sizeof(iterator->name), nullptr, nullptr) == 0)
            iterator->name[0] = 0;

        iterator->info.directory = (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        iterator->info.size = iterator->info.directory ? 0 : ((u64)data->nFileSizeHigh << 32) | data->nFileSizeLow;
        iterator->info.modified = TimepointFromFileTime(data->ftLastWriteTime);
    }

    b8 DirectoryOpen(DirectoryIterator* iterator, const c8* path)
    {
        *iterator = {};

        WidePath wide;
        if (!WidePathConvert(&wide, path, L"\\*"))
            return false;

        WIN32_FIND_DATAW data = {};
        HANDLE handle = FindFirstFileExW(wide.text, FindExInfoBasic, &data, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (handle == INVALID_HANDLE_VALUE)
        {
            // A drive's root can be empty, without even '.' and '..'
            DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND && DirectoryExists(path))
                return true;

            return TOOL_FAIL_WINDOWS_CODE(error);
        }

        iterator->handle = (u64)(UINT_PTR)handle;
        iterator->state = DirectoryStatePending;
        DirectoryFill(iterator, &data);
        return true;
    }

    b8 DirectoryNext(DirectoryIterator* iterator)
    {
        if (iterator->handle == 0)
            return false;

        while (true)
        {
            if (iterator->state == DirectoryStatePending)
            {
                iterator->state = DirectoryStateReady;
            }
            else
            {
                WIN32_FIND_DATAW data = {};
                if (!FindNextFileW((HANDLE)(UINT_PTR)iterator->handle, &data))
                {
                    DWORD error = GetLastError();
                    if (error != ERROR_NO_MORE_FILES)
                        TOOL_FAIL_WINDOWS_CODE(error);
                    return false;
                }

                DirectoryFill(iterator, &data);
            }

            if (!NameIsDots(iterator->name))
                return true;
        }
    }

    void DirectoryClose(DirectoryIterator* iterator)
    {
        if (iterator->handle != 0)
            FindClose((HANDLE)(UINT_PTR)iterator->handle);

        iterator->handle = 0;
    }

    //~ Watching directories

    // Two buffers take turns, so a read is always waiting on the system while the other buffer is handed out.
    struct WatchState
    {
        MemoryAllocator allocator;
        HANDLE directory;
        OVERLAPPED overlapped; // The read into buffers[reading]
        u8* buffers[2];
        u32 reading;
        u32 next;     // Offset of the next entry to hand out in the other buffer, or WatchNone
        b8 recursive;
        b8 extended;  // Entries carry attributes. Not every file system gives them
        b8 pending;   // A read is waiting on the system
        b8 renaming;  // A RenamedFrom was handed out, and its RenamedTo is the next entry
        b8 overflow;  // An Overflow is due
        b8 failed;    // Reading stopped, with the reason recorded. An Overflow is due, then nothing more
        b8 ended;
        WidePath root; // For the attributes of entries that don't carry them
    };

    // One entry of either kind the system writes.
    struct WatchEntry
    {
        DWORD action;
        b8 directory;
        b8 directoryKnown;
        const WCHAR* name;
        u32 length; // In UTF-16 units
        DWORD nextOffset;
    };

    static void WatchEntryAt(WatchState* state, u32 offset, WatchEntry* entry)
    {
        const u8* at = state->buffers[state->reading ^ 1] + offset;
        if (state->extended)
        {
            const FILE_NOTIFY_EXTENDED_INFORMATION* information = (const FILE_NOTIFY_EXTENDED_INFORMATION*)at;
            entry->action = information->Action;
            entry->directory = (information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            entry->directoryKnown = true;
            entry->name = information->FileName;
            entry->length = information->FileNameLength / sizeof(WCHAR);
            entry->nextOffset = information->NextEntryOffset;
        }
        else
        {
            const FILE_NOTIFY_INFORMATION* information = (const FILE_NOTIFY_INFORMATION*)at;
            entry->action = information->Action;
            entry->directory = false;
            entry->directoryKnown = false;
            entry->name = information->FileName;
            entry->length = information->FileNameLength / sizeof(WCHAR);
            entry->nextOffset = information->NextEntryOffset;
        }
    }

    // Starts a read into buffers[reading]. Returns the error, or 0.
    static DWORD WatchIssue(WatchState* state)
    {
        constexpr DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                 FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;

        HANDLE event = state->overlapped.hEvent;
        state->overlapped = {};
        state->overlapped.hEvent = event;
        ResetEvent(event);

        u8* buffer = state->buffers[state->reading];
        BOOL issued = state->extended
            ? ReadDirectoryChangesExW(state->directory, buffer, WatchBufferSize, state->recursive, filter, nullptr, &state->overlapped, nullptr, ReadDirectoryNotifyExtendedInformation)
            : ReadDirectoryChangesW(state->directory, buffer, WatchBufferSize, state->recursive, filter, nullptr, &state->overlapped, nullptr);

        state->pending = issued != 0;
        return issued ? 0 : GetLastError();
    }

    // Waits up to 'timeout' for the read to finish, then hands its buffer out and starts the next read. False if it
    // didn't finish in time.
    static b8 WatchCollect(WatchState* state, u32 timeout)
    {
        if (WaitForSingleObject(state->overlapped.hEvent, timeout) != WAIT_OBJECT_0)
            return false;

        DWORD bytes = 0;
        b8 done = GetOverlappedResult(state->directory, &state->overlapped, &bytes, FALSE) != 0;
        DWORD error = done ? 0 : GetLastError();
        if (!done && error == ERROR_IO_INCOMPLETE)
            return false;

        state->pending = false;
        if (!done && error != ERROR_NOTIFY_ENUM_DIR)
        {
            TOOL_FAIL_WINDOWS_CODE(error);
            state->failed = true;
            return true;
        }

        // More changed than the buffer holds: the system kept none of it
        if (bytes == 0)
            state->overflow = true;
        else
        {
            state->reading ^= 1;
            state->next = 0;
        }

        error = WatchIssue(state);
        if (error != 0)
        {
            TOOL_FAIL_WINDOWS_CODE(error);
            state->failed = true;
        }

        return true;
    }

    static b8 WatchPathConvert(DirectoryChange* change, const WatchEntry* entry)
    {
        i32 bytes = 0;
        if (entry->length > 0)
        {
            bytes = WideCharToMultiByte(CP_UTF8, 0, entry->name, (i32)entry->length, change->path, sizeof(change->path) - 1, nullptr, nullptr);
            if (bytes == 0)
                return false;
        }

        change->path[bytes] = 0;
        for (i32 i = 0; i < bytes; i++)
            if (change->path[i] == '\\')
                change->path[i] = '/';

        return true;
    }

    // Asks the file system, for entries that don't say. Paths gone by now aren't directories.
    static b8 WatchIsDirectory(WatchState* state, const WatchEntry* entry)
    {
        WidePath full = state->root;
        u32 length = 0;
        while (full.text[length] != 0)
            length++;

        if (length + 1 + entry->length + 1 > PathCapacity)
            return false;

        full.text[length++] = L'\\';
        for (u32 i = 0; i < entry->length; i++)
            full.text[length++] = entry->name[i];
        full.text[length] = 0;

        DWORD attributes = GetFileAttributesW(full.text);
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    // Whether the next entry, waiting for none, is the new name of a rename.
    static b8 WatchNextIsNewName(WatchState* state)
    {
        if (state->next == WatchNone && (state->failed || state->overflow || !WatchCollect(state, 0) || state->next == WatchNone))
            return false;

        WatchEntry entry = {};
        WatchEntryAt(state, state->next, &entry);
        return entry.action == FILE_ACTION_RENAMED_NEW_NAME;
    }

    // Turns the next entry into a change. False if it's one that isn't reported.
    static b8 WatchTake(WatchState* state, DirectoryChange* change)
    {
        WatchEntry entry = {};
        WatchEntryAt(state, state->next, &entry);
        state->next = entry.nextOffset != 0 ? state->next + entry.nextOffset : WatchNone;

        if (!WatchPathConvert(change, &entry))
        {
            state->overflow = true; // A path too long to report
            return false;
        }

        b8 gone = entry.action == FILE_ACTION_REMOVED || entry.action == FILE_ACTION_RENAMED_OLD_NAME;
        change->directory = entry.directoryKnown ? entry.directory : !gone && WatchIsDirectory(state, &entry);

        b8 renaming = state->renaming;
        state->renaming = false;
        switch (entry.action)
        {
            case FILE_ACTION_ADDED:
                change->type = DirectoryChangeTypeAdded;
                return true;

            case FILE_ACTION_REMOVED:
                change->type = DirectoryChangeTypeRemoved;
                return true;

            case FILE_ACTION_MODIFIED:
                change->type = DirectoryChangeTypeModified;
                return !change->directory;

            case FILE_ACTION_RENAMED_OLD_NAME:
                state->renaming = WatchNextIsNewName(state);
                change->type = state->renaming ? DirectoryChangeTypeRenamedFrom : DirectoryChangeTypeRemoved;
                return true;

            case FILE_ACTION_RENAMED_NEW_NAME:
                change->type = renaming ? DirectoryChangeTypeRenamedTo : DirectoryChangeTypeAdded;
                return true;
        }

        return false;
    }

    static void WatchFree(WatchState* state)
    {
        if (state->pending)
        {
            DWORD bytes = 0;
            CancelIoEx(state->directory, &state->overlapped);
            GetOverlappedResult(state->directory, &state->overlapped, &bytes, TRUE);
        }

        if (state->overlapped.hEvent != nullptr)
            CloseHandle(state->overlapped.hEvent);
        if (state->directory != INVALID_HANDLE_VALUE)
            CloseHandle(state->directory);

        AllocatorDealloc(state->allocator, state);
    }

    b8 DirectoryWatchOpen(DirectoryWatch* watch, const c8* path, b8 recursive, MemoryAllocator allocator)
    {
        *watch = {};

        if (!DirectoryExists(path))
            return TOOL_FAIL("No directory to watch at %s.", path);

        // The state, then both buffers, aligned for the entries
        u64 stateSize = (sizeof(WatchState) + 7) & ~7ull;
        u8* block = (u8*)AllocatorAlloc(allocator, stateSize + 2 * (u64)WatchBufferSize);
        if (block == nullptr)
            return TOOL_FAIL("Could not allocate a watch for %s.", path);

        WatchState* state = (WatchState*)block;
        *state = {};
        state->allocator = allocator;
        state->directory = INVALID_HANDLE_VALUE;
        state->buffers[0] = block + stateSize;
        state->buffers[1] = block + stateSize + WatchBufferSize;
        state->next = WatchNone;
        state->recursive = recursive;
        state->extended = true;

        if (!WidePathConvert(&state->root, path))
        {
            WatchFree(state);
            return false;
        }

        state->directory = CreateFileW(state->root.text, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                       OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
        state->overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (state->directory == INVALID_HANDLE_VALUE || state->overlapped.hEvent == nullptr)
        {
            DWORD error = GetLastError();
            WatchFree(state);
            return TOOL_FAIL_WINDOWS_CODE(error);
        }

        // File systems without extended entries, like network shares, take the plain kind
        DWORD error = WatchIssue(state);
        if (error != 0)
        {
            state->extended = false;
            error = WatchIssue(state);
        }

        if (error != 0)
        {
            WatchFree(state);
            return TOOL_FAIL_WINDOWS_CODE(error);
        }

        watch->handle = (u64)(UINT_PTR)state;
        return true;
    }

    b8 DirectoryWatchNext(DirectoryWatch* watch, DirectoryChange* outChange, u32 timeout)
    {
        WatchState* state = (WatchState*)(UINT_PTR)watch->handle;
        if (state == nullptr || state->ended)
            return false;

        Timepoint start = TimepointNow();
        while (true)
        {
            if (state->next != WatchNone)
            {
                if (WatchTake(state, outChange))
                    return true;
                continue;
            }

            if (state->overflow || state->failed)
            {
                state->ended = state->failed;
                state->overflow = false;
                WatchOverflow(outChange);
                return true;
            }

            if (!WatchCollect(state, WatchRemaining(start, timeout)))
                return false;
        }
    }

    void DirectoryWatchClose(DirectoryWatch* watch)
    {
        WatchState* state = (WatchState*)(UINT_PTR)watch->handle;
        if (state != nullptr)
            WatchFree(state);

        watch->handle = 0;
    }

#endif // TOOL_WINDOWS



    //- Unix implementation

#if TOOL_UNIX

    // File descriptors start at 0, so a File holds the descriptor plus one.
    static i32 FileDescriptor(File file)
    {
        return (i32)file - 1;
    }

    static SystemTimepoint TimepointFromStat(const struct stat* status)
    {
        constexpr i64 secondsFrom1601To1970 = 11644473600;
#if TOOL_APPLE
        timespec time = status->st_mtimespec;
#else
        timespec time = status->st_mtim;
#endif
        return (SystemTimepoint)((i64)time.tv_sec + secondsFrom1601To1970) * 10000000 + (SystemTimepoint)time.tv_nsec / 100;
    }

    static void PathInfoFromStat(PathInfo* info, const struct stat* status)
    {
        info->directory = S_ISDIR(status->st_mode);
        info->size = info->directory ? 0 : (u64)status->st_size;
        info->modified = TimepointFromStat(status);
    }

    //~ Open files

    b8 FileOpen(File* outFile, const c8* path, OpenMode mode, u32 flags)
    {
        *outFile = 0;

        i32 options = O_CLOEXEC;
        b8 append = false;
        switch (mode)
        {
            case OpenModeRead:              options |= O_RDONLY; break;
            case OpenModeNew:               options |= O_RDWR | O_CREAT | O_EXCL; break;
            case OpenModeNewOrAppend:       options |= O_RDWR | O_CREAT; append = true; break;
            case OpenModeNewOrOverwrite:    options |= O_RDWR | O_CREAT | O_TRUNC; break;
            case OpenModeAppendExisting:    options |= O_RDWR; append = true; break;
            case OpenModeOverwriteExisting: options |= O_RDWR | O_TRUNC; break;
            case OpenModeEditExisting:      options |= O_RDWR; break;
        }

#if defined(O_DIRECT)
        if (flags & OpenFlagsUnbuffered)
            options |= O_DIRECT;
#else
        (void)flags;
#endif

        i32 descriptor = -1;
        do
        {
            descriptor = open(path, options, 0666);
        }
        while (descriptor < 0 && errno == EINTR);

        if (descriptor < 0)
            return TOOL_FAIL_ERRNO();

        // Unix opens directories for reading, Windows doesn't. Match Windows.
        struct stat status = {};
        if (fstat(descriptor, &status) == 0 && S_ISDIR(status.st_mode))
        {
            close(descriptor);
            errno = EISDIR;
            return TOOL_FAIL_ERRNO();
        }

        if (append && lseek(descriptor, 0, SEEK_END) < 0)
        {
            i32 error = errno;
            close(descriptor);
            errno = error;
            return TOOL_FAIL_ERRNO();
        }

        *outFile = (File)descriptor + 1;
        return true;
    }

    void FileClose(File file)
    {
        if (file != 0)
            close(FileDescriptor(file));
    }

    u64 FileSize(File file)
    {
        struct stat status = {};
        if (fstat(FileDescriptor(file), &status) != 0)
        {
            TOOL_FAIL_ERRNO();
            return 0;
        }

        return (u64)status.st_size;
    }

    u64 FilePosition(File file)
    {
        off_t position = lseek(FileDescriptor(file), 0, SEEK_CUR);
        if (position < 0)
        {
            TOOL_FAIL_ERRNO();
            return 0;
        }

        return (u64)position;
    }

    b8 FileSeek(File file, u64 position)
    {
        if (lseek(FileDescriptor(file), (off_t)position, SEEK_SET) < 0)
            return TOOL_FAIL_ERRNO();

        return true;
    }

    // Each call moves at most this much, which every system accepts.
    static constexpr u64 TransferChunk = 1ull << 30;

    b8 FileRead(File file, void* destination, u64 size, u64* outRead)
    {
        u64 total = 0;
        b8 success = true;

        while (total < size)
        {
            ssize_t read = ::read(FileDescriptor(file), (u8*)destination + total, (size_t)U64Min(size - total, TransferChunk));
            if (read < 0)
            {
                if (errno == EINTR)
                    continue;

                success = TOOL_FAIL_ERRNO();
                break;
            }

            if (read == 0)
                break; // The end of the file

            total += (u64)read;
        }

        if (outRead != nullptr)
            *outRead = total;

        return success;
    }

    b8 FileWrite(File file, const void* source, u64 size)
    {
        u64 total = 0;
        while (total < size)
        {
            ssize_t written = ::write(FileDescriptor(file), (const u8*)source + total, (size_t)U64Min(size - total, TransferChunk));
            if (written < 0)
            {
                if (errno == EINTR)
                    continue;

                return TOOL_FAIL_ERRNO();
            }

            if (written == 0)
                return TOOL_FAIL("Writing to the file made no progress.");

            total += (u64)written;
        }

        return true;
    }

    b8 FileFlush(File file)
    {
        if (fsync(FileDescriptor(file)) != 0)
            return TOOL_FAIL_ERRNO();

        return true;
    }

    //~ Paths

    b8 PathInfoGet(const c8* path, PathInfo* outInfo)
    {
        *outInfo = {};

        struct stat status = {};
        if (stat(path, &status) != 0)
        {
            if (errno == ENOENT || errno == ENOTDIR)
                return false;

            return TOOL_FAIL_ERRNO();
        }

        PathInfoFromStat(outInfo, &status);
        return true;
    }

    b8 FileDelete(const c8* path)
    {
        if (unlink(path) != 0)
            return TOOL_FAIL_ERRNO();

        return true;
    }

    b8 FileMove(const c8* from, const c8* to)
    {
        if (rename(from, to) != 0)
            return TOOL_FAIL_ERRNO();

        return true;
    }

    static b8 DirectoryMake(const c8* path, b8 quiet)
    {
        if (mkdir(path, 0777) == 0)
            return true;

        i32 error = errno;
        if (error == EEXIST && DirectoryExists(path))
            return true;

        errno = error;
        return quiet ? false : TOOL_FAIL_ERRNO();
    }

    b8 DirectoryDelete(const c8* path)
    {
        if (rmdir(path) != 0)
            return TOOL_FAIL_ERRNO();

        return true;
    }

    b8 PathExecutableDirectory(c8* outPath, u32 capacity)
    {
#if TOOL_WEB
        if (capacity < 2)
            return TOOL_FAIL("The buffer is too small for the path.");

        outPath[0] = '/';
        outPath[1] = 0;
        return true;
#elif TOOL_LINUX
        ssize_t length = readlink("/proc/self/exe", outPath, capacity);
        if (length < 0)
            return TOOL_FAIL_ERRNO();
        if ((u64)length >= capacity)
            return TOOL_FAIL("The buffer is too small for the path.");

        outPath[length] = 0;
        c8* lastSeparator = strrchr(outPath, '/');
        if (lastSeparator == outPath)
            lastSeparator[1] = 0; // The root keeps its '/'
        else if (lastSeparator != nullptr)
            *lastSeparator = 0;

        return true;
#else
#error "No executable path for this platform"
#endif
    }

    b8 PathAbsolute(const c8* path, c8* outPath, u32 capacity)
    {
        c8 joined[PathCapacity];
        if (path[0] == '/')
        {
            if (!PathCopy(joined, path))
                return false;
        }
        else
        {
            if (getcwd(joined, sizeof(joined)) == nullptr)
                return TOOL_FAIL_ERRNO();

            u32 length = (u32)strlen(joined);
            if (length + 1 + strlen(path) + 1 > sizeof(joined))
                return TOOL_FAIL("Path too long: %.64s...", path);

            joined[length] = '/';
            strcpy(joined + length + 1, path);
        }

        // Each name after the root in turn, in place: what is written never passes what is read
        u32 write = 1;
        const c8* read = joined + 1;
        while (*read != 0)
        {
            const c8* name = read;
            while (*read != 0 && *read != '/')
                read++;
            u32 length = (u32)(read - name);
            if (*read == '/')
                read++;

            if (length == 0 || (length == 1 && name[0] == '.'))
                continue;

            if (length == 2 && name[0] == '.' && name[1] == '.')
            {
                while (write > 1 && joined[write - 1] != '/')
                    write--;
                if (write > 1)
                    write--;
                continue;
            }

            if (write > 1)
                joined[write++] = '/';
            memmove(joined + write, name, length);
            write += length;
        }
        joined[write] = 0;

        return PathAbsoluteOut(joined, outPath, capacity);
    }

    //~ Directory listing

    b8 DirectoryOpen(DirectoryIterator* iterator, const c8* path)
    {
        *iterator = {};

        DIR* directory = opendir(path);
        if (directory == nullptr)
            return TOOL_FAIL_ERRNO();

        iterator->handle = (u64)(uintptr_t)directory;
        return true;
    }

    b8 DirectoryNext(DirectoryIterator* iterator)
    {
        if (iterator->handle == 0)
            return false;

        DIR* directory = (DIR*)(uintptr_t)iterator->handle;
        while (true)
        {
            errno = 0;
            dirent* entry = readdir(directory);
            if (entry == nullptr)
            {
                if (errno != 0)
                    TOOL_FAIL_ERRNO();
                return false;
            }

            if (NameIsDots(entry->d_name))
                continue;

            u64 length = strlen(entry->d_name);
            if (length >= sizeof(iterator->name))
                length = sizeof(iterator->name) - 1;
            memcpy(iterator->name, entry->d_name, length);
            iterator->name[length] = 0;

            // Through links to what they point at, or the link itself if that's gone
            struct stat status = {};
            iterator->info = {};
            if (fstatat(dirfd(directory), entry->d_name, &status, 0) == 0 ||
                fstatat(dirfd(directory), entry->d_name, &status, AT_SYMLINK_NOFOLLOW) == 0)
                PathInfoFromStat(&iterator->info, &status);

            return true;
        }
    }

    void DirectoryClose(DirectoryIterator* iterator)
    {
        if (iterator->handle != 0)
            closedir((DIR*)(uintptr_t)iterator->handle);

        iterator->handle = 0;
    }

    //~ Watching directories

#if TOOL_LINUX

    static constexpr u32 WatchMask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_ONLYDIR | IN_EXCL_UNLINK;

    // A directory with an inotify watch on it. It holds its own name and its parent rather than a path, so a directory
    // that moves changes one record, and the paths below it follow.
    struct WatchDirectory
    {
        i32 descriptor; // -1 for a free record
        u32 parent;     // WatchNone for the watched directory itself
        c8 name[256];
    };

    struct WatchState
    {
        MemoryAllocator allocator;
        i32 inotify;
        b8 recursive;
        b8 overflow; // An Overflow is due
        b8 limited;  // The system refused a watch
        b8 failed;   // Reading stopped, with the reason recorded. An Overflow is due, then nothing more
        b8 ended;

        WatchDirectory* directories; // [0] is the watched directory
        u32 directoryCount;
        u32 directoryCapacity;

        // Changes found by looking into new directories, handed out ahead of the system's: a type byte, a directory
        // byte, a u16 length and the path
        u8* queue;
        u32 queueRead;
        u32 queueSize;
        u32 queueCapacity;

        u32 bufferRead;
        u32 bufferSize;
        alignas(inotify_event) u8 buffer[WatchBufferSize];

        c8 root[PathCapacity];
        c8 relative[PathCapacity]; // Paths being built
        c8 full[PathCapacity];
    };

    // Moves 'data' to room for 'needed' elements if it has less. Null if that can't be allocated, with 'data' kept.
    static void* WatchGrow(MemoryAllocator allocator, void* data, u32 used, u32* capacity, u32 needed, u32 elementSize)
    {
        if (needed <= *capacity)
            return data;

        u32 grown = *capacity != 0 ? *capacity : 16;
        while (grown < needed)
            grown *= 2;

        void* larger = AllocatorAlloc(allocator, (u64)grown * elementSize);
        if (larger == nullptr)
        {
            TOOL_FAIL("Could not allocate %llu bytes for a watch.", (unsigned long long)grown * elementSize);
            return nullptr;
        }

        if (data != nullptr)
        {
            memcpy(larger, data, (u64)used * elementSize);
            AllocatorDealloc(allocator, data);
        }

        *capacity = grown;
        return larger;
    }

    // Puts 'part' in front of the path built back to front in [*at, end), with a '/' between.
    static b8 WatchPrepend(c8* start, c8** at, c8* end, const c8* part)
    {
        u64 length = strlen(part);
        u64 separator = *at != end ? 1 : 0;
        if ((u64)(*at - start) < length + separator)
            return false;

        if (separator != 0)
            *--*at = '/';
        *at -= length;
        memcpy(*at, part, length);
        return true;
    }

    // The path of directory 'index' relative to the watched one, followed by 'name' if there is one. False if too long.
    static b8 WatchPath(WatchState* state, u32 index, const c8* name, c8* out)
    {
        c8* end = out + PathCapacity - 1;
        c8* at = end;
        *end = 0;

        if (name != nullptr && !WatchPrepend(out, &at, end, name))
            return false;

        for (u32 i = index; i != 0; i = state->directories[i].parent)
            if (!WatchPrepend(out, &at, end, state->directories[i].name))
                return false;

        memmove(out, at, (u64)(end - at) + 1);
        return true;
    }

    // Joins the watched directory and 'relative' into 'full', for the system.
    static b8 WatchFull(WatchState* state, const c8* relative)
    {
        u64 rootLength = strlen(state->root);
        u64 relativeLength = strlen(relative);
        u64 length = rootLength + (relativeLength != 0 ? 1 + relativeLength : 0);
        if (length >= PathCapacity)
            return false;

        memcpy(state->full, state->root, rootLength);
        if (relativeLength != 0)
        {
            state->full[rootLength] = '/';
            memcpy(state->full + rootLength + 1, relative, relativeLength);
        }

        state->full[length] = 0;
        return true;
    }

    static u32 WatchFind(WatchState* state, i32 descriptor)
    {
        for (u32 i = 0; i < state->directoryCount; i++)
            if (state->directories[i].descriptor == descriptor)
                return i;

        return WatchNone;
    }

    static u32 WatchFindChild(WatchState* state, u32 parent, const c8* name)
    {
        for (u32 i = 1; i < state->directoryCount; i++)
            if (state->directories[i].descriptor >= 0 && state->directories[i].parent == parent && strcmp(state->directories[i].name, name) == 0)
                return i;

        return WatchNone;
    }

    static b8 WatchQueue(WatchState* state, DirectoryChangeType type, b8 directory, const c8* path)
    {
        u32 length = (u32)strlen(path);
        void* queue = WatchGrow(state->allocator, state->queue, state->queueSize, &state->queueCapacity, state->queueSize + 4 + length, 1);
        if (queue == nullptr)
        {
            state->overflow = true;
            return false;
        }

        state->queue = (u8*)queue;
        u8* at = state->queue + state->queueSize;
        u16 length16 = (u16)length;
        at[0] = (u8)type;
        at[1] = directory ? 1 : 0;
        memcpy(at + 2, &length16, 2);
        memcpy(at + 4, path, length);
        state->queueSize += 4 + length;
        return true;
    }

    static void WatchUnqueue(WatchState* state, DirectoryChange* change)
    {
        const u8* at = state->queue + state->queueRead;
        u16 length = 0;
        memcpy(&length, at + 2, 2);

        change->type = (DirectoryChangeType)at[0];
        change->directory = at[1] != 0;
        memcpy(change->path, at + 4, length);
        change->path[length] = 0;

        state->queueRead += 4 + length;
        if (state->queueRead == state->queueSize)
            state->queueRead = state->queueSize = 0;
    }

    // Watches directory 'name' in directory 'parent'. A directory already watched has moved there, and its record
    // follows. 'outAdded' says whether it's new to the watch.
    static u32 WatchAdd(WatchState* state, u32 parent, const c8* name, b8* outAdded)
    {
        *outAdded = false;
        if (!WatchPath(state, parent, name, state->relative) || !WatchFull(state, state->relative))
        {
            state->overflow = true;
            return WatchNone;
        }

        i32 descriptor = inotify_add_watch(state->inotify, state->full, WatchMask | IN_DONT_FOLLOW);
        if (descriptor < 0)
        {
            // Gone again already, or replaced by a file, which its parent reports
            if (errno == ENOENT || errno == ENOTDIR)
                return WatchNone;

            if (errno == ENOSPC)
                TOOL_FAIL("Out of inotify watches, which fs.inotify.max_user_watches limits, at %s.", state->full);
            else
                TOOL_FAIL_ERRNO();

            state->limited = true;
            state->overflow = true;
            return WatchNone;
        }

        u32 index = WatchFind(state, descriptor);
        if (index == WatchNone)
        {
            index = WatchFind(state, -1);
            if (index == WatchNone)
            {
                void* directories = WatchGrow(state->allocator, state->directories, state->directoryCount, &state->directoryCapacity, state->directoryCount + 1, sizeof(WatchDirectory));
                if (directories == nullptr)
                {
                    inotify_rm_watch(state->inotify, descriptor);
                    state->limited = true;
                    state->overflow = true;
                    return WatchNone;
                }

                state->directories = (WatchDirectory*)directories;
                index = state->directoryCount++;
            }

            *outAdded = true;
        }

        WatchDirectory* directory = &state->directories[index];
        directory->descriptor = descriptor;
        directory->parent = parent;
        strncpy(directory->name, name, sizeof(directory->name) - 1);
        directory->name[sizeof(directory->name) - 1] = 0;
        return index;
    }

    // Watches the directories inside directory 'index', and theirs. With 'report', queues everything found as Added,
    // as it may have arrived before the watch could see it. With 'revisit', looks into directories already watched too.
    static void WatchDescend(WatchState* state, u32 index, b8 report, b8 revisit)
    {
        if (!WatchPath(state, index, nullptr, state->relative) || !WatchFull(state, state->relative))
        {
            state->overflow = true;
            return;
        }

        DIR* directory = opendir(state->full);
        if (directory == nullptr)
            return; // Gone again already

        while (dirent* entry = readdir(directory))
        {
            if (NameIsDots(entry->d_name))
                continue;

            b8 isDirectory = entry->d_type == DT_DIR;
            if (entry->d_type == DT_UNKNOWN)
            {
                struct stat status = {};
                isDirectory = fstatat(dirfd(directory), entry->d_name, &status, AT_SYMLINK_NOFOLLOW) == 0 && S_ISDIR(status.st_mode);
            }

            if (report)
            {
                if (WatchPath(state, index, entry->d_name, state->relative))
                    WatchQueue(state, DirectoryChangeTypeAdded, isDirectory, state->relative);
                else
                    state->overflow = true;
            }

            if (!isDirectory)
                continue;

            b8 added = false;
            u32 child = WatchAdd(state, index, entry->d_name, &added);
            if (child != WatchNone && (added || revisit))
                WatchDescend(state, child, report, revisit);

            if (state->limited)
                break;
        }

        closedir(directory);
    }

    // Watches a directory that appeared, and what's in it if it's new to the watch.
    static void WatchEnter(WatchState* state, u32 parent, const c8* name, b8 report)
    {
        b8 added = false;
        u32 child = WatchAdd(state, parent, name, &added);
        if (child != WatchNone && added)
            WatchDescend(state, child, report, false);
    }

    // Stops watching directory 'index' and those below it.
    static void WatchForget(WatchState* state, u32 index)
    {
        inotify_rm_watch(state->inotify, state->directories[index].descriptor);
        state->directories[index].descriptor = -1;

        for (u32 i = 1; i < state->directoryCount; i++)
            if (state->directories[i].descriptor >= 0 && state->directories[i].parent == index)
                WatchForget(state, i);
    }

    // Reads what the system has, waiting up to 'timeout' for it. False if nothing came.
    static b8 WatchFill(WatchState* state, u32 timeout)
    {
        pollfd poller = {state->inotify, POLLIN, 0};
        i32 ready = poll(&poller, 1, (i32)U32Min(timeout, 0x7FFFFFFF));
        if (ready < 0 && errno != EINTR)
        {
            TOOL_FAIL_ERRNO();
            state->failed = true;
        }

        if (ready <= 0)
            return false;

        ssize_t bytes = read(state->inotify, state->buffer, sizeof(state->buffer));
        if (bytes < 0 && errno != EAGAIN && errno != EINTR)
        {
            TOOL_FAIL_ERRNO();
            state->failed = true;
        }

        if (bytes <= 0)
            return false;

        state->bufferRead = 0;
        state->bufferSize = (u32)bytes;
        return true;
    }

    // An event moving 'change' away. Inside the watch, its other half comes next, if not read yet.
    static b8 WatchMoveFrom(WatchState* state, u32 index, const inotify_event* event, DirectoryChange* change)
    {
        u32 cookie = event->cookie;
        c8 name[256];
        strncpy(name, event->name, sizeof(name) - 1);
        name[sizeof(name) - 1] = 0;

        if (state->bufferRead == state->bufferSize)
            WatchFill(state, 0);

        const inotify_event* next = state->bufferRead < state->bufferSize ? (const inotify_event*)(state->buffer + state->bufferRead) : nullptr;
        if (next != nullptr && (next->mask & IN_MOVED_TO) != 0 && next->cookie == cookie)
        {
            state->bufferRead += sizeof(inotify_event) + next->len;
            u32 to = WatchFind(state, next->wd);
            if (to != WatchNone && WatchPath(state, to, next->name, state->relative) &&
                WatchQueue(state, DirectoryChangeTypeRenamedTo, change->directory, state->relative))
            {
                if (change->directory && state->recursive)
                    WatchEnter(state, to, next->name, false);

                change->type = DirectoryChangeTypeRenamedFrom;
                return true;
            }
        }

        change->type = DirectoryChangeTypeRemoved;
        if (change->directory && state->recursive)
        {
            u32 moved = WatchFindChild(state, index, name);
            if (moved != WatchNone)
                WatchForget(state, moved);
        }

        return true;
    }

    // Turns the next event into a change. False if it's one that isn't reported.
    static b8 WatchTake(WatchState* state, DirectoryChange* change)
    {
        const inotify_event* event = (const inotify_event*)(state->buffer + state->bufferRead);
        state->bufferRead += sizeof(inotify_event) + event->len;

        // The directories made meanwhile weren't seen, so look for those too
        if ((event->mask & IN_Q_OVERFLOW) != 0)
        {
            if (state->recursive)
                WatchDescend(state, 0, false, true);

            WatchOverflow(change);
            return true;
        }

        u32 index = WatchFind(state, event->wd);
        if (index == WatchNone)
            return false; // From a watch since let go

        if ((event->mask & IN_IGNORED) != 0)
        {
            if (index == 0)
            {
                TOOL_FAIL("The watched directory %s is gone.", state->root);
                state->failed = true;
            }
            else
                WatchForget(state, index);

            return false;
        }

        // What happens to a directory itself reaches its parent too, by name
        if (event->len == 0 || event->name[0] == 0)
            return false;

        change->directory = (event->mask & IN_ISDIR) != 0;
        if (!WatchPath(state, index, event->name, change->path))
        {
            state->overflow = true; // A path too long to report
            return false;
        }

        if ((event->mask & (IN_CREATE | IN_MOVED_TO)) != 0)
        {
            // Created ones can fill up before their watch is in place. Moved in ones come as a whole
            change->type = DirectoryChangeTypeAdded;
            if (change->directory && state->recursive)
                WatchEnter(state, index, event->name, (event->mask & IN_CREATE) != 0);
            return true;
        }

        if ((event->mask & IN_DELETE) != 0)
        {
            change->type = DirectoryChangeTypeRemoved;
            return true;
        }

        if ((event->mask & IN_MOVED_FROM) != 0)
            return WatchMoveFrom(state, index, event, change);

        change->type = DirectoryChangeTypeModified;
        return !change->directory;
    }

    static void WatchFree(WatchState* state)
    {
        if (state->inotify >= 0)
            close(state->inotify);
        if (state->directories != nullptr)
            AllocatorDealloc(state->allocator, state->directories);
        if (state->queue != nullptr)
            AllocatorDealloc(state->allocator, state->queue);

        AllocatorDealloc(state->allocator, state);
    }

    b8 DirectoryWatchOpen(DirectoryWatch* watch, const c8* path, b8 recursive, MemoryAllocator allocator)
    {
        *watch = {};

        WatchState* state = (WatchState*)AllocatorAlloc(allocator, sizeof(WatchState));
        if (state == nullptr)
            return TOOL_FAIL("Could not allocate a watch for %s.", path);

        memset(state, 0, sizeof(WatchState));
        state->allocator = allocator;
        state->inotify = -1;
        state->recursive = recursive;

        if (!PathCopy(state->root, path))
        {
            WatchFree(state);
            return false;
        }

        u64 length = strlen(state->root);
        while (length > 1 && state->root[length - 1] == '/')
            state->root[--length] = 0;

        state->inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        i32 descriptor = state->inotify >= 0 ? inotify_add_watch(state->inotify, state->root, WatchMask) : -1;
        if (descriptor < 0)
        {
            i32 error = errno;
            WatchFree(state);
            errno = error;
            return TOOL_FAIL_ERRNO();
        }

        void* directories = WatchGrow(allocator, nullptr, 0, &state->directoryCapacity, 16, sizeof(WatchDirectory));
        if (directories == nullptr)
        {
            WatchFree(state);
            return false;
        }

        state->directories = (WatchDirectory*)directories;
        state->directories[0].descriptor = descriptor;
        state->directories[0].parent = WatchNone;
        state->directoryCount = 1;

        if (recursive)
            WatchDescend(state, 0, false, false);

        if (state->limited)
        {
            WatchFree(state);
            return false;
        }

        watch->handle = (u64)(uintptr_t)state;
        return true;
    }

    b8 DirectoryWatchNext(DirectoryWatch* watch, DirectoryChange* outChange, u32 timeout)
    {
        WatchState* state = (WatchState*)(uintptr_t)watch->handle;
        if (state == nullptr || state->ended)
            return false;

        Timepoint start = TimepointNow();
        while (true)
        {
            if (state->queueRead < state->queueSize)
            {
                WatchUnqueue(state, outChange);
                return true;
            }

            if (state->overflow)
            {
                state->overflow = false;
                WatchOverflow(outChange);
                return true;
            }

            if (state->bufferRead < state->bufferSize)
            {
                if (WatchTake(state, outChange))
                    return true;
                continue;
            }

            if (state->failed)
            {
                state->ended = true;
                WatchOverflow(outChange);
                return true;
            }

            if (!WatchFill(state, WatchRemaining(start, timeout)) && !state->failed)
                return false;
        }
    }

    void DirectoryWatchClose(DirectoryWatch* watch)
    {
        WatchState* state = (WatchState*)(uintptr_t)watch->handle;
        if (state != nullptr)
            WatchFree(state);

        watch->handle = 0;
    }

#else

    b8 DirectoryWatchOpen(DirectoryWatch* watch, const c8* path, b8 recursive, MemoryAllocator allocator)
    {
        (void)recursive;
        (void)allocator;
        *watch = {};
        return TOOL_FAIL("Watching directories isn't supported on this platform, so %s can't be watched.", path);
    }

    b8 DirectoryWatchNext(DirectoryWatch* watch, DirectoryChange* outChange, u32 timeout)
    {
        (void)watch;
        (void)outChange;
        (void)timeout;
        return false;
    }

    void DirectoryWatchClose(DirectoryWatch* watch)
    {
        watch->handle = 0;
    }

#endif // __linux__

#endif // TOOL_UNIX



    //- Platform-agnostic

    b8 FileExists(const c8* path)
    {
        PathInfo info = {};
        return PathInfoGet(path, &info) && !info.directory;
    }

    b8 DirectoryExists(const c8* path)
    {
        PathInfo info = {};
        return PathInfoGet(path, &info) && info.directory;
    }

    // Creates each parent on the way quietly, as some can't be created but exist, like a drive or the root.
    b8 DirectoryCreate(const c8* path)
    {
        c8 partial[PathCapacity];
        if (!PathCopy(partial, path))
            return false;

        u32 length = 0;
        while (partial[length] != 0)
            length++;
        while (length > 1 && PathIsSeparator(partial[length - 1]))
            partial[--length] = 0;

        for (u32 i = 1; i < length; i++)
        {
            if (!PathIsSeparator(partial[i]) || PathIsSeparator(partial[i - 1]))
                continue;

            c8 separator = partial[i];
            partial[i] = 0;
            DirectoryMake(partial, true);
            partial[i] = separator;
        }

        return DirectoryMake(partial, false);
    }

    b8 FileReadAll(const c8* path, void** outData, u64* outSize, MemoryAllocator allocator)
    {
        *outData = nullptr;
        *outSize = 0;

        File file = 0;
        if (!FileOpen(&file, path, OpenModeRead))
            return false;

        u64 size = FileSize(file);
        c8* data = (c8*)AllocatorAlloc(allocator, size + 1);
        if (data == nullptr)
        {
            FileClose(file);
            return TOOL_FAIL("Could not allocate %llu bytes to read %s.", (unsigned long long)size + 1, path);
        }

        u64 read = 0;
        b8 success = FileRead(file, data, size, &read);
        FileClose(file);

        if (!success)
        {
            AllocatorDealloc(allocator, data);
            return false;
        }

        data[read] = 0;
        *outData = data;
        *outSize = read;
        return true;
    }

    b8 DirectoryWatchChanged(DirectoryWatch* watch, u32 timeout)
    {
        DirectoryChange change;
        if (!DirectoryWatchNext(watch, &change, timeout))
            return false;

        while (DirectoryWatchNext(watch, &change, 0))
            ;

        return true;
    }

    b8 FileWriteAll(const c8* path, const void* data, u64 size)
    {
        c8 temporary[PathCapacity];
        if (!PathCopy(temporary, path, ".tmp"))
            return false;

        File file = 0;
        if (!FileOpen(&file, temporary, OpenModeNewOrOverwrite))
            return false;

        // On disk before it replaces anything
        b8 success = FileWrite(file, data, size) && FileFlush(file);
        FileClose(file);

        if (success)
            success = FileMove(temporary, path);

        if (!success)
            FileDelete(temporary);

        return success;
    }
}
