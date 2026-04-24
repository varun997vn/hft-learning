---
sidebar_position: 1
---

# Introduction

Welcome to the NUMA-Aware Lock-Free SPSC Ring Buffer project. This portfolio piece demonstrates strict memory management, low-latency concurrent programming, and precise performance measurement.

## What is NUMA?
Non-Uniform Memory Access (NUMA) is a computer memory design used in multiprocessing, where the memory access time depends on the memory location relative to the processor. Under NUMA, a processor can access its own local memory faster than non-local memory (memory local to another processor or memory shared between processors). In High-Frequency Trading (HFT), ignoring NUMA can lead to significant latency spikes.

## What is SPSC?
Single-Producer Single-Consumer (SPSC) refers to a thread-safe queue where exactly one thread writes to the queue and exactly one thread reads from it. Because the producer and consumer are strictly 1:1, we can implement entirely lock-free operations using standard atomics, completely avoiding expensive OS-level mutexes or semaphores.
