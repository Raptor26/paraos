project(test_paraos_containers VERSION 0.1.0)

find_package(GTest REQUIRED)
include(GoogleTest)

# GTEST ------------------------------------------------------------------------
add_executable(
  ${PROJECT_NAME}
  test_message_buff.cpp test_queue.cpp test_queue_blocking.cpp
  test_queue_with_user_allocator.cpp test_message_buff_with_user_allocator.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos_containers
                                              GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")

gtest_discover_tests(${PROJECT_NAME} TEST_PREFIX "[PARAOS MESSAGE]:")

# Multithread ------------------------------------------------------------------
if(THREAD_ENABLE)
  add_executable(test_queue_multithread_one_producer_many_consumers
                 test_queue_multithread_one_producer_many_consumers.cpp)
  target_link_libraries(test_queue_multithread_one_producer_many_consumers
                        PRIVATE paraos_containers)

  add_executable(test_blocking_queue_multithread_many_prod_and_cons
                 test_blocking_queue_multithread_many_prod_and_cons.cpp)
  target_link_libraries(test_blocking_queue_multithread_many_prod_and_cons
                        PRIVATE paraos_containers)
endif()

add_test(NAME test_queue_multithread_one_producer_many_consumers
         COMMAND test_queue_multithread_one_producer_many_consumers)
