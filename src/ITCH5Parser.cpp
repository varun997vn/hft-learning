#include "ITCH5Parser.hpp"
#include "../include/AsyncLogger.hpp"
#include <cstring>
#include <algorithm>

namespace itch5 {

// Fast byteswapping wrappers
inline uint16_t bswap16(uint16_t x) { return __builtin_bswap16(x); }
inline uint32_t bswap32(uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t bswap64(uint64_t x) { return __builtin_bswap64(x); }

bool Parser::parse(const uint8_t* buffer, size_t length, InternalMessage& out_msg) {
    // If there is not even enough length for the message type byte
    if (length == 0) [[unlikely]] return false;

    uint8_t type = buffer[0];
    out_msg.type = type;

    switch (type) {
        case 'A': {
            if (length < sizeof(AddOrderMsg)) [[unlikely]] return false;
            
            // Zero-copy cast directly onto the packed struct
            const AddOrderMsg* msg = reinterpret_cast<const AddOrderMsg*>(buffer);
            
            // ITCH fields are big-endian, we must convert to host byte order
            out_msg.id = bswap64(msg->order_ref_number);
            out_msg.quantity = bswap32(msg->shares);
            out_msg.price = bswap32(msg->price);
            
            if (msg->buy_sell_indicator == 'B') {
                out_msg.side = Side::BUY;
            } else {
                out_msg.side = Side::SELL;
            }
            return true;
        }
        case 'X': {
            if (length < sizeof(OrderCancelMsg)) [[unlikely]] return false;
            const OrderCancelMsg* msg = reinterpret_cast<const OrderCancelMsg*>(buffer);
            out_msg.id = bswap64(msg->order_ref_number);
            out_msg.quantity = bswap32(msg->canceled_shares);
            return true;
        }
        case 'D': {
            if (length < sizeof(OrderDeleteMsg)) [[unlikely]] return false;
            const OrderDeleteMsg* msg = reinterpret_cast<const OrderDeleteMsg*>(buffer);
            out_msg.id = bswap64(msg->order_ref_number);
            return true;
        }
        case 'E': {
            if (length < sizeof(OrderExecutedMsg)) [[unlikely]] return false;
            const OrderExecutedMsg* msg = reinterpret_cast<const OrderExecutedMsg*>(buffer);
            out_msg.id = bswap64(msg->order_ref_number);
            out_msg.quantity = bswap32(msg->executed_shares);
            return true;
        }
        case 'C': {
            if (length < sizeof(OrderExecutedWithPriceMsg)) [[unlikely]] return false;
            const OrderExecutedWithPriceMsg* msg = reinterpret_cast<const OrderExecutedWithPriceMsg*>(buffer);
            out_msg.id = bswap64(msg->order_ref_number);
            out_msg.quantity = bswap32(msg->executed_shares);
            out_msg.price = bswap32(msg->execution_price);
            return true;
        }
        default:
            // We ignore other ITCH messages for now (e.g. Executions, Trading Action)
            HFT::AsyncLogger::getInstance().log(HFT::LogLevel::WARN, "ITCH5Parser: Unsupported message type %c", type);
            // returning false indicates this message is not meant for our LOB updating path
            return false;
    }
}

static void write_timestamp(uint8_t* dest, uint64_t ts) {
    dest[0] = (ts >> 40) & 0xFF;
    dest[1] = (ts >> 32) & 0xFF;
    dest[2] = (ts >> 24) & 0xFF;
    dest[3] = (ts >> 16) & 0xFF;
    dest[4] = (ts >> 8) & 0xFF;
    dest[5] = ts & 0xFF;
}

bool ITCH5Builder::build_add_order(uint8_t* buffer, size_t length,
                                   uint16_t stock_locate,
                                   uint16_t tracking_number,
                                   uint64_t timestamp,
                                   uint64_t order_ref_number,
                                   char buy_sell,
                                   uint32_t shares,
                                   const char* stock,
                                   uint32_t price) {
    if (length < sizeof(AddOrderMsg)) return false;

    AddOrderMsg* msg = reinterpret_cast<AddOrderMsg*>(buffer);
    msg->msg_type = 'A';
    msg->stock_locate = bswap16(stock_locate);
    msg->tracking_number = bswap16(tracking_number);
    write_timestamp(msg->timestamp, timestamp);
    msg->order_ref_number = bswap64(order_ref_number);
    msg->buy_sell_indicator = buy_sell;
    msg->shares = bswap32(shares);
    
    // Copy stock padded
    size_t len = stock ? std::strlen(stock) : 0;
    size_t copy_len = std::min(len, (size_t)8);
    if (copy_len > 0) std::memcpy(msg->stock, stock, copy_len);
    for (size_t i = copy_len; i < 8; ++i) msg->stock[i] = ' ';

    msg->price = bswap32(price);
    return true;
}

} // namespace itch5
