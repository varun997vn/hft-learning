---
sidebar_position: 4
title: C++ Concepts & Libraries Primer
---

# C++ Concepts & Libraries Primer

This project extensively utilizes modern C++ features (C++11 through C++17) to achieve high-performance, low-latency execution. This section serves as a primer for the core C++ concepts, standard libraries, and specific features utilized in our NUMA-aware ring buffer.

## 1. `std::atomic` and Memory Ordering (`<atomic>`)

### What it is
The `<atomic>` library provides components for fine-grained atomic operations allowing lock-free concurrent programming. An atomic operation is guaranteed to be executed as a single, indivisible action.

### Features Used
- **`std::atomic<T>`**: Wraps our `read_index_` and `write_index_` to ensure that when a thread updates them, it happens safely without data races.
- **`std::memory_order_relaxed`**: Guarantees atomicity but provides no synchronization or ordering constraints. It's the fastest option.
- **`std::memory_order_acquire` / `std::memory_order_release`**: Used in pairs. A *release* store in one thread ensures that all previous writes are visible to another thread that does an *acquire* load on the same atomic variable.

### When to Use
- Use `std::atomic` when you have multiple threads reading and writing to a shared counter, flag, or index, and you absolutely cannot afford the overhead of a standard `std::mutex`.
- Use `relaxed` when you only care about the value itself not being corrupted, but don't care about the state of *other* variables in memory.
- Use `acquire`/`release` when passing data between threads (like in our Producer-Consumer queue) to ensure the Consumer sees the data written by the Producer before reading the updated index.

## 2. Memory Alignment with `alignas` (C++11)

### What it is
`alignas` is a keyword introduced in C++11 that allows developers to dictate the memory alignment of a variable or a class. By default, the compiler aligns variables based on their type (e.g., a 4-byte `int` is aligned to 4 bytes).

### Feature Used
In `SPSCQueue.hpp`, we use `alignas(CACHE_LINE_SIZE)` (typically 64 bytes) on our atomic indices and data array.
```cpp
alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> write_index_{0};
```

### When to Use
- Use `alignas` when writing code for specific hardware architectures (like AVX vector instructions that require 32-byte alignment).
- Use it to prevent **False Sharing** in multi-threaded applications by forcing shared variables to reside on distinct CPU cache lines.

## 3. `std::hardware_destructive_interference_size` (C++17)

### What it is
A constant introduced in C++17 (via `<new>`) that returns the minimum offset between two objects to avoid false sharing. Historically, developers hardcoded `64` because x86 cache lines are typically 64 bytes. This feature asks the compiler directly what the target hardware's cache line size is.

### Feature Used
```cpp
#if defined(__cpp_lib_hardware_interference_size)
    constexpr std::size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
#else
    constexpr std::size_t CACHE_LINE_SIZE = 64;
#endif
```

### When to Use
- Use this whenever you are padding structs or classes to avoid False Sharing. It makes your high-performance code portable across different architectures (e.g., ARM processors might have 128-byte cache lines).

## 4. Placement `new` (`<new>`)

### What it is
Standard `new` does two things: it allocates memory on the heap, and then it calls the constructor. **Placement `new`** separates these steps. It allows you to construct an object in memory that you have *already* allocated.

### Feature Used
In `NumaAllocator.hpp`, we first allocate memory specifically on a target NUMA node using OS-level APIs, and then we construct the C++ object into that exact memory space:
```cpp
void* memory = allocate_on_node(sizeof(T), node);
return new (memory) T(std::forward<Args>(args)...); // Placement new
```

### When to Use
- Use placement `new` when writing Custom Allocators, Memory Pools, or when interfacing with specialized hardware memory (like NUMA nodes or GPU mapped memory) where the standard OS heap allocator (`malloc`) is not appropriate.

## 5. Variadic Templates & Perfect Forwarding (`<utility>`)

### What it is
- **Variadic Templates (C++11)**: Allows a function or class to accept an arbitrary number of arguments of any type (`typename... Args`).
- **Perfect Forwarding (`std::forward`)**: Ensures that arguments passed to a wrapper function preserve their original characteristics (l-value vs. r-value references) when passed to the underlying function.

### Feature Used
Our `NumaMemoryUtils::create_on_node` factory function needs to construct objects of type `T`, but it doesn't know what `T`'s constructor looks like ahead of time.
```cpp
template <typename T, typename... Args>
T* create_on_node(int node, Args&&... args) {
    // ...
    return new (memory) T(std::forward<Args>(args)...);
}
```

### When to Use
- Use this pattern whenever you are writing wrapper functions, factory classes, or dependency injection frameworks. `std::make_unique` and `std::make_shared` in the standard library are implemented using exactly this technique.

## 6. Native Thread Handles (`<thread>`)

### What it is
The C++ `<thread>` library provides a cross-platform abstraction for OS threads. However, some operations (like setting CPU affinity/pinning) are highly OS-specific and aren't in the C++ standard. `native_handle()` allows you to access the underlying OS thread object (e.g., `pthread_t` on Linux).

### Feature Used
In `ThreadAffinity.hpp`, we take a `std::thread::native_handle_type` so we can pass it down to Linux-specific functions like `pthread_setaffinity_np`.

### When to Use
- Use `native_handle()` when you hit the limitations of the C++ standard library and need to invoke platform-specific OS features, such as thread priority adjustments, thread naming, or CPU pinning.
