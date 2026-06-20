---
phase: 9
status: passed
---

# Verification: Phase 9 — PC jthread implementation

## Verification Items

- [x] `port_pc/paraos_jthread.hpp` created with `paraos::jthread`, `paraos::stop_token`, `paraos::stop_source`.
- [x] `port_unix/paraos_jthread.hpp` forwards to PC implementation.
- [x] `port_win/paraos_jthread.hpp` forwards to PC implementation.
- [x] Constructor accepts callable + args and passes `paraos::stop_token` as the last argument.
- [x] `request_stop()`, `join()`, `joinable()` work as expected.
- [x] Destructor calls `request_stop()` and `join()` when joinable.
- [x] Copy is deleted; move is supported.
- [x] Smoke test compiled with `/usr/bin/clang++ -std=c++20` and ran successfully on macOS.

## Test Command

```bash
/usr/bin/clang++ -std=c++20 -I. -Iport_unix -Iport_pc -Wall -Wextra -Wpedantic -Werror /tmp/jthread_smoke.cpp -o /tmp/jthread_smoke && /tmp/jthread_smoke
```

## Result

Smoke test passed.

## Notes

- CMake integration, C++20 switch and CTest registration are deferred to Phase 12.
- FreeRTOS implementation is deferred to Phase 10.
- ThreadAttr integration is deferred to Phase 11.
