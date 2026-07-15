# copyright Copyright (c) 2024 Stilsoft

# Autor Mickle Isaev

# Distributed under the MIT License

project(test_paraos_osal VERSION 0.1.0)

# API alias compile smoke test -------------------------------------------------
add_executable(test_api_aliases_compile test_api_aliases_compile.cpp)
target_link_libraries(test_api_aliases_compile PRIVATE paraos::paraos)

target_compile_features(test_api_aliases_compile PRIVATE cxx_std_17 c_std_11)
target_compile_options(
  test_api_aliases_compile PRIVATE -Wall -Wextra -Wpedantic -Werror
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-deprecated-declarations>)

add_test(NAME test_api_aliases_compile COMMAND test_api_aliases_compile
         TIMEOUT 20)

# Thread only static -----------------------------------------------------------
add_executable(test_thread_only_static test_thread_only_static.cpp)
target_link_libraries(test_thread_only_static PRIVATE paraos::paraos)

target_compile_features(test_thread_only_static PRIVATE cxx_std_20 c_std_11)
target_compile_options(test_thread_only_static PRIVATE -Wall -Wextra -Wpedantic
                                                       -Werror)

add_test(NAME test_thread_only_static COMMAND test_thread_only_static
         TIMEOUT 20)

# Thread only global -----------------------------------------------------------
add_executable(test_thread_only_global test_thread_only_global.cpp)
target_link_libraries(test_thread_only_global PRIVATE paraos::paraos)

target_compile_features(test_thread_only_global PRIVATE cxx_std_20 c_std_11)
target_compile_options(test_thread_only_global PRIVATE -Wall -Wextra -Wpedantic
                                                       -Werror)

add_test(NAME test_thread_only_global COMMAND test_thread_only_global
         TIMEOUT 20)

# Thread only stack ------------------------------------------------------------
add_executable(test_thread_only_stack test_thread_only_stack.cpp)
target_link_libraries(test_thread_only_stack PRIVATE paraos::paraos)

target_compile_features(test_thread_only_stack PRIVATE cxx_std_20 c_std_11)
target_compile_options(test_thread_only_stack PRIVATE -Wall -Wextra -Wpedantic
                                                      -Werror)

add_test(NAME test_thread_only_stack COMMAND test_thread_only_stack
         TIMEOUT 20)

# Thread only stack with multiple threads in one object ------------------------
add_executable(test_thread_only_stack_with_multiple_threads
               test_thread_only_stack_with_multiple_threads.cpp)
target_link_libraries(test_thread_only_stack_with_multiple_threads
                      PRIVATE paraos::paraos)

target_compile_features(test_thread_only_stack_with_multiple_threads
                        PRIVATE cxx_std_20 c_std_11)
target_compile_options(test_thread_only_stack_with_multiple_threads
                       PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_test(NAME test_thread_only_stack_with_multiple_threads
         COMMAND test_thread_only_stack_with_multiple_threads
         TIMEOUT 20)

# jthread basic ----------------------------------------------------------------
add_executable(test_jthread_basic test_jthread_basic.cpp)
target_link_libraries(test_jthread_basic PRIVATE paraos::paraos)

target_compile_features(test_jthread_basic PRIVATE cxx_std_20 c_std_11)
target_compile_options(test_jthread_basic PRIVATE -Wall -Wextra -Wpedantic
                                                  -Werror)

add_test(NAME test_jthread_basic COMMAND test_jthread_basic)

if(CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(test_jthread_basic PROPERTIES CXX_CLANG_TIDY
                                                        "${DO_CLANG_TIDY}")
  endif()
endif()

# mutex basic ------------------------------------------------------------------
add_executable(test_mutex_basic test_mutex_basic.cpp)
target_link_libraries(test_mutex_basic PRIVATE paraos::paraos)

target_compile_features(test_mutex_basic PRIVATE cxx_std_20 c_std_11)
target_compile_options(test_mutex_basic PRIVATE -Wall -Wextra -Wpedantic
                                                -Werror)

add_test(NAME test_mutex_basic COMMAND test_mutex_basic)

if(CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(test_mutex_basic PROPERTIES CXX_CLANG_TIDY
                                                      "${DO_CLANG_TIDY}")
  endif()
endif()

# semaphore std ----------------------------------------------------------------
add_executable(test_semaphore_std test_semaphore_std.cpp)
target_link_libraries(test_semaphore_std PRIVATE paraos::paraos)

target_compile_features(test_semaphore_std PRIVATE cxx_std_20 c_std_11)
target_compile_options(test_semaphore_std PRIVATE -Wall -Wextra -Wpedantic
                                                  -Werror)

add_test(NAME test_semaphore_std COMMAND test_semaphore_std)

if(CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(test_semaphore_std PROPERTIES CXX_CLANG_TIDY
                                                        "${DO_CLANG_TIDY}")
  endif()
endif()

# timer ------------------------------------------------------------------------
add_executable(test_timer test_timer.cpp)
target_link_libraries(test_timer PRIVATE paraos::paraos)

target_compile_features(test_timer PRIVATE cxx_std_20 c_std_11)
target_compile_options(test_timer PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_test(NAME test_timer COMMAND test_timer TIMEOUT 20)

if(CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(test_timer PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
  endif()
endif()

# GTEST ------------------------------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(
  test_paraos_core
  test_runtime_profiler.cpp
  test_switch_context.cpp
  test_testing_semaphore.cpp
  test_version.cpp
  test_isr.cpp
  test_critical.cpp)

target_link_libraries(test_paraos_core PRIVATE paraos::paraos GTest::gtest_main)
target_include_directories(test_paraos_core
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")

target_compile_features(test_paraos_core PRIVATE cxx_std_20 c_std_11)
target_compile_options(test_paraos_core PRIVATE -Wall -Wextra -Wpedantic
                                                -Werror)
add_test(NAME test_paraos_core COMMAND test_paraos_core)

if(CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(test_paraos_core PROPERTIES CXX_CLANG_TIDY
                                                      "${DO_CLANG_TIDY}")

    set_target_properties(test_thread_only_static PROPERTIES CXX_CLANG_TIDY
                                                             "${DO_CLANG_TIDY}")

    set_target_properties(test_thread_only_global PROPERTIES CXX_CLANG_TIDY
                                                             "${DO_CLANG_TIDY}")

    set_target_properties(test_thread_only_stack PROPERTIES CXX_CLANG_TIDY
                                                            "${DO_CLANG_TIDY}")

    set_target_properties(test_thread_only_stack_with_multiple_threads
                          PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
  else()
    message(FATAL_ERROR "clang-tidy not found.")
  endif()
endif()

# Coverage ---------------------------------------------------------------------
# ctest  --test-dir build/pc_debug_gcc  -T Test -T Coverage
# https://stackoverflow.com/questions/52255589/code-coverage-analysis-using-codecoverage-cmake-with-multiple-targets
if(CMAKE_COMPILER_IS_GNUCXX AND CODE_COVERAGE)
  include(CodeCoverage)
  append_coverage_compiler_flags()
  setup_target_for_coverage_lcov(NAME ${PROJECT_NAME}_coverage EXECUTABLE ctest
                                 --schedule-random)
endif()
