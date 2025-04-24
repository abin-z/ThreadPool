#include <thread_pool/thread_pool.h>

#include <iostream>

int main()
{
  abin::threadpool pool(4);     // 创建一个包含4个线程的线程池
  for (int i = 0; i < 10; ++i)  // 提交10个任务
  {
    pool.submit([] {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      std::cout << "Should not always appear\n";
    });
  }
  std::cout << "===All tasks submitted.===\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  // pool.shutdown();  // 关闭线程池, 会等待所有任务完成
  pool.shutdown(abin::threadpool::shutdown_mode::DiscardPendingTasks);  // 关闭线程池, 丢弃未开始的任务
  std::cout << "===Thread pool shutdown.===\n";
}