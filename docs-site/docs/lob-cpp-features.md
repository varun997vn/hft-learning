---
id: lob-cpp-features
title: C++ Implementation Details
sidebar_position: 4
---

# C++ Implementation Details

The Limit Order Book leverages specific C++ features to enforce safety, prevent accidental overhead, and interface directly with the underlying operating system.

## Enforcing Immutability and Move Semantics

A critical aspect of high-performance C++ is preventing accidental copies. If a 1-million-order limit order book is accidentally copied or moved, it would stall the trading engine for hundreds of milliseconds.

To prevent this, we explicitly delete the copy constructor, copy assignment operator, move constructor, and move assignment operator in the `LimitOrderBook` class interface.

```cpp
class LimitOrderBook {
public:
    // ...
    // Prevent copy/move
    LimitOrderBook(const LimitOrderBook&) = delete;
    LimitOrderBook& operator=(const LimitOrderBook&) = delete;
    LimitOrderBook(LimitOrderBook&&) = delete;
    LimitOrderBook& operator=(LimitOrderBook&&) = delete;
    // ...
};
```
By placing `= delete` in the public interface, any attempt to pass the order book by value to a function or assign it to another variable will result in a compile-time error, acting as a strict guardrail for developers.

## Hardware Alignment with `alignas`

As discussed in the system design, cache line alignment is critical. C++11 introduced the `alignas` specifier to enforce memory boundary alignment at the compiler level.

```cpp
struct alignas(64) Order { ... };
```

This tells the compiler that the starting memory address of any `Order` struct must be a multiple of 64. The compiler achieves this by automatically inserting padding bytes if the struct's size isn't a natural multiple of 64. In our case, the `Order` struct size naturally sums to 40 bytes, so the compiler pads it to exactly 64 bytes.

## System-Level Memory with `<sys/mman.h>`

While C++ provides standard allocators, they do not offer the granular control needed for NUMA-aware, zero-page-fault architectures. We bridge the gap between C++ object orientation and POSIX system calls.

```cpp
#include <sys/mman.h>

// ... inside initialization ...
mmap_base_ = mmap(nullptr, total_mmap_size_, PROT_READ | PROT_WRITE, 
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
```

Because `mmap` returns a raw `void*`, we use C++ `reinterpret_cast` to safely interpret these raw byte arrays as strongly-typed arrays of `PriceLevel`, `Order`, and `HashEntry`.

```cpp
char* ptr = static_cast<char*>(mmap_base_);
bids_ = reinterpret_cast<PriceLevel*>(ptr);
// ptr is incremented by byte size, then re-cast
```

## Special Tombstone Pointers

In the custom flat hash map, we need a way to distinguish between a truly empty slot (`nullptr`), an occupied slot (valid `Order*`), and a deleted slot (tombstone).

Instead of adding a boolean flag (which would increase the size of the `HashEntry` struct and hurt cache density), we define a special pointer value:

```cpp
static Order* const TOMBSTONE = reinterpret_cast<Order*>(~0ULL);
```
`~0ULL` is a bitwise NOT operation on a 64-bit unsigned integer zero, resulting in `0xFFFFFFFFFFFFFFFF`. This is a guaranteed invalid memory address in user space. By casting it to `Order*`, we reserve this pointer value as a tombstone marker without consuming extra memory.

## Conclusion

By combining POSIX memory management, strict compiler alignment hints, and O(1) cache-friendly data structures, this Limit Order Book achieves the zero-allocation deterministic performance required for tier-1 high-frequency trading applications.
