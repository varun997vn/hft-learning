#pragma once

#include <cstdint>

// A mock market data tick struct
struct MarketDataTick {
    char symbol[8];       // E.g., "AAPL"
    double price;         // Market price
    uint32_t size;        // Trade size
    uint64_t timestamp;   // Exchange timestamp
    uint64_t creation_tsc; // TSC counter exactly when the message was created
};
