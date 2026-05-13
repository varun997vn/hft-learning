---
id: async-lock-free-logger
title: Async Lock-Free Logger
sidebar_label: Lock-Free Logging
sidebar_position: 7
---

# Asynchronous Lock-Free Logging

In a High-Frequency Trading system, observability is paramount. However, standard logging libraries typically rely on mutexes or synchronous file I/O, which introduce catastrophic latency spikes (jitter) into the critical path. 

To solve this, our pipeline includes a custom **`AsyncLogger`**, ensuring that writing an execution report or a debug statement takes only a few nanoseconds on the trading thread.

## Design Architecture

The `AsyncLogger` completely decouples the formatting and I/O operations from the hot path using a lock-free Single-Producer Single-Consumer (SPSC) queue and a dedicated background thread.

### The Hot Path (Producer)
When the Strategy Engine or Network Thread calls `LOG_INFO()`, the following occurs:
1. The log message string and a high-precision `rdtsc` timestamp are packed into a lightweight `LogEvent` struct.
2. The `LogEvent` is pushed onto a pre-allocated, lock-free ring buffer (similar to the market data `SPSCQueue`).
3. The function returns immediately. There are no allocations, no locks, and no string formatting on the critical path.

### The Background Thread (Consumer)
The `AsyncLogger` spawns a dedicated background I/O thread during initialization.
1. The thread constantly polls the SPSC ring buffer for new `LogEvent` structures.
2. When an event is popped, the background thread formats the timestamp and writes the payload to disk using buffered I/O (`std::ofstream`).
3. If the queue is empty, the background thread yields or sleeps to avoid starving the system, as it operates off the critical NUMA nodes.

## Zero-Allocation Formatting

C++ `std::string` and `std::stringstream` frequently allocate memory on the heap. To maintain a zero-allocation guarantee on the critical path, the `AsyncLogger` accepts static `char*` arrays or `std::string_view`. String formatting, if necessary, is deferred to the background thread wherever possible, or handled via `constexpr` string concatenation at compile-time.

## The Ring Buffer Implementation

The ring buffer powering the logger is cache-line aligned to prevent **False Sharing**. The producer (trading thread) writes to one cache line, while the consumer (logging thread) reads from another. This guarantees that the L1/L2 caches on the trading core are never invalidated by the background I/O operations.

By offloading I/O to a non-critical CPU core, the `AsyncLogger` provides deep introspection into the tick-to-trade pipeline without compromising microsecond latency budgets.
