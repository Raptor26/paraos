#[=[
Copyright (c) 2024 Stilsoft
Autor Mickle Isaev
Distributed under the MIT License
]=]

cmake_minimum_required(VERSION 3.20)

target_sources(${PROJECT_NAME} PRIVATE paraos_status_led.cpp)

target_include_directories(${PROJECT_NAME}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")

if(PARAOS_TESTS)
  add_subdirectory(tests)
endif()
