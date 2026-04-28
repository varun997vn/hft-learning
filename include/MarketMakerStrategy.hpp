#pragma once

#include "IStrategy.hpp"

namespace HFT {

/**
 * @brief A basic implementation of a mock market making strategy.
 */
class MarketMakerStrategy : public IStrategy {
public:
    MarketMakerStrategy(uint32_t threshold_price, uint32_t order_qty, const char* symbol);

    void onMarketDataUpdate(const itch5::Parser::InternalMessage& msg, const LimitOrderBook& lob, StrategyEngine& engine) override;

private:
    uint32_t threshold_price_;
    uint32_t order_qty_;
    char symbol_[8];
    bool position_taken_{false};
};

} // namespace HFT
