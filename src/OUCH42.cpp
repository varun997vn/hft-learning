#include "../include/OUCH42.hpp"
#include <cstring>
#include <algorithm>

namespace ouch42 {

// Fast byteswapping wrappers
inline uint16_t bswap16(uint16_t x) { return __builtin_bswap16(x); }
inline uint32_t bswap32(uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t bswap64(uint64_t x) { return __builtin_bswap64(x); }

// Helper to copy strings to fixed size fields, space padding right
static void copy_string_padded(char* dest, const char* src, size_t size) {
    size_t len = 0;
    if (src != nullptr) {
        len = std::strlen(src);
    }
    size_t copy_len = std::min(len, size);
    if (copy_len > 0) {
        std::memcpy(dest, src, copy_len);
    }
    for (size_t i = copy_len; i < size; ++i) {
        dest[i] = ' ';
    }
}

bool OUCHBuilder::build_enter_order(uint8_t* buffer, size_t length,
                                    const char* order_token,
                                    char buy_sell,
                                    uint32_t shares,
                                    const char* stock,
                                    uint32_t price,
                                    uint32_t time_in_force,
                                    const char* firm,
                                    char display,
                                    char capacity,
                                    char iso,
                                    uint32_t min_qty,
                                    char cross_type,
                                    char customer_type) {
    if (length < sizeof(EnterOrderMsg)) return false;

    EnterOrderMsg* msg = reinterpret_cast<EnterOrderMsg*>(buffer);
    msg->msg_type = 'O';
    copy_string_padded(msg->order_token, order_token, sizeof(msg->order_token));
    msg->buy_sell_indicator = buy_sell;
    msg->shares = bswap32(shares);
    copy_string_padded(msg->stock, stock, sizeof(msg->stock));
    msg->price = bswap32(price);
    msg->time_in_force = bswap32(time_in_force);
    copy_string_padded(msg->firm, firm, sizeof(msg->firm));
    msg->display = display;
    msg->capacity = capacity;
    msg->intermarket_sweep_eligibility = iso;
    msg->minimum_quantity = bswap32(min_qty);
    msg->cross_type = cross_type;
    msg->customer_type = customer_type;

    return true;
}

bool OUCHBuilder::build_replace_order(uint8_t* buffer, size_t length,
                                      const char* existing_token,
                                      const char* replacement_token,
                                      uint32_t shares,
                                      uint32_t price,
                                      uint32_t time_in_force,
                                      char display,
                                      char iso,
                                      uint32_t min_qty) {
    if (length < sizeof(ReplaceOrderMsg)) return false;

    ReplaceOrderMsg* msg = reinterpret_cast<ReplaceOrderMsg*>(buffer);
    msg->msg_type = 'U';
    copy_string_padded(msg->existing_order_token, existing_token, sizeof(msg->existing_order_token));
    copy_string_padded(msg->replacement_order_token, replacement_token, sizeof(msg->replacement_order_token));
    msg->shares = bswap32(shares);
    msg->price = bswap32(price);
    msg->time_in_force = bswap32(time_in_force);
    msg->display = display;
    msg->intermarket_sweep_eligibility = iso;
    msg->minimum_quantity = bswap32(min_qty);

    return true;
}

bool OUCHBuilder::build_cancel_order(uint8_t* buffer, size_t length,
                                     const char* order_token,
                                     uint32_t shares) {
    if (length < sizeof(CancelOrderMsg)) return false;

    CancelOrderMsg* msg = reinterpret_cast<CancelOrderMsg*>(buffer);
    msg->msg_type = 'X';
    copy_string_padded(msg->order_token, order_token, sizeof(msg->order_token));
    msg->shares = bswap32(shares);

    return true;
}

bool OUCHParser::parse_enter_order(const uint8_t* buffer, size_t length, EnterOrderMsg& out_msg) {
    if (length < sizeof(EnterOrderMsg)) return false;
    
    const EnterOrderMsg* in_msg = reinterpret_cast<const EnterOrderMsg*>(buffer);
    if (in_msg->msg_type != 'O') return false;

    std::memcpy(&out_msg, in_msg, sizeof(EnterOrderMsg));
    
    // Convert back from network byte order to host byte order
    out_msg.shares = bswap32(out_msg.shares);
    out_msg.price = bswap32(out_msg.price);
    out_msg.time_in_force = bswap32(out_msg.time_in_force);
    out_msg.minimum_quantity = bswap32(out_msg.minimum_quantity);

    return true;
}

} // namespace ouch42
