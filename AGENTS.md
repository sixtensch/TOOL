# AGENTS.md

TOOL (Tired Of Objects Library): a C++20 standard library for data-oriented code. Plain structs, free functions,
explicit allocators. General-purpose and used by several projects: don't shape APIs around one consumer.

## Where things are

- `include/tool.h`: umbrella include, plus the list of configuration flags.
- `include/tool/<topic>.h`: one header per topic. **Each opens with an overview comment. Read that first**, then
  the declarations block. `README.md` has a one-line index of the headers.
- Opt-in headers, deliberately not included by `tool.h`: `format.h`, `intrinsics.h`.
- `source/<topic>.cpp`: non-inline code, platform code, and anything that needs system headers.
- `source/test/`: the test program. `test.cpp` holds `main`, `test.h` declares `TestX()`, and `testx.cpp`
  implements it.
- `archive/`: dead project files. Ignore it.

## Build and test

```
cmake -S . -B build -G "Ninja Multi-Config"
cmake --build build --config Debug
build/Debug/TOOLTest
```

- Each suite prints `<Name>: passed`/`ok`, and a failed `TOOL_ASSERT` aborts.
- Debug defines `TOOL_DEBUG_ASSERTS`.
- CMake lists sources explicitly: add new `.cpp` and test files to `CMakeLists.txt`.
- New tests:
  - Write `static void` cases plus `void TestX()` in `source/test/testx.cpp`.
  - Declare `TestX()` in `test.h` and call it from `main`.
  - Use `static_assert` for `constexpr` code. Compare against a reference (std, upstream vectors) where one exists.
- Verify with MSVC, GCC and Clang when you touch platform code, templates or anything compiler-sensitive.

## Rules

**Compile cost**
- Headers include only sibling TOOL headers, never system or std headers.
- Wrap system or std functionality in a `.cpp` (example: the libm wrappers in `mathematics.cpp`).
- Keep templates thin, typed wrappers over an untyped core in the `.cpp` (example: `HashMap<F>` over
  `HashMapCore`).

**Header layout**
- Include guard `_TOOL_<NAME>_H`, closed with `#endif //_TOOL_<NAME>_H`.
- Order: overview comment, `//~ Definitions` (macros), a declarations `namespace Tool {}`, then `//- Implementation`
  with a second `namespace Tool {}` for inline and template bodies.
- `//-` marks a section and `//~` a subsection. Separate top-level blocks with three blank lines.

**Naming and types**
- Everything is in `namespace Tool`.
- Functions are prefixed by their type or topic: `ArenaAlloc`, `HashMapFind`, `S8Format`.
- Math functions are named by width: `F32Sin`, `I32Wrap`.
- Macros start with `TOOL_`.
- Use `i8`..`u64`, `f32`, `f64`, `b8`, `b32`, `c8`, `c16`, never `int`, `bool`, `char` or `size_t`.
- Enums are unscoped, and each value repeats the enum name as a prefix: `HashKeyType` → `HashKeyTypeU64`. One value
  per line, each with a trailing comment.

**Data and functions**
- No constructors, destructors, virtuals or inheritance in library types.
- A zeroed struct is either valid or made ready by `XInit`. `XDeInit` releases it.
- The state pointer is the first parameter.
- Memory comes from a `MemoryAllocator` argument (`Allocator()` is the heap; arenas have no deallocate). Don't
  allocate behind the caller's back.

**Errors**
- No exceptions.
- Expected failures record through `TOOL_FAIL(...)` and return false or null.
- Bugs use `TOOL_ASSERT` (every build) or `TOOL_DEBUG_ASSERT` (debug only).

**Stability**
- `random.h` and `hash.h` outputs get persisted (seeds, saved hashes). Changing them is a breaking change.

**Platforms**
- `TOOL_WINDOWS` and `TOOL_UNIX`/`TOOL_LINUX` come from CMake. `TOOL_WEB` is detected from Emscripten.
- Keep platform branches in `.cpp` files.
- The instruction set is the consumer's choice: never add `-march` or similar.

**Style**
- Indent with tabs. Some older files still use spaces; don't mass-convert them unasked.
- Braces on their own line, left-bound pointers (`T* p`), no column limit, comments wrapped near 120 columns.
- Source files are ASCII-only. Write non-ASCII as `\x` escapes.
- Comments explain what holds going forward, short and plain. No change history.
- Doc comments sit above the declaration, not the implementation.

**Commits**
- Commit messages are full sentences stating what the library now does (see `git log`).
