#pragma once

#include <cstdint>
#include <atomic>
#include <string>

namespace risk {

enum class RiskCheckStatus {
    APPROVED,
    REJECTED_FAT_FINGER,
    REJECTED_MAX_ORDER_SIZE,
    REJECTED_MAX_POSITION
};

struct RiskConfig {
    uint32_t max_order_size;
    int64_t max_position;
    double max_price_deviation_pct; // e.g., 0.05 for 5%
};

class PreTradeRiskEngine {
private:
    RiskConfig config_;
    
    // Using atomic for lock-free updates across multiple threads
    std::atomic<int64_t> current_position_{0};

public:
    explicit PreTradeRiskEngine(const RiskConfig& config);

    // Disable copy/move
    PreTradeRiskEngine(const PreTradeRiskEngine&) = delete;
    PreTradeRiskEngine& operator=(const PreTradeRiskEngine&) = delete;

    // The main risk check method called in the critical path
    // Returns APPROVED if all checks pass
    RiskCheckStatus check_order(char side, uint32_t shares, uint32_t price, uint32_t reference_price);

    // Update the position asynchronously (e.g., when an execution is confirmed)
    // For buys, executed_qty is positive. For sells, it is negative.
    void update_position(int64_t executed_qty);

    int64_t get_current_position() const;
    void reset_position();
};

} // namespace risk
