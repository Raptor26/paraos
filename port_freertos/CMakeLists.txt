# Copyright (c) 2024 Stilsoft

# Autor Mickle Isaev

# Autor Vyhodcev Egor

# Distributed under the MIT License

cmake_minimum_required(VERSION 3.28)

project(paraos_osal VERSION 0.1.0)

message("Using freeRTOS implementation")

if(NOT TARGET freertos_config)
  add_library(freertos_config INTERFACE)
  target_include_directories(
    freertos_config SYSTEM
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/freertos_port/"
              "${CMAKE_SOURCE_DIR}")
endif()

target_compile_definitions(freertos_config INTERFACE -DfreeRTOS -DFREERTOS)

if(FREERTOS_USER_CONFIG)
  # in case when user want use freeRTOS with mcu port, user must define
  # `FREERTOS_USER_CONFIG` (in cmakePresets.txt, for example) and next
  # definitions: - FREERTOS_PORT; - freertos_config; In this case,
  # FreeRTOS-Kernel linked with user definitions FREERTOS_PORT and
  # freertos_config. For example, user can define: set(FREERTOS_PORT
  # `GCC_ARM_CM4F CACHE STRING \"\")` and `set(FREERTOS_HEAP "4" CACHE STRING ""
  # FORCE)`
elseif(WIN32)

  set(FREERTOS_PORT
      MSVC_MINGW
      CACHE STRING \"\")

  set(FREERTOS_HEAP
      "3"
      CACHE STRING "" FORCE)

  target_include_directories(
    freertos_config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config/win/")

elseif(UNIX)

  set(FREERTOS_PORT
      GCC_POSIX
      CACHE STRING \"\")

  set(FREERTOS_HEAP
      "3"
      CACHE STRING "" FORCE)

  target_include_directories(
    freertos_config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config/unix/")
endif()

# FreeRTOS-Kernel --------------------------------------------------------------
set(FREERTOS_KERNEL_PATH "FreeRTOS-Kernel")
add_subdirectory(${FREERTOS_KERNEL_PATH} FreeRTOS-Kernel)

# Supress compiler warnings related to the FreeRTOS implementation
target_compile_options(
  freertos_kernel_port PRIVATE -Wno-unused-variable
                               -Wno-unused-but-set-variable)

target_link_libraries(freertos_kernel INTERFACE paraos_setup)
target_compile_options(
  freertos_kernel
  PRIVATE -Wno-unused-variable
          # Suppressions required to build clean with clang.
          -Wno-unused-macros
          -Wno-padded
          -Wno-missing-variable-declarations
          -Wno-covered-switch-default
          -Wno-cast-align)

target_include_directories(
  freertos_kernel PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/"
                         "${CMAKE_CURRENT_SOURCE_DIR}/../")
set_property(TARGET freertos_kernel PROPERTY C_STANDARD 90)

target_sources(freertos_kernel
               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/paraos_freertos_hooks.cpp)

# paraos_thread ----------------------------------------------------------------
add_library(${PROJECT_NAME} STATIC)
target_link_libraries(
  ${PROJECT_NAME} PUBLIC freertos_kernel freertos_config Microsoft.GSL::GSL
                         paraos_setup etl::etl)

target_sources(
  ${PROJECT_NAME}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/paraos_thread.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/paraos_semaphore.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/paraos_mutex.cpp)

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

if(UTEST)
  add_subdirectory(tests)
endif()
