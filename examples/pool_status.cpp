#include <thread_pool/thread_pool.h>

#include <iostream>

int main()
{
  abin::threadpool pool(4);  // 包含4个线程的线程池

  // 查询单个状态
  std::cout << "before pool.is_running() = " << pool.is_running();          // 线程池是否在运行
  std::cout << "\nbefore pool.total_threads() = " << pool.total_threads();  // 线程池总共的线程数
  std::cout << "\nbefore pool.busy_threads() = " << pool.busy_threads();    // 繁忙的线程数量
  std::cout << "\nbefore pool.idle_threads() = " << pool.idle_threads();    // 空闲的线程数量
  std::cout << "\nbefore pool.pending_tasks() = " << pool.pending_tasks();  // 正在等待的任务数量

  for (size_t i = 0; i < 100; ++i)  // 提交100个task
  {
    pool.submit([i] {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 模拟任务
      return i;
    });
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::cout << "\n===All tasks submitted.===\n";
  // 通过status函数获取当前所有的状态信息
  abin::threadpool::status_info ss = pool.status();
  std::cout << "after status_info.running = " << ss.running;                // 线程池是否在运行
  std::cout << "\nafter status_info.total_threads = " << ss.total_threads;  // 线程池总共的线程数
  std::cout << "\nafter status_info.busy_threads = " << ss.busy_threads;    // 繁忙的线程数量
  std::cout << "\nafter status_info.idle_threads = " << ss.idle_threads;    // 空闲的线程数量
  std::cout << "\nafter status_info.pending_tasks = " << ss.pending_tasks;  // 正在等待的任务数量
}