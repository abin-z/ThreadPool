/**************************************************************************************************************
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * @file: thread_pool.h
 * @version: v0.9.2
 * @description: A cross-platform, lightweight, easy-to-use C++11 thread pool that supports submitting tasks with
 *               arbitrary parameters and obtaining return values
 *  - Futures
 *    - Task-based: Supports tasks with arbitrary parameters, and obtains return values ​​through `std::future`.
 *    - Cross-Platform: Works on platforms supporting C++11.
 *    - Thread Safety: Uses `std::mutex`, `std::condition_variable`, and atomic variables for synchronization.
 *    - Flexible Shutdown: Two modes for shutdown: `WaitForAllTasks` and `DiscardPendingTasks`.
 *    - Lightweight & Easy-to-Use: Simple API with minimal setup.
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
  /// @brief 线程池当前状态信息结构体
  struct status_info
  {
    std::size_t total_threads;  // 总线程数
    std::size_t busy_threads;   // 正在执行任务的线程数
    std::size_t idle_threads;   // 空闲线程数
    std::size_t pending_tasks;  // 等待中的任务数
    bool running;               // 线程池是否在运行
  };

  /// @brief 关闭线程池的模式
  enum class shutdown_mode : unsigned char
  {
    /// @brief 等待所有已提交的任务完成后再关闭线程池
    /// 在此模式下, 线程池会等待所有任务(包括已开始和未开始的任务)执行完成后再关闭.
    WaitForAllTasks,

    /// @brief 立即关闭线程池, 丢弃尚未开始的任务.
    /// 在此模式下, 线程池会立即停止接收新任务, 丢弃所有尚未开始执行的任务,
    /// 但已经开始执行的任务会继续执行, 直到它们完成.
    DiscardPendingTasks
  };

 public:
  /// @brief 构造函数, 初始化线程池并启动指定数量的工作线程
  /// @param thread_count 要创建的线程数量, 默认为硬件支持的并发线程数(若无法获取则为 4)
  explicit threadpool(std::size_t thread_count = default_thread_count())
  {
    launch_threads(thread_count);  // 创建线程
  }

  /// @brief 析构函数, 停止所有线程并等待它们完成
  ~threadpool()
  {
    shutdown(shutdown_mode::WaitForAllTasks);
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
    if (!running_) throw std::runtime_error("error: ThreadPool is not running. Cannot submit new tasks.");
    using return_type = decltype(f(args...));
    // 将 f 包装成 task, task 是一个 shared_ptr 指向 packaged_task
    auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...)  // 将函数和参数封装成一个 return_type() 的可调用对象
    );
    std::future<return_type> ret = task->get_future();  // 获取与 task 相关联的 future
    {
      std::lock_guard<std::mutex> lock(mtx_);
      task_queue_.emplace([task] { (*task)(); });  // 将任务添加到任务队列中
    }
    cv_.notify_one();  // 通知一个等待中的工作线程有新的任务可以执行
    return ret;        // 返回 future 对象
  }

  /// @brief 阻塞直到所有任务完成(任务队列为空且没有任务在执行), 若没有任务，立即返回
  void wait_all()
  {
    if (busy_count_ == 0 && pending_tasks() == 0) return;
    std::unique_lock<std::mutex> lock(mtx_done_);
    cv_done_.wait(lock, [this] { return busy_count_ == 0 && pending_tasks() == 0; });
  }

  /// @brief 关闭线程池
  /// @param mode `WaitForAllTasks` 等待所有任务执行完成后再关闭; `DiscardPendingTasks` 立即关闭线程池,
  /// 抛弃尚未开始的任务.
  void shutdown(shutdown_mode mode = shutdown_mode::WaitForAllTasks)
  {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      if (!running_) return;  // 已经关闭则直接返回
      running_ = false;
      if (mode == shutdown_mode::DiscardPendingTasks)  // 放弃任务模式
      {
        std::queue<task_t> empty;
        std::swap(task_queue_, empty);  // 清空任务队列
      }
    }
    cv_.notify_all();
    for (std::thread &worker : workers_)
    {
      if (worker.joinable()) worker.join();
    }
    workers_.clear();
  }

  /// @brief 重启线程池, 先关闭当前线程池(等待所有任务完成), 然后以指定的线程数量重新启动线程池.
  /// @param thread_count 要创建的工作线程数量
  void reboot(std::size_t thread_count)
  {
    shutdown(shutdown_mode::WaitForAllTasks);
    {
      std::lock_guard<std::mutex> lock(mtx_);
      if (running_) return;  // 已重启, 无需再次初始化(幂等)
      running_ = true;
      launch_threads(thread_count);
    }
  }

  /// @brief 当前线程池的总线程数量
  std::size_t total_threads() const noexcept
  {
    return workers_.size();
  }
  /// @brief 获取当前等待的任务数量
  std::size_t pending_tasks() const noexcept
  {
    std::lock_guard<std::mutex> lock(mtx_);
    return task_queue_.size();
  }
  /// @brief 获取繁忙的线程数量
  std::size_t busy_threads() const noexcept
  {
    return busy_count_.load();
  }
  /// @brief 获取空闲线程数量
  std::size_t idle_threads() const noexcept
  {
    return workers_.size() - busy_count_.load();
  }
  /// @brief 当前线程池是否正在运行(未停止)
  bool is_running() const noexcept
  {
    return running_.load();
  }

  /// @brief 获取线程池的当前状态信息
  status_info status() const noexcept
  {
    std::size_t total = 0;
    std::size_t pending = 0;
    {
      std::lock_guard<std::mutex> lock(mtx_);
      total = workers_.size();
      pending = task_queue_.size();
    }
    std::size_t busy = busy_count_.load();
    std::size_t idle = total - busy;
    return {total, busy, idle, pending, running_.load()};
  }

  // 禁用拷贝构造函数和拷贝赋值操作符
  threadpool(const threadpool &) = delete;
  threadpool &operator=(const threadpool &) = delete;
  // 禁用移动构造函数和移动赋值操作符
  threadpool(threadpool &&) = delete;
  threadpool &operator=(threadpool &&) = delete;

 private:
  /// @brief 默认线程数, 获取硬件支持的并发线程数, 若无法获取则默认为4
  static std::size_t default_thread_count()
  {
    auto n = std::thread::hardware_concurrency();
    return n == 0 ? 4 : n;
  }

  /// @brief 启动线程池, 创建指定数量的工作线程
  /// @param thread_count 线程池中线程的数量
  void launch_threads(std::size_t thread_count)
  {
    if (!workers_.empty()) return;  // 已经初始化过
    for (std::size_t i = 0; i < thread_count; ++i)
    {
      // 创建并启动工作线程
      workers_.emplace_back([this] {
        while (true)
        {
          task_t task;
          {
            std::unique_lock<std::mutex> lock(mtx_);
            // 等待直到任务队列中有任务, 或者线程池已停止
            cv_.wait(lock, [this] { return !running_ || !task_queue_.empty(); });
            if (!running_ && task_queue_.empty()) return;  // 如果线程池已经停止并且队列为空, 退出线程
            task = std::move(task_queue_.front());         // 从队列中取出任务
            task_queue_.pop();
          }
          ++busy_count_;
          task();  // 执行任务
          --busy_count_;
          // 判断任务是否已全部完成
          if (busy_count_ == 0 && task_queue_.empty())
          {
            std::lock_guard<std::mutex> lock(mtx_done_);
            if (busy_count_ == 0 && pending_tasks() == 0)  // 二次确认, 避免竞态
            {
              cv_done_.notify_all();
            }
          }
        }
      });
    }
  }

 private:
  std::vector<std::thread> workers_;  // 工作线程集合，用于并发执行任务
  std::queue<task_t> task_queue_;     // 等待执行的任务队列
  std::condition_variable cv_;        // 条件变量，用于通知工作线程有新任务到来
  mutable std::mutex mtx_;            // 主互斥锁，保护任务队列和与其相关的状态

  std::atomic<std::size_t> busy_count_{0};  // 正在执行任务的线程数量
  std::atomic<bool> running_{true};         // 线程池是否处于运行状态

  mutable std::mutex mtx_done_;      // 用于保护完成通知的互斥锁(wait_all 用)
  std::condition_variable cv_done_;  // 条件变量，用于等待所有任务执行完毕(配合 wait_all 使用)
};
}  // namespace abin
#endif  // ABIN_THREADPOOL_H