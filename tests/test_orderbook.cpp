#include <gtest/gtest.h>
#include "../include/OrderBook.hpp"

class LimitOrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup done per test if needed
    }
};

TEST_F(LimitOrderBookTest, EmptyBook) {
    LimitOrderBook lob(100, 100);

    EXPECT_EQ(lob.get_volume_at_price(Side::BUY, 50), 0);
    EXPECT_EQ(lob.get_volume_at_price(Side::SELL, 50), 0);
    
    // Cancel non-existent order
    EXPECT_FALSE(lob.cancel(1));
    
    // Modify non-existent order
    EXPECT_FALSE(lob.modify(1, 100));
}

TEST_F(LimitOrderBookTest, AddAndCancel) {
    LimitOrderBook lob(100, 100);

    // Add orders
    EXPECT_TRUE(lob.add(1, Side::BUY, 50, 100));
    EXPECT_EQ(lob.get_volume_at_price(Side::BUY, 50), 100);

    EXPECT_TRUE(lob.add(2, Side::BUY, 50, 50));
    EXPECT_EQ(lob.get_volume_at_price(Side::BUY, 50), 150);

    // Cancel order
    EXPECT_TRUE(lob.cancel(1));
    EXPECT_EQ(lob.get_volume_at_price(Side::BUY, 50), 50);

    // Cancel remaining
    EXPECT_TRUE(lob.cancel(2));
    EXPECT_EQ(lob.get_volume_at_price(Side::BUY, 50), 0);
}

TEST_F(LimitOrderBookTest, ModifyOrder) {
    LimitOrderBook lob(100, 100);

    EXPECT_TRUE(lob.add(1, Side::SELL, 60, 200));
    EXPECT_EQ(lob.get_volume_at_price(Side::SELL, 60), 200);

    // Reduce quantity (keeps priority)
    EXPECT_TRUE(lob.modify(1, 100));
    EXPECT_EQ(lob.get_volume_at_price(Side::SELL, 60), 100);

    // Increase quantity (loses priority, but volume still updates correctly)
    EXPECT_TRUE(lob.modify(1, 300));
    EXPECT_EQ(lob.get_volume_at_price(Side::SELL, 60), 300);

    // Modify to 0 (should cancel)
    EXPECT_TRUE(lob.modify(1, 0));
    EXPECT_EQ(lob.get_volume_at_price(Side::SELL, 60), 0);
    EXPECT_FALSE(lob.cancel(1)); // Already canceled
}

TEST_F(LimitOrderBookTest, PriorityIntegrity) {
    LimitOrderBook lob(100, 100);

    EXPECT_TRUE(lob.add(1, Side::BUY, 50, 100));
    EXPECT_TRUE(lob.add(2, Side::BUY, 50, 200));
    EXPECT_TRUE(lob.add(3, Side::BUY, 50, 300));

    const Order* o1 = lob.get_order(1);
    const Order* o2 = lob.get_order(2);
    const Order* o3 = lob.get_order(3);

    ASSERT_NE(o1, nullptr);
    ASSERT_NE(o2, nullptr);
    ASSERT_NE(o3, nullptr);

    EXPECT_EQ(o1->prev, nullptr);
    EXPECT_EQ(o1->next, o2);
    EXPECT_EQ(o2->prev, o1);
    EXPECT_EQ(o2->next, o3);
    EXPECT_EQ(o3->prev, o2);
    EXPECT_EQ(o3->next, nullptr);

    // Modify order 1 to increase quantity -> must go to tail
    EXPECT_TRUE(lob.modify(1, 150));
    
    EXPECT_EQ(o2->prev, nullptr);
    EXPECT_EQ(o2->next, o3);
    EXPECT_EQ(o3->prev, o2);
    EXPECT_EQ(o3->next, o1);
    EXPECT_EQ(o1->prev, o3);
    EXPECT_EQ(o1->next, nullptr);
}

TEST_F(LimitOrderBookTest, PriceHoles) {
    LimitOrderBook lob(100, 100);

    EXPECT_TRUE(lob.add(1, Side::BUY, 10, 100));
    EXPECT_TRUE(lob.add(2, Side::BUY, 90, 100));
    
    for(uint64_t p = 11; p < 90; ++p) {
        EXPECT_EQ(lob.get_volume_at_price(Side::BUY, p), 0);
    }
}
