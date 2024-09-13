cmake_minimum_required(VERSION 3.28)

project(paraos_osal VERSION 0.1.0)

message("Using freeRTOS implementation")

if(NOT TARGET freertos_config)
  add_library(freertos_config INTERFACE)
  target_compile_definitions(
    freertos_config INTERFACE -DprojCOVERAGE_TEST=0 -DfreeRTOS
                              -Dicore_checkLOOP_ENABLE)
  target_include_directories(
    freertos_config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/freertos_port/"
                              "${CMAKE_SOURCE_DIR}")
  target_link_libraries(freertos_config INTERFACE paraos_setup)
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
target_link_libraries(${PROJECT_NAME} INTERFACE freertos_kernel)

add_subdirectory(FreeRTOS-Kernel)

if(UTEST)
  target_sources(freertos_config
                 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rtos_test_hooks.cpp)
  target_sources(freertos_config
                 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/rtos_test_hooks.cpp)
  add_subdirectory(tests)
else()
  target_sources(${PROJECT_NAME}
                 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rtos_hooks.cpp)
  target_sources(freertos_kernel
                 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/rtos_hooks.cpp)
endif()
