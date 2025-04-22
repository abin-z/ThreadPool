/**************************************************************************************************************
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * @file: thread_pool.h
 * @version: v0.9.0
 * @description:
 * - Features :
 *   - Lightweight & Easy-to-Use: A C++11 thread pool with task submission and future-based return value support.
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
  template <typename F, typename... Args>
  using return_type = decltype(std::declval<F>()(std::declval<Args>()...));

 public:
  explicit threadpool(std::size_t thread_count = default_thread_count())
  {
    start(thread_count);  // 创建线程
  }

  /// @brief 析构函数, join 所有工作线程
  ~threadpool()
  {
    stop_ = true;
    cv_.notify_all();
    for (std::thread &worker : workers_)
    {
      worker.join();
    }
  }

  template <typename F, typename... Args>
  auto submit(F &&f, Args &&...args) -> std::future<return_type<F, Args...>>
  {
    if (stop_) throw std::runtime_error("error: submit on stopped threadpool");
    using ret_type = return_type<F, Args...>;
    // 将f包装成task, task是一个packaged_task的指针
    auto task =
      std::make_shared<std::packaged_task<ret_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<ret_type> ret = task->get_future();

    {
      std::lock_guard<std::mutex> locker(mtx_);
      task_queue_.emplace([task] { (*task)(); });
    }
    cv_.notify_one();  // 通知worker;
    return ret;
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
          task_t task;
          {
            std::unique_lock<std::mutex> locker(mtx_);
            this->cv_.wait(locker, [this] { return this->stop_ || !this->task_queue_.empty(); });
            if (this->stop_ && this->task_queue_.empty()) return;
            task = std::move(task_queue_.front());
            task_queue_.pop();
          }
          task();  // 执行任务
        }
      });
    }
  }

 private:
  std::vector<std::thread> workers_;  // 工作线程(线程池)
  std::queue<task_t> task_queue_;     // 任务队列
  // 线程安全控制
  std::condition_variable cv_;     // 任务队列条件变量
  std::mutex mtx_;                 // 互斥对象
  std::atomic<bool> stop_{false};  // 线程池是否停止
};
}  // namespace concurrency
#endif  // CONCURRENCY_THREADPOOL_H