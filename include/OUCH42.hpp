#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace ouch42 {

#pragma pack(push, 1)

// OUCH 4.2 Enter Order Message
struct EnterOrderMsg {
    char msg_type;           // 'O'
    char order_token[14];    // Alphanumeric order identifier
    char buy_sell_indicator; // 'B' for Buy, 'S' for Sell, 'T' for Sell Short, 'E' for Sell Short Exempt
    uint32_t shares;         // Number of shares
    char stock[8];           // Symbol
    uint32_t price;          // Price in integer form (implied 4 decimal places)
    uint32_t time_in_force;  // e.g. 99998 for Market Hours
    char firm[4];            // Firm MPID
    char display;            // 'Y' visible, 'N' hidden, etc.
    char capacity;           // 'O' principal, 'A' agency
    char intermarket_sweep_eligibility; // 'Y' or 'N'
    uint32_t minimum_quantity; // Min qty
    char cross_type;         // 'N' for continuous market
    char customer_type;      // 'R' retail, 'N' not retail
};

// OUCH 4.2 Replace Order Message
struct ReplaceOrderMsg {
    char msg_type;           // 'U'
    char existing_order_token[14];
    char replacement_order_token[14];
    uint32_t shares;
    uint32_t price;
    uint32_t time_in_force;
    char display;
    char intermarket_sweep_eligibility;
    uint32_t minimum_quantity;
};

// OUCH 4.2 Cancel Order Message
struct CancelOrderMsg {
    char msg_type;           // 'X'
    char order_token[14];
    uint32_t shares;         // Shares to cancel (0 for all)
};

#pragma pack(pop)

class OUCHBuilder {
public:
    // Builds an Enter Order Message in-place directly into the provided buffer
    // Returns true on success, false if buffer is too small
    static bool build_enter_order(uint8_t* buffer, size_t length,
                                  const char* order_token,
                                  char buy_sell,
                                  uint32_t shares,
                                  const char* stock,
                                  uint32_t price,
                                  uint32_t time_in_force = 99998,
                                  const char* firm = "TEST",
                                  char display = 'Y',
                                  char capacity = 'O',
                                  char iso = 'N',
                                  uint32_t min_qty = 0,
                                  char cross_type = 'N',
                                  char customer_type = 'N');

    static bool build_replace_order(uint8_t* buffer, size_t length,
                                    const char* existing_token,
                                    const char* replacement_token,
                                    uint32_t shares,
                                    uint32_t price,
                                    uint32_t time_in_force = 99998,
                                    char display = 'Y',
                                    char iso = 'N',
                                    uint32_t min_qty = 0);

    static bool build_cancel_order(uint8_t* buffer, size_t length,
                                   const char* order_token,
                                   uint32_t shares);
};

class OUCHParser {
public:
    // Utility for decoding network-received messages (for unit testing/simulation)
    static bool parse_enter_order(const uint8_t* buffer, size_t length, EnterOrderMsg& out_msg);
};

} // namespace ouch42
