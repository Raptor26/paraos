# Copyright (c) 2024 Stilsoft

# Autor Mickle Isaev

# Distributed under the MIT License

project(test_paraos_osal VERSION 0.1.0)

# Thread v2 --------------------------------------------------------------------
add_executable(test_paraos_thread test_paraos_thread.cpp)
target_link_libraries(test_paraos_thread PRIVATE paraos_osal)
add_test(NAME test_paraos_thread COMMAND test_paraos_thread)

# GTEST ------------------------------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(${PROJECT_NAME} test_paraos_semaphore.cpp test_paraos_mutex.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos_osal GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
