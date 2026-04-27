#include "AsyncLogger.hpp"
#include "TSCUtils.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <numeric>

using namespace HFT;

// A simple synchronous logger to compare against
void sync_log(std::ofstream& file, const char* msg) {
    uint64_t ts = TSCUtils::read_tsc();
    file << "[" << ts << "] " << msg << "\n";
    file.flush(); // simulating worst-case synchronous I/O
}

int main() {
    std::cout << "Starting Logger Benchmarks...\n";
    const int NUM_ITERATIONS = 100000;
    std::vector<uint64_t> sync_latencies(NUM_ITERATIONS);
    std::vector<uint64_t> async_latencies(NUM_ITERATIONS);

    // Benchmark 1: Synchronous File I/O
    {
        std::ofstream file("sync_benchmark.log");
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            uint64_t start = TSCUtils::read_tsc();
            sync_log(file, "This is a benchmark message with some payload data.");
            uint64_t end = TSCUtils::read_tsc();
            sync_latencies[i] = end - start;
        }
    }
    std::remove("sync_benchmark.log");

    // Benchmark 2: Async Lock-Free Logger
    {
        auto& logger = AsyncLogger::getInstance();
        logger.init("async_benchmark.log", 50 * 1024 * 1024);
        logger.registerThread(1);

        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            uint64_t start = TSCUtils::read_tsc();
            LOG_INFO("This is a benchmark message with some payload data.");
            uint64_t end = TSCUtils::read_tsc();
            async_latencies[i] = end - start;
        }
        logger.shutdown();
    }
    std::remove("async_benchmark.log");

    // Calculate stats
    auto print_stats = [](const std::string& name, std::vector<uint64_t>& latencies) {
        std::sort(latencies.begin(), latencies.end());
        double avg = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        std::cout << "--- " << name << " Latency (TSC ticks) ---\n";
        std::cout << "Min: " << latencies.front() << "\n";
        std::cout << "Avg: " << avg << "\n";
        std::cout << "P50: " << latencies[latencies.size() * 0.50] << "\n";
        std::cout << "P99: " << latencies[latencies.size() * 0.99] << "\n";
        std::cout << "Max: " << latencies.back() << "\n\n";
    };

    print_stats("Synchronous std::ofstream", sync_latencies);
    print_stats("Async Lock-Free Logger", async_latencies);

    return 0;
}
