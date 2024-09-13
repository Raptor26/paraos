cmake_minimum_required(VERSION 3.28)

project(paraos_osal VERSION 0.1.0)

message("Using freeRTOS implementation")

if(NOT TARGET freertos_config)
  add_library(freertos_config INTERFACE)
  target_compile_definitions(freertos_config INTERFACE -DfreeRTOS
                                                       -Dicore_checkLOOP_ENABLE)
  target_include_directories(
    freertos_config SYSTEM
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/freertos_port/"
              "${CMAKE_SOURCE_DIR}")

endif()

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
      "4"
      CACHE STRING "" FORCE)

  target_include_directories(
    freertos_config
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/freertos_configs/win/")

elseif(UNIX)

  set(FREERTOS_PORT
      GCC_POSIX
      CACHE STRING \"\")

  set(FREERTOS_HEAP
      "3"
      CACHE STRING "" FORCE)

  target_include_directories(
    freertos_config
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/freertos_configs/unix/")
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(
  ${PROJECT_NAME}
  INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/" "${CMAKE_CURRENT_SOURCE_DIR}/../"
            "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/")

# FreeRTOS-Kernel --------------------------------------------------------------
set(FREERTOS_KERNEL_PATH "FreeRTOS-Kernel")
add_subdirectory(${FREERTOS_KERNEL_PATH} FreeRTOS-Kernel)
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

target_sources(freertos_kernel
               PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/paraos_freertos_hooks.cpp)
target_include_directories(
  freertos_kernel
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/" "${CMAKE_CURRENT_SOURCE_DIR}/../"
         "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/")
set_property(TARGET freertos_kernel PROPERTY C_STANDARD 90)

# paraos_osal ------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} INTERFACE freertos_kernel freertos_config
                                                Microsoft.GSL::GSL)

target_compile_options(${PROJECT_NAME} INTERFACE -Wall -Wextra -Werror
                                                 -Wpedantic)

if(UTEST)
  add_subdirectory(tests)
endif()
