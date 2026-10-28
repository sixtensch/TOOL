# TOOL: the Tired Of Objects Library

A small C++20 standard library for games and other data-oriented, performance-minded programs. It's built from plain
structs and free functions, explicit allocators, and no exceptions or class hierarchies. It's C++ rather than C for
namespaces, operator overloading on math types, and the odd template, and little else.

Goals:

- **Slim and cheap to compile.** `tool.h` pulls in no system headers! The heaviest headers are opt-in.
- **Data-oriented.** Zero-initialized structs and explicit `Init`/`DeInit` instead of constructors. No RAII, just data. Wherever practical, allocator injection is used for memory flexibility.
- **Simple and self-documenting.** Every header opens with an overview of what it holds and how to use it. Reading a header from the top should give you the overview you need.

## Contents

| Header | What it holds |
| --- | --- |
| `basics.h` | Fixed-width types (`i32`, `u64`, `f32`, `b8`, `c8`, ...), limits, platform defines |
| `memory.h` | Allocators: heap, pages, chunked `Arena`, `ContiguousArena`, `MagicCircular` ring buffer |
| `text.h` | `s8` string views, slicing, searching, number parsing, UTF-8/UTF-16, bounded C strings |
| `format.h` | Opt-in `std::format`-syntax formatting, checked at compile time, with captures for deferred rendering |
| `hash.h` / `hashmap.h` | FNV-1a and rapidhash, all `constexpr`, and an open-addressing hash map |
| `sort.h` | Introsort by comparison, and a stable radix sort by `u64` key with keys for signed and float values |
| `key.h` / `random.h` | Lossless packed keys; stream, keyed (stateless) and low-discrepancy random generators |
| `mathematics.h` | Scalar math named by width (`F32Sin`, `I32Wrap`) |
| `vector.h` / `matrix.h` / `quaternion.h` / `box.h` | Vectors, column-major matrices, quaternions, AABBs |
| `color.h` | Byte and float colors, HSV/HSL |
| `atomic.h` / `threading.h` | Atomics, threads and zero-init primitives (mutex, semaphore, ...) on futex-style waits |
| `io.h` | Files and directories with UTF-8 paths, and watching directories for changes |
| `network.h` | IPv4/IPv6 addresses, name lookup, non-blocking TCP and UDP sockets, polling |
| `http.h` | URLs, a polled HTTP/1.1 server and client, WebSocket both ends (the browser's on the web) |
| `temporal.h` | Monotonic and wall clocks |
| `linking.h` | Loading shared libraries and their symbols |
| `error.h` | Per-thread error records and asserts |
| `intrinsics.h` | Opt-in portable 128/256-bit SIMD |
| `utility.h` | `TOOL_DEFER` and macro helpers |

## Use

```cmake
add_subdirectory(TOOL)
target_link_libraries(YourTarget PRIVATE TOOL)
```

```cpp
#include "tool.h"        // Everything but the opt-in headers
#include "tool/format.h" // Opt-in

Tool::Arena arena = {};
Tool::ArenaInit(&arena, Tool::Allocator(), 64 * 1024, "Level");
Tool::s8 name = Tool::S8Format(Tool::Allocator(&arena), "Level {}: {:.1f}s", index, time);
```

Builds on Windows and Linux with MSVC, GCC and Clang, and for the web with Emscripten.

## Defines

TOOL's defines come in three kinds, all listed in `include/tool/basics.h` (instruction sets in
`include/tool/intrinsics.h`). Each is always defined, to 1 or 0, so test them by value: `#if TOOL_WINDOWS`, never
`#ifdef`.

- **Facts about the target**, detected from the compiler's own macros, which describe the target and never the
  machine compiling, so cross-compiling needs nothing more than the right compiler. A target TOOL doesn't know
  stops the build with an `#error`.
  - System, exactly one: `TOOL_WINDOWS`, `TOOL_LINUX`, `TOOL_APPLE`, `TOOL_WEB`. `TOOL_UNIX` is the POSIX layer:
    Linux, Apple's systems and the web.
  - Compiler, exactly one: `TOOL_MSVC` (not clang-cl), `TOOL_CLANG` (clang-cl and Emscripten included), `TOOL_GCC`.
  - Architecture, exactly one: `TOOL_X64`, `TOOL_X86`, `TOOL_ARM64`, `TOOL_ARM32`, `TOOL_WASM`.
  - Instruction sets the build enables: `TOOL_CPU_SSE`, `TOOL_CPU_SSE41`, `TOOL_CPU_AVX`, `TOOL_CPU_AVX2`,
    `TOOL_CPU_AVX512`, `TOOL_CPU_FMA`.
- **Capabilities**, derived from the facts, for code that needs something rather than a particular platform:
  `TOOL_THREADS`, `TOOL_VIRTUAL_MEMORY`, `TOOL_MIRRORED_MEMORY`, `TOOL_SOCKETS`, `TOOL_INT128`, and the SIMD backend,
  `TOOL_SIMD_VECTOR`, `TOOL_SIMD_X86` or `TOOL_SIMD_SCALAR`.
- **Options**, which only a build or a program can choose, set to 1 before including TOOL, 0 otherwise:
  `TOOL_DEBUG_ASSERTS` (TOOL's CMake sets it in Debug), `TOOL_NO_ACRONYMS`, `TOOL_NO_UTF`, `TOOL_NO_MATH`,
  `TOOL_NO_FORMAT_VALIDATION`, `TOOL_NO_SIMD`.

TOOL's own sources build with undefined macros in `#if` as errors (`-Werror=undef`, MSVC's `/we4668` with `<...>`
headers excluded); a program using TOOL can do the same.

The test program (`TOOLTest`) builds when TOOL is the top-level CMake project.

## License

MIT. See `LICENSE`.
