#include "../include/SPSCQueue.hpp"
#include "../include/NumaAllocator.hpp"
#include "../include/ThreadAffinity.hpp"

#include <iostream>
#include <thread>
#include <cassert>
#include <vector>

void test_spsc_queue() {
    std::cout << "Testing SPSC Queue..." << std::endl;
    // Capacity must be greater than number of items we produce for this simple sequential test
    SPSCQueue<int, 1024> queue;
    
    // Test push
    for (int i = 0; i < 100; ++i) {
        assert(queue.push(i));
    }

    // Test pop
    int val;
    for (int i = 0; i < 100; ++i) {
        assert(queue.pop(val));
        assert(val == i);
    }
    
    assert(!queue.pop(val)); // Queue should be empty now
    std::cout << "SPSC Queue basic tests passed." << std::endl;
}

void test_numa_allocator() {
    std::cout << "Testing NumaAllocator..." << std::endl;
    
    // Create an SPSC Queue on NUMA node 0
    auto* queue = NumaMemoryUtils::create_on_node<SPSCQueue<int, 1024>>(0);
    assert(queue != nullptr);
    
    assert(queue->push(42));
    int val;
    assert(queue->pop(val));
    assert(val == 42);

    NumaMemoryUtils::destroy_and_free(queue);
    std::cout << "NumaAllocator tests passed." << std::endl;
}

void test_thread_affinity() {
    std::cout << "Testing ThreadAffinity..." << std::endl;
    std::thread t([]() {
        // Just pin to core 0 as a test
        bool success = ThreadPinning::pin_current_thread_to_core(0);
        if (success) {
            std::cout << "Successfully pinned thread to core 0." << std::endl;
        } else {
            std::cout << "Failed to pin thread. (This might be expected if core 0 is unavailable or lacks permissions)." << std::endl;
        }
    });
    t.join();
    std::cout << "ThreadAffinity tests completed." << std::endl;
}

int main() {
    test_spsc_queue();
    test_numa_allocator();
    test_thread_affinity();

    std::cout << "All Phase 1 validations passed!" << std::endl;
    return 0;
}
