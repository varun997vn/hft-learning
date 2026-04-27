# test_orderbook suite
add_executable(test_orderbook test_orderbook.cpp)
target_link_libraries(test_orderbook PRIVATE core_lib gtest_main)
add_test(NAME OrderBookTests COMMAND test_orderbook)

# test_spsc_queue suite
add_executable(test_spsc_queue test_spsc_queue.cpp)
target_link_libraries(test_spsc_queue PRIVATE core_lib gtest_main)
add_test(NAME SPSCQueueTests COMMAND test_spsc_queue)

# test_numa_allocator suite
add_executable(test_numa_allocator test_numa_allocator.cpp)
target_link_libraries(test_numa_allocator PRIVATE core_lib gtest_main)
add_test(NAME NumaAllocatorTests COMMAND test_numa_allocator)

# test_thread_affinity suite
add_executable(test_thread_affinity test_thread_affinity.cpp)
target_link_libraries(test_thread_affinity PRIVATE core_lib gtest_main)
add_test(NAME ThreadAffinityTests COMMAND test_thread_affinity)
