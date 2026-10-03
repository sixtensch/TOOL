#include "tool.h"
#include "test.h"
#include <stdio.h>
#include <string.h>

using namespace Tool;



static const c8* root = "tooltest_io";

// Deletes a directory and everything in it.
static void DeleteTree(const c8* path)
{
	DirectoryIterator iterator;
	if (!DirectoryOpen(&iterator, path))
		return;

	while (DirectoryNext(&iterator))
	{
		c8 child[2048];
		snprintf(child, sizeof(child), "%s/%s", path, iterator.name);
		if (iterator.info.directory)
			DeleteTree(child);
		else
			TOOL_ASSERT(FileDelete(child));
	}

	DirectoryClose(&iterator);
	TOOL_ASSERT(DirectoryDelete(path));
}

static u8 Pattern(u64 i)
{
	return (u8)(i * 131 + (i >> 9));
}

static void TestFiles()
{
	const c8* path = "tooltest_io/a/file.bin";
	constexpr u64 size = 3 * 1024 * 1024 + 17;
	u8* data = (u8*)AllocatorAlloc(Allocator(), size);
	for (u64 i = 0; i < size; i++)
		data[i] = Pattern(i);

	// Write it, and fail to create it again.
	File file = 0;
	TOOL_ASSERT(FileOpen(&file, path, OpenModeNew));
	TOOL_ASSERT(FileWrite(file, data, size) && FilePosition(file) == size && FileSize(file) == size);
	TOOL_ASSERT(FileFlush(file));
	FileClose(file);
	TOOL_ASSERT(!FileOpen(&file, path, OpenModeNew) && file == 0);

	// Read it back in pieces, past the end, and from a seek.
	u8* back = (u8*)AllocatorAlloc(Allocator(), size);
	TOOL_ASSERT(FileOpen(&file, path, OpenModeRead));
	u64 read = 0;
	TOOL_ASSERT(FileRead(file, back, 1000, &read) && read == 1000);
	TOOL_ASSERT(FileRead(file, back + 1000, size, &read) && read == size - 1000);
	TOOL_ASSERT(memcmp(back, data, size) == 0);
	TOOL_ASSERT(FileRead(file, back, 16, &read) && read == 0);
	TOOL_ASSERT(FileSeek(file, 123457) && FilePosition(file) == 123457);
	u8 four[4] = {};
	TOOL_ASSERT(FileRead(file, four, 4, &read) && read == 4 && four[0] == Pattern(123457) && four[3] == Pattern(123460));

	// Read-only files refuse writes.
	TOOL_ASSERT(!FileWrite(file, data, 4));
	FileClose(file);

	// Editing keeps the contents around the change.
	TOOL_ASSERT(FileOpen(&file, path, OpenModeEditExisting));
	TOOL_ASSERT(FilePosition(file) == 0 && FileSeek(file, 10) && FileWrite(file, "edit", 4));
	FileClose(file);
	TOOL_ASSERT(FileOpen(&file, path, OpenModeRead) && FileSize(file) == size);
	TOOL_ASSERT(FileRead(file, back, 20, &read) && memcmp(back + 10, "edit", 4) == 0 && back[9] == Pattern(9) && back[14] == Pattern(14));
	FileClose(file);

	// Appending starts at the end, and overwriting empties.
	TOOL_ASSERT(FileOpen(&file, path, OpenModeAppendExisting) && FilePosition(file) == size);
	TOOL_ASSERT(FileWrite(file, "tail", 4) && FileSize(file) == size + 4);
	FileClose(file);
	TOOL_ASSERT(FileOpen(&file, path, OpenModeOverwriteExisting) && FileSize(file) == 0);
	FileClose(file);
	TOOL_ASSERT(FileOpen(&file, "tooltest_io/a/appended.txt", OpenModeNewOrAppend) && FileWrite(file, "one", 3));
	FileClose(file);
	TOOL_ASSERT(FileOpen(&file, "tooltest_io/a/appended.txt", OpenModeNewOrAppend) && FilePosition(file) == 3 && FileWrite(file, "two", 3));
	FileClose(file);

	// Existing-only modes need the file, and directories aren't files.
	TOOL_ASSERT(!FileOpen(&file, "tooltest_io/a/missing", OpenModeEditExisting));
	TOOL_ASSERT(!FileOpen(&file, "tooltest_io/a/missing", OpenModeRead));
	TOOL_ASSERT(!FileOpen(&file, "tooltest_io/a", OpenModeRead));

	// Whole files: a zero after the contents, and replacing leaves no temporary file behind.
	void* all = nullptr;
	u64 allSize = 0;
	TOOL_ASSERT(FileReadAll("tooltest_io/a/appended.txt", &all, &allSize, Allocator()));
	TOOL_ASSERT(allSize == 6 && memcmp(all, "onetwo", 7) == 0);
	AllocatorDealloc(Allocator(), all);

	TOOL_ASSERT(FileWriteAll("tooltest_io/a/whole.bin", data, size));
	TOOL_ASSERT(FileWriteAll("tooltest_io/a/whole.bin", "replaced", 8));
	TOOL_ASSERT(FileReadAll("tooltest_io/a/whole.bin", &all, &allSize, Allocator()));
	TOOL_ASSERT(allSize == 8 && memcmp(all, "replaced", 9) == 0);
	AllocatorDealloc(Allocator(), all);
	TOOL_ASSERT(!FileExists("tooltest_io/a/whole.bin.tmp"));
	TOOL_ASSERT(!FileReadAll("tooltest_io/a/missing", &all, &allSize, Allocator()) && all == nullptr);

	AllocatorDealloc(Allocator(), data);
	AllocatorDealloc(Allocator(), back);
}

static void TestPaths()
{
	// Info for files and directories, with times on the same clock as SystemTimepointNow.
	PathInfo info = {};
	TOOL_ASSERT(PathInfoGet("tooltest_io/a/whole.bin", &info) && !info.directory && info.size == 8);
	SystemTimepoint now = SystemTimepointNow(false);
	u64 minute = 60ull * 10000000;
	TOOL_ASSERT(info.modified + minute > now && info.modified < now + minute);
	TOOL_ASSERT(PathInfoGet("tooltest_io/a", &info) && info.directory && info.size == 0);
	TOOL_ASSERT(!PathInfoGet("tooltest_io/missing", &info));
	TOOL_ASSERT(FileExists("tooltest_io/a/whole.bin") && !DirectoryExists("tooltest_io/a/whole.bin"));
	TOOL_ASSERT(DirectoryExists("tooltest_io/a") && !FileExists("tooltest_io/a"));
	TOOL_ASSERT(DirectoryExists("tooltest_io/a/"));
#ifdef TOOL_WINDOWS
	TOOL_ASSERT(DirectoryExists("tooltest_io\\a")); // Elsewhere '\' is part of a name
#endif

	// Moves rename, cross directories and replace.
	TOOL_ASSERT(FileMove("tooltest_io/a/appended.txt", "tooltest_io/a/b/moved.txt"));
	TOOL_ASSERT(!FileExists("tooltest_io/a/appended.txt") && FileExists("tooltest_io/a/b/moved.txt"));
	TOOL_ASSERT(FileMove("tooltest_io/a/b/moved.txt", "tooltest_io/a/whole.bin"));
	TOOL_ASSERT(PathInfoGet("tooltest_io/a/whole.bin", &info) && info.size == 6);
	TOOL_ASSERT(FileMove("tooltest_io/a/b/c", "tooltest_io/a/b/renamed") && DirectoryExists("tooltest_io/a/b/renamed"));

	// Deleting: files, and only empty directories.
	TOOL_ASSERT(!DirectoryDelete("tooltest_io/a"));
	TOOL_ASSERT(FileDelete("tooltest_io/a/file.bin") && !FileExists("tooltest_io/a/file.bin"));
	TOOL_ASSERT(!FileDelete("tooltest_io/a/file.bin"));
	TOOL_ASSERT(DirectoryDelete("tooltest_io/a/b/renamed") && !DirectoryExists("tooltest_io/a/b/renamed"));

	// UTF-8 names go in and come back out the same.
	const c8* unicode = "tooltest_io/\xC3\xBCn\xC3\xAF" "c\xC3\xB8" "d\xC3\xA9 \xE2\x9C\x93.txt"; // ünïcødé ✓.txt
	TOOL_ASSERT(FileWriteAll(unicode, "u", 1) && FileExists(unicode));

	// The executable's directory is a real directory, with '/' separators and no trailing one.
	c8 executable[4096];
	TOOL_ASSERT(PathExecutableDirectory(executable, sizeof(executable)));
	u64 length = strlen(executable);
	TOOL_ASSERT(length > 0 && DirectoryExists(executable) && strchr(executable, '\\') == nullptr);
	TOOL_ASSERT(length == 1 || executable[length - 1] != '/');
	TOOL_ASSERT(!PathExecutableDirectory(executable, 2));
}

static void TestListing()
{
	// Exactly the entries, without '.' and '..', each with its info.
	TOOL_ASSERT(DirectoryCreate("tooltest_io/list/sub"));
	TOOL_ASSERT(FileWriteAll("tooltest_io/list/one.txt", "1", 1));
	TOOL_ASSERT(FileWriteAll("tooltest_io/list/two.txt", "22", 2));

	DirectoryIterator iterator;
	TOOL_ASSERT(DirectoryOpen(&iterator, "tooltest_io/list/"));
	u32 seen = 0;
	while (DirectoryNext(&iterator))
	{
		b8 one = strcmp(iterator.name, "one.txt") == 0;
		b8 two = strcmp(iterator.name, "two.txt") == 0;
		b8 sub = strcmp(iterator.name, "sub") == 0;
		TOOL_ASSERT(one || two || sub, "Unexpected entry %s", iterator.name);
		TOOL_ASSERT(iterator.info.directory == sub && iterator.info.size == (one ? 1u : two ? 2u : 0u));
		seen |= one ? 1 : two ? 2 : 4;
	}
	DirectoryClose(&iterator);
	DirectoryClose(&iterator);
	TOOL_ASSERT(seen == 7);

	TOOL_ASSERT(DirectoryOpen(&iterator, "tooltest_io/list/sub"));
	TOOL_ASSERT(!DirectoryNext(&iterator));
	DirectoryClose(&iterator);
	TOOL_ASSERT(!DirectoryOpen(&iterator, "tooltest_io/missing"));
	TOOL_ASSERT(!DirectoryOpen(&iterator, "tooltest_io/list/one.txt"));
}

// Past Windows' classic limit of 260 characters, relative and with '..' in it.
static void TestLongPaths()
{
	c8 path[1024] = "tooltest_io/long";
	for (u32 i = 0; i < 6; i++)
		strcat(path, "/directory_with_a_name_long_enough_to_add_up_quickly_0123456789");
	TOOL_ASSERT(strlen(path) > 300);

	TOOL_ASSERT(DirectoryCreate(path) && DirectoryExists(path), "%s", ErrorMessage());
	c8 file[1100];
	snprintf(file, sizeof(file), "%s/../file_in_a_long_path.txt", path);
	TOOL_ASSERT(FileWriteAll(file, "deep", 4));

	void* data = nullptr;
	u64 size = 0;
	TOOL_ASSERT(FileReadAll(file, &data, &size, Allocator()) && size == 4 && memcmp(data, "deep", 4) == 0);
	AllocatorDealloc(Allocator(), data);

	PathInfo info = {};
	TOOL_ASSERT(PathInfoGet(file, &info) && info.size == 4);
}



void TestIO()
{
	DeleteTree(root);
	TOOL_ASSERT(!DirectoryExists(root));

	// Missing parents get created, and creating what exists succeeds.
	TOOL_ASSERT(DirectoryCreate("tooltest_io/a/b/c"));
	TOOL_ASSERT(DirectoryExists("tooltest_io/a") && DirectoryExists("tooltest_io/a/b/c"));
	TOOL_ASSERT(DirectoryCreate("tooltest_io/a/b/c/"));

	TestFiles();
	TestPaths();
	TestListing();
	TestLongPaths();

	DeleteTree(root);
	TOOL_ASSERT(!DirectoryExists(root));
	printf("IO: passed\n");
}
