# Coding Conventions

**Analysis Date:** 2026-06-20

## Naming Patterns

**Files:**
- Public headers: `paraos_<feature>.hpp` or `paraos_<feature>.h`.
- Implementation/test source: `test_<feature>.cpp`, `example_<feature>.cpp`, `paraos_status_led.cpp`.

**Types / Classes:**
- `PascalCase` for classes, structs, enums, and templates.
- Public API lives in namespace `paraos`.
- Example: `class Thread`, `class MutexGuard`, `struct ISRbool`.

**Functions / Methods:**
- `PascalCase` for public methods (observed style: `GiveName()`, `SetPriority()`, `RegisterDelegate()`, `TryEmplaceBack()`).
- `camelCase` for some helpers and macros.

**Variables / Members:**
- Private members use trailing underscore: `name_`, `handle_`, `is_need_while_`.
- Static members: `to_resume_`, `to_exit_`.
- Constants / constexpr: `kPascalCase` for enum values (`kIdle`, `kNormal`), `snake_case` for constexpr variables (`coop_scheduler_delay_ms`).

**Macros:**
- All library macros are prefixed with `PARAOS_` or `paraos`.
- `PARAOS_CHECK_ASSERT(x)`, `PARAOS_ATTR_UNUSED`, `PARAOS_POLYMORPHIC_EXTRA`, `paraosTRACE_MESSAGE(...)`.
- Attribute macros defined in `paraos_attr.h`.

**Include Guards:**
- Format: `PARAOS_<NAME>_HPP` or `PARAOS_<NAME>_H`.
- Some files use a historical guard name (e.g., `PARAOS_THREAD_V2_HPP`, `PARAOS_DEFERRED_DELETE_HPP`).

## Code Style

**Formatting:**
- `clang-format` with `.clang-format`.
- `BasedOnStyle: Google`.
- `AlignAfterOpenBracket: AlwaysBreak`.

**Linting / Static Analysis:**
- `clang-tidy` with `.clang-tidy`.
- Checks include `clang-analyzer-*`, `google-*`, `hicpp-*`, `llvm-*`, `misc-*`, `modernize-*`, `performance-*`, `portability-*`, `readability-*`.
- `WarningsAsErrors: '*'` — tidy warnings fail the build.
- Header filter applies to `*.hpp`.

**Compiler Warnings:**
- `-Wall -Wextra -Wpedantic -Werror` on the library and all test executables.
- Container tests add `-Wno-cpp` to suppress GCC 15 deprecation warning for `<ciso646>`.

## Language Standards

- C++17 for library and most tests.
- `test_paraos_core` in `port_tests/CMakeLists.txt` uses C++20.
- C11 for C sources.
- FreeRTOS kernel is compiled with C90.

## Documentation Comments

- Doxygen-style `///` comments with `@file`, `@brief`, `@param`, `@return`, `@note`, `@warning`.
- MIT license header in most files.
- Some build scripts and comments mix Russian and English.

## Error Handling

**Patterns:**
- `paraos::exception` for library errors, derived from `std::exception` and `etl::exception` (`paraos_exceptions.hpp`).
- `ETL_ASSERT(..., ETL_ERROR(paraos::thread_not_created_exception))` for hard failures in ETL macros.
- `PARAOS_CHECK_ASSERT(x)` / `PARAOS_CHECK_LOOP()` for runtime invariant checks in Debug.
- Methods return `bool` or `paraos::ISRbool` for soft failures (mutex timeout, queue full).

**Custom Exceptions:**
- Inherit from `paraos::exception`.
- Provide file name and line number through `etl::exception` constructor.

## Object Lifetime / Rule of Five

- Classes explicitly document the rule of five.
- Non-copyable / non-movable classes delete all five operations: `Thread`, `MutexGuard`, `CriticalSection`, `ICooperativeScheduling`, `IThreadSequence`.
- Move-only support exists in `Mutex`/`MutexRecursive`.

## Delegates and Callbacks

- `etl::delegate<void()>` is the standard delegate type (`thread_delegate_type`, `executor_delegate_type`).
- `etl::function` used for member-function callbacks (e.g., idle callback in cooperative scheduler).
- Example: `paraos::thread_delegate_type::create<MyClass, &MyClass::Run>(*this)`.

## RAII

- `MutexGuard` locks on construction and unlocks on destruction.
- `CriticalSection` enters on construction and exits on destruction.
- `ProfilerRAII` / `ProfilerPeriodRAII` automate profiling start/stop.
- `gsl::finally` used for cleanup in `Thread::Make()`.

## Macros and Portability

- `PARAOS_ATTR_*` macros abstract compiler attributes.
- `PARAOS_INLINE_TRIVIAL`, `PARAOS_INLINE_OPERATIONS`, `PARAOS_INLINE_CRITICAL` control inlining.
- `PARAOS_DEPRECATED(msg)` wraps `[[deprecated]]` / `__attribute__((deprecated))`.
- `PARAOS_WORKAROUND(symbol, test)` guards compiler-specific workarounds.

## Module Design

- Headers are self-contained and include what they use.
- Root `paraos_common.hpp` exists as a lightweight aggregation header.
- Port headers are included indirectly via the active `port_*/paraos_*.hpp`.

## Comments

- Prefer English for public API documentation.
- Russian appears in some inline comments, build scripts, and generated templates.
- `// NOLINTBEGIN(...)` / `// NOLINTEND(...)` used to suppress local clang-tidy warnings.

---

*Convention analysis: 2026-06-20*
*Update when patterns change*
