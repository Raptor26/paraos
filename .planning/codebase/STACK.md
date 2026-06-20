# Technology Stack

**Analysis Date:** 2026-06-20

## Languages

**Primary:**
- C++17 — All cross-platform OSAL abstractions, RAII wrappers, containers, and helpers.
- C11 — FreeRTOS integration glue, low-level hooks, and some utility headers.

**Secondary:**
- Python 3 — Build/test orchestration (`builder.py`, `pybuilder/` modules).
- CMake — Build system definition and platform selection.
- Shell/Bash — Docker helpers and CI entrypoint scripts.

## Runtime

**Environment:**
- No single runtime; the library is a source-only OS abstraction layer meant to be compiled into the consuming firmware or PC application.
- Standalone builds run natively on Linux/Windows PC; embedded builds target FreeRTOS.

**Build System:**
- CMake ≥ 3.20 (root `CMakeLists.txt`);
- `CMakePresets.json` requires CMake ≥ 3.28.
- Ninja is the expected generator.

## Frameworks

**Core:**
- None (header + static-library OSAL). The project provides its own abstractions over POSIX threads, WinAPI, and FreeRTOS.

**Testing:**
- GoogleTest — Unit and integration tests (`find_package(GTest REQUIRED)`).
- CTest — Test execution and memcheck integration.
- Google benchmark — Optional container benchmarks (`containers/tests/bench_paraos_containers.cpp`).

**Static Analysis / Quality:**
- clang-tidy — `.clang-tidy` with a broad check list and `WarningsAsErrors: '*'`.
- cppcheck — CMake helpers in `cmake/CppcheckTargets.cmake` and `cmake/Findcppcheck.cmake`.
- Valgrind memcheck — Used in CI (`ctest -T memcheck`).

**Build / Dev:**
- `clang-format` — Google style with `AlignAfterOpenBracket: AlwaysBreak` (`.clang-format`).
- Commitizen / Conventional Commits — Version and changelog managed in `.cz.json`.

## Key Dependencies

**Critical (vendored as git-subrepos):**
- Boost.LEAF (`leaf/`) — Lightweight error handling; used for `etl::exception` integration.
- ETL (`etl/`) — Embedded Template Library; provides delegates, queues, timers, atomics, scheduler.
- Microsoft GSL (`GSL/`) — `gsl::finally`, `gsl::narrow_cast`.
- LwRB (`lwrb/`) — Lightweight ring buffer used by `paraos_ringbuff.hpp`.
- FreeRTOS-Kernel (`port_freertos/FreeRTOS-Kernel/`) — RTOS backend when `RTOS_NAME=FREERTOS`.

**Infrastructure:**
- POSIX threads / `pthread` — Linux thread primitive backend.
- WinAPI (`ws2_32`, `wsock32`) — Windows thread/socket backend.
- FreeRTOS POSIX/MinGW port — Used for FreeRTOS PC simulation.

## Configuration

**Build:**
- `CMakeLists.txt` — Root project definition, port selection, dependency wiring.
- `setup.cmake` — Defines interface target `paraos_setup` for compile definitions and include paths.
- `CMakePresets.json` — Predefined PC and FreeRTOS presets for Clang/GCC, Debug/Release, trace, clang-tidy, polymorphic extra.

**Compile definitions (selected):**
- `PARAOS_LIKE_UNIX` / `PARAOS_LIKE_WINAPI` / `PARAOS_LIKE_FREERTOS` — Selected port.
- `PARAOS_CHECK_LOOP_ENABLE` — Enabled in standalone Debug; enables `PARAOS_CHECK_ASSERT` / `PARAOS_CHECK_LOOP`.
- `paraosTRACE_ENABLE` — Enabled via `TRACE=true` preset/cache variable.
- `PARAOS_USING_POLYMORPHIC_EXTRA` — Adds `virtual` to extra helpers for mocking.
- `CLANG_TIDY_ENABLE` — Attaches clang-tidy to the library and tests.

**Version:**
- `paraos_version.hpp` is generated from `paraos_version.hpp.in` and is gitignored.
- Project version (`0.13.0`) is read from `.cz.json` at configure time.

## Platform Requirements

**Development:**
- Linux recommended for full PC and FreeRTOS POSIX builds.
- Windows with MinGW/MSVC for `port_win` builds.
- macOS is **not** supported for `port_unix` because it relies on POSIX timer APIs (`timer_create`, `itimerspec`) unavailable on macOS.

**Production:**
- Distributed only as source; consumers add the repo as a CMake subdirectory and link `paraos::paraos`.
- Embedded targets set `RTOS_NAME=FREERTOS`, `FREERTOS_PORT`, `FREERTOS_HEAP`, optionally `FREERTOS_USER_CONFIG`.

---

*Stack analysis: 2026-06-20*
*Update after major dependency or preset changes*
