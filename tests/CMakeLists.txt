# 如果是 MSVC 编译器，设置源文件为 UTF-8 编码
if (MSVC)
  add_compile_options("/utf-8")
endif()

# 添加测试可执行文件，将 test_thread_pool.cpp 作为源文件
add_executable(test_thread_pool test_thread_pool.cpp)

# 链接被测库 thread_pool
target_link_libraries(test_thread_pool PRIVATE thread_pool)

# 设置 test_thread_pool 目标的头文件搜索路径
# 这里包含 Catch2 头文件，以便 test_thread_pool.cpp 能够使用 Catch2 进行单元测试
target_include_directories(test_thread_pool PRIVATE ${CATCH2_INCLUDE_DIR})

# 允许 add_test() 添加测试
enable_testing()

# 注册 test_thread_pool 作为 CTest 可识别的测试用例
# 当执行 `ctest` 时，会运行 test_thread_pool 并检查其返回值
add_test(NAME test_thread_pool COMMAND test_thread_pool)