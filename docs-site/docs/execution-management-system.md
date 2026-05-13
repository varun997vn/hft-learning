---
id: execution-management-system
title: Execution Management System
sidebar_label: EMS & Epoll Loop
sidebar_position: 6
---

# Execution Management System & Epoll Loop

Sending an order to the exchange is only half of the execution equation. The HFT pipeline must asynchronously track the state of that order, handle exchange acknowledgments, and manage the underlying network events efficiently. This is handled by the **Execution Management System (EMS)** and the **`NetworkEventLoop`**.

## The Execution Management System (EMS)

The EMS acts as a state machine for every order sent by the Strategy Engine. Because latency is critical, the EMS relies on pre-allocated, flat arrays rather than dynamically allocated structures like `std::unordered_map`.

### Pre-Allocated State Tracking
At initialization, the EMS allocates an array capable of tracking tens of thousands of active orders. Each order is assigned a unique, monotonically increasing Integer ID (mapped to the OUCH `Order Token`).

When an order is dispatched:
1. The `StrategyEngine` registers the order with the EMS.
2. The EMS transitions the order state to `SENT`.
3. The order metadata (Price, Qty, Side, Strategy ID) is saved in the pre-allocated array slot corresponding to its ID.

### State Transitions
When the exchange replies via TCP, the EMS updates the order's state:
- **`ACCEPTED`**: The exchange has acknowledged the order and placed it on the Limit Order Book.
- **`EXECUTED`**: A trade has occurred. The EMS calculates the filled quantity and immediately routes an `onOrderExecuted` callback back to the originating `IStrategy`.
- **`CANCELED`**: The order was pulled from the market. The EMS frees the slot for future use.

By utilizing array lookups (`O(1)` with virtually no constant overhead), the EMS processes exchange acks in nanoseconds, keeping the Strategy Engine perfectly synchronized with the true exchange state.

## The Network Event Loop (`epoll`)

To handle incoming TCP acknowledgments and UDP market data simultaneously, the system uses a dedicated `NetworkThread` running an event loop powered by Linux `epoll`.

### Why `epoll`?
Unlike traditional `select()` or `poll()`, `epoll` scales efficiently to thousands of file descriptors. However, in our architecture, we use it for a very specific, latency-sensitive reason: **Edge-Triggered I/O (`EPOLLET`)**.

1. **UDP Ingress**: The socket receiving ITCH multicast packets is registered with `epoll`. When a packet arrives, `epoll_wait` unblocks, and the thread drains the UDP buffer into the `ITCH5Parser`.
2. **TCP Ingress**: The TCP connection to the exchange is also registered. When an OUCH acknowledgment arrives, `epoll` wakes the thread to read the execution report.

### Thread Decoupling
By placing the `epoll` loop on `Core 0` (Network Thread), we completely decouple the unpredictable jitter of network interrupts from the deterministic `Core 1` (Strategy Thread). 
- The Network Thread handles all `epoll_wait` blocking, context switches, and parsing.
- It translates network bytes into clean `InternalMessage` structs.
- It pushes these structs onto the NUMA-aware lock-free `SPSCQueue`.

This architecture ensures that the Strategy Engine on `Core 1` simply spins on cache, oblivious to the complexities of the network layer, achieving the lowest possible tick-to-trade latencies.
