---
id: pre-trade-risk-engine
title: Pre-Trade Risk Engine
sidebar_label: Pre-Trade Risk
sidebar_position: 3
---

# Pre-Trade Risk Engine

In high-frequency trading, software bugs or algorithmic feedback loops can generate millions of erroneous orders in seconds—a scenario that has bankrupted real-world trading firms (e.g., Knight Capital).

To prevent this, every order must pass through a **Pre-Trade Risk Engine** before it is serialized to the network. Because this engine sits squarely in the critical "Tick-to-Trade" latency path, it must be exceptionally fast, executing in single-digit nanoseconds.

## Lock-Free Risk Architecture

Our `PreTradeRiskEngine` evaluates outgoing orders against three primary constraints:

### 1. Max Order Size
The simplest check: ensuring no single order exceeds a predefined `max_order_size` (e.g., preventing a "fat finger" algorithm from attempting to buy 1,000,000 shares when it meant to buy 1,000).

### 2. Fat Finger Price Deviation
If the algorithm tries to buy a stock at \$150.00 when the Limit Order Book's current best ask is \$100.00, it is likely a bug.
The engine calculates the deviation of the requested order price from the current reference price, rejecting the order if it exceeds `max_price_deviation_pct` (e.g., 5%).

### 3. Maximum Position Limit
The most complex constraint to check concurrently is the maximum position. If multiple strategy threads are executing concurrently, they need to check and update the firm's global position.

To handle this without slow mutex locks, we use `std::atomic<int64_t>`:

```cpp
std::atomic<int64_t> current_position_{0};

// Inside the risk check:
int64_t current_pos = current_position_.load(std::memory_order_relaxed);
int64_t new_pos = (side == 'B') ? (current_pos + shares) : (current_pos - shares);

if (std::abs(new_pos) > config_.max_position) {
    return RiskCheckStatus::REJECTED_MAX_POSITION;
}
```

By utilizing `std::memory_order_relaxed`, we instruct the compiler and CPU that we do not require strict memory synchronization fences around this read, allowing the processor to fetch the value from cache instantly. 

Once an order is approved and successfully sent to the network buffer, the position is updated using an atomic fetch-and-add:

```cpp
current_position_.fetch_add(executed_qty, std::memory_order_relaxed);
```

## Performance Impact
Because these checks utilize branch-predicted integer arithmetic and relaxed atomics, the entire risk evaluation executes in approximately **5 to 10 nanoseconds**, ensuring safety without sacrificing competitiveness in the order book queue.
