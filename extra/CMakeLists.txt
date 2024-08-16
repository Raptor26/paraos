cmake_minimum_required(VERSION 3.28)

project(paraos_extra VERSION 0.1.0)

add_library(${PROJECT_NAME} INTERFACE)

target_link_libraries(${PROJECT_NAME} INTERFACE paraos_containers)

target_include_directories(
  ${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/"
                            "${CMAKE_SOURCE_DIR}/")

if(UTEST)
  add_subdirectory(tests)
endif()
