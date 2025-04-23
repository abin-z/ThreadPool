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

  pool.shutdown();     // 第一次关闭
  pool.shutdown();     // 再次调用应无异常
  pool.shutdown();     // 多次也应该没问题

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
  pool.shutdown(threadpool::shutdown_mode::DiscardTasks);

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

  pool.shutdown(threadpool::shutdown_mode::DiscardTasks);

  REQUIRE(pool.is_running() == false);
  REQUIRE(executed_tasks.load() <= 2);  // 最多只能有2个任务开始执行
}


TEST_CASE("ThreadPool throws if submit after shutdown", "[submit][error]")
{
  threadpool pool(1);
  pool.shutdown();

  REQUIRE_THROWS_AS(pool.submit([] { return 1; }), std::runtime_error);
}