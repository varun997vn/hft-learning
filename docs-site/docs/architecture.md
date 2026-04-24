---
sidebar_position: 2
---

# Architecture

The core architecture revolves around mechanical sympathy: structuring data strictly to how the CPU cache and memory subsystem operate.

## False Sharing and Cache Line Padding
Modern CPUs read memory in chunks called cache lines (typically 64 bytes on x86). If two threads modify different variables that happen to sit on the same cache line, the CPU will continuously invalidate and fetch the cache line across cores, causing a severe performance degradation known as **false sharing**.

To prevent this, our SPSC Queue pads the `read_index_` and `write_index_`:

```cpp
// Force the write index onto its own cache line
alignas(64) std::atomic<std::size_t> write_index_{0};

// Force the read index onto a separate cache line
alignas(64) std::atomic<std::size_t> read_index_{0};
```

## Memory Ordering
By using C++ `std::memory_order_acquire` and `std::memory_order_release`, we ensure cross-thread visibility without heavy sequential consistency overhead.

- The Producer uses `memory_order_release` when updating the write index, ensuring the data is strictly committed to memory first.
- The Consumer uses `memory_order_acquire` when reading the write index, ensuring it strictly observes the memory operations that happened before the producer's release.
