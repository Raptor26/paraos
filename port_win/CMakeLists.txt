# Copyright (c) 2024 Stilsoft

# Autor Mickle Isaev

# Distributed under the MIT License

cmake_minimum_required(VERSION 3.28)

project(paraos_osal VERSION 0.1.0)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(
  ${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(${PROJECT_NAME} INTERFACE paraos_setup Microsoft.GSL::GSL
                                                etl::etl)

if(IS_PARAOS_STAND_ALONE_PROJECT)
  # Added user etl_profile.h in etl::etl library
  target_include_directories(etl
                             INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config/")
else()
  message(
    FATAL_ERROR
      "User project must provide etl_profile.h file and link them with etl target"
  )
endif()

set(THREAD_ENABLE on)

if(UTEST)
  add_subdirectory(tests)
endif()
