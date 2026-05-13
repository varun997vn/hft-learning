---
id: strategy-engine-framework
title: Strategy Engine Framework
sidebar_label: Strategy Engine
sidebar_position: 4
---

# Modular Trading Strategy Framework

At the heart of the decision-making process lies the **Strategy Engine**. It bridges the gap between passive market data ingestion and active order execution. The framework is designed to be highly modular, allowing quantitative researchers to deploy multiple distinct alpha models without modifying the core ultra-low latency infrastructure.

## Core Components

The framework is built around three primary C++ abstractions:

### 1. `IStrategy` (The Interface)
`IStrategy` is a pure virtual C++ class defining the lifecycle and event hooks for any trading strategy. 
- **`onMarketDataUpdate(const OrderBook& book)`**: Triggered immediately after the LOB processes a tick.
- **`onOrderAccepted(const Order& order)`**: Callback when the exchange acknowledges our order.
- **`onOrderExecuted(const Execution& exec)`**: Callback when our resting limit order is filled.

By adhering to this interface, researchers can inject custom logic ranging from simple market making to complex statistical arbitrage.

### 2. `StrategyEngine` (The Orchestrator)
The `StrategyEngine` acts as the central router and context manager on the Strategy Core (Thread 1). 
- It maintains a registry of all active `IStrategy` instances.
- When the `OrderBook` is updated, the `StrategyEngine` iterates through the active strategies, firing `onMarketDataUpdate` callbacks.
- It exposes the critical `sendOrder()` function to the strategies.

### 3. `MarketMakerStrategy` (The Implementation)
As a reference implementation, the codebase includes a `MarketMakerStrategy`. It demonstrates how to consume LOB state (e.g., Best Bid and Offer, spread calculation) and generate dynamic liquidity provision orders.

## Execution Flow

1. **Market Data Arrives**: The `SPSCQueue` pops an `InternalMessage` and applies it to the `OrderBook`.
2. **Strategy Trigger**: The main trading loop immediately calls `StrategyEngine::onMarketDataUpdate()`.
3. **Alpha Calculation**: Inside `MarketMakerStrategy`, the spread is evaluated. If the spread crosses a certain threshold or positions need hedging, it determines order parameters (Price, Quantity, Side).
4. **Order Dispatch**: The strategy calls `engine->sendOrder(symbol, price, qty, side)`.

## Integration with Risk and Networking

The `StrategyEngine` does not send orders to the network directly. Instead, when `sendOrder()` is called, the engine securely routes the request through the **Pre-Trade Risk Engine**. 

If the risk checks pass (e.g., fat finger checks, max position limits), the engine delegates the binary serialization to the `OUCHBuilder` and the physical transmission to the `TCPEgress` layer. This strict separation of concerns ensures that buggy strategy code cannot bypass risk limits or corrupt the TCP socket stream.

## Performance Considerations

Virtual function calls (`vtable` lookups) introduce a small amount of latency (typically a few nanoseconds). While usually negligible, in hyper-optimized scenarios, this polymorphism can be optimized out using **C++ CRTP (Curiously Recurring Template Pattern)** to achieve compile-time static polymorphism, maintaining a clean interface without runtime overhead.
