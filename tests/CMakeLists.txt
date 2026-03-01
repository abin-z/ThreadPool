# 如果是 MSVC 编译器，设置源文件为 UTF-8 编码
if (MSVC)
  add_compile_options("/utf-8")
endif()


# 添加测试可执行文件，将 test_thread_pool.cpp 作为源文件
add_executable(unit_test test_thread_pool.cpp)

target_compile_options(unit_test PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

# 链接被测库 thread_pool
target_link_libraries(unit_test PRIVATE thread_pool::thread_pool)

# 设置 unit_test 目标的头文件搜索路径
# 这里包含 Catch2 头文件，以便 unit_test.cpp 能够使用 Catch2 进行单元测试
target_include_directories(unit_test PRIVATE ${CATCH2_INCLUDE_DIR})

# 允许 add_test() 添加测试
enable_testing()

# 注册 unit_test 作为 CTest 可识别的测试用例
# 当执行 `ctest` 时，会运行 unit_test 并检查其返回值
add_test(NAME unit_test COMMAND unit_test)