cmake_minimum_required(VERSION 3.20)

# User can specifiet building paros libnrary for link anything with paraos_setup
# interface taget
project(paraos_setup)

add_library(${PROJECT_NAME} INTERFACE)

if(TRACE)
  message("paraos trace enable")
  target_compile_definitions(${PROJECT_NAME} INTERFACE -DparaosTRACE_ENABLE)
endif()
