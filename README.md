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
| `key.h` / `random.h` | Lossless packed keys; stream, keyed (stateless) and low-discrepancy random generators |
| `mathematics.h` | Scalar math named by width (`F32Sin`, `I32Wrap`) |
| `vector.h` / `matrix.h` / `quaternion.h` / `box.h` | Vectors, column-major matrices, quaternions, AABBs |
| `color.h` | Byte and float colors, HSV/HSL |
| `atomic.h` / `threading.h` | Atomics, threads and zero-init primitives (mutex, semaphore, ...) on futex-style waits |
| `io.h` | Files and directories with UTF-8 paths |
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

Configuration flags are listed at the top of `include/tool.h`. Builds on Windows and Linux with MSVC, GCC and Clang.
The web (Emscripten) has code paths but isn't routinely tested yet.

The test program (`TOOLTest`) builds when TOOL is the top-level CMake project.

## License

MIT. See `LICENSE`.
