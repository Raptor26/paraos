cmake_minimum_required(VERSION 3.28)

project(paraos_setup)

add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
target_link_libraries(${PROJECT_NAME} INTERFACE Boost::leaf)

if(TRACE)
  message("paraos trace enable")
  target_compile_definitions(${PROJECT_NAME} INTERFACE -DparaosTRACE_ENABLE)
endif()
