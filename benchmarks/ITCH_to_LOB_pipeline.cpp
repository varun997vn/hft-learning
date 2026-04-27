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
#include <random>

constexpr size_t QUEUE_CAPACITY = 65536; 
constexpr size_t NUM_MESSAGES = 1000000;

using namespace itch5;

// Mock network generator producing high churn (Adds, Executions, Cancels, Deletes)
void generate_mock_itch_data(std::vector<std::vector<uint8_t>>& messages) {
    messages.reserve(NUM_MESSAGES);
    
    std::mt19937 gen(42); // Deterministic seed
    std::uniform_int_distribution<int> action_dist(0, 99);
    
    // Track active orders to create valid Executions and Cancels
    struct ActiveOrder {
        uint64_t id;
        uint32_t remaining_shares;
    };
    std::vector<ActiveOrder> active_orders;
    active_orders.reserve(10000);
    
    uint64_t next_id = 1;

    for (size_t i = 0; i < NUM_MESSAGES; ++i) {
        int action = action_dist(gen);
        
        // 50% Add, 20% Execute, 20% Cancel, 10% Delete
        if (action < 50 || active_orders.empty()) {
            // ADD ORDER
            AddOrderMsg msg;
            msg.msg_type = 'A';
            msg.stock_locate = __builtin_bswap16(1);
            msg.tracking_number = __builtin_bswap16(i);
            msg.order_ref_number = __builtin_bswap64(next_id);
            msg.buy_sell_indicator = (i % 2 == 0) ? 'B' : 'S';
            msg.shares = __builtin_bswap32(100);
            msg.price = __builtin_bswap32(15000 + (i % 100));
            
            std::vector<uint8_t> buf(sizeof(AddOrderMsg));
            std::memcpy(buf.data(), &msg, sizeof(AddOrderMsg));
            messages.push_back(std::move(buf));
            
            active_orders.push_back({next_id, 100});
            next_id++;
        } 
        else if (action < 70) {
            // EXECUTE ORDER
            size_t idx = i % active_orders.size(); // Pick pseudo-random active order
            ActiveOrder& ao = active_orders[idx];
            uint32_t exec_qty = (ao.remaining_shares > 10) ? 10 : ao.remaining_shares;
            
            OrderExecutedMsg msg;
            msg.msg_type = 'E';
            msg.stock_locate = __builtin_bswap16(1);
            msg.tracking_number = __builtin_bswap16(i);
            msg.order_ref_number = __builtin_bswap64(ao.id);
            msg.executed_shares = __builtin_bswap32(exec_qty);
            msg.match_number = __builtin_bswap64(i);
            
            std::vector<uint8_t> buf(sizeof(OrderExecutedMsg));
            std::memcpy(buf.data(), &msg, sizeof(OrderExecutedMsg));
            messages.push_back(std::move(buf));
            
            ao.remaining_shares -= exec_qty;
            if (ao.remaining_shares == 0) {
                active_orders.erase(active_orders.begin() + idx);
            }
        }
        else if (action < 90) {
            // CANCEL ORDER (Partial)
            size_t idx = i % active_orders.size();
            ActiveOrder& ao = active_orders[idx];
            uint32_t cx_qty = (ao.remaining_shares > 10) ? 10 : ao.remaining_shares;
            
            OrderCancelMsg msg;
            msg.msg_type = 'X';
            msg.stock_locate = __builtin_bswap16(1);
            msg.tracking_number = __builtin_bswap16(i);
            msg.order_ref_number = __builtin_bswap64(ao.id);
            msg.canceled_shares = __builtin_bswap32(cx_qty);
            
            std::vector<uint8_t> buf(sizeof(OrderCancelMsg));
            std::memcpy(buf.data(), &msg, sizeof(OrderCancelMsg));
            messages.push_back(std::move(buf));
            
            ao.remaining_shares -= cx_qty;
            if (ao.remaining_shares == 0) {
                active_orders.erase(active_orders.begin() + idx);
            }
        }
        else {
            // DELETE ORDER (Full)
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

    std::cout << "Starting E2E Pipeline Benchmark (High Churn)...\n";

    std::thread network_thread([&]() {
        ThreadPinning::pin_current_thread_to_core(producer_core);
        Parser::InternalMessage parsed_msg;
        auto start_time = TSCUtils::read_tsc();

        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
            const uint8_t* buffer = raw_network_buffers[i].data();
            size_t length = raw_network_buffers[i].size();

            if (Parser::parse(buffer, length, parsed_msg)) {
                while (!queue->push(parsed_msg)) {
                    // Busy Wait
                }
            }
        }
        
        auto end_time = TSCUtils::read_tsc();
        double nanos = TSCUtils::cycles_to_nanoseconds(end_time - start_time, tsc_freq);
        std::cout << "Network Producer finished in " << (nanos / 1e6) << " ms\n";
        producer_done.store(true, std::memory_order_release);
    });

    std::thread matching_thread([&]() {
        ThreadPinning::pin_current_thread_to_core(consumer_core);
        LimitOrderBook lob;
        Parser::InternalMessage msg;
        size_t processed = 0;

        auto start_time = TSCUtils::read_tsc();

        while (!producer_done.load(std::memory_order_acquire) || processed < NUM_MESSAGES) {
            if (queue->pop(msg)) {
                switch (msg.type) {
                    case 'A':
                        lob.add(msg.id, msg.side, msg.price, msg.quantity);
                        break;
                    case 'E':
                    case 'C':
                    case 'X': {
                        // All these reduce the quantity by the amount specified
                        const Order* order = lob.get_order(msg.id);
                        if (order) {
                            if (order->quantity <= msg.quantity) {
                                lob.cancel(msg.id);
                            } else {
                                lob.modify(msg.id, order->quantity - msg.quantity);
                            }
                        }
                        break;
                    }
                    case 'D':
                        lob.cancel(msg.id);
                        break;
                }
                processed++;
            }
        }

        auto end_time = TSCUtils::read_tsc();
        double nanos = TSCUtils::cycles_to_nanoseconds(end_time - start_time, tsc_freq);
        std::cout << "Matching Consumer finished in " << (nanos / 1e6) << " ms\n";
    });

    network_thread.join();
    matching_thread.join();

    NumaMemoryUtils::destroy_and_free(queue);
    std::cout << "Pipeline integration successful!" << std::endl;
    return 0;
}
