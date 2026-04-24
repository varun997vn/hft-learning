#include "../include/NumaAllocator.hpp"
#include <numa.h>
#include <stdexcept>
#include <iostream>

namespace NumaMemoryUtils {

    void* allocate_on_node(std::size_t size, int node) {
        // Check if NUMA is available on the system
        if (numa_available() < 0) {
            std::cerr << "Warning: NUMA is not available on this system. Falling back to standard allocation." << std::endl;
            // Fallback to standard allocation, though in an HFT setting we might prefer to fail hard.
            return ::operator new(size);
        }

        // Check if the requested node is valid
        if (node > numa_max_node()) {
            std::cerr << "Warning: Requested NUMA node " << node << " exceeds maximum available node " << numa_max_node() << ". Falling back to node 0." << std::endl;
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
