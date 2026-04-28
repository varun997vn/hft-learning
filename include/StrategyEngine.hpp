#pragma once

#include "IStrategy.hpp"
#include "EMS.hpp"
#include "PreTradeRisk.hpp"
#include "ITCH5Parser.hpp"
#include "OrderBook.hpp"
#include "TCPEgress.hpp"
#include <vector>
#include <memory>

namespace HFT {

/**
 * @brief Strategy Engine acts as the container and execution context for strategies.
 */
class StrategyEngine {
public:
    StrategyEngine(ExecutionManagementSystem& ems, risk::PreTradeRiskEngine& risk_engine, TCPEgressGateway& tcp_egress);

    // Prevent copy/move
    StrategyEngine(const StrategyEngine&) = delete;
    StrategyEngine& operator=(const StrategyEngine&) = delete;

    /**
     * @brief Register a new strategy into the engine.
     */
    void addStrategy(std::unique_ptr<IStrategy> strategy);

    /**
     * @brief Distribute market data updates to all registered strategies.
     */
    void onMarketDataUpdate(const itch5::Parser::InternalMessage& msg, const LimitOrderBook& lob);

    /**
     * @brief Submits a new order to the market. Used by strategies.
     * @param side 'B' for Buy, 'S' for Sell
     * @param shares Number of shares
     * @param price Limit price
     * @param symbol Ticker symbol (e.g., "AAPL")
     * @param reference_price Best bid/ask for risk checks
     * @return true if order was approved and sent, false otherwise.
     */
    bool sendOrder(char side, uint32_t shares, uint32_t price, const char* symbol, uint32_t reference_price);

private:
    ExecutionManagementSystem& ems_;
    risk::PreTradeRiskEngine& risk_engine_;
    TCPEgressGateway& tcp_egress_;
    std::vector<std::unique_ptr<IStrategy>> strategies_;
    uint64_t order_id_counter_{1000};
};

} // namespace HFT
