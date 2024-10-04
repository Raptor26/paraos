# Copyright (c) 2024 Stilsoft

# Autor Mickle Isaev

# Distributed under the MIT License

project(test_paraos_osal VERSION 0.1.0)

# Thread v2 --------------------------------------------------------------------
add_executable(test_paraos_thread test_paraos_thread.cpp )
target_link_libraries(test_paraos_thread PRIVATE paraos::paraos)
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

add_executable(${PROJECT_NAME} test_paraos_semaphore.cpp test_paraos_mutex.cpp test_paraos_timer.cpp test_paraos_utils.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos::paraos GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
