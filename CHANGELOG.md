## v0.8.0 (2025-01-21)

### Feat

- **clang_tidy**: В библиотеку добавлена поддержка clang-tidy

### Refactor

- **queue_blocking**: Изменена логика полученя данных из очереди

## v0.7.0 (2025-01-14)

### Feat

- **Semaphore**: Доработана реализация семафоров под различные платформы:

### Fix

- **pybuilder**: fixed errors, depends on linux/windows path styles
- **trace**: fixed trace config for freertos

### Refactor

- **UDP_socket**: Рефакторинг после code review:
- **UDP_Socket**: Рефакторинг UDP сокета для платформ win и unix:

## v0.6.0 (2025-01-10)

### Feat

- **unix-sem**: addded move semantic
- **win-sem**: added move semantic
- **freertos-sem**: added move semantic
- **win-mutex**: added move semantic
- **freertos-mutex**: added move semantic

### Fix

- **freertos-mutex**: Added atomic variables in mutex class

## v0.5.4 (2024-12-25)

### Fix

- **paraos**: fixed decltype in socket udp

## v0.5.3 (2024-12-25)

### Fix

- **socket**: delay variable has type as paraos::max_delay

## v0.5.2 (2024-12-23)

### Fix

- **paraos**: Now cmake in paraos generate paraos_version.hpp if paraos using as no stand alone project

## v0.5.1 (2024-12-23)

### Fix

- **containers**: Added atomic variable for counting producers and consumers threads

## v0.5.0 (2024-12-23)

### Feat

- **cmake**: CMake generate std::string_view with actual paraos version
- **cmake**: Cmake use commitizen config for getting project version
- **git**: Added commitizen json config in project
- Added runtime in cooperative scheduler
- Added SwitchContext class for automatically switch thread context if needed.
- Added RAII profiler for calculate calling period

### Fix

- In paraos check exist PARAOS_CHECK_LOOP() and PARAOS_CHECK_ASSERT() before definition
- Fixed calculate period in cooperative scheduler
- Delete macros
- In cooperative scheduler fixed return value from NotifyBive(). Now returned class, indicates is need switch context.

## v0.4.0 (2024-12-09)

### Feat

- Added RAII class for automatically start and stop runtime profiler.
- Added SBUS implementation
- Thread sequence ctor can don't create thread if user set this option. Useful for unit tests
- Added Peek() API in ring buff
- Added new API in ring buff
- Moved task scheduling from stavpilot
- Added new profiler version with only pointers on counters
- Added runtime profiler in cooperative scheduler
- Added more tests for runtime profiler
- Added profiler without templates

### Fix

- Fix paraos ring buff exception short message (if using etl)
- Fixed lwrb (in paraos) build error, when lwrb generate compiler error when try build with arm_gcc
- Catch anything in Pop() if moved object throw exception
- Fixed build errors
- Fix checking period in paraos_thread_cooperative_scheduling

## v0.3.0 (2024-10-31)

### Feat

- Added stack debug in freeRTOS
- Added Exit() method in paraos cooperative scheduler
- Added test for TryRead()
- Added check if producers and consumers offline before exit from thread
- Added atomic method for write data in multi ring buffer. Redesigned multi thread test
- Stress tests run when periodical start gitlab scheduler
- Added new blocking queue version with while cycle if wait new data in queue
- Added API for periodical check timeout and corrected remaining time
- Added unit tests, docs
- Added force read form multi ring buff in no queue space available for manage where data available
- Added stress presets for gcc compiler
- Added code coverage estimate when using gcc compiler
- RingbBuff params in MultiRingBuff set individually by template parameter packs.
- Added in blocking queue API bool flag <is_isr>
- Added static function for thread sleep
- -Wall -Wextra -Wpedantic -Werror flags added in tests for unix
- Added convertor from miliseconds to timespec
- Added timers in unix port
- If not read all available bytes from ring buff, we can read remaining bytes on next step
- Added total read and written bytes in ringbuff mpmc test

### Fix

- In cooperative scheduler etl scheduler used as base class, because etl scheduler must initialize before call ICooperativeScheduling() ctor
- Now CheckTimeout() for win/unix ports update delay timeout
- PARAOS_ConvertMsToTicks() return freeRTOS ticks type
- Fixed data race in binary semaphore (unix port)
- Fixed coverage conditions in cmake
- CodeCoverage.cmake comment FATAL_ERROR message
- Fixed build in trace config
- paraos config definitions moved inside #ifndef -- #endif directives
- Fix merge request train in gitlab
- Increased read buffer size in ringbuff mpmc
- Delete parse git tag message for determine library version in ETL

## v0.2.0 (2024-10-03)

### Feat

- Methods marked as virtual when using test in paraos thread
- Added stress tests
- Added stress test for linux if merge request
- Added gcc debug configure
- Added release configs for gitlab runners
- Added new ctest configs
- Moved socket windows wrapper in paraos
- Jobs for windows in linux run parallels on different runners
- Added tags for run specific platform runners
- Added skip method in ringbuff
- Added ctor exception in ring buffer
- Added base wrapper functions for ring buffer
- Added lwrb in cmake project
- Added gitlab runner yml test config
- Added throw catch
- Added loop in paraos macros in stavpilot project
- Added extra flags for test executable files
- Adder FreeRTOS timer API wrapper
- Added software timer API in windows port
- FreeRTOS semaphore use specific bool class for returned value, which indicated is need switch RTOS context
- Registered oscilloscope method in thread sequence
- Added in unix port binary mutex implementation
- Added cooperative scheduling in separate thread
- Added led blinker in paraos
- Notify give have parameter for calling from isr
- Added stress mode for test in bash script
- Added thread sequence implementation based etl::callback_timer
- Update builder script
- Added base functional for thread sequence execution
- Added functor class
- Added runtime profiler
- Added method for enable/disable loop for Run() method
- Added build bash script
- BoolAtomic as template specialization for generic VarAtomic class
- Thread::Start() now has return value
- Added work queue API
- Trace macros used critical section
- When call Thread::Thread() we can block join this thread
- Added IsSchedulerStarted() in unix
- Added Thread::Sleep()
- Added bench for message buffer
- Added bench for blocking queue version
- Added gsl in paraos
- Added paraos cmake library for simple linking for external projects
- Added boost::leaf in cmake
- Added optional for returned value by queue
- Added message buffer class
- Added mutex for blocking queue operations
- Added 'operator bool()' overload for unix semaphore
- Added blocking version trivial operations for queue
- Added deduced noexcept annotation for blocking Push() and Pop() methods
- Message buffer don't alloc memory if queue is full
- Added SetPriority for unix
- Added SetPriority in winapi
- Added queue version with blocking
- Added new test case for queue
- Added thread and semaphore wrappers
- Added mutex for unix
- Added operator bool for check container status after creation
- Added new thread wrapper
- Added multithread test for queue
- Added paraos queue
- Added trace messages macros
- Added config macros
- Add bash script for run tests with memcheck
- Added tests in CTest
- Added prototyping message buffer
- overload operator in safe thread bool variable
- Added binary mutex API
- Added trace cmake configuration
- Added mutex and sem base api
- Added RAII critical section
- Added cmake and clang-tidy

### Fix

- Added no atomic definition in lwrb when use GCC compiler
- Automatic start stress tests when merge requested
- Fixed build errors in freertos debug preset
- Fixed message copy tor
- Fixed memory leak in FreeRTOS semaphore
- Fixed building errors when build release version
- Fixed semaphore for FreeRTOS API wrapper
- Fixed delete timer API in paraos
- If semaphore create as recursive, then called specific freeRTOS method with *Recursive postfixum
- Restore timer callback without etl lock
- Added GetStackMinimumSizeInBytes() in unix port
- Fixed test for thread sequence
- Fixed errors after paraos update
- Fixed build errors
- Fixed test with memcheck
- Fixed test test_queue_multithread_one_producer_many_consumers for pc_debug windows
- fixed multithread tests in freertos port
- Fixed build errors
- Fixed build errors in some configurations
- Fixed gsl errors when using gcc base compiler for stm32
- Fixed build errors in unix
- Suppress warnings if build in linux with clang
- Update cmake and fix path to port folder
- Added check "is thread created" before try join thread
- Now we started test threads after class creation complete
- Deleted 'virtual auto Push(const T& element, std::size_t timeout_ms) -> bool = 0' interface.
- Delete force Join() when user run Start()
- Replaced sem_close() on sem_destroy()
- Fixed include directive
- pthread_exit() replaced by return operator
- Fixed run test errors in docker
- Fixed header include in blocking queue version
- Fixed thread priority in test
- Dtor call allocator::deallocate() if only ptr for memory not nullptr

### Refactor

- Refactoring thread delete operations
