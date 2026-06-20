# External Integrations

**Analysis Date:** 2026-06-20

## APIs & External Services

PARAOS is an OS abstraction layer; it does not call external HTTP APIs, payment processors, identity providers, or message brokers. Network code is limited to a local UDP socket abstraction over Berkeley sockets / WinSock.

**Networking (internal):**
- UDP sockets — `port_unix/paraos_socket_udp.hpp`, `port_win/paraos_socket_udp.hpp`
  - Integration method: Berkeley sockets on POSIX, WinSock2 on Windows.
  - No external endpoints; tests use loopback.

## Data Storage

**Databases:**
- None.

**File Storage:**
- None at the library level. Some example/test code in `port_tests/` uses the standard file system or UDP for demonstration only.

**Caching:**
- None.

## Authentication & Identity

- None. No auth provider, OAuth, or JWT handling.

## Monitoring & Observability

**Error Tracking:**
- None. Errors are reported via `paraos::exception` (derived from `std::exception` and `etl::exception`) and optional trace macros.

**Analytics:**
- None.

**Logs:**
- Optional trace output controlled by `paraosTRACE_ENABLE`.
- `paraosTRACE_MESSAGE` writes to `std::cout` under a global critical section.
- No external log aggregation; output is local stdout only.

## CI/CD & Deployment

**CI Pipeline:**
- GitLab CI (`.gitlab-ci.yml`)
  - Stages: `test_windows`, `test_windows_freertos`, `test_windows_stress`, `test_linux`, `test_linux_valgrind`, `test_linux_freertos`, `test_linux_stress`, plus FreeRTOS stress variants.
  - Uses `pybuilder/pycmakebuilder.py` to filter presets by include/exclude regex and pass `ctest` options (`--output-on-failure`, `--stop-on-failure`, `--schedule-random`, `--timeout 20`, `--repeat-until-fail`).
  - Valgrind memcheck stage runs `ctest -T memcheck -j4` for `pc_debug_gcc`.

**Hosting / Deployment:**
- No runtime deployment. The deliverable is the source tree.
- Docker support via `Dockerfile` and `docker/*.sh` helpers for CI-style builds and tests.

## Environment Configuration

**Development:**
- No env-file secrets. Configuration is via CMake cache variables and presets.
- Required tools: CMake ≥ 3.28, Ninja, Clang/GCC, GTest, optional benchmark/Valgrind/clang-tidy/cppcheck.
- `builder.py` provides an interactive menu and non-interactive `-a <action>` mode.

**Staging / Production:**
- Consumers configure the library through CMake options (`RTOS_NAME`, `FREERTOS_PORT`, `TRACE`, etc.).
- Secrets management is not applicable; the library performs no external authentication.

## Webhooks & Callbacks

- None.

## Third-Party Code Ingestion

- `leaf/`, `etl/`, `GSL/`, `lwrb/`, and `port_freertos/FreeRTOS-Kernel/` are vendored dependencies managed as git-subrepos (each contains a `.gitrepo` file).
- They are added as CMake subdirectories only if the parent project has not already defined matching targets (`Boost::leaf`, `etl::etl`, `Microsoft.GSL::GSL`, `lwrb_ex`, `freertos_kernel`).

---

*Integration audit: 2026-06-20*
*Update when adding/removing external services or CI stages*
