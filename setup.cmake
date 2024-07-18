project(paraos_setup)

add_library(${PROJECT_NAME} INTERFACE)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

if(TRACE)
  message("paraos trace enable")
  target_compile_definitions(${PROJECT_NAME} INTERFACE -DparaosTRACE_ENABLE)
endif()
