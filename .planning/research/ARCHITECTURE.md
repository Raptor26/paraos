# Research: Architecture — std::mutex-style mutex for PARAOS

**Milestone:** v1.3 std::mutex-style Mutex API
**Dimension:** How `paraos::mutex` integrates with existing architecture.

## Integration points

### New files

| File | Purpose |
|------|---------|
| `port_pc/paraos_mutex.hpp` | Shared PC implementation as thin `std::mutex` wrapper. |
| `port_unix/paraos_mutex.hpp` | Forwarding header. |
| `port_win/paraos_mutex.hpp` | Forwarding header. |
| `port_freertos/paraos_mutex.hpp` | FreeRTOS-backed implementation. |
| `port_tests/test_mutex_basic.cpp` | Basic standalone test. |

### Public API surface

```cpp
namespace paraos {
class mutex {
 public:
  mutex() noexcept = default;
  ~mutex() = default;

  mutex(const mutex&) = delete;
  mutex& operator=(const mutex&) = delete;

  // move allowed or deleted — mimic std::mutex (std::mutex is non-movable)
  mutex(mutex&&) = delete;
  mutex& operator=(mutex&&) = delete;

  void lock();
  bool try_lock();
  void unlock();
};
}  // namespace paraos
```

### Relationship to existing `paraos::Mutex`

- `paraos::mutex` is a new, separate class.
- `paraos::Mutex` keeps its `Lock(timeout_ms, is_isr)` / `Unlock(is_isr)` API.
- Existing `MutexGuard` and `CriticalSection` continue to use `paraos::Mutex` / `MutexBase`.
- No migration of `extra/` or `containers/` in this milestone.

### Data flow with `std::lock_guard`

1. User code: `std::lock_guard<paraos::mutex> guard{m};`
2. `std::lock_guard` calls `m.lock()`.
3. On PC: forwarded to internal `std::mutex::lock()`.
4. On FreeRTOS: calls `xSemaphoreTake(handle_, portMAX_DELAY)`.
5. Scope exit: `std::lock_guard` calls `m.unlock()`.
6. On PC: forwarded to internal `std::mutex::unlock()`.
7. On FreeRTOS: calls `xSemaphoreGive(handle_)`.

## Build integration

- Add `test_mutex_basic.cpp` to `port_tests/CMakeLists.txt`.
- Use `cxx_std_20` for the test (consistent with `test_jthread_basic.cpp`).
- Attach `CXX_CLANG_TIDY` if `CLANG_TIDY_ENABLE` is on (copy pattern from `test_jthread_basic`).

## Suggested build order

1. Implement `port_pc/paraos_mutex.hpp`.
2. Add forwarding headers in `port_unix/` and `port_win/`.
3. Implement `port_freertos/paraos_mutex.hpp`.
4. Add `test_mutex_basic.cpp` and CMake wiring.
5. Run PC presets and `*_clang_tidy` presets.
