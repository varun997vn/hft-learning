---
id: tcp-egress-networking
title: TCP Egress Networking
sidebar_label: TCP Egress Network
sidebar_position: 5
---

# TCP Egress Networking Layer

In a tick-to-trade pipeline, transmitting the decision to the exchange is just as critical as receiving market data. The `TCPEgress` module handles the reliable, ultra-low latency transmission of OUCH protocol messages back to the exchange.

## Overview

Unlike inbound UDP multicast market data, order entry requires a reliable transport mechanism to ensure orders are not lost and sequencing is maintained. `TCPEgress` manages standard POSIX TCP sockets to maintain this connection while minimizing system call overhead on the critical path.

## Design Highlights

### Non-Blocking I/O
The TCP socket is configured in non-blocking mode using `fcntl()`. When a trading strategy triggers an order, the `TCPEgress::send_order` function attempts to write directly to the kernel socket buffer via the `send()` system call.

```cpp
// Pseudocode for non-blocking send
ssize_t sent = send(socket_fd, buffer, length, MSG_DONTWAIT | MSG_NOSIGNAL);
```

### Eliminating Nagle's Algorithm
Standard TCP stacks use Nagle's Algorithm, which buffers small packets to reduce network congestion. In HFT, this introduces unacceptable latency (often up to 40ms). The `TCPEgress` module explicitly disables this by setting the `TCP_NODELAY` socket option:

```cpp
int flag = 1;
setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
```
This forces the kernel to flush the OUCH payload to the network interface card (NIC) immediately upon calling `send()`.

### Error Handling & Retries
Because the socket is non-blocking, a `send()` call might return `EAGAIN` or `EWOULDBLOCK` if the kernel buffer is full. The `TCPEgress` class includes lightweight spin-retry logic to guarantee transmission without yielding the CPU thread context, which is vital for maintaining cache locality on the strategy core.

## Integration with OUCH Builder

The `TCPEgress` layer operates seamlessly with the zero-allocation `OUCHBuilder`. The `StrategyEngine` constructs the binary OUCH `EnterOrderMsg` directly inside a pre-allocated byte array, and passes the pointer and length straight to `TCPEgress`. This ensures zero intermediate copies between the strategy logic and the network stack.

## Future Optimization (Kernel Bypass)

While standard POSIX sockets are highly optimized in modern Linux kernels, the current `TCPEgress` layer acts as a cleanly abstracted interface. In extreme latency-sensitive deployments, this class is designed to be easily swapped out for a Kernel Bypass implementation, such as:
- **Solarflare OpenOnload** (transparent bypass)
- **DPDK** (Data Plane Development Kit)
- **Direct Verbs / RDMA**

For our current educational pipeline, standard optimized POSIX TCP combined with `TCP_NODELAY` provides an excellent balance of simplicity and high performance.
