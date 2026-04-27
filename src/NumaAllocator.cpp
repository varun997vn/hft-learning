#include "../include/NumaAllocator.hpp"
#include <numa.h>
#include <stdexcept>
#include <iostream>
#include "../include/AsyncLogger.hpp"

namespace NumaMemoryUtils {

    void* allocate_on_node(std::size_t size, int node) {
        // Check if NUMA is available on the system
        if (numa_available() < 0) {
            HFT::AsyncLogger::getInstance().log(HFT::LogLevel::WARN, "NUMA is not available on this system. Falling back to standard allocation.");
            // Fallback to standard allocation, though in an HFT setting we might prefer to fail hard.
            return ::operator new(size);
        }

        // Check if the requested node is valid
        if (node > numa_max_node()) {
            HFT::AsyncLogger::getInstance().log(HFT::LogLevel::WARN, "Requested NUMA node %d exceeds maximum available node %d. Falling back to node 0.", node, numa_max_node());
            node = 0;
        }

        void* ptr = numa_alloc_onnode(size, node);
        if (!ptr) {
            throw std::bad_alloc();
        }
        
        return ptr;
    }

    void free_memory(void* ptr, std::size_t size) {
        if (!ptr) return;

        if (numa_available() < 0) {
            ::operator delete(ptr);
        } else {
            numa_free(ptr, size);
        }
    }

} // namespace NumaMemoryUtils
