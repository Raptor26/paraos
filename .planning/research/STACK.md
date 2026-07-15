# Stack Research

**Domain:** Unix timer refactor for PARAOS v1.10
**Researched:** 2026-06-25
**Confidence:** HIGH

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `paraos::jthread` | v1.2+ | Replace `pthread_create`/`pthread_join` and POSIX `timer_create` callback threads | Validated cross-platform `std::jthread` wrapper for PC/Unix; gives RAII join, `stop_token`, and consistent scheduler integration already used by `extra/` helpers. |
| `paraos::binary_semaphore` | v1.4+ | Replace `pthread_cond_signal`/`pthread_cond_timedwait` for wake-up and interruptible waits | Validated `std::counting_semaphore<1>` wrapper; `try_acquire_for()` provides an interruptible timed wait that works uniformly on Linux, macOS, and FreeRTOS. |
| `paraos::mutex` | v1.3+ | Replace `pthread_mutex_t` for protecting timer state (`period_ms_`, `is_running_`, `is_auto_reload_`) | Validated `std::mutex` wrapper; compatible with `std::lock_guard`/`std::unique_lock` and already the project-wide locking primitive. |
| `paraos::sleep_for` | v1.5+ | Optional fallback for unconditional sleeps | Cross-platform sleep already used in tests; delegates to `std::this_thread::sleep_for` on PC/Unix and `vTaskDelay` on FreeRTOS. |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `<chrono>` | C++20 | Express periods and timed waits as `std::chrono::milliseconds` | Required for `binary_semaphore::try_acquire_for()` and any `sleep_for` calls. |
| `<optional>` | C++20 | Hold the worker `paraos::jthread` only after `start()` | Use if lazy thread creation is preferred over default-constructed `jthread` member; not strictly required because `paraos::jthread` default-constructs to a non-joinable state. |
| `<atomic>` | C++20 | Lightweight stop/running flags without locking | Optional; can rely on `paraos::stop_token` from the worker and `paraos::mutex` for state changes to keep the design simple. |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| `clang-tidy` | Static analysis | Existing `.clang-tidy` runs with `WarningsAsErrors: '*'`; new code must avoid new suppressions. |
| CMake presets (`pc_debug_clang`, `pc_debug_gcc`, `*_clang_tidy`) | Build/test validation | Use these presets to verify no regressions on PC/Unix. |

## Installation

No new dependencies. All primitives are already part of the PARAOS source tree:

- `port_pc/paraos_jthread.hpp` (included via `port_unix/paraos_jthread.hpp`)
- `port_pc/paraos_mutex_std.hpp` (included via `port_unix/paraos_mutex_std.hpp`)
- `port_pc/paraos_semaphore_std.hpp` (included via `port_unix/paraos_semaphore_std.hpp`)
- `paraos_sleep.hpp` (root header)

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| `paraos::binary_semaphore` for interruptible waits | `std::condition_variable` | Could be used directly on PC, but it is not the project's validated cross-platform primitive and would reintroduce platform-specific code. |
| `paraos::jthread` worker loop | Keep POSIX `timer_create` on Linux | Rejected by the milestone: the goal is a single implementation for Linux and macOS, and POSIX timers are unavailable on macOS. |
| `paraos::sleep_for` in the worker loop | `paraos::binary_semaphore::try_acquire_for()` | `sleep_for` is simpler but not interruptible; use `binary_semaphore` when `change_period()`/`reset()`/`stop()` must wake the worker before the next expiry. |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| `timer_create` / `timer_settime` / `timer_delete` | Linux-only POSIX API; unavailable on macOS | `paraos::jthread` + `paraos::binary_semaphore` |
| `pthread_mutex_t` / `pthread_cond_t` | Platform-specific pthread APIs; the milestone explicitly removes them from `port_unix/paraos_timer.hpp` | `paraos::mutex` + `paraos::binary_semaphore` |
| `pthread_create` / `pthread_join` | Already replaced project-wide by `paraos::jthread` | `paraos::jthread` |
| Raw `std::thread` | Bypasses the project's validated scheduler integration and stop-token contract | `paraos::jthread` |
| New third-party timer libraries or OS APIs | Not needed; all required primitives already exist and are validated | Internal PARAOS primitives |

## Stack Patterns by Variant

**If the timer is periodic (`is_auto_reload == true`):**
- Worker loop waits on `binary_semaphore` for `period_ms_` using `try_acquire_for()`.
- On timeout, call `run()` and repeat until `stop_token.stop_requested()`.
- On wake (`try_acquire_for()` returns `true`), re-read protected state and restart the wait.

**If the timer is one-shot (`is_auto_reload == false`):**
- Worker performs one timed wait, calls `run()`, then exits the loop.
- No need to track an explicit `is_running_` bool beyond the joinability of the `jthread`.

**If `change_period()` or `reset()` is called while the timer is running:**
- Update `period_ms_` under `paraos::mutex`.
- Call `binary_semaphore::release()` to interrupt the current wait.
- The worker wakes, re-reads the period, and starts a new wait relative to "now".

**If `stop()` is called:**
- Request stop on the `jthread` (`request_stop()`).
- Release the semaphore to wake the worker immediately.
- Join the thread (RAII destructor of `jthread` also handles this).

## Version Compatibility

| Package/Primitive | Compatible With | Notes |
|-------------------|-----------------|-------|
| `paraos::jthread` | C++20 | Project minimum already C++20 since v1.2. |
| `paraos::binary_semaphore` | `port_pc/` Unix/Windows, `port_freertos/` | Alias of `counting_semaphore<1>`; validated in v1.4/v1.5. |
| `paraos::mutex` | `std::lock_guard`, `std::unique_lock` | Confirmed by `test_mutex_basic.cpp` and container stress tests. |
| `paraos::sleep_for` | PC/Unix/Windows/FreeRTOS | Root header with `#ifdef PARAOS_LIKE_FREERTOS` fallback. |

## Sources

- `port_unix/paraos_timer.hpp` — current Linux/macOS POSIX/pthread implementation to be replaced.
- `port_freertos/paraos_timer.hpp` — reference for the preserved public API (`start`, `stop`, `reset`, `change_period`, `run`, `is_auto_reload`).
- `port_win/paraos_timer.hpp` — reference for one-shot vs periodic handling using OS timer primitives.
- `port_pc/paraos_jthread.hpp` — validated `std::jthread` wrapper with `stop_token`/`stop_source`.
- `port_pc/paraos_mutex_std.hpp` — validated `std::mutex` wrapper.
- `port_pc/paraos_semaphore_std.hpp` — validated `std::counting_semaphore` wrapper and `binary_semaphore` alias.
- `paraos_sleep.hpp` — cross-platform `sleep_for` abstraction.
- `.planning/PROJECT.md` — milestone v1.10 goals and constraints.

---
*Stack research for: Unix timer refactor in PARAOS v1.10*
*Researched: 2026-06-25*
