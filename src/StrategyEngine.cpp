#include "../include/StrategyEngine.hpp"
#include "../include/AsyncLogger.hpp"
#include "../include/OUCH42.hpp"

#include <cstdio>
#include <cstring>

namespace HFT {

StrategyEngine::StrategyEngine(ExecutionManagementSystem& ems, risk::PreTradeRiskEngine& risk_engine, TCPEgressGateway& tcp_egress)
    : ems_(ems), risk_engine_(risk_engine), tcp_egress_(tcp_egress) {
}

void StrategyEngine::addStrategy(std::unique_ptr<IStrategy> strategy) {
    strategies_.push_back(std::move(strategy));
}

void StrategyEngine::onMarketDataUpdate(const itch5::Parser::InternalMessage& msg, const LimitOrderBook& lob) {
    for (auto& strategy : strategies_) {
        strategy->onMarketDataUpdate(msg, lob, *this);
    }
}

bool StrategyEngine::sendOrder(char side, uint32_t shares, uint32_t price, const char* symbol, uint32_t reference_price) {
    risk::RiskCheckStatus status = risk_engine_.check_order(side, shares, price, reference_price);
    
    if (status != risk::RiskCheckStatus::APPROVED) {
        LOG_WARN("Strategy order rejected by Pre-Trade Risk. Status: %d", static_cast<int>(status));
        return false;
    }

    uint64_t my_order_id = order_id_counter_++;
    
    // Update EMS
    ems_.onOrderSent(my_order_id, price, shares);

    // Format Token
    char token[15];
    std::snprintf(token, sizeof(token), "ORD%011lu", my_order_id);

    // Build OUCH
    uint8_t ouch_buffer[64];
    bool encoded = ouch42::OUCHBuilder::build_enter_order(
        ouch_buffer, sizeof(ouch_buffer),
        token,
        side,
        shares,
        symbol,
        price
    );

    if (encoded) {
        int64_t risk_qty = (side == 'B') ? shares : -static_cast<int64_t>(shares);
        risk_engine_.update_position(risk_qty);
        LOG_INFO("Strategy fired! Order %lu sent to market.", my_order_id);
        
        // Write `ouch_buffer` to TCP socket directly
        tcp_egress_.send(ouch_buffer, sizeof(ouch42::EnterOrderMsg));
        return true;
    } else {
        LOG_ERROR("Failed to encode OUCH message for Order %lu", my_order_id);
        return false;
    }
}

} // namespace HFT
