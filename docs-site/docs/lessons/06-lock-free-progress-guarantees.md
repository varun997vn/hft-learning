---
sidebar_position: 6
title: Progress Guarantees (Wait-Free)
---

# Deep Dive: Progress Guarantees

When building concurrent systems, engineers often use terms like "lock-free" interchangeably with "fast." However, in computer science, these terms have strict mathematical definitions regarding "progress guarantees." This lesson explores why our SPSC ring buffer achieves the holy grail of concurrency: **Wait-Freedom**.

## 1. Blocking Algorithms
Traditional multi-threading uses synchronization primitives like `std::mutex` or semaphores.
*   **The Guarantee:** No two threads will enter the critical section at the same time.
*   **The Problem:** If Thread A acquires a lock and is suddenly preempted by the OS scheduler (put to sleep), Thread B will block forever waiting for the lock. The entire system's progress halts.
*   **Analogy:** A traffic light. Even if the cross-street is completely empty, you must stop and wait for a green light. If the light breaks, traffic stops forever.

## 2. Non-Blocking: Obstruction-Free
*   **The Guarantee:** A thread guarantees progress only if it executes in isolation (all other threads are paused). 
*   **The Problem:** If multiple threads run simultaneously, they might livelock (constantly interfering with each other's work without making progress).
*   **Analogy:** Two people trying to pass each other in a narrow hallway. They both step left, then both step right, getting stuck in an infinite dance.

## 3. Non-Blocking: Lock-Free
Many high-performance queues are "Lock-Free." They typically rely on a `Compare-And-Swap` (CAS) operation in a `while` loop.
*   **The Guarantee:** The *system as a whole* makes progress. At least one thread will succeed in its operation.
*   **The Problem:** A specific thread might fail its CAS loop thousands of times if other threads keep beating it to the punch. It is subject to starvation.
*   **Analogy:** A 4-way stop sign. The intersection is always processing cars (the system makes progress), but one timid driver might get stuck waiting for minutes while more aggressive drivers go first.

## 4. Non-Blocking: Wait-Free (Our Architecture)
Wait-freedom is the strongest progress guarantee.
*   **The Guarantee:** *Every single thread* makes progress in a bounded number of steps, regardless of what any other thread is doing or if they are suspended.
*   **The Implementation:** Look at our `SPSCQueue.hpp`. There are no locks. There are no CAS `while` loops. The Producer does a single atomic load, an array write, and a single atomic store. The operations complete in a fixed, predictable number of CPU cycles.
*   **Analogy:** A highway overpass/cloverleaf intersection. Traffic flowing North/South physically never intersects with traffic flowing East/West. No one stops. No one waits.

## Why SPSC allows Wait-Freedom
The Single-Producer, Single-Consumer constraint is the secret. Because exactly one thread modifies the `write_index_` and exactly one thread modifies the `read_index_`, there is no contention. We don't need CAS loops to resolve conflicts because conflicts are architecturally impossible. This predictability is why HFT systems rely heavily on SPSC ring buffers over general-purpose Multi-Producer queues.
