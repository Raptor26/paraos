# Phase 19: Inventory & gap analysis - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `19-CONTEXT.md` — this log preserves the alternatives considered.

**Date:** 2026-06-22
**Phase:** 19-Inventory & gap analysis
**Areas discussed:** Test termination pattern, jthread lifetime management, FreeRTOS test finalization, Synchronization primitives in container tests

---

## Test termination pattern

| Option | Description | Selected |
|--------|-------------|----------|
| Move to `main()` | `main()` joins all worker threads and runs assertions; watcher thread removed | ✓ |
| Keep watcher on `jthread` | Watcher thread remains, signals `main()` via semaphore/atomic | |
| Split PC/FreeRTOS logic | Watcher only for FreeRTOS, `main()` for PC | |

**User's choice:** Move to `main()` — `main()` does `join()` and asserts. Watcher thread removed entirely.
**Notes:** User added that if the test does not finish within 10 seconds it must be considered failed. Clarified that the 10-second timeout is enforced by CTest (`TIMEOUT 10`) rather than inside `join()`.

---

## jthread lifetime management

| Option | Description | Selected |
|--------|-------------|----------|
| Local `std::vector<paraos::jthread>` in `main()` | Idiomatic RAII; destructor joins all threads | ✓ |
| Keep `Producer`/`Consumer` owning `jthread` | Replace internal `Thread` with `jthread`, keep static objects | |
| `std::optional` / `std::unique_ptr` wrapping | Manual lifetime control | |

**User's choice:** `std::vector<paraos::jthread>` in `main()` — the most idiomatic approach.
**Notes:** `Producer`/`Consumer` will become ordinary functors/lambdas.

---

## FreeRTOS test finalization

| Option | Description | Selected |
|--------|-------------|----------|
| Inner scope + `_Exit()` after destructors | Vector destroyed inside scope, then `std::_Exit(EXIT_SUCCESS)` on FreeRTOS | ✓ |
| Keep `_Exit()` as today | Call `_Exit()` directly, skipping remaining destructors | |
| `vTaskEndScheduler()` | Use FreeRTOS API to stop scheduler | |
| No `_Exit()` at all | Rely only on `~jthread()` / task deletion | |

**User's choice:** Option 5 — place `jthread` vector in an inner scope so destructors run before `_Exit()`, use `_Exit()` only on FreeRTOS, normal `return 0` on PC.
**Notes:** User explicitly wanted to avoid skipping destructors and asked what could be done for FreeRTOS; the inner-scope pattern was selected.

---

## Synchronization primitives in container tests

| Option | Description | Selected |
|--------|-------------|----------|
| Keep `CriticalSection` everywhere | Minimum changes; ISR-safe and already works | ✓ |
| Replace around `std::vector` with `paraos::mutex` | More idiomatic for non-ISR shared state | |
| Replace `PrintDebug` lock with `paraos::mutex` | `std::cout` does not need ISR safety | |
| Replace both | Full migration to std-like primitives | |

**User's choice:** Keep `CriticalSection` everywhere.
**Notes:** No migration of `CriticalSection` to `paraos::mutex` in this phase.

---

## Claude's Discretion

None — all major choices were made by the user.

## Deferred Ideas

None — discussion stayed within phase scope.
