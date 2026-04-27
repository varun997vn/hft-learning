#pragma once

#include <cstdint>
#include <cstddef>
#include "OrderBook.hpp"

namespace itch5 {

// Enforce tight packing for direct memory casting
#pragma pack(push, 1)

// 'A' - Add Order Message
struct AddOrderMsg {
    uint8_t msg_type; // 'A'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref_number;
    uint8_t buy_sell_indicator; // 'B' or 'S'
    uint32_t shares;
    char stock[8];
    uint32_t price;
};

// 'X' - Order Cancel Message
struct OrderCancelMsg {
    uint8_t msg_type; // 'X'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref_number;
    uint32_t canceled_shares;
};

// 'D' - Order Delete Message
struct OrderDeleteMsg {
    uint8_t msg_type; // 'D'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref_number;
};

#pragma pack(pop)

// Zero-allocation, fast ITCH 5.0 message parser
class Parser {
public:
    // Internal representation of an order action
    struct InternalMessage {
        char type; // 'A' for Add, 'X' for Cancel, 'D' for Delete
        uint64_t id;
        uint64_t price;
        uint64_t quantity;
        Side side;
    };

    static inline uint64_t parse_timestamp(const uint8_t* ts) {
        // 6 bytes, big endian
        return ((uint64_t)ts[0] << 40) |
               ((uint64_t)ts[1] << 32) |
               ((uint64_t)ts[2] << 24) |
               ((uint64_t)ts[3] << 16) |
               ((uint64_t)ts[4] << 8) |
               ((uint64_t)ts[5]);
    }
    
    // Parses a single raw ITCH 5.0 message buffer into our InternalMessage format.
    // Returns true if successfully parsed into a relevant message, false otherwise.
    static bool parse(const uint8_t* buffer, size_t length, InternalMessage& out_msg);
};

} // namespace itch5
