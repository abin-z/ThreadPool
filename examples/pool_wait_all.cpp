#include <thread_pool/thread_pool.h>

#include <iostream>
#include <string>

int func(int n)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 模拟任务
  std::string msg = "task " + std::to_string(n) + " executed.\n";
  std::cout << msg;
  return n * n;
}

int main()
{
  abin::threadpool pool(8);
  pool.wait_all();  // 没有提交任务会直接返回

  std::cout << "===start submit tasks===\n";
  for (size_t i = 0; i < 42; i++)
  {
    pool.submit(func, i);
  }
  pool.wait_all();  // 等待已提交的任务完成
  std::cout << "===half tasks completed.===\n";

  // 继续添加任务
  for (size_t i = 42; i < 84; i++)
  {
    pool.submit(func, i);
  }
  pool.wait_all();
  std::cout << "===all tasks completed.===\n";
}