---
id: lob-benchmarking
title: LOB Benchmarking
sidebar_label: Before vs After Benchmark
sidebar_position: 5
---

# Limit Order Book Benchmarking

To demonstrate the impact of our zero-allocation, cache-aligned design, we benchmarked our highly optimized `LimitOrderBook` against a typical standard library approach (`NaiveOrderBook`).

## Methodology

The benchmark measures the latency of two critical path operations:
1. **Add Order**: Submitting a new limit order to the book.
2. **Cancel Order**: Removing an existing order from the book.

We measure operations across 100,000 iterations after a 10,000 iteration cache warm-up phase. The clock cycles are recorded using the CPU's Time Stamp Counter (TSC) via `rdtsc` for nanosecond precision.

### The Naive Baseline

The `NaiveOrderBook` is implemented exactly how a developer might write a first-pass solution:
- It uses `std::map<uint64_t, std::list<Order>>` to manage price levels and queues.
- It uses `std::unordered_map<uint64_t, std::list<Order>::iterator>` to provide fast order lookups by ID.

This approach relies heavily on standard dynamic allocation (`new` and `delete`), pointer chasing, and tree re-balancing.

### The Optimized LOB

Our custom `LimitOrderBook` uses:
- Pre-allocated memory pools (Mmap).
- Intrusive doubly-linked lists.
- Flat arrays instead of trees for price indexing.

## Results: Before vs After

*System Details: Tested on an Intel Xeon architecture utilizing `std::chrono` and `rdtsc` instruction calibration.*

### 1. Add Order Latency

| Metric  | Naive Order Book (Before) | Optimized Zero-Allocation (After) | Improvement |
|---------|---------------------------|-----------------------------------|-------------|
| **Mean**| 701.32 ns                | 28.53 ns                        | **24.5x**   |
| **p50** | 447.72 ns                | 26.73 ns                        | **16.7x**   |
| **p99** | 3992.33 ns               | 100.23 ns                       | **39.8x**   |
| **p99.9**| 7362.22 ns              | 135.56 ns                       | **54.3x**   |

### 2. Cancel Order Latency

| Metric  | Naive Order Book (Before) | Optimized Zero-Allocation (After) | Improvement |
|---------|---------------------------|-----------------------------------|-------------|
| **Mean**| 269.59 ns                | 29.46 ns                        | **9.1x**    |
| **p50** | 273.98 ns                | 27.68 ns                        | **9.8x**    |
| **p99** | 523.14 ns                | 62.05 ns                        | **8.4x**    |
| **p99.9**| 757.98 ns               | 133.65 ns                       | **5.6x**    |

## Analysis

The results are staggering. The optimized book achieves **sub-30ns mean latency** for both Adds and Cancels, outperforming the naive approach by up to **54x** at the 99.9th percentile.

### Why is the naive approach so slow?
1. **Kernel Overhead**: `std::map` and `std::list` perform heap allocations (`malloc`/`new`). When the allocator is called, it often has to take kernel locks.
2. **Cache Misses**: Tree nodes in `std::map` are scattered across the heap. Traversing the map to find a price level incurs costly L1/L2 cache misses.

### Why does our optimized version win?
1. **Flat Arrays**: Lookups to a price level are an $O(1)$ flat array index, avoiding tree traversal entirely.
2. **Memory Locality**: Orders are allocated from a single contiguous block of memory (`mmap`), meaning adjacent elements are frequently loaded together into the CPU cache line (64 bytes).
3. **Intrusive Lists**: Removing an order from a price level simply changes two pointers (`prev` and `next`), avoiding any container logic overhead.
