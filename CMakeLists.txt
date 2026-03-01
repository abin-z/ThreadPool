# 指定CMake最低版本要求
cmake_minimum_required(VERSION 3.20)

project(ThreadPool LANGUAGES C CXX VERSION 0.9.2)

include(GNUInstallDirs)

# ---------------------------------------------------------------------------------------
# Set default build to Release for single-config generators (Makefile/Ninja)
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  if((DEFINED PROJECT_IS_TOP_LEVEL AND PROJECT_IS_TOP_LEVEL)
    OR (NOT DEFINED PROJECT_IS_TOP_LEVEL AND CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR))
      set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
  endif()
endif()

# 仅头文件库 (Header-Only) INTERFACE 类型
# INTERFACE 目标不会生成 .a 或 .so 文件, 但可以被 target_link_libraries 依赖.
add_library(thread_pool INTERFACE)
add_library(thread_pool::thread_pool ALIAS thread_pool)
target_include_directories(thread_pool
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(thread_pool INTERFACE cxx_std_11)

# POSIX 系统下使用 pthread 库
find_package(Threads REQUIRED)
target_link_libraries(thread_pool INTERFACE Threads::Threads)

# Compiler-specific warning/options
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(thread_pool INTERFACE
  "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
  "$<${msvc_cxx}:$<BUILD_INTERFACE:/W3;/utf-8>>"
)

# ----------------------------------------------------------
# Detect master (top-level) project
# ----------------------------------------------------------
if(NOT DEFINED THREAD_POOL_MASTER_PROJECT)
  if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(THREAD_POOL_MASTER_PROJECT ON)
  else()
    set(THREAD_POOL_MASTER_PROJECT OFF)
  endif()
endif()

if(THREAD_POOL_MASTER_PROJECT)
  message(STATUS "[thread_pool] Master project: ${THREAD_POOL_MASTER_PROJECT}")
  # Generate compile_commands.json for clangd/clang-tidy
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  # Enable PIC for all targets (safest for static libs)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

option(THREAD_POOL_BUILD_EXAMPLES  "Build examples" ${THREAD_POOL_MASTER_PROJECT})
option(THREAD_POOL_BUILD_TESTS "Build tests" ${THREAD_POOL_MASTER_PROJECT})

if(THREAD_POOL_BUILD_EXAMPLES)
  message(STATUS "[thread_pool] Building examples...")
  add_subdirectory(examples)
endif()

if(THREAD_POOL_BUILD_TESTS)
  message(STATUS "[thread_pool] Building tests...")
  include(CTest)  # enable add_test
  add_subdirectory(external)
  add_subdirectory(tests)
endif()

