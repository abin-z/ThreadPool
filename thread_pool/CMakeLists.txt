# 仅头文件库 (Header-Only) INTERFACE 类型
# INTERFACE 目标不会生成 .a 或 .so 文件, 但可以被 target_link_libraries 依赖.
set(tgt_name thread_pool)
add_library(${tgt_name} INTERFACE)
target_include_directories(${tgt_name} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

# POSIX 系统下使用 pthread 库
find_package(Threads REQUIRED)
target_link_libraries(${tgt_name} INTERFACE Threads::Threads)