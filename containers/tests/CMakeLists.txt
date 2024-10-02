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
  test_message_buff.cpp test_queue_blocking.cpp
  test_message_buff_with_user_allocator.cpp test_lwrb.cpp
  test_multi_ringbuff.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos::paraos GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17 c_std_11)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)

gtest_discover_tests(${PROJECT_NAME} TEST_PREFIX "[PARAOS CONTAINERS]:")

# Multithread ------------------------------------------------------------------
if(THREAD_ENABLE)
  add_executable(test_queue_blocking_multithread_many_prod_and_cons
                 test_queue_blocking_multithread_many_prod_and_cons.cpp)
  target_link_libraries(test_queue_blocking_multithread_many_prod_and_cons
                        PRIVATE paraos::paraos)
  add_test(NAME test_queue_blocking_multithread_many_prod_and_cons
           COMMAND test_queue_blocking_multithread_many_prod_and_cons)

  # ----------------------------------------------------------------------------
  add_executable(test_message_multithread_many_producer_many_consumers
                 test_message_multithread_many_producer_many_consumers.cpp)
  target_link_libraries(test_message_multithread_many_producer_many_consumers
                        PRIVATE paraos::paraos)
  add_test(NAME test_message_multithread_many_producer_many_consumers
           COMMAND test_message_multithread_many_producer_many_consumers)

  # ----------------------------------------------------------------------------
  add_executable(test_lwrb_multithread_spsc test_lwrb_multithread_spsc.cpp)
  target_link_libraries(test_lwrb_multithread_spsc PRIVATE paraos::paraos)
  add_test(NAME test_lwrb_multithread_spsc COMMAND test_lwrb_multithread_spsc)

  # ----------------------------------------------------------------------------
  add_executable(test_multi_ringbuff_mpmc test_multi_ringbuff_mpmc.cpp)
  target_link_libraries(test_multi_ringbuff_mpmc PRIVATE paraos::paraos)
  add_test(NAME test_multi_ringbuff_mpmc COMMAND test_multi_ringbuff_mpmc)
endif()

# Benchmark --------------------------------------------------------------------
find_package(benchmark)
if(benchmark_FOUND)
  set(project_name_bench bench_paraos_containers)
  add_executable(${project_name_bench} bench_paraos_containers.cpp)
  target_link_libraries(${project_name_bench} PRIVATE benchmark::benchmark
                                                      paraos::paraos)
endif()
