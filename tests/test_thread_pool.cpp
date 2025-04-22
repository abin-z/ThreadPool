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
