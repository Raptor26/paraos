cmake_minimum_required(VERSION 3.28)

project(paraos_osal VERSION 0.1.0)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(
  ${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(${PROJECT_NAME} INTERFACE paraos_setup)

if(UTEST)
  add_subdirectory(tests)
endif()
