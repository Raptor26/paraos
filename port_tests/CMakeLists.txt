# copyright Copyright (c) 2024 Stilsoft

# Autor Mickle Isaev

# Distributed under the MIT License

project(test_paraos_osal VERSION 0.1.0)

# Thread --------------------------------------------------------------------
add_executable(test_paraos_thread test_paraos_thread.cpp)
target_link_libraries(test_paraos_thread PRIVATE paraos::paraos)

target_compile_features(test_paraos_thread PRIVATE cxx_std_17 c_std_11)
target_compile_options(test_paraos_thread PRIVATE -Wall -Wextra -Wpedantic
                                                  -Werror)

add_test(NAME test_paraos_thread COMMAND test_paraos_thread)

# Timer ------------------------------------------------------------------------
# It's not test, so not registered as test in Ctest
add_executable(example_paraos_timer example_paraos_timer.cpp)
target_link_libraries(example_paraos_timer PRIVATE paraos::paraos)

target_compile_features(example_paraos_timer PRIVATE cxx_std_17 c_std_11)
target_compile_options(example_paraos_timer PRIVATE -Wall -Wextra -Wpedantic
                                                    -Werror)

# GTEST ------------------------------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(paraos_core test_paraos_mutex.cpp test_paraos_semaphore.cpp)
target_link_libraries(paraos_core PRIVATE paraos::paraos GTest::gtest_main)
target_include_directories(paraos_core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")

target_compile_features(paraos_core PRIVATE cxx_std_20 c_std_11)
target_compile_options(paraos_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME paraos_core COMMAND paraos_core)

# Coverage ---------------------------------------------------------------------
# ctest  --test-dir build/pc_debug_gcc  -T Test -T Coverage
# https://stackoverflow.com/questions/52255589/code-coverage-analysis-using-codecoverage-cmake-with-multiple-targets
if(CMAKE_COMPILER_IS_GNUCXX)
  include(${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake)
  append_coverage_compiler_flags()
  setup_target_for_coverage_lcov(NAME ${PROJECT_NAME}_coverage EXECUTABLE ctest
                                 --schedule-random)
endif()
