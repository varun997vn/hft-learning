#pragma once

#include <cstddef>
#include <utility>
#include <new>

namespace NumaMemoryUtils {

    /**
     * @brief Allocates memory on a specific NUMA node.
     * @param size The number of bytes to allocate.
     * @param node The NUMA node ID to allocate memory on.
     * @return Pointer to the allocated memory, or nullptr on failure.
     */
    void* allocate_on_node(std::size_t size, int node);

    /**
     * @brief Frees memory allocated via allocate_on_node.
     * @param ptr Pointer to the allocated memory.
     * @param size The number of bytes originally allocated.
     */
    void free_memory(void* ptr, std::size_t size);

    /**
     * @brief Utility function to construct an object of type T on a specific NUMA node.
     * @tparam T The type of the object.
     * @tparam Args Arguments to pass to T's constructor.
     * @param node The NUMA node ID.
     * @param args The arguments.
     * @return Pointer to the constructed object.
     */
    template <typename T, typename... Args>
    T* create_on_node(int node, Args&&... args) {
        void* memory = allocate_on_node(sizeof(T), node);
        if (!memory) {
            return nullptr;
        }
        // Use placement new to construct the object in the NUMA-allocated memory
        return new (memory) T(std::forward<Args>(args)...);
    }

    /**
     * @brief Utility function to destroy an object created via create_on_node.
     * @tparam T The type of the object.
     * @param ptr Pointer to the object.
     */
    template <typename T>
    void destroy_and_free(T* ptr) {
        if (ptr) {
            ptr->~T(); // Call the destructor
            free_memory(ptr, sizeof(T));
        }
    }

} // namespace NumaMemoryUtils
