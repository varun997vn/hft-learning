#pragma once

#include "ITCH5Parser.hpp"
#include "OrderBook.hpp"

namespace HFT {

class StrategyEngine;

/**
 * @brief Abstract interface for all Trading Strategies.
 */
class IStrategy {
public:
    virtual ~IStrategy() = default;

    /**
     * @brief Called when a new market data message is processed and the LOB is updated.
     * @param msg The parsed ITCH message.
     * @param lob The current state of the Limit Order Book.
     * @param engine Reference to the StrategyEngine to submit orders.
     */
    virtual void onMarketDataUpdate(const itch5::Parser::InternalMessage& msg, const LimitOrderBook& lob, StrategyEngine& engine) = 0;
};

} // namespace HFT
