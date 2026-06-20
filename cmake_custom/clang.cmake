# cmake_custom/clang.cmake Clang toolchain file. On macOS: prefers Apple Clang
# (/usr/bin/clang), then Homebrew LLVM (/opt/homebrew/opt/llvm/bin or
# /usr/local/opt/llvm/bin). On other platforms: uses clang/clang++ from PATH.

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE INTERNAL "")

if(WIN32)
  set(TOOLCHAIN_SUFFIX ".exe")
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  set(SEARCH_PATHS
      /usr/bin # Apple Clang (системный)
      /opt/homebrew/opt/llvm/bin # Homebrew Apple Silicon
      /usr/local/opt/llvm/bin # Homebrew Intel
      /usr/local/bin)

  find_program(
    CMAKE_C_COMPILER clang${TOOLCHAIN_SUFFIX}
    PATHS ${SEARCH_PATHS}
    NO_DEFAULT_PATH)
  if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "clang${TOOLCHAIN_SUFFIX} не найден")
  endif()

  find_program(
    CMAKE_CXX_COMPILER clang++${TOOLCHAIN_SUFFIX}
    PATHS ${SEARCH_PATHS}
    NO_DEFAULT_PATH)
  if(NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "clang++${TOOLCHAIN_SUFFIX} не найден")
  endif()
else()
  set(CMAKE_C_COMPILER
      "clang${TOOLCHAIN_SUFFIX}"
      CACHE FILEPATH "C compiler" FORCE)
  set(CMAKE_CXX_COMPILER
      "clang++${TOOLCHAIN_SUFFIX}"
      CACHE FILEPATH "C++ compiler" FORCE)
  message(STATUS "Using Clang: ${CMAKE_C_COMPILER}")
endif()
