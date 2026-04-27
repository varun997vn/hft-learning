#include <gtest/gtest.h>
#include "../include/SPSCQueue.hpp"

class SPSCQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

TEST_F(SPSCQueueTest, BasicPushPop) {
    // Capacity must be greater than number of items we produce for this simple sequential test
    SPSCQueue<int, 1024> queue;
    
    // Test push
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(queue.push(i));
    }

    // Test pop
    int val;
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(queue.pop(val));
        EXPECT_EQ(val, i);
    }
    
    EXPECT_FALSE(queue.pop(val)); // Queue should be empty now
}
