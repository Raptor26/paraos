#[=[
Copyright (c) 2024 Stilsoft
Autor Mickle Isaev
Distributed under the MIT License
]=]

cmake_minimum_required(VERSION 3.20)

target_include_directories(${PROJECT_NAME}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")

if(UTEST)
  add_subdirectory(tests)
endif()
