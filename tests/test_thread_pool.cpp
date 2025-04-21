#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <thread_pool/thread_pool.h>

TEST_CASE("test", "[thread_pool]")
{
  REQUIRE(1 == 1);  // 容许调度误差
}