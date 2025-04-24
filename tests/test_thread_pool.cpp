#define CATCH_CONFIG_MAIN
#include <thread_pool/thread_pool.h>

#include <atomic>
#include <catch.hpp>
#include <chrono>
#include <future>
#include <thread>

using namespace std::chrono;
using namespace abin;

// 测试线程池是否可以正常创建和销毁
TEST_CASE("ThreadPool creation and destruction", "[thread_pool]")
{
  threadpool pool(4);  // 创建一个包含4个线程的线程池
  REQUIRE_NOTHROW(
    pool.submit([]() { std::this_thread::sleep_for(milliseconds(100)); }));  // 提交一个任务，应该不会抛出异常
}

// 测试线程池任务提交功能
TEST_CASE("ThreadPool submit tasks", "[thread_pool]")
{
  threadpool pool(4);

  std::atomic<int> counter(0);
  auto task = pool.submit([&]() { counter++; });

  task.get();             // 等待任务完成
  REQUIRE(counter == 1);  // 确保任务执行成功，counter 应该是1
}

// 测试线程池并发执行任务
TEST_CASE("ThreadPool executes multiple tasks concurrently", "[thread_pool]")
{
  threadpool pool(4);

  std::atomic<int> counter(0);
  auto task1 = pool.submit([&]() { counter++; });
  auto task2 = pool.submit([&]() { counter++; });
  auto task3 = pool.submit([&]() { counter++; });

  task1.get();
  task2.get();
  task3.get();

  REQUIRE(counter == 3);  // 三个任务都应成功执行
}

// 测试任务在指定时间内执行
TEST_CASE("ThreadPool task execution time", "[thread_pool]")
{
  threadpool pool(4);

  auto start = steady_clock::now();
  auto task = pool.submit([]() { std::this_thread::sleep_for(milliseconds(200)); });

  task.get();  // 等待任务完成
  auto end = steady_clock::now();
  auto duration = duration_cast<milliseconds>(end - start);

  REQUIRE(duration.count() >= 200);  // 任务执行时间应该大于等于200ms
}

// 测试线程池在任务队列为空时的等待
TEST_CASE("ThreadPool waits for tasks", "[thread_pool]")
{
  threadpool pool(4);

  std::atomic<int> counter(0);
  auto task1 = pool.submit([&]() {
    std::this_thread::sleep_for(milliseconds(100));
    counter++;
  });
  std::this_thread::sleep_for(milliseconds(50));  // 等待一段时间再提交另一个任务
  auto task2 = pool.submit([&]() { counter++; });

  task1.get();
  task2.get();

  REQUIRE(counter == 2);  // 确保两个任务都已执行
}

// 测试线程池任务完成后的返回值
TEST_CASE("ThreadPool task returns value", "[thread_pool]")
{
  threadpool pool(4);

  auto task = pool.submit([]() -> int {
    std::this_thread::sleep_for(milliseconds(100));
    return 42;
  });

  REQUIRE(task.get() == 42);  // 确保任务返回值正确
}

// 测试多个线程池实例的独立性
TEST_CASE("Multiple thread pools independent", "[thread_pool]")
{
  threadpool pool1(4);
  threadpool pool2(2);

  std::atomic<int> counter1(0);
  std::atomic<int> counter2(0);

  auto task1 = pool1.submit([&]() { counter1++; });
  auto task2 = pool2.submit([&]() { counter2++; });

  task1.get();
  task2.get();

  REQUIRE(counter1 == 1);  // pool1 应该执行了 task1
  REQUIRE(counter2 == 1);  // pool2 应该执行了 task2
}

// 测试线程池空队列时阻塞
TEST_CASE("ThreadPool blocks on empty task queue", "[thread_pool]")
{
  threadpool pool(4);

  std::atomic<bool> task_started(false);
  auto task = pool.submit([&]() {
    std::this_thread::sleep_for(milliseconds(200));  // 模拟任务处理时间
    task_started = true;
  });

  // 检查任务是否启动前，其他任务还没加入线程池
  std::this_thread::sleep_for(milliseconds(50));  // 等待一会
  REQUIRE(task_started == false);                 // 确保任务没有立即执行

  task.get();  // 等待任务完成

  REQUIRE(task_started == true);  // 确保任务执行成功
}

TEST_CASE("ThreadPool submits task and returns correct result", "[submit]")
{
  threadpool pool(2);

  auto fut1 = pool.submit([] { return 1 + 1; });
  auto fut2 = pool.submit([](int x) { return x * 10; }, 5);

  REQUIRE(fut1.get() == 2);
  REQUIRE(fut2.get() == 50);
}

TEST_CASE("ThreadPool shutdown is idempotent", "[shutdown][idempotent]")
{
  threadpool pool(2);
  pool.submit([] { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }).get();

  pool.shutdown();  // 第一次关闭
  pool.shutdown();  // 再次调用应无异常
  pool.shutdown();  // 多次也应该没问题

  REQUIRE(pool.is_running() == false);
}

TEST_CASE("ThreadPool reboot is idempotent and safe", "[reboot][idempotent]")
{
  threadpool pool(2);

  pool.reboot(3);  // 第一次重启
  REQUIRE(pool.total_threads() == 3);

  pool.reboot(6);  // 相同参数再次重启, 不应出错
  REQUIRE(pool.total_threads() == 6);

  auto fut = pool.submit([] { return 100; });
  REQUIRE(fut.get() == 100);
}

TEST_CASE("ThreadPool reboot after shutdown recovers correctly", "[reboot][recover]")
{
  threadpool pool(2);
  pool.shutdown();  // 关闭线程池

  REQUIRE(pool.is_running() == false);

  pool.reboot(4);  // 重新启动
  REQUIRE(pool.is_running() == true);
  REQUIRE(pool.total_threads() == 4);

  auto fut = pool.submit([] { return 42; });
  REQUIRE(fut.get() == 42);
}

TEST_CASE("ThreadPool discard tasks on shutdown", "[shutdown][discard]")
{
  threadpool pool(2);
  std::promise<void> start_flag;
  std::shared_future<void> start_future(start_flag.get_future());

  for (int i = 0; i < 5; ++i)
  {
    pool.submit([start_future] { start_future.wait(); });
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  start_flag.set_value();  // 先触发所有正在执行的任务，不让它们挂住
  pool.shutdown(threadpool::shutdown_mode::DiscardPendingTasks);

  REQUIRE(pool.is_running() == false);
}

TEST_CASE("ThreadPool discard tasks on shutdown2", "[shutdown][discard]")
{
  threadpool pool(2);
  std::promise<void> start_flag;
  std::shared_future<void> start_future(start_flag.get_future());
  std::atomic<int> executed_tasks{0};

  for (int i = 0; i < 5; ++i)
  {
    pool.submit([start_future, &executed_tasks] {
      start_future.wait();
      ++executed_tasks;
    });
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  start_flag.set_value();  // 让已开始任务结束

  pool.shutdown(threadpool::shutdown_mode::DiscardPendingTasks);

  REQUIRE(pool.is_running() == false);
  REQUIRE(executed_tasks.load() <= 2);  // 最多只能有2个任务开始执行
}

TEST_CASE("ThreadPool throws if submit after shutdown", "[submit][error]")
{
  threadpool pool(1);
  pool.shutdown();

  REQUIRE_THROWS_AS(pool.submit([] { return 1; }), std::runtime_error);
}

TEST_CASE("ThreadPool discard tasks under high load", "[stress][shutdown][discard]")
{
  threadpool pool(4);            // 线程池最多有 4 个工作线程
  std::atomic<int> executed{0};  // 记录执行过的任务数

  // 提交 10000 个任务，模拟高负载
  for (int i = 0; i < 10000; ++i)
  {
    pool.submit([&executed] {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 模拟任务执行时间
      ++executed;
    });
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 等待任务开始执行

  pool.shutdown(threadpool::shutdown_mode::DiscardPendingTasks);  // 放弃未开始的任务

  // 确保线程池已关闭
  REQUIRE_FALSE(pool.is_running());  // 线程池应该已经停止
  REQUIRE(executed <= 50);           // 只执行了部分任务
}

TEST_CASE("ThreadPool rejects new tasks after shutdown", "[reject][shutdown]")
{
  threadpool pool(2);
  pool.shutdown();

  REQUIRE_FALSE(pool.is_running());
  REQUIRE_THROWS_AS(pool.submit([] {}), std::runtime_error);
}

TEST_CASE("ThreadPool can reboot and accept tasks", "[reboot][submit]")
{
  threadpool pool(2);
  pool.shutdown();
  REQUIRE_FALSE(pool.is_running());

  pool.reboot(3);
  REQUIRE(pool.is_running());
  REQUIRE(pool.total_threads() == 3);

  auto f = pool.submit([] { return 42; });
  REQUIRE(f.get() == 42);
}

TEST_CASE("ThreadPool survives multiple shutdown and reboot", "[reboot][stability]")
{
  threadpool pool(2);

  for (int i = 0; i < 50; ++i)
  {
    pool.shutdown();
    REQUIRE_FALSE(pool.is_running());
    pool.reboot(2);
    REQUIRE(pool.is_running());

    auto f = pool.submit([] { return 7; });
    REQUIRE(f.get() == 7);
  }
}

TEST_CASE("ThreadPool shutdown while tasks being submitted", "[race][shutdown]")
{
  threadpool pool(4);
  std::atomic<int> success_count{0};
  std::atomic<int> fail_count{0};
  std::atomic<bool> stop{false};

  std::thread submitter([&] {
    while (!stop)
    {
      try
      {
        pool.submit([&] { success_count++; });
      }
      catch (...)
      {
        fail_count++;
      }
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  pool.shutdown(threadpool::shutdown_mode::DiscardPendingTasks);
  stop = true;
  submitter.join();

  REQUIRE_FALSE(pool.is_running());
  REQUIRE(success_count >= 0);
}

TEST_CASE("Heavy concurrent submissions and value fetching", "[submit][future][stress]")
{
  threadpool pool(16);
  constexpr int submitter_threads = 32;  // 启动 32 个提交线程
  constexpr int tasks_per_thread = 500;  // 每个线程提交 100 个任务
  std::atomic<int> counter{0};
  std::vector<std::future<int>> futures;
  std::mutex futures_mutex;

  // 多线程并发提交任务，并收集 future 对象
  std::vector<std::thread> submitters;
  for (int i = 0; i < submitter_threads; ++i)
  {
    submitters.emplace_back([&] {
      for (int j = 0; j < tasks_per_thread; ++j)
      {
        try
        {
          auto fut = pool.submit([&counter] {
            // std::this_thread::sleep_for(std::chrono::microseconds(1));  // 模拟小工作量
            return counter.fetch_add(1, std::memory_order_relaxed);
          });
          std::lock_guard<std::mutex> lock(futures_mutex);
          futures.push_back(std::move(fut));
        }
        catch (...)
        {
          // pool 被关闭也不会崩
        }
      }
    });
  }

  for (auto& t : submitters) t.join();

  // 获取所有 future 的结果
  std::set<int> results;
  for (auto& fut : futures)
  {
    results.insert(fut.get());
  }

  pool.shutdown();
  REQUIRE(results.size() == submitter_threads * tasks_per_thread);
}

TEST_CASE("Race between concurrent submissions and shutdown", "[race][shutdown][stress]")
{
  threadpool pool(4);
  std::atomic<bool> shutdown_requested{false};
  std::atomic<int> submitted{0};
  std::atomic<int> executed{0};

  // 启动多个提交线程
  std::vector<std::thread> submitters;
  for (int i = 0; i < 16; ++i)
  {
    submitters.emplace_back([&] {
      while (!shutdown_requested)
      {
        try
        {
          pool.submit([&executed] {
            std::this_thread::sleep_for(std::chrono::microseconds(20));
            executed.fetch_add(1, std::memory_order_relaxed);
          });
          submitted++;
        }
        catch (...)
        {
          // 提交失败（线程池已关闭）时跳过
        }
      }
    });
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));    // 允许提交一段时间
  pool.shutdown(threadpool::shutdown_mode::DiscardPendingTasks);  // 启动关闭流程
  shutdown_requested = true;

  for (auto& t : submitters) t.join();

  REQUIRE_FALSE(pool.is_running());
  REQUIRE(executed <= submitted);  // 执行数应该不大于提交数
}

TEST_CASE("Recursive task submissions (C++11)", "[recursive][submit][stress]")
{
  threadpool pool(4);         // 使用 4 个线程池
  std::atomic<int> depth{0};  // 跟踪递归深度

  // 递归提交任务
  std::function<void(int)> recursive_submit = [&](int level) -> void {
    if (level <= 0) return;  // 递归结束条件
    pool.submit([&, level] {
      depth.fetch_add(1, std::memory_order_relaxed);  // 增加深度计数
      recursive_submit(level - 1);                    // 递归提交
    });
  };

  for (int i = 0; i < 8; ++i)
  {
    recursive_submit(50);  // 每条链递归 50 次
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(300));  // 等待任务完成
  pool.shutdown();                                              // 关闭线程池

  REQUIRE(depth > 0);  // 确保任务已经提交并执行
}

TEST_CASE("wait_all returns immediately if no task is pending", "[wait_all][empty]")
{
  abin::threadpool pool(2);
  pool.wait_all();  // Should return instantly
  REQUIRE(pool.pending_tasks() == 0);
  REQUIRE(pool.busy_threads() == 0);
}

TEST_CASE("wait_all waits until all tasks complete", "[wait_all][basic]")
{
  abin::threadpool pool(2);
  std::atomic<int> counter{0};

  for (int i = 0; i < 10; ++i)
  {
    pool.submit([&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(30));
      ++counter;
    });
  }

  pool.wait_all();  // Should block until all 10 tasks are done
  REQUIRE(counter == 10);
}

TEST_CASE("wait_all with recursive submissions", "[wait_all][recursive]")
{
  abin::threadpool pool(4);
  std::atomic<int> depth{0};

  std::function<void(int)> recursive_submit = [&](int level) {
    if (level <= 0) return;
    pool.submit([&, level] {
      depth.fetch_add(1, std::memory_order_relaxed);
      recursive_submit(level - 1);
    });
  };

  for (int i = 0; i < 5; ++i)
  {
    recursive_submit(30);  // Recursive depth chains
  }

  pool.wait_all();  // Ensure all recursive chains complete
  REQUIRE(depth > 0);
}

TEST_CASE("wait_all works with packaged_task", "[wait_all][packaged_task]")
{
  abin::threadpool pool(2);
  std::vector<std::future<int>> results;

  for (int i = 0; i < 5; ++i)
  {
    std::packaged_task<int()> task([i] {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      return i * i;
    });
    results.push_back(task.get_future());
    pool.submit(std::move(task));
  }

  pool.wait_all();  // Wait for all packaged_tasks
  for (int i = 0; i < 5; ++i)
  {
    REQUIRE(results[i].get() == i * i);
  }
}

TEST_CASE("wait_all then shutdown is safe", "[wait_all][shutdown]")
{
  abin::threadpool pool(3);
  std::atomic<int> sum{0};

  for (int i = 0; i < 6; ++i)
  {
    pool.submit([&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      sum += i;
    });
  }

  pool.wait_all();   // Ensure all tasks are done
  pool.shutdown();   // Shutdown after wait
  REQUIRE(sum > 0);  // Validate execution
}

TEST_CASE("submit after shutdown throws", "[submit][shutdown][exception]") {
  abin::threadpool pool(2);
  pool.shutdown();  // Explicit shutdown

  REQUIRE_THROWS_AS(pool.submit([] { return 42; }), std::runtime_error);
}

TEST_CASE("shutdown is idempotent", "[shutdown][safe]") {
  abin::threadpool pool(2);
  pool.shutdown();  // First shutdown
  REQUIRE_NOTHROW(pool.shutdown());  // Should not crash or throw again
}

TEST_CASE("reboot is idempotent when running", "[reboot][idempotent]") {
  abin::threadpool pool(2);
  pool.reboot(4);  // shutdown then restart
  pool.reboot(4);  // should be ignored if already running
  REQUIRE(pool.total_threads() == 4);
}

TEST_CASE("wait_all after shutdown returns immediately", "[wait_all][post-shutdown]") {
  abin::threadpool pool(2);
  pool.submit([] { std::this_thread::sleep_for(std::chrono::milliseconds(10)); });
  pool.shutdown();  // All done
  REQUIRE_NOTHROW(pool.wait_all());  // Should not hang or crash
}

TEST_CASE("exception inside task doesn't crash pool", "[exception][submit]") {
  abin::threadpool pool(2);
  auto fut = pool.submit([]() -> int {
    throw std::runtime_error("task failed");
  });

  REQUIRE_THROWS_AS(fut.get(), std::runtime_error);
  REQUIRE(pool.is_running());  // Pool should still be alive
}

TEST_CASE("submit supports void return type", "[submit][void]") {
  abin::threadpool pool(2);
  bool flag = false;

  auto fut = pool.submit([&] { flag = true; });
  fut.get();  // Should not throw

  REQUIRE(flag);
}

TEST_CASE("threadpool with 1 thread handles all tasks", "[single-threaded][submit]") {
  abin::threadpool pool(1);  // Single thread mode
  std::vector<std::future<int>> results;
  for (int i = 0; i < 10; ++i) {
    results.emplace_back(pool.submit([i] { return i * 2; }));
  }

  for (int i = 0; i < 10; ++i) {
    REQUIRE(results[i].get() == i * 2);
  }
}

TEST_CASE("destructor waits for all tasks", "[destructor][RAII]") {
  std::atomic<int> count{0};

  {
    abin::threadpool pool(2);
    for (int i = 0; i < 5; ++i) {
      pool.submit([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++count;
      });
    }
  }  // RAII scope ends, destructor calls wait_all + shutdown

  REQUIRE(count == 5);
}

TEST_CASE("invalid thread counts are rejected", "[validate][range]") {
  REQUIRE_THROWS_AS(abin::threadpool(0), std::invalid_argument);       // zero
  REQUIRE_THROWS_AS(abin::threadpool(4097), std::invalid_argument);    // too large
  REQUIRE_THROWS_AS(abin::threadpool(static_cast<std::size_t>(-5)), std::invalid_argument);  // negative converted

  // Valid
  REQUIRE_NOTHROW(abin::threadpool(1));
  REQUIRE_NOTHROW(abin::threadpool(8));
  REQUIRE_NOTHROW(abin::threadpool(1024));
  REQUIRE_NOTHROW(abin::threadpool(4096));
}
