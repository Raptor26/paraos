# Copyright (c) 2024 Stilsoft

# Autor Mickle Isaev

# Distributed under the MIT License

cmake_minimum_required(VERSION 3.28)

include(setup.cmake)

project(paraos VERSION 0.1.0)

# Is paraos stand alone project?
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(paraos_setup
                               INTERFACE -DPARAOS_CHECK_LOOP_ENABLE)
  endif()

  # If paraos is standalone project, we always want use unit testing
  include(CTest)
  enable_testing()
endif()

add_subdirectory(leaf)

if(FREERTOS)
  add_subdirectory(freertos_port)
  add_subdirectory(FreeRTOS-Kernel)
elseif(WIN32)
  add_subdirectory(win)
elseif(UNIX)
  add_subdirectory(unix)
endif()

add_subdirectory(containers)
add_subdirectory(extra)
add_subdirectory(GSL)

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE paraos_containers)
