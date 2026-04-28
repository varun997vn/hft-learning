#include "../include/NetworkEventLoop.hpp"
#include "../include/AsyncLogger.hpp"
#include "../include/SPSCQueue.hpp"
#include "../include/ITCH5Parser.hpp"
#include "../include/OrderBook.hpp"
#include "../include/EMS.hpp"
#include "../include/PreTradeRisk.hpp"
#include "../include/OUCH42.hpp"
#include "../include/NumaAllocator.hpp"
#include "../include/ThreadAffinity.hpp"
#include "../include/StrategyEngine.hpp"
#include "../include/MarketMakerStrategy.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <sys/socket.h>

using namespace HFT;
using namespace itch5;
using namespace ouch42;
using namespace risk;

constexpr size_t QUEUE_CAPACITY = 65536;

int main(int argc, char** argv) {
    std::cout << "Starting HFT System..." << std::endl;

    // 1. Initialize Logger
    AsyncLogger::getInstance().init("hft_system.log", 50 * 1024 * 1024);
    AsyncLogger::getInstance().registerThread(0); // Main thread

    LOG_INFO("System Initialization Started.");

    // 2. Allocate SPSC Queue on NUMA Node 0
    auto* queue = NumaMemoryUtils::create_on_node<SPSCQueue<itch5::Parser::InternalMessage, 65536>>(0);
    if (!queue) {
        LOG_FATAL("Failed to allocate SPSCQueue on NUMA Node 0.");
        return 1;
    }

    std::atomic<bool> is_running{true};

    // 3. Setup Strategy / Execution Thread (Core 1)
    std::thread strategy_thread([&]() {
        // Pin to Core 1
        ThreadPinning::pin_current_thread_to_core(1);
        AsyncLogger::getInstance().registerThread(1);
        LOG_INFO("Strategy Thread initialized on Core 1.");

        LimitOrderBook lob;
        ExecutionManagementSystem ems(10000);
        
        RiskConfig risk_config;
        risk_config.max_order_size = 500;
        risk_config.max_position = 10000;
        risk_config.max_price_deviation_pct = 0.05;
        PreTradeRiskEngine risk_engine(risk_config);

        StrategyEngine strategy_engine(ems, risk_engine);
        strategy_engine.addStrategy(std::make_unique<MarketMakerStrategy>(15000, 100, "AAPL"));

        Parser::InternalMessage msg;

        while (is_running.load(std::memory_order_relaxed)) {
            if (queue->pop(msg)) {
                // Update LOB
                switch (msg.type) {
                    case 'A':
                        lob.add(msg.id, msg.side, msg.price, msg.quantity);
                        break;
                    case 'D':
                        lob.cancel(msg.id);
                        break;
                    case 'X':
                        lob.modify(msg.id, msg.quantity); // Simplify cancel logic
                        break;
                }

                // Push update to Strategy Engine
                strategy_engine.onMarketDataUpdate(msg, lob);
            } else {
                // Queue is empty, yield briefly (in pure HFT, we spin)
                std::this_thread::yield();
            }
        }
        
        LOG_INFO("Strategy Thread shutting down.");
    });

    // 4. Setup Network / Parsing Thread (Core 0)
    std::thread network_thread([&]() {
        // Pin to Core 0
        ThreadPinning::pin_current_thread_to_core(0);
        AsyncLogger::getInstance().registerThread(2);
        LOG_INFO("Network Thread initialized on Core 0.");

        NetworkEventLoop loop;
        if (!loop.init()) {
            LOG_FATAL("Failed to init NetworkEventLoop.");
            return;
        }

        // Dummy multicast for testing: 239.255.0.1:12345
        int udp_fd = loop.addUDPMulticastListener("239.255.0.1", 12345, [&](int fd) {
            uint8_t buffer[2048];
            ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);
            
            if (bytes_read > 0) {
                Parser::InternalMessage parsed_msg;
                // Parse the ITCH packet
                if (Parser::parse(buffer, bytes_read, parsed_msg)) {
                    // Push to Strategy thread
                    while (!queue->push(parsed_msg)) {
                        // Queue full, spin wait
                    }
                }
            }
        });

        if (udp_fd < 0) {
            LOG_ERROR("Failed to bind UDP listener.");
        } else {
            LOG_INFO("Listening for ITCH multicast on 239.255.0.1:12345...");
        }

        loop.run(); // Blocks until loop.stop()
        LOG_INFO("Network Thread shutting down.");
    });


    // Keep main thread alive. In a real system, we'd wait for a SIGINT handler.
    // For this portfolio demo, we'll sleep for a long time or wait for user input.
    std::cout << "System running. Press Enter to stop." << std::endl;
    std::cin.get();

    is_running.store(false);
    
    // We would need to cleanly stop the event loop. Assuming we added a stop mechanism,
    // or we just terminate.
    // loop.stop() would need a reference to loop. For simplicity, we just exit.

    strategy_thread.join();
    
    // We can't easily join network_thread because loop.run() is blocking on epoll.
    // We'd have to signal it. We'll let the OS clean it up for now in this demo.
    network_thread.detach();

    NumaMemoryUtils::destroy_and_free(queue);
    AsyncLogger::getInstance().shutdown();
    
    std::cout << "System shut down gracefully." << std::endl;
    return 0;
}
