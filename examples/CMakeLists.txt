add_executable(pool_basic pool_basic.cpp)
target_link_libraries(pool_basic PRIVATE thread_pool)

add_executable(pool_multiple_tasks pool_multiple_tasks.cpp)
target_link_libraries(pool_multiple_tasks PRIVATE thread_pool)

add_executable(pool_shutdown pool_shutdown.cpp)
target_link_libraries(pool_shutdown PRIVATE thread_pool)

add_executable(pool_status pool_status.cpp)
target_link_libraries(pool_status PRIVATE thread_pool)

add_executable(pool_callable pool_callable.cpp)
target_link_libraries(pool_callable PRIVATE thread_pool)

add_executable(pool_wait_all pool_wait_all.cpp)
target_link_libraries(pool_wait_all PRIVATE thread_pool)