#include "../include/MarketMakerStrategy.hpp"
#include "../include/StrategyEngine.hpp"
#include <cstring>

namespace HFT {

MarketMakerStrategy::MarketMakerStrategy(uint32_t threshold_price, uint32_t order_qty, const char* symbol)
    : threshold_price_(threshold_price), order_qty_(order_qty) {
    std::strncpy(symbol_, symbol, sizeof(symbol_) - 1);
    symbol_[sizeof(symbol_) - 1] = '\0';
}

void MarketMakerStrategy::onMarketDataUpdate(const itch5::Parser::InternalMessage& msg, const LimitOrderBook& lob, StrategyEngine& engine) {
    if (position_taken_) {
        // Simple mock: only take one position
        return;
    }

    // Simple Mock Strategy logic: If someone adds a Buy order above threshold_price_, we Sell to them
    if (msg.type == 'A' && msg.side == Side::BUY && msg.price >= threshold_price_) {
        // Send a sell order to hit the bid
        bool success = engine.sendOrder('S', order_qty_, msg.price, symbol_, msg.price);
        if (success) {
            position_taken_ = true;
        }
    }
}

} // namespace HFT
