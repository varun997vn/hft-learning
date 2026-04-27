#include "ITCH5Parser.hpp"

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
            // returning false indicates this message is not meant for our LOB updating path
            return false;
    }
}

} // namespace itch5
