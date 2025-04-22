#include <iostream>

#include "thread_pool/thread_pool.h"

double add(double x, double y)
{
  return x + y;
}

void sayhello()
{
  std::cout << "[sayhello] hello world!" << std::endl;
}

int main()
{
  concurrency::threadpool pool(4);

  auto future = pool.submit([] {
    std::cout << "[lambda] Hello from thread pool!" << std::endl;
    return 42;
  });
  auto future2 = pool.submit(add, 1.2, 2.5);
  pool.submit(sayhello);  // 无返回值的task, 摒弃submit的返回值

  int result = future.get();  // 阻塞直到任务完成
  std::cout << "Result: " << result << std::endl;

  double result2 = future2.get();  // 阻塞直到任务完成
  std::cout << "Result2: " << result2 << std::endl;

  return 0;
}
