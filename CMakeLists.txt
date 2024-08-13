cmake_minimum_required(VERSION 3.28)

include(setup.cmake)

project(paraos VERSION 0.1.0)

if(UTEST)
  include(CTest)
  enable_testing()
endif()

add_subdirectory(leaf)

if(WIN32)
  option(THREAD_ENABLE "Thread support enable" ON)
  add_subdirectory(win)
elseif(UNIX)
  option(THREAD_ENABLE "Thread support enable" ON)
  add_subdirectory(unix)
endif()

add_subdirectory(containers)

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE paraos_containers)
