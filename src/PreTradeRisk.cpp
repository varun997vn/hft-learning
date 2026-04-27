#include "../include/PreTradeRisk.hpp"
#include <cmath>
#include <algorithm>

namespace risk {

PreTradeRiskEngine::PreTradeRiskEngine(const RiskConfig& config)
    : config_(config) {}

RiskCheckStatus PreTradeRiskEngine::check_order(char side, uint32_t shares, uint32_t price, uint32_t reference_price) {
    // 1. Max Order Size Check
    if (shares > config_.max_order_size) {
        return RiskCheckStatus::REJECTED_MAX_ORDER_SIZE;
    }

    // 2. Fat Finger Check (Price Deviation)
    if (reference_price > 0) {
        double deviation = std::abs(static_cast<double>(price) - static_cast<double>(reference_price)) / static_cast<double>(reference_price);
        if (deviation > config_.max_price_deviation_pct) {
            return RiskCheckStatus::REJECTED_FAT_FINGER;
        }
    }

    // 3. Max Position Check
    // We do a fast lock-free read. This is slightly optimistic, as executions could be in flight,
    // but in HFT, optimistic risk checks are common. We also typically update the position
    // proactively when sending the order, and revert if canceled, or update only on execution.
    // Here we check against current assumed position.
    int64_t current_pos = current_position_.load(std::memory_order_relaxed);
    int64_t new_pos = (side == 'B') ? (current_pos + shares) : (current_pos - shares);
    
    if (std::abs(new_pos) > config_.max_position) {
        return RiskCheckStatus::REJECTED_MAX_POSITION;
    }

    return RiskCheckStatus::APPROVED;
}

void PreTradeRiskEngine::update_position(int64_t executed_qty) {
    // Lock-free atomic addition
    current_position_.fetch_add(executed_qty, std::memory_order_relaxed);
}

int64_t PreTradeRiskEngine::get_current_position() const {
    return current_position_.load(std::memory_order_relaxed);
}

void PreTradeRiskEngine::reset_position() {
    current_position_.store(0, std::memory_order_relaxed);
}

} // namespace risk
