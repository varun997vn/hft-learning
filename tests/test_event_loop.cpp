#include <gtest/gtest.h>
#include "NetworkEventLoop.hpp"
#include "AsyncLogger.hpp"
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <atomic>

using namespace HFT;

class EventLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        AsyncLogger::getInstance().init("test_event_loop.log", 1024*1024);
        AsyncLogger::getInstance().registerThread(1);
    }
    void TearDown() override {
        AsyncLogger::getInstance().shutdown();
    }
};

TEST_F(EventLoopTest, BasicTCP) {
    NetworkEventLoop loop;
    ASSERT_TRUE(loop.init());

    std::atomic<bool> connection_received{false};

    int listen_fd = loop.addTCPListener(12345, [&](int fd) {
        // Accept connection
        int client_fd = accept(fd, nullptr, nullptr);
        if (client_fd >= 0) {
            connection_received = true;
            close(client_fd);
            loop.stop();
        }
    });

    ASSERT_GE(listen_fd, 0);

    // Background thread to run the loop
    std::thread loop_thread([&]() {
        AsyncLogger::getInstance().registerThread(2);
        loop.run();
    });

    // Client thread to connect
    std::thread client_thread([]() {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(12345);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        
        // Small sleep to ensure server is listening
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            // Success
        }
        close(sock);
    });

    client_thread.join();
    loop_thread.join();

    EXPECT_TRUE(connection_received.load());
}
