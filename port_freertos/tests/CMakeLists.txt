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

target_compile_features(test_rtos_thread_joined PRIVATE cxx_std_17 c_std_11)
target_compile_options(test_rtos_thread_joined PRIVATE -Wall -Wextra -Wpedantic
                                                       -Werror)

# Timer example ----------------------------------------------------------------
add_executable(example_paraos_timer example_paraos_timer.cpp)
target_link_libraries(example_paraos_timer PRIVATE paraos::paraos)

target_compile_features(example_paraos_timer PRIVATE cxx_std_17 c_std_11)
target_compile_options(example_paraos_timer PRIVATE -Wall -Wextra -Wpedantic
                                                    -Werror)

# GTEST ------------------------------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(${PROJECT_NAME} test_mutex.cpp test_semaphore.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE paraos::paraos GTest::gtest_main)
target_include_directories(${PROJECT_NAME}
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17 c_std_11)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
