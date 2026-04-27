#include <gtest/gtest.h>
#include "EMS.hpp"
#include "AsyncLogger.hpp"

using namespace HFT;

class EMSTest : public ::testing::Test {
protected:
    void SetUp() override {
        AsyncLogger::getInstance().init("test_ems.log", 1024*1024);
        AsyncLogger::getInstance().registerThread(1);
    }
    void TearDown() override {
        AsyncLogger::getInstance().shutdown();
    }
};

TEST_F(EMSTest, OrderLifecycle) {
    ExecutionManagementSystem ems(100);

    // 1. Send Order
    EXPECT_TRUE(ems.onOrderSent(1001, 5000, 100));
    EXPECT_EQ(ems.getOrderState(1001), OrderState::PENDING_NEW);

    // 2. Accept Order
    EXPECT_TRUE(ems.onOrderAccepted(1001));
    EXPECT_EQ(ems.getOrderState(1001), OrderState::NEW);

    // 3. Partial Fill
    EXPECT_TRUE(ems.onOrderExecuted(1001, 40, 5000));
    EXPECT_EQ(ems.getOrderState(1001), OrderState::PARTIALLY_FILLED);
    EXPECT_EQ(ems.getOrder(1001)->executed_quantity, 40);

    // 4. Remaining Fill
    EXPECT_TRUE(ems.onOrderExecuted(1001, 60, 5000));
    EXPECT_EQ(ems.getOrderState(1001), OrderState::FILLED);
    EXPECT_EQ(ems.getOrder(1001)->executed_quantity, 100);
}

TEST_F(EMSTest, InvalidTransitions) {
    ExecutionManagementSystem ems(100);

    ems.onOrderSent(2002, 100, 10);
    ems.onOrderRejected(2002);
    EXPECT_EQ(ems.getOrderState(2002), OrderState::REJECTED);

    // Cannot execute a rejected order
    EXPECT_FALSE(ems.onOrderExecuted(2002, 10, 100));
}
