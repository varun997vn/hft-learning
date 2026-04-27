#include "../include/SPSCQueue.hpp"
#include "../include/NumaAllocator.hpp"
#include "../include/ThreadAffinity.hpp"
#include "../include/ITCH5Parser.hpp"
#include "../include/OrderBook.hpp"
#include "../include/TSCUtils.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

constexpr size_t QUEUE_CAPACITY = 65536; 
constexpr size_t NUM_MESSAGES = 1000000;

using namespace itch5;

// Mock network generator
void generate_mock_itch_data(std::vector<AddOrderMsg>& messages) {
    messages.reserve(NUM_MESSAGES);
    for (size_t i = 0; i < NUM_MESSAGES; ++i) {
        AddOrderMsg msg;
        msg.msg_type = 'A';
        msg.stock_locate = __builtin_bswap16(1);
        msg.tracking_number = __builtin_bswap16(i);
        msg.order_ref_number = __builtin_bswap64(i + 1); // ID must be > 0
        msg.buy_sell_indicator = (i % 2 == 0) ? 'B' : 'S';
        msg.shares = __builtin_bswap32(100);
        msg.price = __builtin_bswap32(15000 + (i % 100)); // prices between 15000 and 15099
        messages.push_back(msg);
    }
}

int main(int argc, char** argv) {
    int producer_core = 0;
    int consumer_core = 1;
    int numa_node = 0; // Default to node 0

    if (argc >= 4) {
        producer_core = std::stoi(argv[1]);
        consumer_core = std::stoi(argv[2]);
        numa_node = std::stoi(argv[3]);
    } else {
        std::cout << "Usage: " << argv[0] << " [producer_core] [consumer_core] [numa_node]\n";
        std::cout << "Using defaults: producer=0, consumer=1, numa=0\n";
    }

    // 1. Setup the SPSC Queue for Internal Messages
    auto* queue = NumaMemoryUtils::create_on_node<SPSCQueue<Parser::InternalMessage, QUEUE_CAPACITY>>(numa_node);
    if (!queue) {
        std::cerr << "Failed to allocate queue on NUMA node " << numa_node << std::endl;
        return 1;
    }

    // 2. Generate Mock Network Data (simulating PCAP or raw UDP socket receive)
    std::vector<AddOrderMsg> raw_network_buffers;
    generate_mock_itch_data(raw_network_buffers);

    std::atomic<bool> producer_done{false};
    double tsc_freq = TSCUtils::estimate_tsc_frequency_hz();

    std::cout << "Starting E2E Pipeline Benchmark...\n";

    // Producer Thread (Network Ingress + ITCH Decoder)
    std::thread network_thread([&]() {
        ThreadPinning::pin_current_thread_to_core(producer_core);

        Parser::InternalMessage parsed_msg;

        auto start_time = TSCUtils::read_tsc();

        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
            // 1. Simulate reading a packet
            const uint8_t* buffer = reinterpret_cast<const uint8_t*>(&raw_network_buffers[i]);
            size_t length = sizeof(AddOrderMsg);

            // 2. Parse ITCH to InternalMessage (Zero-Copy)
            if (Parser::parse(buffer, length, parsed_msg)) {
                // 3. Dispatch to LOB via SPSC Queue
                while (!queue->push(parsed_msg)) {
                    // Busy Wait (Queue Full)
                }
            }
        }
        
        auto end_time = TSCUtils::read_tsc();
        double nanos = TSCUtils::cycles_to_nanoseconds(end_time - start_time, tsc_freq);
        
        std::cout << "Network Producer finished parsing & pushing " << NUM_MESSAGES 
                  << " messages in " << (nanos / 1e6) << " ms\n";
        producer_done.store(true, std::memory_order_release);
    });

    // Consumer Thread (Trading Engine / LOB)
    std::thread matching_thread([&]() {
        ThreadPinning::pin_current_thread_to_core(consumer_core);

        // Limit Order Book (Zero Allocation)
        LimitOrderBook lob;

        Parser::InternalMessage msg;
        size_t processed = 0;

        auto start_time = TSCUtils::read_tsc();

        while (!producer_done.load(std::memory_order_acquire) || processed < NUM_MESSAGES) {
            // 1. Poll the queue
            if (queue->pop(msg)) {
                // 2. Apply to LOB
                switch (msg.type) {
                    case 'A':
                        lob.add(msg.id, msg.side, msg.price, msg.quantity);
                        break;
                    case 'X':
                        lob.cancel(msg.id); // For simplicity, we just cancel
                        break;
                    case 'D':
                        lob.cancel(msg.id);
                        break;
                }
                processed++;
            }
        }

        auto end_time = TSCUtils::read_tsc();
        double nanos = TSCUtils::cycles_to_nanoseconds(end_time - start_time, tsc_freq);
        
        std::cout << "Matching Consumer finished popping & applying " << processed 
                  << " messages in " << (nanos / 1e6) << " ms\n";
    });

    network_thread.join();
    matching_thread.join();

    NumaMemoryUtils::destroy_and_free(queue);
    
    std::cout << "Pipeline integration successful!" << std::endl;

    return 0;
}
