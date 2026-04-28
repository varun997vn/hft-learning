#include "../include/NetworkEventLoop.hpp"
#include "../include/AsyncLogger.hpp"
#include "../include/ITCH5Parser.hpp"
#include "../include/OUCH42.hpp"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <atomic>
#include <chrono>

using namespace HFT;
using namespace itch5;
using namespace ouch42;

int main() {
    std::cout << "Starting Exchange Emulator..." << std::endl;

    AsyncLogger::getInstance().init("exchange_emulator.log", 10 * 1024 * 1024);
    AsyncLogger::getInstance().registerThread(0);
    LOG_INFO("Exchange Emulator initialized.");

    NetworkEventLoop loop;
    if (!loop.init()) {
        LOG_FATAL("Failed to init NetworkEventLoop.");
        return 1;
    }

    // 1. Setup UDP socket for broadcasting ITCH
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        LOG_FATAL("Failed to create UDP socket.");
        return 1;
    }
    struct sockaddr_in multicast_addr;
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr("239.255.0.1");
    multicast_addr.sin_port = htons(12345);

    uint64_t order_ref_counter = 500000;

    // 2. Setup TCP listener for incoming OUCH orders
    int tcp_fd = loop.addTCPListener(9000, [&](int fd) {
        // Accept new connection
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            LOG_ERROR("Failed to accept client connection.");
            return;
        }

        LOG_INFO("Client connected to TCP Exchange port 9000.");

        // Register the client socket in epoll to read incoming orders
        loop.addTCPClient("0.0.0.0", 0, [client_fd, udp_sock, multicast_addr, &order_ref_counter](int cfd) {
            uint8_t buffer[1024];
            ssize_t bytes_read = recv(cfd, buffer, sizeof(buffer), 0);
            
            if (bytes_read == 0) {
                LOG_INFO("Client disconnected.");
                close(cfd);
                return;
            } else if (bytes_read < 0) {
                // Could be EAGAIN, ignore for now
                return;
            }

            // Parse OUCH Message
            EnterOrderMsg ouch_msg;
            if (OUCHParser::parse_enter_order(buffer, bytes_read, ouch_msg)) {
                LOG_INFO("Exchange received EnterOrder: %u shares of %.8s at %u", ouch_msg.shares, ouch_msg.stock, ouch_msg.price);
                
                // Construct an ITCH Add Order message to simulate matching engine action
                uint8_t itch_buffer[128];
                
                // Get timestamp (ns since midnight - mock)
                auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
                uint64_t ts_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

                bool built = ITCH5Builder::build_add_order(
                    itch_buffer, sizeof(itch_buffer),
                    1, // stock locate
                    1, // tracking number
                    ts_ns,
                    order_ref_counter++,
                    ouch_msg.buy_sell_indicator,
                    ouch_msg.shares,
                    ouch_msg.stock,
                    ouch_msg.price
                );

                if (built) {
                    // Broadcast over UDP Multicast
                    sendto(udp_sock, itch_buffer, sizeof(AddOrderMsg), 0, (struct sockaddr*)&multicast_addr, sizeof(multicast_addr));
                    LOG_INFO("Exchange broadcasted ITCH AddOrder on multicast.");
                }
            } else {
                LOG_WARN("Failed to parse OUCH EnterOrder message.");
            }
        });
    });

    if (tcp_fd < 0) {
        LOG_FATAL("Failed to bind TCP listener on port 9000.");
        return 1;
    }

    std::cout << "Emulator listening on TCP port 9000..." << std::endl;
    loop.run();

    close(udp_sock);
    AsyncLogger::getInstance().shutdown();
    return 0;
}
