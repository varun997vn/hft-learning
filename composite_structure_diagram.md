# Composite Structure Diagram: HFT System

A Composite Structure Diagram illustrates the internal structure of a class or component, including its parts, ports, and connectors. Below is the Mermaid representation of the `numa-portfolio` project's internal structure.

## 1. Component Internal Wiring (Parts & Connectors)

This diagram focuses on the runtime instantiation of parts (components/objects) within the main system, and the connectors (queues, sockets, method calls) that wire them together.

```mermaid
flowchart TB
    subgraph System ["«system» HFT Platform"]
        direction TB

        subgraph Core0 ["Part: Network Thread (Pin: Core 0)"]
            direction TB
            NEL[NetworkEventLoop]
            UDP_In((UDP Port 12345))
            TCP_In((TCP Port 9000))
            Parser[ITCH 5.0 Parser]
            
            NEL -- "epoll_wait" --> UDP_In
            NEL -- "epoll_wait" --> TCP_In
            UDP_In -- "raw buffer" --> Parser
        end

        subgraph NUMA0 ["Part: NUMA Node 0 Memory"]
            SPSC[(SPSC Queue)]
            LOB_Pool[(LOB Memory Pool)]
        end

        subgraph Core1 ["Part: Strategy Thread (Pin: Core 1)"]
            direction TB
            SE[StrategyEngine]
            LOB[LimitOrderBook]
            EMS[ExecutionManagementSystem]
            PTR[PreTradeRiskEngine]
            TCPE[TCPEgressGateway]
            MMS[MarketMakerStrategy]

            %% Internal associations
            SE -- "contains" --> MMS
            SE -- "ref" --> EMS
            SE -- "ref" --> PTR
            SE -- "ref" --> TCPE
            
            MMS -- "reads" --> LOB
        end

        %% Global Parts
        Logger[AsyncLogger]

        %% Connectors / Information Flow
        Parser -- "push(InternalMessage)" --> SPSC
        SPSC -- "pop(InternalMessage)" --> Core1
        Core1 -- "apply(Add/Cancel/Mod)" --> LOB
        Core1 -- "onMarketDataUpdate" --> SE
        SE -- "sendOrder" --> PTR
        PTR -- "check_order" --> EMS
        EMS -- "onOrderSent" --> TCPE
        TCPE -- "send()" --> TCPOut((Network Outbound))
        
        Core0 -. "LOG_INFO" .-> Logger
        Core1 -. "LOG_INFO" .-> Logger
    end

    style System fill:#f9f9f9,stroke:#333,stroke-width:2px
    style Core0 fill:#e1f5fe,stroke:#03a9f4,stroke-width:2px
    style Core1 fill:#e8f5e9,stroke:#4caf50,stroke-width:2px
    style NUMA0 fill:#fff3e0,stroke:#ff9800,stroke-width:2px
```

## 2. UML Composite Structure via Class Diagram

This diagram explicitly uses UML composition (`*--`) to demonstrate how the global `HFTSystem` is composed of its constituent parts, and how those parts are internally structured with their own sub-parts.

```mermaid
classDiagram
    %% Top-Level System Composition
    class HFTSystem {
        <<system>>
    }

    HFTSystem *-- NetworkEventLoop : «part» Core 0
    HFTSystem *-- SPSCQueue : «part» NUMA 0
    HFTSystem *-- LimitOrderBook : «part» Core 1
    HFTSystem *-- StrategyEngine : «part» Core 1
    HFTSystem *-- ExecutionManagementSystem : «part» Core 1
    HFTSystem *-- PreTradeRiskEngine : «part» Core 1
    HFTSystem *-- TCPEgressGateway : «part» Core 1
    HFTSystem *-- AsyncLogger : «part» Global

    %% Internal Structure of Strategy Engine
    class StrategyEngine {
        <<composite>>
        -ExecutionManagementSystem& ems_
        -PreTradeRiskEngine& risk_engine_
        -TCPEgressGateway& tcp_egress_
        -vector~unique_ptr~IStrategy~~ strategies_
    }
    StrategyEngine *-- IStrategy : «part» vector
    IStrategy <|-- MarketMakerStrategy : realizes

    %% Internal Structure of Limit Order Book
    class LimitOrderBook {
        <<composite>>
        -PriceLevel* bids_
        -PriceLevel* asks_
        -Order* order_pool_
        -HashEntry* order_map_
    }
    LimitOrderBook *-- PriceLevel : «part» flat array
    LimitOrderBook *-- Order : «part» mmap pool
    LimitOrderBook *-- HashEntry : «part» open addressing map

    %% Internal Structure of EMS
    class ExecutionManagementSystem {
        <<composite>>
        -HashEntry* order_map_
    }
    ExecutionManagementSystem *-- EMSOrder : «part» open addressing map

    %% Internal Structure of Network Event Loop
    class NetworkEventLoop {
        <<composite>>
        -vector~ReadCallback~ fd_callbacks_
        -int epoll_fd_
    }
```
