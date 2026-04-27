---
id: lob-introduction
title: The HFT Limit Order Book
sidebar_position: 1
---

# The HFT Limit Order Book

Welcome to the Limit Order Book (LOB) implementation guide. This section of the documentation dives deep into the architecture, design, and C++ engineering behind a high-performance matching engine suitable for High-Frequency Trading (HFT).

## The Big Picture

In the context of financial exchanges (both traditional equity markets and cryptocurrency exchanges), the **Limit Order Book** is the core component that pairs buyers and sellers. It maintains all outstanding limit orders (orders to buy or sell at a specific price) and executes them according to strict rules of **Price-Time Priority**.

### Role in the Ecosystem

When a market participant sends an order to an exchange, it typically traverses a complex journey:
1. **Gateway**: Receives the network packet (often FIX or binary protocol).
2. **Risk Check**: Validates the participant has sufficient margin/capital.
3. **Sequencer / Message Bus**: Assigns a globally unique monotonic sequence number to the event.
4. **Matching Engine (LOB)**: The order reaches the limit order book where it is either matched against resting liquidity or added to the book.

The speed at which the matching engine processes incoming orders directly defines the exchange's capacity and latency. In HFT, latency is measured in microseconds or nanoseconds. To remain competitive, modern matching engines must process millions of events per second with entirely deterministic latency profiles.

## Engineering for Zero Latency Variance

To achieve deterministic nanosecond latency, standard software engineering paradigms must be discarded. The primary enemy of predictability is **dynamic memory allocation**.

Calling `new` or `malloc` in the critical path introduces unacceptable variance because the operating system's memory allocator must search for free memory blocks and may occasionally trigger page faults or invoke complex locking mechanisms.

Therefore, this Limit Order Book project is designed with a **zero-allocation matching path**. This means once the system is initialized, no dynamic memory allocation or deallocation occurs during the processing of `add`, `modify`, or `cancel` operations. 

We accomplish this through:
- **Pre-allocated Object Pools**: A massive contiguous block of memory allocated upfront via `mmap`.
- **Intrusive Data Structures**: Linked lists that don't require allocating distinct node wrappers.
- **Flat Arrays and Custom Hash Maps**: Trading memory space for guaranteed O(1) access times.

In the following sections, we will explore the system design, the specific data structures utilized, and the modern C++ features that make this performance possible.
