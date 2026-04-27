#include "../include/SPSCQueue.hpp"
#include "../include/NumaAllocator.hpp"
#include "../include/ThreadAffinity.hpp"
#include "../include/Payload.hpp"
#include "../include/TSCUtils.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <string>
#include <chrono>

constexpr size_t QUEUE_CAPACITY = 65536; // 64K elements
constexpr size_t WARMUP_MESSAGES = 100000;
constexpr size_t MEASURE_MESSAGES = 1000000;

void run_benchmark(int producer_core, int consumer_core, int numa_node, const std::string& output_csv) {
    std::cout << "Starting benchmark: Producer on Core " << producer_core 
              << ", Consumer on Core " << consumer_core 
              << ", Memory on Node " << numa_node << std::endl;

    double tsc_freq = TSCUtils::estimate_tsc_frequency_hz();

    // Allocate queue on specific NUMA node
    auto* queue = NumaMemoryUtils::create_on_node<SPSCQueue<MarketDataTick, QUEUE_CAPACITY>>(numa_node);
    if (!queue) {
        std::cerr << "Failed to allocate queue on NUMA node " << numa_node << std::endl;
        return;
    }

    // Vector to store latency results. Pre-allocate to avoid allocations during measurement.
    std::vector<uint64_t> latencies;
    latencies.reserve(MEASURE_MESSAGES);

    std::thread producer([&]() {
        if (!ThreadPinning::pin_current_thread_to_core(producer_core)) {
            std::cerr << "Producer failed to pin to core " << producer_core << std::endl;
        }

        MarketDataTick tick;
        tick.price = 150.0;
        tick.size = 100;
        tick.timestamp = 0;

        // Warm-up phase
        for (size_t i = 0; i < WARMUP_MESSAGES; ++i) {
            tick.creation_tsc = TSCUtils::read_tsc();
            while (!queue->push(tick)) {
                // Busy wait
            }
        }

        // Measurement phase
        for (size_t i = 0; i < MEASURE_MESSAGES; ++i) {
            tick.creation_tsc = TSCUtils::read_tsc();
            while (!queue->push(tick)) {
                // Busy wait
            }
        }
    });

    std::thread consumer([&]() {
        if (!ThreadPinning::pin_current_thread_to_core(consumer_core)) {
            std::cerr << "Consumer failed to pin to core " << consumer_core << std::endl;
        }

        MarketDataTick tick;

        // Warm-up phase
        for (size_t i = 0; i < WARMUP_MESSAGES; ++i) {
            while (!queue->pop(tick)) {
                // Busy wait
            }
        }

        // Measurement phase
        for (size_t i = 0; i < MEASURE_MESSAGES; ++i) {
            while (!queue->pop(tick)) {
                // Busy wait
            }
            uint64_t pop_tsc = TSCUtils::read_tsc();
            latencies.push_back(pop_tsc - tick.creation_tsc);
        }
    });

    producer.join();
    consumer.join();

    NumaMemoryUtils::destroy_and_free(queue);

    // Write to CSV
    std::cout << "Writing results to " << output_csv << "..." << std::endl;
    std::ofstream csv_file(output_csv);
    csv_file << "MessageId,LatencyCycles,LatencyNs\n";
    for (size_t i = 0; i < latencies.size(); ++i) {
        csv_file << i << "," 
                 << latencies[i] << "," 
                 << TSCUtils::cycles_to_nanoseconds(latencies[i], tsc_freq) << "\n";
    }
    csv_file.close();

    std::cout << "Benchmark complete." << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <producer_core> <consumer_core> <numa_node> <output_csv>\n";
        return 1;
    }

    int producer_core = std::stoi(argv[1]);
    int consumer_core = std::stoi(argv[2]);
    int numa_node = std::stoi(argv[3]);
    std::string output_csv = argv[4];

    run_benchmark(producer_core, consumer_core, numa_node, output_csv);

    return 0;
}
