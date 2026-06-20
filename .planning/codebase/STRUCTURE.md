# Codebase Structure

**Analysis Date:** 2026-06-20

## Directory Layout

```
paraos/
├── cmake/                    # CMake modules (coverage, cppcheck, profiling)
├── containers/               # Message buffers, blocking queues, ring buffers
│   ├── tests/                # GoogleTest + stress tests + optional benchmark
├── docker/                   # Docker helper scripts
├── etl/                      # Embedded Template Library (subrepo)
├── extra/                    # High-level helpers (executor, scheduler, LED)
│   ├── docs/                 # Helper documentation
│   └── tests/                # GoogleTest + multithread tests
├── GSL/                      # Microsoft Guidelines Support Library (subrepo)
├── leaf/                     # Boost.LEAF (subrepo)
├── lwrb/                     # Lightweight ring buffer (subrepo)
├── port_freertos/            # FreeRTOS port implementation
│   ├── config/               # FreeRTOS etl_profile.h / config
│   └── FreeRTOS-Kernel/      # FreeRTOS kernel sources (subrepo)
├── port_tests/               # OSAL-level tests and standalone examples
├── port_unix/                # POSIX/Linux port implementation
│   ├── config/               # etl_profile.h for unix
│   └── tests/                # Port-specific utility tests
├── port_win/                 # WinAPI port implementation
│   └── config/               # etl_profile.h for windows
├── pybuilder/                # Python build/test automation
├── AGENTS.md                 # Project guide for AI agents
├── CMakeLists.txt            # Root project definition
├── CMakePresets.json         # CMake presets (PC/FreeRTOS, Clang/GCC)
├── Dockerfile                # Multi-stage CI Docker image
├── LICENSE.txt               # MIT license
├── builder.py                # Interactive / non-interactive build runner
├── paraos_*.hpp / paraos_*.h # Core cross-platform headers
├── paraos_version.hpp.in     # Version header template
└── setup.cmake               # Interface target setup
```

## Directory Purposes

**Root headers (`paraos_*.hpp` / `paraos_*.h`):**
- Purpose: Portable public API used by all ports.
- Key files: `paraos_base.hpp`, `paraos_thread_common.hpp`, `paraos_mutex_raii.hpp`, `paraos_runtime_profiler.hpp`, `paraos_bool_atomic.hpp`, `paraos_isr.hpp`, `paraos_exceptions.hpp`, `paraos_check.h`, `paraos_trace.hpp`, `paraos_attr.h`.

**`port_unix/`:**
- Purpose: POSIX/Linux backend.
- Contains: `paraos_thread.hpp`, `paraos_mutex.hpp`, `paraos_semaphore.hpp`, `paraos_timer.hpp`, `paraos_critical.hpp`, `paraos_time.hpp`, `paraos_utils.hpp`, `paraos_socket_udp.hpp`.
- Subdirectories: `config/` (ETL profile), `tests/` (utility tests).

**`port_win/`:**
- Purpose: WinAPI backend.
- Mirrors `port_unix/` with Windows implementations and links `ws2_32`/`wsock32`.
- Subdirectories: `config/`.

**`port_freertos/`:**
- Purpose: FreeRTOS backend.
- Contains: `paraos_thread.hpp`, `paraos_mutex.hpp`, `paraos_semaphore.hpp`, `paraos_timer.hpp`, `paraos_critical.hpp`, `paraos_time.hpp`, `paraos_utils.hpp`, `paraos_switch_context.hpp`, `paraos_freertos_hooks.cpp`.
- Subdirectories: `config/`, `FreeRTOS-Kernel/`.

**`containers/`:**
- Purpose: Thread-safe containers.
- Contains: `paraos_message_buffer.hpp`, `paraos_queue_blocking.hpp`, `paraos_ringbuff.hpp`, `paraos_multi_ringbuff.hpp`.
- Subdirectories: `tests/`.

**`extra/`:**
- Purpose: Higher-level reusable components.
- Contains: `paraos_oneshot_executor.hpp`, `paraos_thread_cooperative_scheduling.hpp`, `paraos_thread_sequence.hpp`, `paraos_status_led.hpp`/`cpp`.
- Subdirectories: `docs/`, `tests/`.

**`port_tests/`:**
- Purpose: OSAL-level tests and runnable examples.
- Contains: `test_*.cpp`, `example_*.cpp`, plus a Python-driven UDP test (`test_udp_socket.py`).

**`pybuilder/`:**
- Purpose: Build/test automation.
- Contains: `builder_functions.py`, `pycmakebuilder.py`, `test_pycmakebuilder.py`, `install_builder_dependencies.py`, `builder_requirements.txt`, `ruff.toml`.

**`cmake/`:**
- Purpose: Reusable CMake modules.
- Key files: `CodeCoverage.cmake`, `CppcheckTargets.cmake`, `EnableProfiling.cmake`, `Findcppcheck.cmake`.

## Key File Locations

**Entry Points:**
- `CMakeLists.txt` — Library build entry.
- `builder.py` — Interactive/non-interactive test runner.
- `pybuilder/pycmakebuilder.py` — CI driver.

**Configuration:**
- `CMakePresets.json` — Preset definitions.
- `.clang-format` — C++ formatting.
- `.clang-tidy` — Static analysis checks.
- `.cz.json` — Commitizen / version config.
- `setup.cmake` — `paraos_setup` interface target.

**Core Logic:**
- Root `paraos_*.hpp` — Public API.
- `port_*/paraos_*.hpp` — Platform implementations.
- `containers/paraos_*.hpp` — Container logic.
- `extra/paraos_*.hpp` — Helper logic.

**Testing:**
- `port_tests/CMakeLists.txt` — OSAL test executable definitions.
- `containers/tests/CMakeLists.txt` — Container tests and stress tests.
- `extra/tests/CMakeLists.txt` — Extra helper tests.

**Documentation:**
- `AGENTS.md` — Project overview and agent guidelines.
- `README.md` (root) — Public project readme.
- `CHANGELOG.md` — Commitizen-managed changelog.

## Naming Conventions

**Files:**
- All public headers: `paraos_<feature>.hpp` or `paraos_<feature>.h`.
- Test files: `test_<feature>.cpp`, `test_<feature>_thread.cpp` for multithread variants.
- Example files: `example_<feature>.cpp`.
- CMake files: `CMakeLists.txt` per directory.

**Directories:**
- `port_<platform>/` for platform implementations.
- `tests/` subdirectories for tests.
- Lowercase with underscores for helper scripts (`docker/dbuild.sh`).

**Special Patterns:**
- Include guards: `PARAOS_<NAME>_HPP` / `_H`.
- Namespace: `paraos` for all public symbols.
- File names sometimes differ from include guard (e.g., `paraos_thread.hpp` uses guard `PARAOS_THREAD_V2_HPP`).

## Where to Add New Code

**New Core Primitive:**
- Public interface/common types: root `paraos_<name>.hpp`.
- Platform implementation: `port_unix/paraos_<name>.hpp`, `port_win/paraos_<name>.hpp`, `port_freertos/paraos_<name>.hpp`.
- Tests: `port_tests/test_<name>.cpp`.

**New Container:**
- Implementation: `containers/paraos_<name>.hpp`.
- Tests: `containers/tests/test_<name>.cpp`.
- Stress tests: `containers/tests/test_<name>_mpmc.cpp` or similar, labeled `stress` in CMake.

**New Helper:**
- Implementation: `extra/paraos_<name>.hpp` (and `.cpp` if needed).
- Tests: `extra/tests/test_<name>.cpp`.
- Docs: `extra/docs/<name>.md` if user-facing.

**New Port:**
- Add `port_<platform>/` directory with `CMakeLists.txt` and `config/etl_profile.h`.
- Hook into root `CMakeLists.txt` port-selection block.

## Special Directories

**`build/`:**
- Purpose: CMake build output for active presets.
- Source: Generated by CMake.
- Committed: No (`.gitignore`).

**`.cache/`:**
- Purpose: clangd index cache.
- Source: Generated by clangd extension.
- Committed: No (`.gitignore`).

**`paraos_version.hpp`:**
- Purpose: Generated version header.
- Source: CMake `configure_file` from `paraos_version.hpp.in`.
- Committed: No (`.gitignore`); regenerated at configure time.

---

*Structure analysis: 2026-06-20*
*Update when directory structure changes*
