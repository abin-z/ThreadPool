#include <thread_pool/thread_pool.h>

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

  // 提交一个普通函数
  pool.submit(normal_function, 42);

  // 提交一个无捕获 lambda
  pool.submit([] { std::cout << "lambda no capture\n"; });

  // 提交一个有捕获 lambda
  int value = 99;
  pool.submit([value] { std::cout << "lambda with capture: " << value << "\n"; });

  // 提交成员函数, 使用lambda
  MyClass obj;
  pool.submit([&obj] { obj.member_function(123); });

  // 提交成员函数, 使用 std::mem_fn
  std::future<int> ret = pool.submit(std::mem_fn(&MyClass::add), &obj, 3, 4);
  std::cout << "add result1: " << ret.get() << "\n";

  // 提交成员函数, 使用 std::bind
  std::future<int> fut_add = pool.submit(std::bind(&MyClass::add, &obj, 2, 3));
  std::cout << "add result2: " << fut_add.get() << "\n";

  // 提交一个函数对象(仿函数)
  Functor f;
  pool.submit(f, "hello functor");

  // 使用 std::bind 提交
  auto bound = std::bind(&MyClass::add, &obj, 5, 6);
  std::future<int> fut_bound = pool.submit(bound);
  std::cout << "bound result: " << fut_bound.get() << "\n";

  // 提交一个 std::packaged_task(注意: 低版本msvc可能报错)
  std::packaged_task<std::string()> task([] { return std::string("from packaged_task"); });
  std::future<std::string> fut_str = task.get_future();
  pool.submit(std::move(task));  // 必须 move
  std::cout << "packaged_task result: " << fut_str.get() << "\n";

  pool.shutdown();  // 停止线程池，等待任务完成
}
