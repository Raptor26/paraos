#[=[
Copyright (c) 2024 Stilsoft
Autor Mickle Isaev
Distributed under the MIT License
]=]

cmake_minimum_required(VERSION 3.20)

target_include_directories(${PROJECT_NAME}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")

if(IS_PARAOS_STAND_ALONE_PROJECT)
  # Added user etl_profile.h in etl::etl library
  target_include_directories(etl
                             INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config/")
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE wsock32 ws2_32)

set(THREAD_ENABLE on)

if(UTEST)
  add_subdirectory(tests)
endif()
