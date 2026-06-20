# copyright Copyright (c) 2024 Stilsoft

# Autor Mickle Isaev

# Distributed under the MIT License

project(test_paraos_osal VERSION 0.1.0)

# Thread only static -----------------------------------------------------------
add_executable(test_thread_only_static test_thread_only_static.cpp)
target_link_libraries(test_thread_only_static PRIVATE paraos::paraos)

target_compile_features(test_thread_only_static PRIVATE cxx_std_17 c_std_11)
target_compile_options(test_thread_only_static PRIVATE -Wall -Wextra -Wpedantic
                                                       -Werror)

add_test(NAME test_thread_only_static COMMAND test_thread_only_static)

# Thread only global -----------------------------------------------------------
add_executable(test_thread_only_global test_thread_only_global.cpp)
target_link_libraries(test_thread_only_global PRIVATE paraos::paraos)

target_compile_features(test_thread_only_global PRIVATE cxx_std_17 c_std_11)
target_compile_options(test_thread_only_global PRIVATE -Wall -Wextra -Wpedantic
                                                       -Werror)

add_test(NAME test_thread_only_global COMMAND test_thread_only_global)

# Thread only stack ------------------------------------------------------------
add_executable(test_thread_only_stack test_thread_only_stack.cpp)
target_link_libraries(test_thread_only_stack PRIVATE paraos::paraos)

target_compile_features(test_thread_only_stack PRIVATE cxx_std_17 c_std_11)
target_compile_options(test_thread_only_stack PRIVATE -Wall -Wextra -Wpedantic
                                                      -Werror)

add_test(NAME test_thread_only_stack COMMAND test_thread_only_stack)

# Thread only stack with multiple threads in one object ------------------------
add_executable(test_thread_only_stack_with_multiple_threads
               test_thread_only_stack_with_multiple_threads.cpp)
target_link_libraries(test_thread_only_stack_with_multiple_threads
                      PRIVATE paraos::paraos)

target_compile_features(test_thread_only_stack_with_multiple_threads
                        PRIVATE cxx_std_17 c_std_11)
target_compile_options(test_thread_only_stack_with_multiple_threads
                       PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_test(NAME test_thread_only_stack_with_multiple_threads
         COMMAND test_thread_only_stack_with_multiple_threads)

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

# test_thread_create_then_delete_many_threads ---------------------------

add_executable(test_thread_create_then_delete_many_threads
               test_thread_create_then_delete_many_threads.cpp)
target_link_libraries(test_thread_create_then_delete_many_threads
                      PRIVATE paraos::paraos)

target_compile_features(test_thread_create_then_delete_many_threads
                        PRIVATE cxx_std_17 c_std_11)
target_compile_options(test_thread_create_then_delete_many_threads
                       PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Timer ------------------------------------------------------------------------
# It's not test, so not registered as test in Ctest
add_executable(example_timer example_timer.cpp)
target_link_libraries(example_timer PRIVATE paraos::paraos)

target_compile_features(example_timer PRIVATE cxx_std_17 c_std_11)
target_compile_options(example_timer PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Timeout check ----------------------------------------------------------------
# It's not test, so not registered as test in Ctest
add_executable(example_thread_check_timeout example_thread_check_timeout.cpp)
target_link_libraries(example_thread_check_timeout PRIVATE paraos::paraos)

target_compile_features(example_thread_check_timeout PRIVATE cxx_std_17
                                                             c_std_11)
target_compile_options(example_thread_check_timeout PRIVATE -Wall -Wextra
                                                            -Wpedantic -Werror)

if(NOT RTOS_NAME STREQUAL FREERTOS)
  # UDP Socket -----------------------------------------------------------------
  add_executable(example_socket_udp example_socket_udp.cpp)
  target_link_libraries(example_socket_udp PRIVATE paraos::paraos)

  target_compile_features(example_socket_udp PRIVATE cxx_std_17 c_std_11)
  target_compile_options(example_socket_udp PRIVATE -Wall -Wextra -Wpedantic
                                                    -Werror)
endif()

# GTEST ------------------------------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(
  test_paraos_core
  test_mutex_raii.cpp
  test_mutex.cpp
  test_semaphore.cpp
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
    set_target_properties(example_thread_check_timeout
                          PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

    set_target_properties(example_timer PROPERTIES CXX_CLANG_TIDY
                                                   "${DO_CLANG_TIDY}")

    set_target_properties(test_thread_only_static PROPERTIES CXX_CLANG_TIDY
                                                             "${DO_CLANG_TIDY}")

    set_target_properties(test_thread_only_global PROPERTIES CXX_CLANG_TIDY
                                                             "${DO_CLANG_TIDY}")

    set_target_properties(test_thread_only_stack PROPERTIES CXX_CLANG_TIDY
                                                            "${DO_CLANG_TIDY}")

    set_target_properties(test_thread_only_stack_with_multiple_threads
                          PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

    if(NOT RTOS_NAME)
      set_target_properties(example_socket_udp PROPERTIES CXX_CLANG_TIDY
                                                          "${DO_CLANG_TIDY}")
    endif()
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
