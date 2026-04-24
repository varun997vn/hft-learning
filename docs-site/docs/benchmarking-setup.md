---
sidebar_position: 3
---

# Benchmarking Setup

To accurately measure sub-microsecond latencies, we bypass standard OS clocks (`std::chrono`) which carry significant overhead and jitter.

## Using RDTSC
We use the CPU's Time Stamp Counter (TSC). The `__rdtscp()` intrinsic reads the hardware cycle counter and acts as a barrier to prevent the CPU from reordering instructions.

```cpp
inline uint64_t read_tsc() {
    unsigned int aux;
    return __rdtscp(&aux);
}
```

By estimating the TSC frequency against a standard clock over a long duration (100ms), we can convert raw cycle deltas into nanoseconds.

## Thread Pinning and NUMA Memory
We pin our Producer and Consumer threads to specific logical cores using `pthread_setaffinity_np()`. The memory for the SPSC queue is explicitly allocated on a defined NUMA node using `numa_alloc_onnode()` from `libnuma`.

We run two identical test binaries to observe the "Latency Cliff":
- **Test A (Local NUMA):** Cores are on the same NUMA node as the queue memory.
- **Test B (Cross-NUMA):** One core is forced to cross the QPI/UPI interconnect to read the other node's memory.
