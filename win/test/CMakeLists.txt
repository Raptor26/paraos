project(test_paraos_win VERSION 0.1.0)

add_executable(thread_prototype_win prototype.cpp)
target_link_libraries(thread_prototype_win PRIVATE paraos)
