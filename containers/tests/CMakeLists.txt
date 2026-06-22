#[=[
Copyright (c) 2024 Stilsoft
Autor Mickle Isaev
Distributed under the MIT License
]=]

project(test_paraos_containers VERSION 0.1.0)

find_package(GTest REQUIRED)
include(GoogleTest)

# GTEST ------------------------------------------------------------------------
add_executable(
  ${PROJECT_NAME}
  test_queue_blocking.cpp test_message_buff.cpp
  test_message_buff_with_user_allocator.cpp test_lwrb.cpp
  test_multi_ringbuff.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos::paraos GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17 c_std_11)

# '-Wno-cpp' added to suppress warnings after gcc update to 15.1.0:
#[=[
warning "<ciso646> is deprecated in C++17, use <version> to detect implementation-specific macros" [-Werror=cpp]
]=]
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror
                                               -Wno-cpp)

gtest_discover_tests(${PROJECT_NAME} TEST_PREFIX "[PARAOS CONTAINERS]:")

# Multithread ------------------------------------------------------------------
if(THREAD_ENABLE)
  # ----------------------------------------------------------------------------
  add_executable(test_message_multithread_many_producer_many_consumers
                 test_message_multithread_many_producer_many_consumers.cpp)
  target_link_libraries(test_message_multithread_many_producer_many_consumers
                        PRIVATE paraos::paraos)
  add_test(NAME test_message_multithread_many_producer_many_consumers
           COMMAND test_message_multithread_many_producer_many_consumers)

  set_tests_properties(test_message_multithread_many_producer_many_consumers
                       PROPERTIES LABELS stress TIMEOUT 20)

  # ----------------------------------------------------------------------------
  add_executable(test_multi_ringbuff_mpmc test_multi_ringbuff_mpmc.cpp)
  target_link_libraries(test_multi_ringbuff_mpmc PRIVATE paraos::paraos)
  add_test(NAME test_multi_ringbuff_mpmc COMMAND test_multi_ringbuff_mpmc)
  set_tests_properties(test_multi_ringbuff_mpmc PROPERTIES LABELS stress TIMEOUT 10)

  # ----------------------------------------------------------------------------
  add_executable(test_queue_blocking_spmc test_queue_blocking_spmc.cpp)
  target_link_libraries(test_queue_blocking_spmc PRIVATE paraos::paraos)
  add_test(NAME test_queue_blocking_spmc COMMAND test_queue_blocking_spmc)
  set_tests_properties(test_queue_blocking_spmc PROPERTIES LABELS stress TIMEOUT 10)

  # ----------------------------------------------------------------------------
  add_executable(test_queue_blocking_mpsc test_queue_blocking_mpsc.cpp)
  target_link_libraries(test_queue_blocking_mpsc PRIVATE paraos::paraos)
  add_test(NAME test_queue_blocking_mpsc COMMAND test_queue_blocking_mpsc)
  set_tests_properties(test_queue_blocking_mpsc PROPERTIES LABELS stress TIMEOUT 10)

  # ----------------------------------------------------------------------------
  add_executable(test_queue_blocking_mpmc test_queue_blocking_mpmc.cpp)
  target_link_libraries(test_queue_blocking_mpmc PRIVATE paraos::paraos)
  add_test(NAME test_queue_blocking_mpmc COMMAND test_queue_blocking_mpmc)
  set_tests_properties(test_queue_blocking_mpmc PROPERTIES LABELS stress TIMEOUT 10)
endif()

# Benchmark --------------------------------------------------------------------
find_package(benchmark)
if(benchmark_FOUND)
  set(project_name_bench bench_paraos_containers)
  add_executable(${project_name_bench} bench_paraos_containers.cpp)
  target_link_libraries(${project_name_bench} PRIVATE benchmark::benchmark
                                                      paraos::paraos)
endif()

if(CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY
                                                     "${DO_CLANG_TIDY}")

    if(THREAD_ENABLE)
      set_target_properties(
        test_message_multithread_many_producer_many_consumers
        PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

      set_target_properties(test_multi_ringbuff_mpmc
                            PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

      set_target_properties(test_queue_blocking_spmc
                            PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

      set_target_properties(test_queue_blocking_mpsc
                            PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

      set_target_properties(test_queue_blocking_mpmc
                            PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
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
