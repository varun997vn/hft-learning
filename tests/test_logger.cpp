#include <gtest/gtest.h>
#include "AsyncLogger.hpp"
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

using namespace HFT;

class AsyncLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Remove existing log file if any
        std::remove("test_log.txt");
    }

    void TearDown() override {
        // Shut down the logger explicitly
        AsyncLogger::getInstance().shutdown();
        std::remove("test_log.txt");
    }
};

TEST_F(AsyncLoggerTest, BasicLogging) {
    auto& logger = AsyncLogger::getInstance();
    logger.init("test_log.txt", 1024 * 1024); // 1MB
    
    // Must register thread before logging
    logger.registerThread(1);
    
    LOG_INFO("This is a test message: %d", 42);
    LOG_WARN("This is a warning: %s", "danger");
    
    // Shut down to flush logs to disk
    logger.shutdown();
    
    // Read file and verify
    std::ifstream file("test_log.txt");
    ASSERT_TRUE(file.is_open());
    
    std::string line1, line2;
    std::getline(file, line1);
    std::getline(file, line2);
    
    EXPECT_NE(line1.find("This is a test message: 42"), std::string::npos);
    EXPECT_NE(line1.find("[INFO]"), std::string::npos);
    
    EXPECT_NE(line2.find("This is a warning: danger"), std::string::npos);
    EXPECT_NE(line2.find("[WARN]"), std::string::npos);
}

TEST_F(AsyncLoggerTest, MultiThreadLogging) {
    auto& logger = AsyncLogger::getInstance();
    logger.init("test_log.txt", 10 * 1024 * 1024);
    
    const int num_threads = 4;
    const int messages_per_thread = 10000;
    
    auto worker = [&logger](int thread_id) {
        logger.registerThread(thread_id);
        for (int i = 0; i < messages_per_thread; ++i) {
            LOG_INFO("Thread %d message %d", thread_id, i);
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i + 1);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    logger.shutdown();
    
    std::ifstream file("test_log.txt");
    ASSERT_TRUE(file.is_open());
    
    int total_lines = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            total_lines++;
        }
    }
    
    EXPECT_EQ(total_lines, num_threads * messages_per_thread);
}
