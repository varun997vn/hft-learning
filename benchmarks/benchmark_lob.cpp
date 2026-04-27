#include "../include/OrderBook.hpp"
#include "../include/TSCUtils.hpp"
#include "NaiveOrderBook.hpp"

#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <string>

constexpr size_t NUM_ORDERS = 100000;
constexpr size_t WARMUP_ORDERS = 10000;

template<typename BookType, typename SideType>
void run_benchmark(const std::string& name, BookType& lob, double tsc_freq) {
    std::cout << "========================================" << std::endl;
    std::cout << "Benchmarking: " << name << std::endl;
    std::cout << "========================================" << std::endl;

    // Warmup cache and CPU
    for (size_t i = 1; i <= WARMUP_ORDERS; ++i) {
        lob.add(i, SideType::BUY, i % 1000, 100);
    }
    for (size_t i = 1; i <= WARMUP_ORDERS; ++i) {
        lob.cancel(i);
    }

    std::vector<double> add_latencies;
    add_latencies.reserve(NUM_ORDERS);

    std::vector<double> cancel_latencies;
    cancel_latencies.reserve(NUM_ORDERS);

    for (size_t i = 1; i <= NUM_ORDERS; ++i) {
        uint64_t id = WARMUP_ORDERS + i;
        uint64_t price = i % 100000;
        
        uint64_t start = TSCUtils::read_tsc();
        lob.add(id, SideType::BUY, price, 100);
        uint64_t end = TSCUtils::read_tsc();

        add_latencies.push_back(TSCUtils::cycles_to_nanoseconds(end - start, tsc_freq));
    }

    for (size_t i = 1; i <= NUM_ORDERS; ++i) {
        uint64_t id = WARMUP_ORDERS + i;

        uint64_t start = TSCUtils::read_tsc();
        lob.cancel(id);
        uint64_t end = TSCUtils::read_tsc();

        cancel_latencies.push_back(TSCUtils::cycles_to_nanoseconds(end - start, tsc_freq));
    }

    auto print_stats = [](const std::string& op_name, std::vector<double>& lat) {
        std::sort(lat.begin(), lat.end());
        double sum = std::accumulate(lat.begin(), lat.end(), 0.0);
        double mean = sum / lat.size();
        double p50 = lat[lat.size() * 0.5];
        double p99 = lat[lat.size() * 0.99];
        double p999 = lat[lat.size() * 0.999];

        std::cout << "--- " << op_name << " Latency (ns) ---" << std::endl;
        std::cout << "Mean:  " << mean << " ns" << std::endl;
        std::cout << "p50:   " << p50 << " ns" << std::endl;
        std::cout << "p99:   " << p99 << " ns" << std::endl;
        std::cout << "p99.9: " << p999 << " ns" << std::endl;
        std::cout << std::endl;
    };

    print_stats("Add Order", add_latencies);
    print_stats("Cancel Order", cancel_latencies);
}

int main() {
    double tsc_freq = TSCUtils::estimate_tsc_frequency_hz();
    std::cout << "TSC Frequency: " << tsc_freq << " Hz\n" << std::endl;

    NaiveOrderBook naive_lob;
    run_benchmark<NaiveOrderBook, NaiveSide>("Naive Order Book (std::map, std::list)", naive_lob, tsc_freq);

    LimitOrderBook opt_lob(NUM_ORDERS * 2, 100000);
    run_benchmark<LimitOrderBook, Side>("Optimized Zero-Allocation Order Book", opt_lob, tsc_freq);

    return 0;
}
