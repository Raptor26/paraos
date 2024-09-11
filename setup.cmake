cmake_minimum_required(VERSION 3.28)

project(paraos_setup)

add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
target_link_libraries(${PROJECT_NAME} INTERFACE Boost::leaf Microsoft.GSL::GSL)

if(TRACE)
  message("paraos trace enable")
  target_compile_definitions(${PROJECT_NAME} INTERFACE -DparaosTRACE_ENABLE)
endif()

if(RTOS)
  message("Using RTOS implementation")

  add_library(freertos_config INTERFACE)

  target_include_directories(
    freertos_config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/freertos_port/")

  if(WIN32)
    set(FREERTOS_PORT
        MSVC_MINGW
        CACHE STRING \"\")

    set(FREERTOS_HEAP
        "4"
        CACHE STRING "" FORCE)

    target_include_directories(
      freertos_config
      INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/freertos_port/freertos_configs/win/")

  elseif(UNIX)
    set(FREERTOS_PORT
        GCC_POSIX
        CACHE STRING \"\")

    set(FREERTOS_HEAP
        "3"
        CACHE STRING "" FORCE)

    target_include_directories(
      freertos_config
      INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/freertos_port/freertos_configs/unix/")

  endif()

  target_compile_definitions(freertos_config INTERFACE -DprojCOVERAGE_TEST=0)

  add_subdirectory(FreeRTOS-Kernel)
  target_link_libraries(${PROJECT_NAME} INTERFACE freertos_kernel)
  target_compile_definitions(
    ${PROJECT_NAME} INTERFACE -DfreeRTOS -Dicore_checkLOOP_ENABLE
                              -DprojCOVERAGE_TEST=0)

endif()
