# PARAOS

**PARAOS** is a library that provides an abstraction layer over the APIs of various operating systems (OSAL), such as Windows, Linux, and FreeRTOS. It is specifically designed to simplify interactions with OSes in embedded systems, such as microcontrollers.

If you want to run your code written for microcontrollers on Windows/Linux in native mode (e.g., for testing purposes), **PARAOS** can help you achieve that.

## Table of Contents

- [PARAOS](#paraos)
  - [Table of Contents](#table-of-contents)
  - [Supported Operating Systems](#supported-operating-systems)
  - [Key Features](#key-features)
    - [Core Functionality:](#core-functionality)
    - [Auxiliary Tools:](#auxiliary-tools)
  - [Highlights](#highlights)
  - [Code Quality](#code-quality)
  - [Installation and Usage](#installation-and-usage)
    - [Installation](#installation)
    - [Usage Example](#usage-example)
    - [Running Tests](#running-tests)
  - [License](#license)
  - [Frequently Asked Questions (FAQ)](#frequently-asked-questions-faq)
    - [What is paraos?](#what-is-paraos)
    - [Can paraos be used on multiple platforms?](#can-paraos-be-used-on-multiple-platforms)
    - [What tools are needed to work with the library?](#what-tools-are-needed-to-work-with-the-library)
    - [What are the main advantages of paraos?](#what-are-the-main-advantages-of-paraos)
    - [Is paraos free to use?](#is-paraos-free-to-use)

## Supported Operating Systems

**PARAOS** supports the following operating systems:

- **Windows** (default)
- **Linux** (default)
- **macOS** (via `port_unix`, community-tested)
- **FreeRTOS** (requires configuration)

To use **FreeRTOS**, you need to define the CMake cache variable `FREERTOS=true` during the configuration process. FreeRTOS can also be used on **Windows** and **Linux** if specified during the FreeRTOS configuration. Ensure that the appropriate FreeRTOS port is selected by setting the variable `FREERTOS_PORT` accordingly.

An example of how to define these variables can be found in [Installation and Usage](#installation-and-usage).

> **macOS note:** The `port_unix` implementation now builds on macOS using native
> substitutes for unavailable POSIX timers and unnamed semaphores. On Apple
> Silicon machines where the default `clang` in `PATH` may target Linux, use
> AppleClang explicitly when configuring a Clang preset:
> ```bash
> cmake --preset pc_debug_clang \
>       -D CMAKE_C_COMPILER=/usr/bin/clang \
>       -D CMAKE_CXX_COMPILER=/usr/bin/clang++
> ```
> The FreeRTOS POSIX simulator presets currently hang at runtime on macOS and
> are considered unsupported on this platform; PC presets pass the full test
> suite.

## Key Features

The **PARAOS** library includes the following features:

### Core Functionality:
- **paraos_thread** — Thread management;
- **paraos_semaphore** — Semaphores;
- **paraos_mutex** — Mutexes;
- **paraos_timer** — Software timers.

### Auxiliary Tools:
- **paraos_queue_blocking** — Message queues;
- **paraos_message_buffer** — Message buffers;
- **paraos_multi_ringbuff** — Circular buffers.

## Highlights

- **Cross-platform** — Enables use on various operating systems.
- **RAII Support** — Simplifies resource management and ensures safer code.
- **SemVer** — Uses [Semantic Versioning](https://semver.org/) for version management.
- **Source Code** — Distributed as source code, compiled alongside the user's project, and does not require pre-installation on the developer's machine.

## Code Quality

To ensure high code quality, **PARAOS** employs:

- **Unit testing** to verify functionality;
- **Clang-tidy** static code analysis for maintaining code cleanliness and quality.

## Installation and Usage

### Installation

No installation is required for **PARAOS**. The library is distributed as source code and is compiled as part of the user's project.

### Usage Example

Below is an example of how to integrate **PARAOS** into your CMake-based project:

1. Clone the **PARAOS** repository into your project directory:
   ```bash
   git clone https://github.com/Raptor26/paraos.git
   ```

2. Add the following to your CMake configuration:

   ```cmake
   cmake_minimum_required(VERSION 3.20)

   project(MyProject LANGUAGES CXX)

   # If you want to configure the project for FreeRTOS, you can specify the variables like this:
   set(FREERTOS ON CACHE BOOL "Enable FreeRTOS support")
   set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "")
   set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
   add_library(freertos_config INTERFACE)
   # Additional build parameters for FreeRTOS can be passed through the freertos_config target

   # Add PARAOS as a subdirectory
   add_subdirectory(paraos)

   # Add your application executable
   add_executable(my_app main.cpp)

   # Link PARAOS to your application
   target_link_libraries(my_app PUBLIC paraos::paraos)
   ```

3. Use the library in your C++ code:

   ```cpp
   #include "paraos_jthread.hpp"
   #include "paraos_runtime_profiler.hpp"
   #include "paraos_mutex_raii.hpp"

   int main() {
       paraos::os_profiler profiler;
       profiler.start();
       // application code ...
       auto elapsed_us = profiler.stop();
       (void)elapsed_us;

       paraos::mutex mtx;
       {
           std::scoped_lock<paraos::mutex> guard{mtx};
           // protected access
       }

       return 0;
   }
   ```

   > **Note:** Starting with the next release, public C++ API names follow the
   > STL `snake_case` convention. Old `PascalCase` names are kept as
   > `PARAOS_DEPRECATED` aliases for one release cycle. See the migration
   > section in [CHANGELOG.md](CHANGELOG.md) for details.

### Running Tests

In most cases, users do not need to run **PARAOS** tests in their projects. However, if you want to execute the tests, follow these steps:

1. List available CMake presets using the command:
   ```bash
   cmake --list-presets
   ```

2. Select the desired preset and configure it using the command:
   ```bash
   cmake --preset <name-of-preset>
   ```

3. Build the configured preset:
   ```bash
   cmake --build build/<name-of-preset>
   ```

4. Run the tests using the command:
   ```bash
   ctest --test-dir build/<name-of-preset>
   ```

5. To run stress tests with 100 repetitions, use the command:
   ```bash
   ctest --test-dir build/<name-of-preset> -L stress --repeat-until-fail 100 --timeout 20 --schedule-random
   ```
   Stress tests are marked with the CTest label `stress`. The command above
   repeats each stress test up to 100 times or until a failure occurs.

## License

**PARAOS** is distributed under the [MIT License](LICENSE), allowing free use, modification, and distribution.

## Frequently Asked Questions (FAQ)

### What is paraos?
**PARAOS** is a library that provides an abstraction layer for interacting with various operating systems (OSAL), such as Windows, Linux, and FreeRTOS. It is designed to simplify development in embedded systems, such as microcontrollers.

### Can paraos be used on multiple platforms?
Yes, **PARAOS** supports cross-platform functionality, allowing you to run your code on both microcontrollers and Windows/Linux in native mode.

### What tools are needed to work with the library?
To integrate **PARAOS** into your project, you need CMake and a C++ compiler supporting at least C++17.

### What are the main advantages of paraos?
The main advantages include cross-platform support, RAII idiom for resource management, and a foundational set of features, such as thread management, semaphores, mutexes, and software timers.

### Is paraos free to use?
Yes, **PARAOS** is distributed under the MIT License, which allows free use, modification, and distribution.

---

For more detailed information about the library and its usage, visit the repository or contact the authors. We hope **PARAOS** will be a valuable addition to your project!
