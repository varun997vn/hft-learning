#pragma once

#include <atomic>
#include <cstddef>
#include <new>

// Hardware destructive interference size is generally 64 bytes on x86 architectures.
// Some compilers support std::hardware_destructive_interference_size (C++17),
// but defining it directly as 64 is standard practice in many low-latency codebases
// to avoid compiler inconsistencies.
#if defined(__cpp_lib_hardware_interference_size)
    constexpr std::size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
#else
    constexpr std::size_t CACHE_LINE_SIZE = 64;
#endif

template <typename T, std::size_t Capacity>
class SPSCQueue {
private:
    // Force the write index onto its own cache line to avoid false sharing with the read index.
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> write_index_{0};

    // Force the read index onto a separate cache line.
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> read_index_{0};

    // Pad the data array to prevent it from sharing a cache line with the indices.
    // The alignment guarantees that the start of the array is on a new cache line.
    alignas(CACHE_LINE_SIZE) T data_[Capacity];

public:
    SPSCQueue() = default;
    ~SPSCQueue() = default;

    // Delete copy and move semantics to prevent accidental copying of the queue structure
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    /**
     * @brief Pushes an item into the queue.
     * @param item The item to insert.
     * @return true if successful, false if the queue is full.
     */
    bool push(const T& item) {
        // Load the current write index. Relaxed is fine here because only the producer modifies it.
        const std::size_t current_write = write_index_.load(std::memory_order_relaxed);
        const std::size_t next_write = (current_write + 1) % Capacity;

        // Load the read index with acquire semantics to see the latest progress from the consumer.
        // If the next write position equals the current read position, the queue is full.
        if (next_write == read_index_.load(std::memory_order_acquire)) {
            return false; 
        }

        // Write the data into the array
        data_[current_write] = item;
        
        // Update the write index with release semantics. This guarantees that the data write
        // above is fully committed to memory before the consumer sees the updated write index.
        write_index_.store(next_write, std::memory_order_release);
        return true;
    }

    /**
     * @brief Pops an item from the queue.
     * @param item Reference to store the popped item.
     * @return true if successful, false if the queue is empty.
     */
    bool pop(T& item) {
        // Load the current read index. Relaxed is fine because only the consumer modifies it.
        const std::size_t current_read = read_index_.load(std::memory_order_relaxed);

        // Load the write index with acquire semantics to see the latest progress from the producer.
        // If the read position equals the write position, the queue is empty.
        if (current_read == write_index_.load(std::memory_order_acquire)) {
            return false; 
        }

        // Read the data from the array
        item = data_[current_read];

        // Update the read index with release semantics. This guarantees the data read
        // completes before the producer sees the updated read index (freeing up the slot).
        const std::size_t next_read = (current_read + 1) % Capacity;
        read_index_.store(next_read, std::memory_order_release);
        return true;
    }
};
