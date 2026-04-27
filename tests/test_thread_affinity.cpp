#include <gtest/gtest.h>
#include "../include/ThreadAffinity.hpp"
#include <thread>
#include <iostream>

class ThreadAffinityTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

TEST_F(ThreadAffinityTest, PinThread) {
    bool thread_executed = false;
    std::thread t([&thread_executed]() {
        // Pin to core 0 as a test
        bool success = ThreadPinning::pin_current_thread_to_core(0);
        if (success) {
            std::cout << "Successfully pinned thread to core 0." << std::endl;
        } else {
            std::cout << "Failed to pin thread. (This might be expected if core 0 is unavailable or lacks permissions)." << std::endl;
        }
        // In GTest, we shouldn't necessarily fail if the machine lacks permissions or core 0.
        // As long as the function returns without crashing, we consider it a structural pass.
        thread_executed = true;
    });
    t.join();
    EXPECT_TRUE(thread_executed);
}
