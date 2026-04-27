---
id: lob-system-design
title: Zero-Allocation System Design
sidebar_position: 2
---

# System Design for Nanosecond Latency

Achieving ultra-low, deterministic latency requires designing the Limit Order Book (LOB) with hardware sympathies in mind. The architecture must align with how modern CPUs cache and fetch memory.

## The Zero-Allocation Mandate

The most critical architectural decision in this LOB is the **zero dynamic allocation** requirement during the critical path (order matching, adding, canceling, and modifying).

### The Problem with `new` and `malloc`
Dynamic memory allocation involves the operating system traversing free lists, potentially splitting memory blocks, and sometimes requesting more memory pages from the kernel (triggering a context switch). This introduces non-deterministic latency spikes, which are fatal in High-Frequency Trading.

### The Solution: Upfront Allocation with `mmap`
Instead of allocating memory dynamically per order, the entire memory footprint of the Limit Order Book is allocated *once* at initialization.

In `OrderBook.cpp`, we utilize POSIX `mmap`:

```cpp
mmap_base_ = mmap(nullptr, total_mmap_size_, PROT_READ | PROT_WRITE, 
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
```

#### Why `MAP_POPULATE`?
The `MAP_POPULATE` flag is crucial. By default, `mmap` only reserves the virtual address space. When the program actually tries to write to that memory, the CPU triggers a **page fault**, pausing execution while the OS finds a physical RAM page to back it. `MAP_POPULATE` forces the OS to fault in all pages *during initialization*, ensuring no page faults occur during live trading.

#### Partitioning the Memory Block
The single massive block of memory returned by `mmap` is then deterministically partitioned into four continuous regions:
1. The `bids_` price levels array.
2. The `asks_` price levels array.
3. The contiguous `order_pool_`.
4. The contiguous `order_map_` (flat hash map).

This layout guarantees spatial locality and prevents fragmentation.

## Cache Line Alignment and False Sharing

Modern CPUs read memory from RAM in chunks called **cache lines**, typically 64 bytes in size on x86 architectures. If you request a single 4-byte integer, the CPU fetches the entire 64-byte line encompassing it into the L1/L2 cache.

### The `alignas(64)` Directive

To maximize cache efficiency and prevent false sharing, the fundamental `Order` structure is padded and aligned to exactly 64 bytes.

```cpp
struct alignas(64) Order {
    uint64_t id;         // 8 bytes
    uint64_t price;      // 8 bytes
    uint64_t quantity;   // 8 bytes
    Side side;           // 4 bytes (often padded to 8)
    
    Order* prev;         // 8 bytes
    Order* next;         // 8 bytes
};                       // Fits perfectly in one 64-byte L1 cache line
```

By ensuring that each `Order` starts on a 64-byte boundary and consumes exactly one cache line, we guarantee:
1. **No Cache Line Crossings**: The CPU never needs to fetch two separate cache lines to read a single order.
2. **Predictable Prefetching**: Because orders are allocated sequentially from the memory pool, the CPU's hardware prefetcher can easily predict and load upcoming orders into the cache before the software explicitly requests them.

In the next section, we will look at how this contiguous memory pool interacts with the data structures that power the O(1) matching engine.
