# copyright Copyright (c) 2024 Stilsoft

# Autor Mickle Isaev

# Distributed under the MIT License

project(test_paraos_osal VERSION 0.1.0)

# Thread only static -----------------------------------------------------------
add_executable(test_paraos_thread_only_static
               test_paraos_thread_only_static.cpp)
target_link_libraries(test_paraos_thread_only_static PRIVATE paraos::paraos)

target_compile_features(test_paraos_thread_only_static PRIVATE cxx_std_17
                                                               c_std_11)
target_compile_options(test_paraos_thread_only_static
                       PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_test(NAME test_paraos_thread_only_static
         COMMAND test_paraos_thread_only_static)

# Thread only global -----------------------------------------------------------
add_executable(test_paraos_thread_only_global
               test_paraos_thread_only_global.cpp)
target_link_libraries(test_paraos_thread_only_global PRIVATE paraos::paraos)

target_compile_features(test_paraos_thread_only_global PRIVATE cxx_std_17
                                                               c_std_11)
target_compile_options(test_paraos_thread_only_global
                       PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_test(NAME test_paraos_thread_only_global
         COMMAND test_paraos_thread_only_global)

# Thread only stack ------------------------------------------------------------
add_executable(test_paraos_thread_only_stack test_paraos_thread_only_stack.cpp)
target_link_libraries(test_paraos_thread_only_stack PRIVATE paraos::paraos)

target_compile_features(test_paraos_thread_only_stack PRIVATE cxx_std_17
                                                              c_std_11)
target_compile_options(test_paraos_thread_only_stack PRIVATE -Wall -Wextra
                                                             -Wpedantic -Werror)

add_test(NAME test_paraos_thread_only_stack
         COMMAND test_paraos_thread_only_stack)
set_tests_properties(test_paraos_thread_only_stack PROPERTIES LABELS stress)

# Thread only stack with multiple threads in one object ------------------------
add_executable(
  test_paraos_thread_only_stack_with_multiple_threads_in_one_object
  test_paraos_thread_only_stack_with_multiple_threads_in_one_object.cpp)
target_link_libraries(
  test_paraos_thread_only_stack_with_multiple_threads_in_one_object
  PRIVATE paraos::paraos)

target_compile_features(
  test_paraos_thread_only_stack_with_multiple_threads_in_one_object
  PRIVATE cxx_std_17 c_std_11)
target_compile_options(
  test_paraos_thread_only_stack_with_multiple_threads_in_one_object
  PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_test(
  NAME test_paraos_thread_only_stack_with_multiple_threads_in_one_object
  COMMAND test_paraos_thread_only_stack_with_multiple_threads_in_one_object)
set_tests_properties(
  test_paraos_thread_only_stack_with_multiple_threads_in_one_object
  PROPERTIES LABELS stress)

# test_paraos_thread_create_then_delete_many_threads ---------------------------

add_executable(test_paraos_thread_create_then_delete_many_threads
               test_paraos_thread_create_then_delete_many_threads.cpp)
target_link_libraries(test_paraos_thread_create_then_delete_many_threads
                      PRIVATE paraos::paraos)

target_compile_features(test_paraos_thread_create_then_delete_many_threads
                        PRIVATE cxx_std_17 c_std_11)
target_compile_options(test_paraos_thread_create_then_delete_many_threads
                       PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Timer ------------------------------------------------------------------------
# It's not test, so not registered as test in Ctest
add_executable(example_paraos_timer example_paraos_timer.cpp)
target_link_libraries(example_paraos_timer PRIVATE paraos::paraos)

target_compile_features(example_paraos_timer PRIVATE cxx_std_17 c_std_11)
target_compile_options(example_paraos_timer PRIVATE -Wall -Wextra -Wpedantic
                                                    -Werror)

# Timeout check ----------------------------------------------------------------
# It's not test, so not registered as test in Ctest
add_executable(example_thread_check_timeout example_thread_check_timeout.cpp)
target_link_libraries(example_thread_check_timeout PRIVATE paraos::paraos)

target_compile_features(example_thread_check_timeout PRIVATE cxx_std_17
                                                             c_std_11)
target_compile_options(example_thread_check_timeout PRIVATE -Wall -Wextra
                                                            -Wpedantic -Werror)

if(NOT FREERTOS)
  # UDP Socket -----------------------------------------------------------------
  add_executable(example_paraos_socket_udp example_paraos_socket_udp.cpp)
  target_link_libraries(example_paraos_socket_udp PRIVATE paraos::paraos)

  target_compile_features(example_paraos_socket_udp PRIVATE cxx_std_17 c_std_11)
  target_compile_options(example_paraos_socket_udp PRIVATE -Wall -Wextra
                                                           -Wpedantic -Werror)
endif()

# GTEST ------------------------------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(
  test_paraos_core
  test_paraos_mutex.cpp test_paraos_semaphore.cpp test_runtime_profiler.cpp
  test_paraos_switch_context.cpp test_paraos_testing_semaphore.cpp
  test_paraos_version.cpp)
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

    set_target_properties(example_paraos_timer PROPERTIES CXX_CLANG_TIDY
                                                          "${DO_CLANG_TIDY}")

    set_target_properties(test_paraos_thread_only_static
                          PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

    set_target_properties(test_paraos_thread_only_global
                          PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

    set_target_properties(test_paraos_thread_only_stack
                          PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

    set_target_properties(
      test_paraos_thread_only_stack_with_multiple_threads_in_one_object
      PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

    if(NOT FREERTOS)
      set_target_properties(example_paraos_socket_udp
                            PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    endif()
  else()
    message(STATUS "clang-tidy not found.")
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
