#include <gtest/gtest.h>
#include "../include/NumaAllocator.hpp"
#include "../include/SPSCQueue.hpp"

class NumaAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
};

TEST_F(NumaAllocatorTest, AllocationAndFree) {
    // Create an SPSC Queue on NUMA node 0
    auto* queue = NumaMemoryUtils::create_on_node<SPSCQueue<int, 1024>>(0);
    ASSERT_NE(queue, nullptr);
    
    EXPECT_TRUE(queue->push(42));
    int val;
    EXPECT_TRUE(queue->pop(val));
    EXPECT_EQ(val, 42);

    NumaMemoryUtils::destroy_and_free(queue);
}
