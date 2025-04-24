# Lightweight Thread Pool (C++11)

[![threadpool](https://img.shields.io/badge/Thread_Pool-8A2BE2)](https://github.com/abin-z/ThreadPool) [![headeronly](https://img.shields.io/badge/Header_Only-green)](include/thread_pool/thread_pool.h) [![moderncpp](https://img.shields.io/badge/Modern_C%2B%2B-218c73)](https://learn.microsoft.com/en-us/cpp/cpp/welcome-back-to-cpp-modern-cpp?view=msvc-170) [![licenseMIT](https://img.shields.io/badge/License-MIT-green)](https://opensource.org/license/MIT) [![version](https://img.shields.io/badge/version-0.9.2-green)](https://github.com/abin-z/ThreadPool/releases)

🌍 Languages/语言:  [English](README.md)  |  [简体中文](README.zh-CN.md)

**A cross-platform, header-only, task-based C++11 thread pool supporting arbitrary arguments and return values via `std::future`.**

## 📌 Overview

A **Thread Pool** is a tool that manages threads based on the **pooling concept**, commonly used in multithreaded programming.

The core idea is: **a certain number of threads are pre-created and placed in a "pool". When a task arrives, it is assigned to an idle thread for processing, rather than creating a new thread every time.**

![Thread_pool.svg](assets/Thread_pool.svg.png)

## ✨ Features

- **Flexible Task Submission**: Supports arbitrary callable types with arguments; returns a `std::future<T>`
- **Thread-Safe**: Built using `std::mutex`, `std::condition_variable`, and `std::atomic`
- **Cross-Platform**: Pure C++11 implementation, works on Windows, Linux, and more
- **Header-Only**: Just include the single file `thread_pool.h` in your project
- **RAII Resource Management**: Threads are cleanly shut down in destructor
- **Task Completion Wait**: Supports waiting for all tasks to finish with `wait_all()`
- **Shutdown Modes**:
  - `WaitForAllTasks`: Complete all tasks before shutdown
  - `DiscardPendingTasks`: Discard pending tasks and shut down immediately

## 📦 Getting Started

### Installation

Copy [`thread_pool.h`](include/thread_pool/thread_pool.h) into your project and include it:

```cpp
#include "thread_pool.h"
```

No additional dependencies required.

### Basic Example

**basic usage**

```cpp
#include "thread_pool.h"
#include <iostream>

int main() {
  abin::threadpool pool(4);

  auto future1 = pool.submit([] { return 42; });
  std::cout << "Result: " << future1.get() << "\n";

  auto future2 = pool.submit([](int a, int b) { return a + b; }, 5, 7);
  std::cout << "Sum: " << future2.get() << "\n";

  return 0;
}
```

**Submit a callable object of any type with any arguments**

<details>
<summary>Click to expand and view the code</summary>

```cpp
#include "thread_pool.h"

#include <functional>
#include <future>
#include <iostream>
#include <string>

void normal_function(int x)
{
  std::cout << "normal_function: " << x << std::endl;
}

struct MyClass
{
  void member_function(int y)
  {
    std::cout << "MyClass::member_function: " << y << std::endl;
  }
  int add(int a, int b)
  {
    return a + b;
  }
};

struct Functor
{
  void operator()(const std::string& msg) const
  {
    std::cout << "Functor called with: " << msg << std::endl;
  }
};

int main()
{
  abin::threadpool pool(4);

  // Submit a regular function
  pool.submit(normal_function, 42);

  // Submit a lambda without capture
  pool.submit([] { std::cout << "lambda no capture\n"; });

  // Submit a lambda with capture
  int value = 99;
  pool.submit([value] { std::cout << "lambda with capture: " << value << "\n"; });

  // Submit a member function using lambda
  MyClass obj;
  pool.submit([&obj] { obj.member_function(123); });

  // Submit a member function using std::mem_fn
  std::future<int> ret = pool.submit(std::mem_fn(&MyClass::add), &obj, 3, 4);
  std::cout << "add result1: " << ret.get() << "\n";

  // Submit a member function using std::bind
  std::future<int> fut_add = pool.submit(std::bind(&MyClass::add, &obj, 2, 3));
  std::cout << "add result2: " << fut_add.get() << "\n";

  // Submit a function object (functor)
  Functor f;
  pool.submit(f, "hello functor");

  // Submit using std::bind
  auto bound = std::bind(&MyClass::add, &obj, 5, 6);
  std::future<int> fut_bound = pool.submit(bound);
  std::cout << "bound result: " << fut_bound.get() << "\n";

  // Submit a std::packaged_task (Note: older versions of MSVC may report errors)
  std::packaged_task<std::string()> task([] { return std::string("from packaged_task"); });
  std::future<std::string> fut_str = task.get_future();
  pool.submit(std::move(task));  // Must be moved
  std::cout << "packaged_task result: " << fut_str.get() << "\n";

  pool.wait_all();  // Wait for all tasks to finish
  std::cout << "===All tasks completed.===\n";
}
```

</details>

For more detailed use cases, please go to the [`examples`](examples/) folder to view them.

## 📄 API Reference

### Constructor & Destructor

```cpp
explicit threadpool(std::size_t thread_count = default_thread_count());
~threadpool();
```

- Initializes and launches the thread pool
- Thread count defaults to `std::thread::hardware_concurrency()`, or 4 if unknown
- Destructor automatically shuts down all threads (waits for tasks to complete)

------

### Task Submission

```cpp
template <typename F, typename... Args>
auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;
```

- Asynchronously submits a task
- Returns a `std::future<T>` for the result
- Throws `std::runtime_error` if the pool is stopped

------

### Waiting for Completion

```cpp
void wait_all();
```

- Blocks until all tasks have finished
- Returns immediately if no tasks are pending

------

### Shutdown

```cpp
void shutdown(shutdown_mode mode = shutdown_mode::WaitForAllTasks);
```

- Gracefully or forcefully stops the thread pool
- Cannot submit new tasks after shutdown

------

### Restart

```cpp
void reboot(std::size_t thread_count);
```

- Shuts down current pool and restarts with new thread count
- No effect if the pool is already running

------

### Status Queries

```cpp
bool is_running() const noexcept;                 // Whether the thread pool is running  
std::size_t total_threads() const noexcept;       // Total number of threads  
std::size_t busy_threads() const noexcept;        // Number of busy threads  
std::size_t idle_threads() const noexcept;        // Number of idle threads  
std::size_t pending_tasks() const noexcept;       // Number of pending tasks  
threadpool::status_info status() const noexcept;  // Summary of status info  
```

- Provides detailed insight into the internal state of the pool

------

## 💡 Contribution Guidelines

🗨️ Welcome to submit **Issue** and **Pull request** to improve this project!

-----

## 🙌 Acknowledgements

Thanks to **[Catch2](https://github.com/catchorg/Catch2)** for providing great support and helping with unit testing of this project!

Thanks to **https://github.com/progschj/ThreadPool** for providing inspiration for this project!

------

## 📜 License

This project uses the [ **MIT** License](./LICENSE). 

Copyright © 2025–Present Abin.

------

## 🙋‍♂️ Author

**Abin**  📎 [GitHub](https://github.com/abin-z)

