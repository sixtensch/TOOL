#include "io.h"
#include "error.h"
#include "mathematics.h"



//~ Windows
#if defined(TOOL_WINDOWS)

#include <Windows.h>

//~ Unix
#elif defined(TOOL_UNIX)

#define _FILE_OFFSET_BITS 64 // 64-bit sizes and offsets on 32-bit Linux too

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#endif



namespace Tool
{
    //- Platform-agnostic helpers

    // Longest path the helpers below copy, in bytes with the terminator.
    static constexpr u32 PathCapacity = 4096;

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

    // Creates one directory. True if it exists afterwards. Quiet leaves failures unrecorded.
    static b8 DirectoryMake(const c8* path, b8 quiet);



    //- Windows implementation

#if defined(TOOL_WINDOWS)

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

#endif // TOOL_WINDOWS



    //- Unix implementation

#if defined(TOOL_UNIX)

    // File descriptors start at 0, so a File holds the descriptor plus one.
    static i32 FileDescriptor(File file)
    {
        return (i32)file - 1;
    }

    static SystemTimepoint TimepointFromStat(const struct stat* status)
    {
        constexpr i64 secondsFrom1601To1970 = 11644473600;
#if defined(__APPLE__)
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
#if defined(TOOL_WEB)
        if (capacity < 2)
            return TOOL_FAIL("The buffer is too small for the path.");

        outPath[0] = '/';
        outPath[1] = 0;
        return true;
#elif defined(__linux__)
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
