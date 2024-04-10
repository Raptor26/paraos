project(test_paraos_win VERSION 0.1.0)

add_executable(thread_prototype_win prototype.cpp)
target_link_libraries(thread_prototype_win PRIVATE paraos)

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(${PROJECT_NAME} test_mutex.cpp test_semaphore.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")
