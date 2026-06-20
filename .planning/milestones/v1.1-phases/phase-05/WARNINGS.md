# Phase 5: clang-tidy Warning Classification

**Preset:** `pc_debug_gcc_clang_tidy` and `freertos_debug_gcc_clang_tidy` on macOS (Apple Clang 21 / Homebrew clang-tidy 22).

**Method:** Both presets were configured from a clean state and built with `ninja -k 0` so every translation unit that reached clang-tidy produced diagnostics. The logs were parsed and deduplicated by `(file, line, check)`.

## Summary

- `pc_debug_gcc_clang_tidy`: 56 unique warnings
- `freertos_debug_gcc_clang_tidy`: 45 unique warnings
- Cross-preset overlap: 45 unique warnings
- Unique to PC preset: 11
- Unique to FreeRTOS preset: 0

### Warnings by check

- `misc-use-internal-linkage`: 40
- `llvm-prefer-static-over-anonymous-namespace`: 30
- `readability-use-concise-preprocessor-directives`: 21
- `misc-multiple-inheritance`: 3
- `readability-redundant-parentheses`: 3
- `misc-override-with-different-visibility`: 2
- `llvm-use-ranges`: 2

## pc_debug_gcc_clang_tidy

### paraos_config.hpp:17

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### paraos_config.hpp:33

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### paraos_exceptions.hpp:60

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### paraos_exceptions.hpp:74

- **Check:** `misc-multiple-inheritance`
- **Message:** inheriting multiple classes that aren't pure virtual is discouraged
- **Classification:** false-positive
- **Proposed action:** Intentional design: paraos::exception must be catchable as both std::exception and etl::exception. Suppress with documented inline NOLINT in Phase 6.

### paraos_thread_common.hpp:37

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### paraos_thread_common.hpp:46

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### port_tests/example_timer.cpp:46

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'UserTimer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/example_timer.cpp:64

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'UserTimerWithCnt' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/example_thread_check_timeout.cpp:105

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### port_tests/example_socket_udp.cpp:135

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'NonBlockingSocketThread' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/example_socket_udp.cpp:208

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'EmptySocketThread' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/example_socket_udp.cpp:262

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'BlockingSocketThread' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/example_socket_udp.cpp:336

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'ForeverBlockingSocketThread' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/example_socket_udp.cpp:389

- **Check:** `readability-redundant-parentheses`
- **Message:** redundant parentheses around expression
- **Classification:** true-positive
- **Proposed action:** Remove redundant parentheses in Phase 6 or 7 depending on file location.

### port_tests/test_runtime_profiler.cpp:38

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Profiler' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/test_runtime_profiler.cpp:48

- **Check:** `misc-override-with-different-visibility`
- **Message:** visibility of function 'SetUp' is changed from protected in class 'Test' to public
- **Classification:** true-positive
- **Proposed action:** Change `SetUp()` visibility back to `protected` in Phase 7 (GoogleTest fixture convention).

### containers/paraos_queue_blocking.hpp:65

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifndef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### containers/tests/test_message_buff_with_user_allocator.cpp:42

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'MyAllocBuffer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:83

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:142

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:186

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:210

- **Check:** `llvm-use-ranges`
- **Message:** use an LLVM range-based algorithm
- **Classification:** true-positive
- **Proposed action:** Replace `std::find(begin, end, value)` with `std::ranges::find` in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:216

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:236

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### containers/tests/test_multi_ringbuff_mpmc.cpp:67

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CalcTotalBytesInStringArray' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:108

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:163

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:207

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'AssertsForTestComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:223

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:236

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### containers/tests/test_queue_blocking_spmc.cpp:76

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:122

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:163

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:174

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:187

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### extra/paraos_thread_cooperative_scheduling.hpp:67

- **Check:** `readability-redundant-parentheses`
- **Message:** redundant parentheses around expression
- **Classification:** true-positive
- **Proposed action:** Remove redundant parentheses in Phase 6 or 7 depending on file location.

### extra/paraos_thread_cooperative_scheduling.hpp:279

- **Check:** `misc-multiple-inheritance`
- **Message:** inheriting multiple classes that aren't pure virtual is discouraged
- **Classification:** false-positive
- **Proposed action:** Intentional design: CooperativeScheduling inherits interface + implementation mix. Suppress with documented inline NOLINT in Phase 6/7.

### containers/tests/test_queue_blocking_mpsc.cpp:78

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:122

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:165

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:176

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:189

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### containers/tests/test_queue_blocking_mpmc.cpp:77

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:123

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:162

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:171

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:184

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### extra/tests/test_oneshot_executor.cpp:36

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'MockDelegate' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### extra/tests/test_paraos_thread_sequence.cpp:148

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### extra/tests/test_paraos_thread_sequence.cpp:161

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:63

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Task1' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:84

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Task2' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:105

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Task3' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:126

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Idle' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:167

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:179

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

## freertos_debug_gcc_clang_tidy

### port_tests/example_timer.cpp:46

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'UserTimer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/example_timer.cpp:64

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'UserTimerWithCnt' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/example_thread_check_timeout.cpp:105

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/paraos_queue_blocking.hpp:65

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifndef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### port_tests/test_runtime_profiler.cpp:38

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Profiler' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/test_runtime_profiler.cpp:48

- **Check:** `misc-override-with-different-visibility`
- **Message:** visibility of function 'SetUp' is changed from protected in class 'Test' to public
- **Classification:** true-positive
- **Proposed action:** Change `SetUp()` visibility back to `protected` in Phase 7 (GoogleTest fixture convention).

### containers/tests/test_message_buff_with_user_allocator.cpp:42

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'MyAllocBuffer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:83

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:142

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:186

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:210

- **Check:** `llvm-use-ranges`
- **Message:** use an LLVM range-based algorithm
- **Classification:** true-positive
- **Proposed action:** Replace `std::find(begin, end, value)` with `std::ranges::find` in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:216

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:236

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### containers/tests/test_multi_ringbuff_mpmc.cpp:67

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CalcTotalBytesInStringArray' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:108

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:163

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:207

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'AssertsForTestComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:223

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:236

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### containers/tests/test_queue_blocking_spmc.cpp:76

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:122

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:163

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:174

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:187

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### extra/paraos_thread_cooperative_scheduling.hpp:67

- **Check:** `readability-redundant-parentheses`
- **Message:** redundant parentheses around expression
- **Classification:** true-positive
- **Proposed action:** Remove redundant parentheses in Phase 6 or 7 depending on file location.

### extra/paraos_thread_cooperative_scheduling.hpp:279

- **Check:** `misc-multiple-inheritance`
- **Message:** inheriting multiple classes that aren't pure virtual is discouraged
- **Classification:** false-positive
- **Proposed action:** Intentional design: CooperativeScheduling inherits interface + implementation mix. Suppress with documented inline NOLINT in Phase 6/7.

### containers/tests/test_queue_blocking_mpsc.cpp:78

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:122

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:165

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:176

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:189

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### extra/tests/test_oneshot_executor.cpp:36

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'MockDelegate' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:77

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:123

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:162

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:171

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:184

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### extra/tests/test_paraos_thread_sequence.cpp:148

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### extra/tests/test_paraos_thread_sequence.cpp:161

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:63

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Task1' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:84

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Task2' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:105

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Task3' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:126

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Idle' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:167

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:179

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

## Cross-preset overlap

### port_tests/example_timer.cpp:46

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'UserTimer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/example_timer.cpp:64

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'UserTimerWithCnt' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/example_thread_check_timeout.cpp:105

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### port_tests/test_runtime_profiler.cpp:38

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Profiler' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### port_tests/test_runtime_profiler.cpp:48

- **Check:** `misc-override-with-different-visibility`
- **Message:** visibility of function 'SetUp' is changed from protected in class 'Test' to public
- **Classification:** true-positive
- **Proposed action:** Change `SetUp()` visibility back to `protected` in Phase 7 (GoogleTest fixture convention).

### containers/paraos_queue_blocking.hpp:65

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifndef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### containers/tests/test_message_buff_with_user_allocator.cpp:42

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'MyAllocBuffer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:83

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:142

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:186

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:210

- **Check:** `llvm-use-ranges`
- **Message:** use an LLVM range-based algorithm
- **Classification:** true-positive
- **Proposed action:** Replace `std::find(begin, end, value)` with `std::ranges::find` in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:216

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_message_multithread_many_producer_many_consumers.cpp:236

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### containers/tests/test_multi_ringbuff_mpmc.cpp:67

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CalcTotalBytesInStringArray' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:108

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:163

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:207

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'AssertsForTestComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:223

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_multi_ringbuff_mpmc.cpp:236

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### containers/tests/test_queue_blocking_spmc.cpp:76

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:122

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:163

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:174

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_spmc.cpp:187

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### extra/paraos_thread_cooperative_scheduling.hpp:67

- **Check:** `readability-redundant-parentheses`
- **Message:** redundant parentheses around expression
- **Classification:** true-positive
- **Proposed action:** Remove redundant parentheses in Phase 6 or 7 depending on file location.

### extra/paraos_thread_cooperative_scheduling.hpp:279

- **Check:** `misc-multiple-inheritance`
- **Message:** inheriting multiple classes that aren't pure virtual is discouraged
- **Classification:** false-positive
- **Proposed action:** Intentional design: CooperativeScheduling inherits interface + implementation mix. Suppress with documented inline NOLINT in Phase 6/7.

### containers/tests/test_queue_blocking_mpsc.cpp:78

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:122

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:165

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:176

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpsc.cpp:189

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### containers/tests/test_queue_blocking_mpmc.cpp:77

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Producer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:123

- **Check:** `misc-use-internal-linkage`
- **Message:** struct 'Consumer' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:162

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'CheckIfTestSuccessfullyComplete' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:171

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### containers/tests/test_queue_blocking_mpmc.cpp:184

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### extra/tests/test_oneshot_executor.cpp:36

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'MockDelegate' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### extra/tests/test_paraos_thread_sequence.cpp:148

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### extra/tests/test_paraos_thread_sequence.cpp:161

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:63

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Task1' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:84

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Task2' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:105

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Task3' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:126

- **Check:** `misc-use-internal-linkage`
- **Message:** class 'Idle' can be moved into an anonymous namespace to enforce internal linkage
- **Classification:** true-positive
- **Proposed action:** Move file-local structs/classes into an anonymous namespace in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:167

- **Check:** `llvm-prefer-static-over-anonymous-namespace`
- **Message:** function 'ExitFromTest' is declared in an anonymous namespace; prefer using 'static' for restricting visibility
- **Classification:** true-positive
- **Proposed action:** Add `static` to file-local helper functions in anonymous namespaces in Phase 7.

### extra/tests/test_paraos_cooperative_scheduling_thread.cpp:179

- **Check:** `readability-use-concise-preprocessor-directives`
- **Message:** preprocessor condition can be written more concisely using '#ifdef'
- **Classification:** true-positive
- **Proposed action:** Replace `#if defined(...)` / `#if !defined(...)` with `#ifdef` / `#ifndef` in Phase 6.

## Third-party / excluded

No warnings in this section.

## Notes

- No `.clang-tidy` check categories were disabled during this phase.
- No temporary inline `// TODO(phase5)` suppressions were added to source files; this phase is purely diagnostic.
- FreeRTOS-Kernel POSIX simulator sources produced only compiler warnings (e.g., `inline variables are a C++17 extension` in `port_freertos/paraos_utils.hpp:67`), not clang-tidy diagnostics in the captured logs, so they are not listed here.
