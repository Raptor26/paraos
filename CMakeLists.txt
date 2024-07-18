cmake_minimum_required(VERSION 3.28)

include(setup.cmake)

project(paraos VERSION 0.1.0)

if(WIN32)
  add_subdirectory(win)
elseif(UNIX)
  add_subdirectory(unix)
endif()

add_subdirectory(lockfree)
target_link_libraries(paraos INTERFACE lockfree)

if(UTEST)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()
