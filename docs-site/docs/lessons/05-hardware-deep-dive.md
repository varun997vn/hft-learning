---
sidebar_position: 1
title: CPU Architecture & Interconnects
---

# Deep Dive: CPU Architecture & Interconnects

To fully appreciate the performance gains of NUMA-aware programming, it is crucial to understand the physical hardware components that dictate memory latency. This lesson explores the hidden mechanisms inside modern servers.

## 1. The Memory Hierarchy

A CPU does not read from main memory (RAM) directly when it can avoid it; RAM is incredibly slow compared to the CPU's processing speed. Instead, data is pulled through a hierarchy of caches.

*   **L1 Cache (Registers & Level 1):** The fastest memory, embedded directly into the CPU core. Access time is roughly **1-2 nanoseconds**. It is very small (often 32KB to 64KB per core).
*   **L2 Cache:** Larger than L1 (e.g., 256KB to 1MB per core), but slightly slower. Access time is roughly **3-5 nanoseconds**.
*   **L3 Cache (Last Level Cache - LLC):** Shared across all cores on a single CPU socket/node. It is much larger (e.g., 20MB+) but slower, taking roughly **10-20 nanoseconds**.
*   **Main Memory (Local RAM):** Accessing the RAM sticks physically attached to the CPU socket takes roughly **60-100 nanoseconds**.
*   **Main Memory (Remote RAM):** Accessing RAM attached to a *different* CPU socket over the interconnect (cross-NUMA) can take **120+ nanoseconds**.

**The Lesson:** Every time your thread experiences a "cache miss" (the data isn't in L1/L2/L3) and has to fetch from RAM, your program stalls. If it has to fetch from *Remote* RAM, it stalls significantly longer.

## 2. QPI / UPI Interconnects

In a multi-socket server (like the Dual Intel Xeon Silver 4110 used in this project), the two physical CPU chips must communicate. They do this via a dedicated high-speed bus.

*   **Intel QPI (QuickPath Interconnect) / UPI (UltraPath Interconnect):** These are point-to-point interconnects that link the sockets. 
*   **The Bottleneck:** When a thread on Node 0 requests memory located on Node 1, the request must travel over the UPI link. This link has finite bandwidth. If many threads are heavily communicating across nodes, the UPI link becomes congested, leading to massive tail latencies (jitter). 

Our benchmarks explicitly visualize this congestion. By pinning threads and memory to the same node, we completely bypass the UPI link.

## 3. The MESI Protocol (Cache Coherence)

When multiple cores cache the same memory address, how does the system ensure they don't read stale data? Hardware implements a Cache Coherence protocol, the most common being **MESI**:

*   **Modified (M):** The cache line is valid only in this cache, and it has been changed (it is "dirty").
*   **Exclusive (E):** The cache line is valid only in this cache, and it matches main memory.
*   **Shared (S):** The cache line is valid in multiple caches, and matches main memory.
*   **Invalid (I):** The cache line data is stale and cannot be used.

**How it causes False Sharing:**
If Core 0 writes to `write_index_` (changing it to Modified), and Core 1 reads `read_index_` (which sits on the same 64-byte cache line), the hardware must invalidate Core 1's cache line, force Core 0 to flush its changes to RAM or L3, and then force Core 1 to fetch it again. 
By padding our indices with `alignas(64)`, we put them on separate cache lines, so Core 0 modifying its line does not trigger a MESI Invalid state on Core 1's line.

## 4. TLB and Huge Pages

When your program accesses memory, it uses "Virtual Addresses". The CPU must translate these to physical hardware addresses.

*   **TLB (Translation Lookaside Buffer):** A small, extremely fast hardware cache inside the MMU (Memory Management Unit) that stores recent virtual-to-physical translations.
*   **TLB Miss:** If the translation isn't in the TLB, the CPU must walk the "page tables" in RAM, which is very slow.
*   **Huge Pages:** By default, OS memory pages are 4KB. If your ring buffer is 1GB, that's 262,144 pages, easily overwhelming the TLB and causing constant TLB misses. Using "Huge Pages" (e.g., 2MB or 1GB pages) means the TLB can map massive amounts of memory with just one entry, significantly reducing latency.

*(Note: While our current SPSC queue focuses on cache-line padding and NUMA pinning, utilizing Huge Pages via `mmap` is the next critical step for scaling such systems.)*
