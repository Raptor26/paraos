project(test_paraos_containers VERSION 0.1.0)

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(${PROJECT_NAME} test_message_buff.cpp test_queue.cpp
                               test_queue_with_user_allocator.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos_containers
                                              GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")

gtest_discover_tests(${PROJECT_NAME} TEST_PREFIX "[PARAOS MESSAGE]:")
