project(test_paraos_osal VERSION 0.1.0)

# Thread v2 --------------------------------------------------------------------
add_executable(test_thread_v2 test_thread_v2.cpp)
target_link_libraries(test_thread_v2 PRIVATE paraos_osal)
add_test(NAME test_thread_v2 COMMAND test_thread_v2)

# GTEST ------------------------------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(${PROJECT_NAME} test_semaphore.cpp test_mutex.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos_osal GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
