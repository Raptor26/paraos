# Codebase Concerns

**Analysis Date:** 2026-06-20

## Tech Debt

**macOS build unsupported**
- Issue: `port_unix` uses POSIX timers (`timer_create`, `timer_delete`, `itimerspec`, `timer_t`) that are not available on macOS.
- Files: `port_unix/paraos_timer.hpp`, `port_unix/paraos_utils.hpp` (indirectly).
- Why: Implementation targeted Linux/PC POSIX, not portable POSIX.
- Impact: Native macOS builds fail; contributors must use Linux or Docker.
- Fix approach: Provide a macOS-compatible port (e.g., using `dispatch` or `kqueue`) or document the limitation and require Docker/Linux.

**Docker preset mismatch**
- Issue: `Dockerfile` and docker entrypoint scripts reference `pc_debug_clang_docker`, but `CMakePresets.json` only defines `pc_debug_clang`.
- Files: `Dockerfile`, `docker/run_docker_tests.sh`, `docker_tests_entrypoint_single.sh`, `docker_tests_entrypoint_stress.sh`, `docker_tests_entrypoint_memcheck.sh`.
- Why: Preset was renamed or removed; Docker build was not updated.
- Impact: Docker-based CI and local test runs fail at configure time.
- Fix approach: Re-add `pc_debug_clang_docker` preset or update Docker scripts to use `pc_debug_clang`.

**Header include guard / file name mismatches**
- Issue: Several files have include guards or internal names that do not match the file name, making search and onboarding harder.
- Files:
  - `paraos_base.hpp` uses guard `PARAOS_DEFERRED_DELETE_HPP`.
  - `paraos_thread_common.hpp` originally named `paraos_thread_v2.hpp` historically; `port_*/paraos_thread.hpp` also uses guard `PARAOS_THREAD_V2_HPP`.
- Why: Refactors renamed files without updating guards.
- Impact: Confusion when grepping; potential duplicate-symbol risk if copy-pasted.
- Fix approach: Align include guards with current file names.

**`std::vector` used in thread scheduler startup path**
- Issue: `port_unix/paraos_thread.hpp` stores created thread pointers in a static `std::vector<paraos::Thread*> to_resume_` until `StartScheduler()` is called.
- Files: `port_unix/paraos_thread.hpp`.
- Why: Convenience for PC test startup pattern.
- Impact: Dynamic allocation and non-ISR-safe structure in embedded-style code; inconsistent with ETL/no-heap philosophy.
- Fix approach: Replace with a fixed-capacity ETL container or allocate at construction.

**Runtime profiler brings `using namespace std::chrono_literals` into headers**
- Issue: `paraos_runtime_profiler.hpp` contains `using namespace std::chrono_literals;` inside a header.
- Files: `paraos_runtime_profiler.hpp`.
- Why: Convenience for literal syntax.
- Impact: Namespace pollution for consumers; can conflict with user code.
- Fix approach: Qualify literals (`std::chrono::microseconds`) or move `using` into an internal detail namespace.

## Known Bugs

**Critical section relies on a global recursive mutex**
- Symptoms: `DisableIsr()` / `EnableIsr()` on `port_unix` are implemented as lock/unlock of a recursive mutex, not actual interrupt masking.
- Trigger: Using `CriticalSection` or `VarAtomic` under the assumption that interrupts are disabled.
- Files: `port_unix/paraos_critical.hpp`.
- Workaround: On PC this is acceptable because there are no real hardware interrupts; on embedded the FreeRTOS port uses proper critical sections.
- Root cause: POSIX port does not model interrupts; the API is a best-effort serialization layer.

## Security Considerations

**No input validation on UDP socket example**
- Risk: `example_socket_udp.cpp` and the Python UDP test are demonstration code and do not validate packet sizes or origins.
- Files: `port_tests/example_socket_udp.cpp`, `port_tests/test_udp_socket.py`.
- Current mitigation: Examples are not part of the library target.
- Recommendations: Add bounds checks and warnings in example documentation; do not ship example code as production UDP server.

**Trace macros use `std::cout` under a global critical section**
- Risk: `paraosTRACE_MESSAGE` can cause priority inversion or deadlock if called from contexts where the critical section mutex is already held or from ISR-like contexts on ports that do not support recursion.
- Files: `paraos_trace.hpp`.
- Current mitigation: Disabled by default; enabled only via `TRACE=true`.
- Recommendations: Document ISR safety clearly and consider per-port trace backends.

## Performance Bottlenecks

**Global recursive mutex for every `CriticalSection`**
- Problem: All critical sections in the POSIX port serialize through one recursive mutex.
- Measurement: Not quantified, but it serializes unrelated subsystems.
- Cause: `static inline MutexRecursive mutex_` in `CriticalSection`.
- Improvement path: Use finer-grained mutexes per subsystem, or use true interrupt masking on capable platforms.

**Thread startup waits on a binary semaphore until `StartScheduler()`**
- Problem: Every created thread blocks on `sem_` until the application explicitly starts the scheduler.
- Cause: Cooperative scheduler / test-oriented design.
- Improvement path: Optionally auto-start threads when no scheduler is in use.

## Fragile Areas

**FreeRTOS port thread self-deletion**
- Files: `port_freertos/paraos_thread.hpp`.
- Why fragile: `RunThreadContext()` copies the handle, nulls the member under a critical section, then calls `vTaskDelete(handle)`. If object lifetime is not carefully managed, the task may delete itself while still referencing stack memory.
- Common failures: Use-after-free or missed deletion in tests.
- Safe modification: Keep object lifetime rules documented and add explicit test coverage for heap-allocated threads.

**Polymorphic extra toggle**
- Files: `extra/paraos_thread_sequence.hpp`, `extra/paraos_thread_cooperative_scheduling.hpp`.
- Why fragile: `PARAOS_POLYMORPHIC_EXTRA` changes whether key methods are `virtual`. Tests compiled with the flag differ from library consumers without it; ABI is not stable.
- Safe modification: Always build tests and library with the same extra polymorphism setting, or avoid mixing virtual and non-virtual builds.

## Dependencies at Risk

**FreeRTOS-Kernel vendored subrepo**
- Risk: If the subrepo is not regularly updated, security or bug fixes from upstream FreeRTOS are missed.
- Impact: Embedded users inherit any kernel vulnerabilities.
- Migration plan: Track upstream FreeRTOS releases and update the subrepo on a schedule.

**GoogleTest / benchmark assumed installed**
- Risk: Tests fail if system GTest is missing or version-incompatible.
- Impact: Local development friction.
- Migration plan: Add a `FetchContent` fallback for GTest and benchmark, or document required system packages precisely.

## Missing Critical Features

**macOS port**
- Problem: No native macOS build path.
- Current workaround: Use Linux or Docker.
- Blocks: macOS-based contributors cannot run tests natively.
- Implementation complexity: Medium (rewrite timer/critical-section code).

**Consistent English documentation**
- Problem: Build scripts (`Dockerfile`, `builder.py`) and some comments mix Russian and English.
- Current workaround: Translators/readers must be bilingual.
- Blocks: Onboarding of non-Russian speakers.
- Implementation complexity: Low (documentation and comment translation).

## Test Coverage Gaps

**Windows port automated testing**
- What's not tested: `.gitlab-ci.yml` runs Windows jobs, but local/agent environment may not exercise `port_win`.
- Risk: WinAPI-specific regressions go unnoticed.
- Priority: Medium.
- Difficulty to test: Requires Windows runner or cross-compilation setup.

**ISR paths on PC ports**
- What's not tested: `is_isr=true` branches in mutex/semaphore/queue APIs on POSIX/WinAPI are mostly stubs (parameters marked unused).
- Risk: ISR-safe usage is only truly validated on FreeRTOS.
- Priority: Medium.
- Difficulty to test: POSIX does not have real ISR context; requires emulation or FreeRTOS hardware-in-the-loop tests.

**UDP socket example not registered as CTest**
- What's not tested: `example_socket_udp.cpp` is built but not run by CTest.
- Risk: Socket API regressions require manual verification.
- Priority: Low.
- Difficulty to test: Add a CTest test that runs the example with a short timeout.

---

*Concerns audit: 2026-06-20*
*Update as issues are fixed or new ones discovered*
