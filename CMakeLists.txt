#[=[
Copyright (c) 2024 Stilsoft
Autor Mickle Isaev
Distributed under the MIT License
]=]

cmake_minimum_required(VERSION 3.20)

include(setup.cmake)

# Read project version from commitizen config file.
# https://stackoverflow.com/questions/68759588/parse-a-config-file-ini-or-cfg-with-cmake-to-create-a-header-file
file(READ "${CMAKE_CURRENT_LIST_DIR}/.cz.json" commitizen_json)
string(JSON paraos_project_version GET "${commitizen_json}" commitizen version)
message(STATUS "PARAOS project version is ${paraos_project_version}")

project(
  paraos
  VERSION ${paraos_project_version}
  DESCRIPTION
    "Cross-platform wrapper over operating system primitives with an emphasis on embedded systems"
  LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Is paraos standalone project?
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(paraos_setup
                               INTERFACE -DPARAOS_CHECK_LOOP_ENABLE)
  endif()

  option(IS_PARAOS_STAND_ALONE_PROJECT "Use paraos version for etl_profile.h"
         ON)

  # If paraos is standalone project, we want use unit testing only if debug
  # configuration
  if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    include(CTest)
    enable_testing()
    option(PARAOS_TESTS "Enable testing" ON)
  endif()
endif()

# Boost::leaf ------------------------------------------------------------------
if(NOT TARGET Boost::leaf)
  add_subdirectory(leaf)
endif()

# etl::etl ---------------------------------------------------------------------
if(NOT TARGET etl::etl)
  add_subdirectory(etl)
endif()

# Link etl::etl with paraos_setup so that user code can determine to the path,
# which contained etl_profile.h
target_link_libraries(etl INTERFACE paraos_setup)

# Microsoft.GSL::GSL -----------------------------------------------------------
if(NOT TARGET Microsoft.GSL::GSL)
  add_subdirectory(GSL)
endif()

# LWRB -------------------------------------------------------------------------
if(NOT TARGET lwrb_ex)
  add_subdirectory(lwrb)

  target_link_libraries(lwrb PUBLIC paraos_setup)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Fixed build error when gcc can't resolve lwrb_sz_atomic_t type
    target_compile_definitions(lwrb PUBLIC -DLWRB_DISABLE_ATOMIC)
  endif()
endif()

add_library(${PROJECT_NAME} STATIC)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft.GSL::GSL etl::etl
                                             Boost::leaf paraos_setup lwrb_ex)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17 c_std_11)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(TRACE)
  message("paraos trace enable")
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DparaosTRACE_ENABLE)
endif()

target_include_directories(${PROJECT_NAME}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")

if(FREERTOS)
  option(THREAD_ENABLE "Thread support enable" ON)
  add_subdirectory(port_freertos)
  option(FREERTOS_ENABLE "Using FreeRTOS port" ON)
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DPARAOS_LIKE_FREERTOS)
elseif(WIN32)
  option(THREAD_ENABLE "Thread support enable" ON)
  add_subdirectory(port_win)
  option(WINAPI "Using WINAPI" ON)
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DPARAOS_LIKE_WINAPI)
elseif(UNIX)
  option(THREAD_ENABLE "Thread support enable" ON)
  add_subdirectory(port_unix)
  option(UNIX "Using UNIX" ON)
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DPARAOS_LIKE_UNIX)
endif()

if(PARAOS_TESTS)
  add_subdirectory(port_tests)
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DUNIT_TESTS_ENABLE)
endif()

if(PARAOS_USING_POLYMORPHIC_EXTRA)
  target_compile_definitions(paraos_setup
                             INTERFACE -DPARAOS_USING_POLYMORPHIC_EXTRA)
endif()

if(IS_PARAOS_STAND_ALONE_PROJECT AND CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY
                                                     "${DO_CLANG_TIDY}")
  else()
    message(FATAL_ERROR "clang-tidy not found.")
  endif()
endif()

add_subdirectory(containers)
add_subdirectory(extra)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/paraos_version.hpp.in
               ${CMAKE_CURRENT_SOURCE_DIR}/paraos_version.hpp @ONLY)
