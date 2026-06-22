# Research: Features — std::mutex-style mutex for PARAOS

**Milestone:** v1.3 std::mutex-style Mutex API
**Dimension:** Feature table stakes and differentiators.

## Table stakes (must have for std::BasicLockable compatibility)

| Feature | Notes |
|---------|-------|
| `lock()` | Blocks until mutex is acquired. Required by `std::lock_guard`. |
| `try_lock()` | Non-blocking attempt; returns `bool`. Required by `std::unique_lock`. |
| `unlock()` | Releases ownership. Required by all RAII wrappers. |
| Non-copyable | `std::mutex` is non-copyable; RAII wrappers expect this. |
| Move semantics | Optional for `std::lock_guard` compatibility, but useful. |
| `std::lock_guard<paraos::mutex>` compatibility | Depends only on `lock()`/`unlock()`. |
| `std::unique_lock<paraos::mutex>` compatibility | Depends on `lock()`/`try_lock()`/`unlock()`. |
| `std::scoped_lock<paraos::mutex>` compatibility | Depends on `lock()`/`unlock()`. |

## Differentiators (not strictly required for this milestone)

| Feature | Notes |
|---------|-------|
| `try_lock_for` / `try_lock_until` | `std::timed_mutex` interface; out of scope for v1.3. |
| Recursive mutex variant | Existing `paraos::MutexRecursive` covers this use case; a `paraos::recursive_mutex` could be added later. |
| Native handle exposure | `std::mutex` does not expose a native handle universally; avoid. |
| Shared/timed mutex | `std::shared_mutex` / `std::shared_timed_mutex`; out of scope. |

## Anti-features

- Do not support copying — mutexes own OS resources.
- Do not expose `Lock(timeout_ms, is_isr)` API in the new wrapper; this is the existing `paraos::Mutex` API.
- Do not change the public API of existing `paraos::Mutex` / `MutexRecursive`.
