#include <iostream>
#include <string>

#include "thread_pool/thread_pool.h"

int main()
{
  abin::threadpool pool(30);  // 创建一个包含30个线程的线程池
  std::cout << "===Thread pool created.===\n";
  std::vector<std::future<int>> futures;
  for (int i = 0; i < 100; ++i)
  {
    futures.emplace_back(pool.submit([i] {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 模拟任务执行时间
      std::string msg = "Task " + std::to_string(i) + " is executed.\n";
      std::cout << msg;
      return i * i;
    }));
  }
  std::cout << "===All tasks submitted.===\n";
  for (auto &future : futures)
  {
    future.wait();  // 等待所有任务完成, 可以使用 std::future::get() 获取结果
  }
  std::cout << "===All tasks completed.===\n";
}