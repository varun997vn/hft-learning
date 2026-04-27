#include "../include/SPSCQueue.hpp"
#include "../include/NumaAllocator.hpp"
#include "../include/ThreadAffinity.hpp"
#include "../include/ITCH5Parser.hpp"
#include "../include/OrderBook.hpp"
#include "../include/TSCUtils.hpp"
#include "../include/OUCH42.hpp"
#include "../include/PreTradeRisk.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <cstring>

constexpr size_t QUEUE_CAPACITY = 65536; 
constexpr size_t NUM_MESSAGES = 1000000;

using namespace itch5;
using namespace ouch42;
using namespace risk;

// Mock network generator producing high churn (Adds, Executions, Cancels, Deletes)
void generate_mock_itch_data(std::vector<std::vector<uint8_t>>& messages) {
    messages.reserve(NUM_MESSAGES);
    
    std::mt19937 gen(42); // Deterministic seed
    std::uniform_int_distribution<int> action_dist(0, 99);
    
    struct ActiveOrder {
        uint64_t id;
        uint32_t remaining_shares;
    };
    std::vector<ActiveOrder> active_orders;
    active_orders.reserve(10000);
    
    uint64_t next_id = 1;

    for (size_t i = 0; i < NUM_MESSAGES; ++i) {
        int action = action_dist(gen);
        
        if (action < 50 || active_orders.empty()) {
            AddOrderMsg msg;
            msg.msg_type = 'A';
            msg.stock_locate = __builtin_bswap16(1);
            msg.tracking_number = __builtin_bswap16(i);
            msg.order_ref_number = __builtin_bswap64(next_id);
            msg.buy_sell_indicator = (i % 2 == 0) ? 'B' : 'S';
            msg.shares = __builtin_bswap32(100);
            msg.price = __builtin_bswap32(15000 + (i % 100)); // 150.00
            
            std::vector<uint8_t> buf(sizeof(AddOrderMsg));
            std::memcpy(buf.data(), &msg, sizeof(AddOrderMsg));
            messages.push_back(std::move(buf));
            
            active_orders.push_back({next_id, 100});
            next_id++;
        } 
        else if (action < 100) {
            // Keep it simple for this benchmark: 50% Add, 50% Delete
            size_t idx = i % active_orders.size();
            ActiveOrder ao = active_orders[idx];
            
            OrderDeleteMsg msg;
            msg.msg_type = 'D';
            msg.stock_locate = __builtin_bswap16(1);
            msg.tracking_number = __builtin_bswap16(i);
            msg.order_ref_number = __builtin_bswap64(ao.id);
            
            std::vector<uint8_t> buf(sizeof(OrderDeleteMsg));
            std::memcpy(buf.data(), &msg, sizeof(OrderDeleteMsg));
            messages.push_back(std::move(buf));
            
            active_orders.erase(active_orders.begin() + idx);
        }
    }
}

int main(int argc, char** argv) {
    int producer_core = 0;
    int consumer_core = 1;
    int numa_node = 0;

    if (argc >= 4) {
        producer_core = std::stoi(argv[1]);
        consumer_core = std::stoi(argv[2]);
        numa_node = std::stoi(argv[3]);
    } else {
        std::cout << "Usage: " << argv[0] << " [producer_core] [consumer_core] [numa_node]\n";
        std::cout << "Using defaults: producer=0, consumer=1, numa=0\n";
    }

    auto* queue = NumaMemoryUtils::create_on_node<SPSCQueue<Parser::InternalMessage, QUEUE_CAPACITY>>(numa_node);
    if (!queue) {
        std::cerr << "Failed to allocate queue on NUMA node " << numa_node << std::endl;
        return 1;
    }

    std::vector<std::vector<uint8_t>> raw_network_buffers;
    generate_mock_itch_data(raw_network_buffers);

    std::atomic<bool> producer_done{false};
    double tsc_freq = TSCUtils::estimate_tsc_frequency_hz();

    std::cout << "Starting E2E Tick-to-Trade Benchmark...\n";

    std::thread network_thread([&]() {
        ThreadPinning::pin_current_thread_to_core(producer_core);
        Parser::InternalMessage parsed_msg;

        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
            const uint8_t* buffer = raw_network_buffers[i].data();
            size_t length = raw_network_buffers[i].size();

            if (Parser::parse(buffer, length, parsed_msg)) {
                while (!queue->push(parsed_msg)) {
                    // Busy Wait
                }
            }
        }
        
        producer_done.store(true, std::memory_order_release);
    });

    std::thread strategy_thread([&]() {
        ThreadPinning::pin_current_thread_to_core(consumer_core);
        LimitOrderBook lob;
        Parser::InternalMessage msg;
        size_t processed = 0;
        
        // Risk Engine setup
        RiskConfig config;
        config.max_order_size = 500;
        config.max_position = 10000;
        config.max_price_deviation_pct = 0.05; // 5%
        PreTradeRiskEngine risk_engine(config);
        
        // Egress buffer setup
        uint8_t ouch_buffer[64];
        
        // Latency collection
        std::vector<double> tick_to_trade_latencies;
        tick_to_trade_latencies.reserve(100000);

        while (!producer_done.load(std::memory_order_acquire) || processed < NUM_MESSAGES) {
            if (queue->pop(msg)) {
                // 1. Process market data (Update LOB)
                switch (msg.type) {
                    case 'A':
                        lob.add(msg.id, msg.side, msg.price, msg.quantity);
                        break;
                    case 'D':
                        lob.cancel(msg.id);
                        break;
                }
                
                // 2. Simple Strategy Signal
                // If we see an add order with price > 15050, we want to SELL 100 shares.
                if (msg.type == 'A' && msg.price > 15050) {
                    uint64_t t0 = TSCUtils::read_tsc(); // Start measuring Tick-to-Trade latency
                    
                    // 3. Pre-Trade Risk Check
                    uint32_t order_shares = 100;
                    uint32_t order_price = msg.price;
                    RiskCheckStatus status = risk_engine.check_order('S', order_shares, order_price, msg.price);
                    
                    if (status == RiskCheckStatus::APPROVED) {
                        // 4. Encode OUCH Message
                        bool encoded = OUCHBuilder::build_enter_order(
                            ouch_buffer, sizeof(ouch_buffer),
                            "ORD12345", // token
                            'S',        // side
                            order_shares, // shares
                            "AAPL",     // stock
                            order_price // price
                        );
                        
                        if (encoded) {
                            risk_engine.update_position(-static_cast<int64_t>(order_shares));
                            uint64_t t1 = TSCUtils::read_tsc(); // End measuring
                            double nanos = TSCUtils::cycles_to_nanoseconds(t1 - t0, tsc_freq);
                            tick_to_trade_latencies.push_back(nanos);
                        }
                    }
                }

                processed++;
            }
        }

        // Compute metrics
        if (!tick_to_trade_latencies.empty()) {
            double sum = 0;
            double max_lat = 0;
            double min_lat = 1e9;
            for (double lat : tick_to_trade_latencies) {
                sum += lat;
                if (lat > max_lat) max_lat = lat;
                if (lat < min_lat) min_lat = lat;
            }
            double avg = sum / tick_to_trade_latencies.size();
            
            std::cout << "\n=== Tick-to-Trade Egress Metrics ===" << std::endl;
            std::cout << "Trades Triggered : " << tick_to_trade_latencies.size() << std::endl;
            std::cout << "Min Latency      : " << min_lat << " ns" << std::endl;
            std::cout << "Avg Latency      : " << avg << " ns" << std::endl;
            std::cout << "Max Latency      : " << max_lat << " ns" << std::endl;
            std::cout << "Final Position   : " << risk_engine.get_current_position() << std::endl;
            std::cout << "====================================\n" << std::endl;
        }

    });

    network_thread.join();
    strategy_thread.join();

    NumaMemoryUtils::destroy_and_free(queue);
    std::cout << "Tick-to-Trade Pipeline Benchmark Complete!" << std::endl;
    return 0;
}
