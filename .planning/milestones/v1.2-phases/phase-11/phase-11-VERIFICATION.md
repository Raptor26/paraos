---
phase: 11
status: passed
---

# Verification: Phase 11 — Thread attributes integration

## Verification Items

- [x] `paraos::jthread` constructor overload `jthread(const ThreadAttr&, Function&&, Args&&...)` added to PC and FreeRTOS ports.
- [x] The original `jthread(Function&&, Args&&...)` constructor delegates to the ThreadAttr overload with default attributes.
- [x] SFINAE prevents the original constructor from matching when the first argument is a `ThreadAttr`.
- [x] FreeRTOS `xTaskCreate` uses `attr.thread_name.data()`, `ConvertStackSizeInWords(attr.stack_depth)` and `static_cast<UBaseType_t>(attr.priority)`.
- [x] Unix PC attempts to set priority via `pthread_setschedparam(native_handle(), SCHED_RR, mapped_priority)` and ignores privilege failures.
- [x] Windows PC attempts to set priority via `SetThreadPriority(native_handle(), attr.priority)`.
- [x] PC smoke test with `ThreadAttr` compiled and ran successfully on macOS (`/usr/bin/clang++ -std=c++20 -DPARAOS_LIKE_UNIX`).
- [x] FreeRTOS smoke test with `ThreadAttr` compiled successfully (`/usr/bin/g++ -std=gnu++17` with FreeRTOS includes).
- [x] Existing `pc_debug_clang` preset builds successfully with system Clang.
- [x] Existing `freertos_debug_gcc` preset builds successfully.

## Test Commands

```bash
# PC ThreadAttr smoke
/usr/bin/clang++ -std=c++20 -DPARAOS_LIKE_UNIX -I. -Iport_unix -Iport_pc -Ietl/include \
  -Wall -Wextra -Wpedantic -Werror /tmp/jthread_attr_smoke.cpp -o /tmp/jthread_attr_smoke && /tmp/jthread_attr_smoke

# FreeRTOS ThreadAttr smoke compile
/usr/bin/g++ -DFREERTOS -DPARAOS_CHECK_LOOP_ENABLE -DPARAOS_LIKE_FREERTOS -DUNIT_TESTS_ENABLE -DfreeRTOS \
  -I. -Icontainers -Iextra -Iport_freertos/config -Ileaf/include -Ilwrb/lwrb/src/include \
  -isystem port_freertos -isystem GSL/include -isystem etl/include \
  -g -std=gnu++17 -arch arm64 -Wall -Wextra -Wpedantic -Werror \
  -c /tmp/freertos_jthread_attr_smoke.cpp -o /tmp/freertos_jthread_attr_smoke.o

# Existing presets
cmake --build build/pc_debug_clang/
cmake --build build/freertos_debug_gcc/
```

## Result

All smoke tests and preset builds passed.

## Notes

- CMake C++20 switch, test registration and static analysis are deferred to Phase 12.
- PC `thread_name` and `stack_depth` are stored but do not affect `std::jthread` (no standard API for name/stack on PC).
