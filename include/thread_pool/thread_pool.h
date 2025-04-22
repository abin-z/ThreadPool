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

#ifndef ABIN_THREADPOOL_H
#define ABIN_THREADPOOL_H

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

namespace abin
{
/// @brief C++11的线程池, 提交任务支持任意多参数, 支持获取返回值
class threadpool
{
  using task_t = std::function<void()>;  // 定义任务类型为可调用对象

 public:
  /// @brief 构造函数, 初始化线程池, 创建指定数量的工作线程
  explicit threadpool(std::size_t thread_count = default_thread_count())
  {
    start(thread_count);  // 创建线程
  }

  /// @brief 析构函数, 停止所有线程并等待它们完成
  ~threadpool()
  {
    stop_ = true;
    cv_.notify_all();
    for (std::thread &worker : workers_)
    {
      worker.join();
    }
  }

  /// @brief 提交任务到线程池并返回一个 future 对象, 用户可以通过它获取任务的返回值
  ///
  /// @tparam F 任务类型的可调用对象
  /// @tparam Args 可调用对象的参数类型
  /// @param f 需要提交的任务
  /// @param args 任务的参数
  /// @return std::future<decltype(f(args...))> 返回一个 future 对象, 允许用户获取任务的返回值
  template <typename F, typename... Args>
  auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
  {
    if (stop_) throw std::runtime_error("error: submit on stopped threadpool");  // 防止在已停止的线程池上提交任务
    using return_type = decltype(f(args...));
    // 将 f 包装成 task, task 是一个 shared_ptr 指向 packaged_task
    auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...)  // 将函数和参数封装成一个 return_type() 的可调用对象
    );
    std::future<return_type> ret = task->get_future();  // 获取与 task 相关联的 future
    {
      std::lock_guard<std::mutex> locker(mtx_);
      task_queue_.emplace([task] { (*task)(); });  // 将任务添加到任务队列中
    }
    cv_.notify_one();  // 通知一个等待中的工作线程有新的任务可以执行
    return ret;        // 返回 future 对象
  }

 private:
  /// @brief 默认线程数, 获取硬件支持的并发线程数, 若无法获取则默认为4
  static std::size_t default_thread_count()
  {
    auto n = std::thread::hardware_concurrency();
    return n == 0 ? 4 : n;
  }

  /// @brief 启动线程池, 创建指定数量的工作线程
  /// @param thread_count 线程池中线程的数量
  void start(std::size_t thread_count)
  {
    for (int i = 0; i < thread_count; ++i)
    {
      // 创建并启动工作线程
      workers_.emplace_back([this] {
        while (true)
        {
          task_t task;
          {
            std::unique_lock<std::mutex> locker(mtx_);
            // 等待直到任务队列中有任务, 或者线程池已停止
            cv_.wait(locker, [this] { return stop_ || !task_queue_.empty(); });
            if (stop_ && task_queue_.empty()) return;  // 如果线程池已经停止并且队列为空, 退出线程
            task = std::move(task_queue_.front());     // 从队列中取出任务
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
  std::condition_variable cv_;        // 条件变量, 用于线程同步
  std::mutex mtx_;                    // 互斥锁, 保护共享资源(任务队列)
  std::atomic<bool> stop_{false};     // 线程池是否停止
};
}  // namespace abin
#endif  // ABIN_THREADPOOL_H