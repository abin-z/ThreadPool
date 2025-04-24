# 基于c++11的轻量级线程池

**一个跨平台, 简单易用的Header-only线程池库, 基于Task提交, 支持提交任意参数提交, 支持获取返回值.**

------

### 📌 线程池简介

**线程池（Thread Pool）**是一种基于**池化思想**管理线程的工具，经常出现在多线程编程中。

它的核心思想是：**预先创建一定数量的线程放在“池子”里，任务来了就把任务交给空闲的线程来处理，而不是每次都新建线程。**

![Thread_pool.svg](assets/Thread_pool.svg.png)

------

### 🚀特性亮点

- **任务提交灵活**：支持任意可调用对象与参数组合，返回 `std::future<T>` 获取执行结果
- **线程安全**：使用 `std::mutex` / `std::condition_variable` / `std::atomic` 构建同步机制
- **跨平台**：纯 C++11 实现，兼容 Windows 与 POSIX 等系统
- **Header-only**：仅需包含 `thread_pool.h`，零依赖，即可使用
- **RAII 自动管理资源**：析构时自动关闭线程池，防止资源泄露
- **任务等待机制**：支持主动调用 `wait_all()` 等待所有任务完成
- **灵活关闭策略**：默认是自动关闭线程池的, 如果有需要可以手动关闭线程池:
  - `WaitForAllTasks`: 等待所有任务完成后关闭
  - `DiscardPendingTasks`: 丢弃未开始的任务立即关闭

------

### 📦 快速开始

#### 安装使用

拷贝[`thread_pool.h`](include/thread_pool/thread_pool.h)到你的项目目录，然后在代码中引入：

```cpp
#include "thread_pool.h"
```

无需额外依赖，完全头文件实现。

#### 基础示例代码

```cpp
#include "thread_pool.h"
#include <iostream>

int main() {
  abin::threadpool executor(4);

  auto future1 = executor.submit([] { return 42; });
  std::cout << "结果: " << future1.get() << "\n";

  auto future2 = executor.submit([](int a, int b) { return a + b; }, 5, 7);
  std::cout << "加法结果: " << future2.get() << "\n";

  return 0;
}
```

更多更详细的使用案例, 请移步到[`examples`](examples/)文件夹下查看

------

### 📄  API 文档

#### 构造与析构

```cpp
explicit threadpool(std::size_t thread_count = default_thread_count());
~threadpool();
```

- 构造时自动启动 `thread_count` 个工作线程
- 默认线程数使用 `std::thread::hardware_concurrency()`（若不可用则为 4）
- 析构时自动调用 `shutdown(WaitForAllTasks)`

#### 提交任务

```cpp
template <typename F, typename... Args>
auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;
```

- 将任务提交至线程池异步执行
- 支持任意函数和参数组合 (结合lambda可调用成员函数)
- 返回 `std::future` 对象以获取结果

#### 等待任务完成

```cpp
void wait_all();
```

- 阻塞直到所有任务执行完毕（任务队列为空且无活跃线程）
- 若无任务，立即返回

#### 关闭线程池

```cpp
void shutdown(shutdown_mode mode = shutdown_mode::WaitForAllTasks);
```

- 等待所有任务完成或立即丢弃未开始任务并退出
- 调用后线程池不可继续提交任务

#### 重启线程池

```cpp
void reboot(std::size_t thread_count);
```

- 安全地关闭并以指定线程数重启线程池

#### 获取状态信息

```cpp
bool is_running() const noexcept;               // 线程池是否在运行
std::size_t total_threads() const noexcept;     // 线程池总共的线程数
std::size_t busy_threads() const noexcept;      // 繁忙的线程数量
std::size_t idle_threads() const noexcept;      // 空闲的线程数量
std::size_t pending_tasks() const noexcept;     // 正在等待的任务数量
threadpool::status status() const noexcept;     // 状态信息汇总
```

------

### 💡 贡献指南

🗨️ 欢迎提交 **Issue** 和 **Pull request** 来改进本项目！

-----

### 🙌 致谢

感谢 **[Catch2](https://github.com/catchorg/Catch2)** 提供强大支持，助力本项目的单元测试!

------

### 📜 许可证

本项目采用[ **MIT** 许可证](./LICENSE)。版权所有 © 2025–Present Abin。

------

### 🙋‍♂️ 作者

Abin 📧 [GitHub](https://github.com/abin-z)
