# Copyright (c) 2024 Stilsoft

# Autor Mickle Isaev

# Distributed under the MIT License

cmake_minimum_required(VERSION 3.28)

project(test_paraos_extra VERSION 0.1.0)

find_package(GTest REQUIRED)
include(GoogleTest)

# GTEST ------------------------------------------------------------------------
add_executable(${PROJECT_NAME} test_paraos_work_queue.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos_extra GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")

gtest_discover_tests(${PROJECT_NAME} TEST_PREFIX "[PARAOS EXTRA]:")

# Multithread ------------------------------------------------------------------
if(THREAD_ENABLE)
  add_executable(test_paraos_work_queue_multithread
                 test_paraos_work_queue_multithread.cpp)
  target_link_libraries(test_paraos_work_queue_multithread PRIVATE paraos_extra)
  add_test(NAME test_paraos_work_queue_multithread
           COMMAND test_paraos_work_queue_multithread)

  # ----------------------------------------------------------------------------
  add_executable(test_paraos_thread_sequence test_paraos_thread_sequence.cpp)
  target_link_libraries(test_paraos_thread_sequence PRIVATE paraos_extra)
  add_test(NAME test_paraos_thread_sequence COMMAND test_paraos_thread_sequence)

  # ----------------------------------------------------------------------------
  add_executable(test_paraos_cooperative_scheduling
                 test_paraos_cooperative_scheduling.cpp)
  target_link_libraries(test_paraos_cooperative_scheduling PRIVATE paraos_extra)
  add_test(NAME test_paraos_cooperative_scheduling
           COMMAND test_paraos_cooperative_scheduling)
endif()
