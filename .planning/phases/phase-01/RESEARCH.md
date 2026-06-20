# Phase 1 Research: macOS Compilation in `port_unix/`

**Date:** 2026-06-20
**Phase:** 1 — Diagnose and Fix macOS Compilation
**Researcher:** gsd-plan-phase

## Executive Summary

`port_unix/` cannot compile on macOS for two independent reasons:

1. **Missing POSIX APIs** — macOS does not implement POSIX interval timers (`timer_create`/`timer_settime`/`timer_delete`, `timer_t`, `itimerspec`, `SIGEV_THREAD`) or timed variants of mutex/semaphore waits (`pthread_mutex_timedlock`, `sem_timedwait`).
2. **Compiler target mismatch** — the `clang` first in `PATH` on this machine is the Arm Toolchain for Embedded (`aarch64-unknown-linux-gnu`). CMake adds `-arch arm64` for the Apple host, causing an immediate configure failure.

Both issues must be addressed for Phase 1 success criteria to pass on this machine.

## Evidence

### 1. Configure failure with default toolchain

```text
$ cmake --preset pc_debug_clang
...
clang: error: unsupported option '-arch' for target 'aarch64-unknown-linux-gnu'
```

`clang --version` confirms the compiler defaults to Linux:

```text
$ clang --version
clang version 22.1.0
Target: aarch64-unknown-linux-gnu
Thread model: posix
InstalledDir: /Users/raptor/ATfE-22.1.0-Darwin-universal/bin
```

### 2. Missing POSIX APIs on macOS

A minimal probe compiled with `/usr/bin/clang` on macOS fails for all four APIs used by `port_unix/`:

```c
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
```

| API | File | macOS status |
|-----|------|--------------|
| `timer_create` / `timer_t` / `itimerspec` / `SIGEV_THREAD` | `port_unix/paraos_timer.hpp` | **Not available** |
| `pthread_mutex_timedlock` | `port_unix/paraos_mutex.hpp` | **Not available** |
| `sem_timedwait` | `port_unix/paraos_semaphore.hpp` | **Not available** |
| `clock_gettime` | `port_unix/paraos_mutex.hpp`, `paraos_semaphore.hpp` | Available since macOS 10.12 |
| `pthread_mutex_trylock` / `pthread_mutex_lock` | `port_unix/paraos_mutex.hpp` | Available |
| `sem_trywait` / `sem_wait` / `sem_post` | `port_unix/paraos_semaphore.hpp` | Available |
| `pthread_*` | `port_unix/paraos_thread.hpp` | Available |
| BSD sockets | `port_unix/paraos_socket_udp.hpp` | Available |

## Affected Files

### `port_unix/paraos_timer.hpp`
- Uses `timer_create(CLOCK_REALTIME, &sev, &timer_id_)` to create a per-`Timer` thread via `SIGEV_THREAD`.
- Uses `timer_settime(timer_id_, ...)` for start/period/one-shot control.
- Uses `timer_delete(timer_id_)` in the destructor.
- All these symbols must be replaced on macOS.

### `port_unix/paraos_mutex.hpp`
- `MutexBase::Lock()` calls `pthread_mutex_timedlock(&m_obj_, &delay)` for finite timeouts.
- Must be replaced with a macOS-compatible wait loop or alternative primitive.

### `port_unix/paraos_semaphore.hpp`
- `SemaphoreBase::Take()` calls `sem_timedwait(&handle_, &delay)` for finite timeouts.
- Must be replaced with a macOS-compatible wait loop or alternative primitive.

### `port_unix/paraos_utils.hpp`
- Provides `MillisecondsInTimeSpec()` and `TimespecAdd()` used by the timed waits above.
- These helpers remain valid on macOS because `timespec` and `clock_gettime` are available.

## Compatibility Strategy Options

### Timers

| Option | Pros | Cons |
|--------|------|------|
| **A. Grand Central Dispatch (`dispatch_source_t`)** | Native, accurate, handles periodic/one-shot, cancellation | Apple-only, requires `<dispatch/dispatch.h>` |
| **B. Dedicated `pthread` + `pthread_cond_timedwait`** | Matches existing thread-per-timer model, no extra framework | `pthread_cond_timedwait` on macOS uses `gettimeofday` (wall clock); must handle spurious wakeups and stop/reset |
| **C. Dedicated `pthread` + `usleep` polling** | Simplest to implement | Wastes CPU, poor precision at short periods |

**Recommendation:** Option B (dedicated `pthread` + condition variable). It preserves the Linux behavior of spawning a thread per timer and avoids introducing an Apple-only framework. Option C is acceptable only as a fallback if B proves unstable during verification.

### Mutex Timed Lock

| Option | Pros | Cons |
|--------|------|------|
| **A. `pthread_mutex_trylock` loop with `usleep`** | Minimal code change, keeps `pthread_mutex_t` | Busy-wait-ish, coarse timeout granularity |
| **B. `pthread_cond_t` guard** | Proper blocking wait | Requires restructuring `MutexBase` to add a condition variable and a flag |

**Recommendation:** Option A. `port_unix` is a PC/test port; a small sleep-backed retry loop is acceptable and keeps the change localized. Use `clock_gettime(CLOCK_MONOTONIC)` when available, otherwise `CLOCK_REALTIME`, to compute remaining timeout.

### Semaphore Timed Wait

| Option | Pros | Cons |
|--------|------|------|
| **A. `sem_trywait` loop with `usleep`** | Minimal change, keeps `sem_t` | Same granularity concerns as mutex |
| **B. `dispatch_semaphore_t`** | Native and efficient | Apple-only, changes max-count semantics, requires reimplementing counting/binary behavior |

**Recommendation:** Option A. Keep the existing `sem_t` backend and add a macOS-only retry loop.

### Compiler Target Mismatch

| Option | Pros | Cons |
|--------|------|------|
| **A. Use system `/usr/bin/clang` for validation** | No preset changes, standard macOS toolchain | Requires user to override `CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` or adjust `PATH` |
| **B. Add `-target arm64-apple-darwin` to compile flags** | Works with the Arm toolchain in `PATH` | May conflict with other targets; changes build flags for all users |
| **C. Add a macOS toolchain file / helper script** | Clean separation | Adds new file; must be documented |

**Recommendation:** Option A for Phase 1. The preset defines `clang`; on a standard macOS install that resolves to `/usr/bin/clang`. The current environment has a custom toolchain prepended to `PATH`. Validation will use explicit compiler overrides (`-D CMAKE_C_COMPILER=/usr/bin/clang -D CMAKE_CXX_COMPILER=/usr/bin/clang++`). If this proves insufficient, Option C can be revisited in Phase 4.

## Open Questions

1. Does the project permit using `CLOCK_MONOTONIC` on macOS, or must `CLOCK_REALTIME` be used for parity with the Linux code path?
2. Are there any existing tests that rely on sub-millisecond timer accuracy? If so, the retry-loop approach may need finer granularity.
3. Should the binary semaphore `is_given_` flag be protected by a separate mutex on macOS to avoid race conditions in the retry loop?

## References

- `port_unix/paraos_timer.hpp`
- `port_unix/paraos_mutex.hpp`
- `port_unix/paraos_semaphore.hpp`
- `port_unix/paraos_utils.hpp`
- `.planning/codebase/CONCERNS.md` (macOS build unsupported)
- macOS `pthread.h`, `semaphore.h`, `time.h` headers (probe executed 2026-06-20)
