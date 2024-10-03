#[=[
Copyright (c) 2024 Stilsoft
Autor Mickle Isaev
Distributed under the MIT License
]=]

cmake_minimum_required(VERSION 3.20)

project(test_paraos_extra VERSION 0.1.0)

find_package(GTest REQUIRED)
include(GoogleTest)

# Multithread ------------------------------------------------------------------
if(THREAD_ENABLE)
  add_executable(test_paraos_thread_sequence test_paraos_thread_sequence.cpp)
  target_link_libraries(test_paraos_thread_sequence PRIVATE paraos::paraos)
  add_test(NAME test_paraos_thread_sequence COMMAND test_paraos_thread_sequence)

  # ----------------------------------------------------------------------------
  add_executable(test_paraos_cooperative_scheduling
                 test_paraos_cooperative_scheduling.cpp)
  target_link_libraries(test_paraos_cooperative_scheduling
                        PRIVATE paraos::paraos)
  add_test(NAME test_paraos_cooperative_scheduling
           COMMAND test_paraos_cooperative_scheduling)
endif()
