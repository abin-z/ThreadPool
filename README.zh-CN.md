# ThreadPool

一个轻量、易用、支持任务返回值、基于 C++11 的跨平台线程池库。

📦 Header-only
 🚀 支持任意参数任务提交
 🔄 可获取返回值 (`std::future`)
 🧵 自动管理线程池
 💡 简洁 API，快速上手

------

## 📌 特性

- ✅ **任务提交灵活**：支持任意参数的任务提交，返回 `std::future` 获取结果
- ✅ **线程安全**：使用 `std::mutex` 和 `std::condition_variable` 实现同步
- ✅ **可选关闭策略**：默认是自动管理线程池的, 如果有需要可以手动关闭线程池:
  - `WaitForAllTasks`: 等待所有任务完成后关闭
  - `DiscardPendingTasks`: 丢弃未开始的任务立即关闭
- ✅ **跨平台**：兼容支持 C++11 的编译器
- ✅ **纯头文件**：仅包含一个头文件 `thread_pool.h`，无需编译、链接

------

## 📦 快速开始

### 安装

只需包含头文件：

```cpp
#include "thread_pool.h"
```

无需额外依赖，完全头文件实现。

### 示例代码

```cpp
#include "thread_pool.h"
#include <iostream>

int main() {
  abin::threadpool pool;

  // 提交一个简单任务
  auto future = pool.submit([] { return 42; });
  std::cout << "结果: " << future.get() << std::endl;

  // 提交带参数任务
  auto f2 = pool.submit([](int a, int b) { return a + b; }, 3, 4);
  std::cout << "和: " << f2.get() << std::endl;

  // 自动析构时会等待任务完成并关闭线程池
  return 0;
}
```

------

## ⚙️ API 文档

### 构造函数

```cpp
explicit threadpool(std::size_t thread_count = default_thread_count());
```

- 自动启动 `thread_count` 个工作线程，默认使用 `std::thread::hardware_concurrency()`。

### 提交任务

```cpp
template <typename F, typename... Args>
auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;
```

- 支持任意函数和参数
- 返回 `std::future` 对象以获取结果

### 关闭线程池

```cpp
void shutdown(shutdown_mode mode = shutdown_mode::WaitForAllTasks);
```

- 等待所有任务完成或立即丢弃未开始任务并退出
- 调用后线程池不可继续提交任务

### 重启线程池

```cpp
void reboot(std::size_t thread_count);
```

- 安全地关闭并以指定线程数重启线程池

### 获取状态信息

```cpp
bool is_running() const noexcept;
std::size_t total_threads() const noexcept;
std::size_t busy_threads() const noexcept;
std::size_t idle_threads() const noexcept;
std::size_t pending_tasks() const noexcept;
threadpool::status status() const noexcept;
```

------

## 🧪 单元测试

你可以使用 Catch2/C++ 测试框架进行测试，示例见 `tests/` 目录。

------

## 📄 License

本项目基于 [MIT License](LICENSE) 开源。

------

## 🙋‍♂️ 作者

Abin
 📧 [GitHub](https://github.com/abin-z)
 🗨️ 欢迎 Issue 与 PR！