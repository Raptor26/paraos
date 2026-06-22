# Requirements: PARAOS v1.4 std::semaphore-style Semaphore API

**Defined:** 2026-06-22
**Core Value:** Кроссплатформенная переносимость PARAOS сохраняется: код, работающий на Linux/Windows/FreeRTOS, продолжает работать, а новая macOS-разработка ведётся на равных с остальными платформами, включая статический анализ clang-tidy.

## v1 Requirements

### Semaphore API

- [x] **SEM-01**: `paraos::counting_semaphore<LeastMaxValue>` предоставляет `acquire()`, блокирующий вызов до появления ресурса.
- [x] **SEM-02**: `paraos::counting_semaphore<LeastMaxValue>` предоставляет `try_acquire()`, неблокирующий вызов, возвращающий `bool`.
- [x] **SEM-03**: `paraos::counting_semaphore<LeastMaxValue>` предоставляет `release(update = 1)`.
- [x] **SEM-04**: `paraos::counting_semaphore<LeastMaxValue>` предоставляет `try_acquire_for(std::chrono::duration<Rep, Period>&)`.
- [x] **SEM-05**: `paraos::counting_semaphore<LeastMaxValue>` предоставляет `try_acquire_until(std::chrono::time_point<Clock, Duration>&)`.
- [x] **SEM-06**: `paraos::counting_semaphore<LeastMaxValue>` предоставляет `static constexpr max()`.
- [x] **SEM-07**: `paraos::binary_semaphore` — тип-псевдоним для `paraos::counting_semaphore<1>`.
- [x] **SEM-08**: Семафор не копируется и не перемещается.

### Port Implementation

- [x] **SEM-09**: `port_pc/paraos_semaphore_std.hpp` реализует PC/Unix/Windows семафор поверх `std::counting_semaphore`.
- [x] **SEM-10**: `port_unix/paraos_semaphore_std.hpp` перенаправляет к `port_pc/paraos_semaphore_std.hpp`.
- [x] **SEM-11**: `port_win/paraos_semaphore_std.hpp` перенаправляет к `port_pc/paraos_semaphore_std.hpp`.
- [x] **SEM-12**: `port_freertos/paraos_semaphore_std.hpp` реализует семафор поверх FreeRTOS counting semaphore API.

### Build, Tests and Static Analysis

- [x] **SEM-13**: `port_tests/test_semaphore_std.cpp` создан, зарегистрирован в CTest и проходит на `pc_debug_clang` / `pc_debug_gcc`.
- [x] **SEM-14**: `*_clang_tidy` пресеты собираются без новых предупреждений.

## v2 Requirements

_No deferred requirements for this milestone._

## Out of Scope

| Feature | Reason |
|---------|--------|
| Замена legacy `paraos::SemaphoreCounting` / `paraos::SemaphoreBinary` | Новый API добавляется рядом со старым, полная замена — отдельная веха. |
| `std::latch` или `std::barrier` | Не входит в текущую веху. |
| Runtime-проверка на физических FreeRTOS-таргетах или Windows-хосте | Нет доступного хоста/таргета; проверяются host-сборки. |
| Изменения в `extra/` или `containers/` | Потребители не мигрируют в рамках v1.4. |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| SEM-01 | Phase 16 | Complete |
| SEM-02 | Phase 16 | Complete |
| SEM-03 | Phase 16 | Complete |
| SEM-04 | Phase 16 | Complete |
| SEM-05 | Phase 16 | Complete |
| SEM-06 | Phase 16 | Complete |
| SEM-07 | Phase 16 | Complete |
| SEM-08 | Phase 16 | Complete |
| SEM-09 | Phase 16 | Complete |
| SEM-10 | Phase 16 | Complete |
| SEM-11 | Phase 16 | Complete |
| SEM-12 | Phase 17 | Complete |
| SEM-13 | Phase 18 | Complete |
| SEM-14 | Phase 18 | Complete |

**Coverage:**
- v1 requirements: 14 total
- Mapped to phases: 14
- Unmapped: 0 ✓

---
*Requirements defined: 2026-06-22*
*Last updated: 2026-06-22 after milestone verification*
