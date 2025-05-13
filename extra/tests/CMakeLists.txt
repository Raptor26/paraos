#[=[
Copyright (c) 2024 Stilsoft
Autor Mickle Isaev
Distributed under the MIT License
]=]

cmake_minimum_required(VERSION 3.20)

project(test_paraos_extra VERSION 0.1.0)

# GTEST ------------------------------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(${PROJECT_NAME} test_paraos_cooperative_scheduling.cpp)
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

gtest_discover_tests(${PROJECT_NAME} TEST_PREFIX "[PARAOS EXTRA]:")

# Multithread ------------------------------------------------------------------
if(THREAD_ENABLE)
  add_executable(test_paraos_thread_sequence test_paraos_thread_sequence.cpp)
  target_link_libraries(test_paraos_thread_sequence PRIVATE paraos::paraos)
  add_test(NAME test_paraos_thread_sequence COMMAND test_paraos_thread_sequence)

  # ----------------------------------------------------------------------------
  add_executable(test_paraos_cooperative_scheduling_thread
                 test_paraos_cooperative_scheduling_thread.cpp)
  target_link_libraries(test_paraos_cooperative_scheduling_thread
                        PRIVATE paraos::paraos)
  add_test(NAME test_paraos_cooperative_scheduling_thread
           COMMAND test_paraos_cooperative_scheduling_thread)
endif()

if(CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(test_paraos_extra PROPERTIES CXX_CLANG_TIDY
                                                       "${DO_CLANG_TIDY}")

    if(THREAD_ENABLE)
      set_target_properties(test_paraos_thread_sequence
                            PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

      set_target_properties(test_paraos_cooperative_scheduling_thread
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
