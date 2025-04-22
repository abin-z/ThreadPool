/**************************************************************************************************************
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * @file: thread_pool.h
 * @version: v0.9.0
 * @description:
 * - Features :
 *   - Lightweight & Easy-to-Use: A header-only INI parser with no external dependencies (C++11 only).
 *
 * @author: abin
 * @date: 2025-04-20
 * @license: MIT
 * @repository: https://github.com/abin-z/ThreadPool
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 **************************************************************************************************************/

#ifndef CONCURRENCY_THREADPOOL_H
#define CONCURRENCY_THREADPOOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

namespace concurrency
{
/// @brief C++11的线程池, 提交任务支持任意多参数, 支持获取返回值
class threadpool
{
  using task_t = std::function<void()>;
  // 获取可调用对象返回值类型
  template <typename Func, typename... Args>
  using return_type = decltype(std::declval<Func>()(std::declval<Args>()...));

 public:
  explicit threadpool(std::size_t thread_count = default_thread_count())
  {
    start(thread_count);  // 创建线程
  }

  ~threadpool()
  {
    stop_ = true;
  }

  template <typename Func, typename... Args>
  auto submit(Func &&f, Args &&...args) -> std::future<return_type<Func, Args...>>
  {
    // TODO
  }

 private:
  static std::size_t default_thread_count()
  {
    auto n = std::thread::hardware_concurrency();
    return n == 0 ? 4 : n;
  }

  void start(std::size_t thread_count)
  {
    for (int i = 0; i < thread_count; ++i)
    {
      // 添加工作线程
      workers_.emplace_back([this] {
        while (true)
        {
          // TODO
        }
      });
    }
  }

 private:
  std::vector<std::thread> workers_;  // 工作线程(线程池)
  std::queue<task_t> task_;           // 任务队列
  // 线程安全控制
  std::condition_variable cv_;     // 任务队列条件变量
  std::mutex mtx_;                 // 互斥对象
  std::atomic<bool> stop_{false};  // 线程池是否停止
};
}  // namespace concurrency
#endif  // CONCURRENCY_THREADPOOL_H