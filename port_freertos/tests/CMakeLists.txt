#[=[
Copyright (c) 2024 Stilsoft
Autor Vyhodcev Egor
Distributed under the MIT License
]=]

project(test_freertos_paraos_osal VERSION 0.1.0)

# Thread -----------------------------------------------------------------------
add_executable(test_rtos_thread_joined test_thread_join.cpp)
target_link_libraries(test_rtos_thread_joined PRIVATE paraos::paraos)
add_test(NAME test_rtos_thread_joined COMMAND test_rtos_thread_joined)

# Timer example ----------------------------------------------------------------
add_executable(example_timer example_timer.cpp)
target_link_libraries(example_timer PRIVATE paraos::paraos)

# GTEST ------------------------------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(${PROJECT_NAME} test_mutex.cpp test_semaphore.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos::paraos GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
