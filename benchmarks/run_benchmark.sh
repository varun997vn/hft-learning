#!/bin/bash
set -e

echo "Compiling BenchmarkLOB..."
g++ -std=c++17 -O3 -Wall -Wextra -march=native \
    OrderBook.cpp \
    TSCUtils.cpp \
    BenchmarkLOB.cpp \
    -o benchmark_lob

echo "Running BenchmarkLOB..."
# Taskset to pin to core 0 to minimize context switching latency jitter
taskset -c 0 ./benchmark_lob

echo ""
echo "Running perf stat for cache misses..."
perf stat -e L1-dcache-load-misses,LLC-load-misses taskset -c 0 ./benchmark_lob
