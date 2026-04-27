#include <gtest/gtest.h>
#include "ITCH5Parser.hpp"

using namespace itch5;

// Helper to quickly convert host byte order back to network byte order for the test mock
inline uint16_t hton16(uint16_t x) { return __builtin_bswap16(x); }
inline uint32_t hton32(uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t hton64(uint64_t x) { return __builtin_bswap64(x); }

TEST(ITCH5ParserTest, ParseAddOrderMessage) {
    AddOrderMsg raw_msg;
    raw_msg.msg_type = 'A';
    raw_msg.stock_locate = hton16(1234);
    raw_msg.tracking_number = hton16(5678);
    // Timestamp is 6 bytes, leaving it zeroed for this test
    raw_msg.order_ref_number = hton64(9999);
    raw_msg.buy_sell_indicator = 'B';
    raw_msg.shares = hton32(1500);
    raw_msg.price = hton32(10050); // e.g. 100.50

    Parser::InternalMessage parsed;
    bool success = Parser::parse(reinterpret_cast<const uint8_t*>(&raw_msg), sizeof(raw_msg), parsed);

    EXPECT_TRUE(success);
    EXPECT_EQ(parsed.type, 'A');
    EXPECT_EQ(parsed.id, 9999);
    EXPECT_EQ(parsed.side, Side::BUY);
    EXPECT_EQ(parsed.quantity, 1500);
    EXPECT_EQ(parsed.price, 10050);
}

TEST(ITCH5ParserTest, ParseCancelOrderMessage) {
    OrderCancelMsg raw_msg;
    raw_msg.msg_type = 'X';
    raw_msg.stock_locate = hton16(1234);
    raw_msg.tracking_number = hton16(5678);
    raw_msg.order_ref_number = hton64(9999);
    raw_msg.canceled_shares = hton32(500);

    Parser::InternalMessage parsed;
    bool success = Parser::parse(reinterpret_cast<const uint8_t*>(&raw_msg), sizeof(raw_msg), parsed);

    EXPECT_TRUE(success);
    EXPECT_EQ(parsed.type, 'X');
    EXPECT_EQ(parsed.id, 9999);
    EXPECT_EQ(parsed.quantity, 500);
}

TEST(ITCH5ParserTest, ParseDeleteOrderMessage) {
    OrderDeleteMsg raw_msg;
    raw_msg.msg_type = 'D';
    raw_msg.stock_locate = hton16(1234);
    raw_msg.tracking_number = hton16(5678);
    raw_msg.order_ref_number = hton64(9999);

    Parser::InternalMessage parsed;
    bool success = Parser::parse(reinterpret_cast<const uint8_t*>(&raw_msg), sizeof(raw_msg), parsed);

    EXPECT_TRUE(success);
    EXPECT_EQ(parsed.type, 'D');
    EXPECT_EQ(parsed.id, 9999);
}

TEST(ITCH5ParserTest, IgnoreUnsupportedMessage) {
    uint8_t dummy_message[20];
    dummy_message[0] = 'Z'; // Unrecognized type
    
    Parser::InternalMessage parsed;
    bool success = Parser::parse(dummy_message, sizeof(dummy_message), parsed);

    EXPECT_FALSE(success);
}

TEST(ITCH5ParserTest, ParseOrderExecutedMessage) {
    OrderExecutedMsg raw_msg;
    raw_msg.msg_type = 'E';
    raw_msg.stock_locate = hton16(1234);
    raw_msg.tracking_number = hton16(5678);
    raw_msg.order_ref_number = hton64(9999);
    raw_msg.executed_shares = hton32(500);
    raw_msg.match_number = hton64(1111);

    Parser::InternalMessage parsed;
    bool success = Parser::parse(reinterpret_cast<const uint8_t*>(&raw_msg), sizeof(raw_msg), parsed);

    EXPECT_TRUE(success);
    EXPECT_EQ(parsed.type, 'E');
    EXPECT_EQ(parsed.id, 9999);
    EXPECT_EQ(parsed.quantity, 500);
}

TEST(ITCH5ParserTest, ParseOrderExecutedWithPriceMessage) {
    OrderExecutedWithPriceMsg raw_msg;
    raw_msg.msg_type = 'C';
    raw_msg.stock_locate = hton16(1234);
    raw_msg.tracking_number = hton16(5678);
    raw_msg.order_ref_number = hton64(9999);
    raw_msg.executed_shares = hton32(500);
    raw_msg.match_number = hton64(1111);
    raw_msg.printable = 'Y';
    raw_msg.execution_price = hton32(10050);

    Parser::InternalMessage parsed;
    bool success = Parser::parse(reinterpret_cast<const uint8_t*>(&raw_msg), sizeof(raw_msg), parsed);

    EXPECT_TRUE(success);
    EXPECT_EQ(parsed.type, 'C');
    EXPECT_EQ(parsed.id, 9999);
    EXPECT_EQ(parsed.quantity, 500);
    EXPECT_EQ(parsed.price, 10050);
}
