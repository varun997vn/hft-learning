#include "../include/OrderBook.hpp"
#include "../include/TSCUtils.hpp"

#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>

constexpr size_t NUM_ORDERS = 100000;
constexpr size_t WARMUP_ORDERS = 10000;

int main() {
    std::cout << "Initializing Limit Order Book for Benchmarking..." << std::endl;
    // Calibrate TSC first
    double tsc_freq = TSCUtils::estimate_tsc_frequency_hz();
    std::cout << "TSC Frequency: " << tsc_freq << " Hz" << std::endl;

    LimitOrderBook lob(NUM_ORDERS * 2, 100000);

    // Warmup cache and CPU
    for (size_t i = 1; i <= WARMUP_ORDERS; ++i) {
        lob.add(i, Side::BUY, i % 1000, 100);
    }
    for (size_t i = 1; i <= WARMUP_ORDERS; ++i) {
        lob.cancel(i);
    }

    std::vector<double> add_latencies;
    add_latencies.reserve(NUM_ORDERS);

    std::vector<double> cancel_latencies;
    cancel_latencies.reserve(NUM_ORDERS);

    std::cout << "Benchmarking Add operations..." << std::endl;
    for (size_t i = 1; i <= NUM_ORDERS; ++i) {
        uint64_t id = WARMUP_ORDERS + i;
        uint64_t price = i % 100000;
        
        uint64_t start = TSCUtils::read_tsc();
        lob.add(id, Side::BUY, price, 100);
        uint64_t end = TSCUtils::read_tsc();

        add_latencies.push_back(TSCUtils::cycles_to_nanoseconds(end - start, tsc_freq));
    }

    std::cout << "Benchmarking Cancel operations..." << std::endl;
    for (size_t i = 1; i <= NUM_ORDERS; ++i) {
        uint64_t id = WARMUP_ORDERS + i;

        uint64_t start = TSCUtils::read_tsc();
        lob.cancel(id);
        uint64_t end = TSCUtils::read_tsc();

        cancel_latencies.push_back(TSCUtils::cycles_to_nanoseconds(end - start, tsc_freq));
    }

    auto print_stats = [](const std::string& name, std::vector<double>& lat) {
        std::sort(lat.begin(), lat.end());
        double sum = std::accumulate(lat.begin(), lat.end(), 0.0);
        double mean = sum / lat.size();
        double p50 = lat[lat.size() * 0.5];
        double p99 = lat[lat.size() * 0.99];
        double p999 = lat[lat.size() * 0.999];

        std::cout << "--- " << name << " Latency (ns) ---" << std::endl;
        std::cout << "Mean:  " << mean << " ns" << std::endl;
        std::cout << "p50:   " << p50 << " ns" << std::endl;
        std::cout << "p99:   " << p99 << " ns" << std::endl;
        std::cout << "p99.9: " << p999 << " ns" << std::endl;
        std::cout << std::endl;
    };

    print_stats("Add Order", add_latencies);
    print_stats("Cancel Order", cancel_latencies);

    return 0;
}
