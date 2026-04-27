# High-Performance NUMA-Aware Systems & LOB

A collection of ultra-low latency, hardware-optimized components designed for high-frequency trading (HFT) environments. This repository includes a NUMA-aware SPSC queue, custom thread-affinity utilities, and a zero-allocation L2 Limit Order Book.

## Build Requirements

- **C++17 Compiler** (GCC / Clang)
- **CMake 3.14+**
- **libnuma** (`numactl-devel` on RHEL/CentOS, `libnuma-dev` on Ubuntu)

## Building the Project

We use CMake to manage the build system and fetch dependencies like GoogleTest automatically.

```bash
# 1. Create a build directory
mkdir build && cd build

# 2. Configure the project
cmake -DCMAKE_BUILD_TYPE=Release ..

# 3. Compile
make -j$(nproc)
```

## Running Tests

We use **GoogleTest (GTest)** as our production-ready testing framework. Once the project is built, you can run all granular tests using `ctest`.

```bash
cd build
ctest --output-on-failure
```

You can also run individual suites directly:
```bash
./tests/test_orderbook
./tests/test_spsc_queue
./tests/test_numa_allocator
./tests/test_thread_affinity
```

## Running Benchmarks

Performance utilities measuring hardware cache misses and precise `TSC` latency are located in the `benchmarks/` directory. They are compiled alongside the project in the `build/` directory.

```bash
cd build

# To get the lowest jitter, pin the benchmark to an isolated CPU core:
taskset -c 0 ./benchmarks/benchmark_lob

# If your environment supports perf, you can trace cache hits:
perf stat -e L1-dcache-load-misses,LLC-load-misses taskset -c 0 ./benchmarks/benchmark_lob
```

## Core Architecture

- **`core_lib`**: A static library housing the optimized matching engines (`OrderBook.cpp`) and hardware handlers (`NumaAllocator.cpp`, `ThreadAffinity.cpp`).
- **`tests/`**: Segmented GoogleTest suites for regression and unit validation.
- **`benchmarks/`**: Highly-tuned latency micro-benchmarks.
