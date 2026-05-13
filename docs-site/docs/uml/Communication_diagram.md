---
id: uml-Communication_diagram
title: Communication Diagram
sidebar_label: Communication Diagram
---

# Communication Diagram

This document contains the Communication Diagram (UML) for the `numa-portfolio` project, capturing the objects involved in the system and the numbered sequence of messages exchanged between them.

```mermaid
flowchart TD
    %% Node Definitions
    Main["<u>:Main</u>"]
    StratThread["<u>:StrategyThread</u>"]
    NetThread["<u>:NetworkThread</u>"]
    
    Logger["<u>:AsyncLogger</u>"]
    Numa["<u>:NumaMemoryUtils</u>"]
    ThreadPin["<u>:ThreadPinning</u>"]
    
    ExchEmu["<u>:ExchangeEmulator</u>"]
    NetEvent["<u>:NetworkEventLoop</u>"]
    ITCHParser["<u>:ITCH5Parser</u>"]
    SPSC["<u>queue:SPSCQueue</u>"]
    
    LOB["<u>lob:LimitOrderBook</u>"]
    StratEng["<u>:StrategyEngine</u>"]
    MMStrat["<u>:MarketMakerStrategy</u>"]
    
    Risk["<u>:PreTradeRiskEngine</u>"]
    EMS["<u>:ExecutionManagementSystem</u>"]
    OUCH["<u>:OUCHBuilder</u>"]
    TCP["<u>:TCPEgressGateway</u>"]

    %% Grouping for context
    subgraph Initialization
        Main
        Numa
    end

    subgraph "NUMA Node 0 (I/O & Network)"
        NetThread
        NetEvent
        ITCHParser
        TCP
        ExchEmu
    end

    subgraph "NUMA Node 1 (Compute)"
        StratThread
        LOB
        StratEng
        MMStrat
        Risk
        EMS
        OUCH
    end

    %% 1. Initialization Sequence
    Main -->|"1.1 init(), registerThread(0)"| Logger
    Main -->|"1.2 create_on_node(0)"| Numa
    Main -->|"1.3 spawn()"| StratThread
    Main -->|"1.4 spawn()"| NetThread

    %% 2. Network Thread Execution & Market Data Ingestion
    NetThread -->|"2.1 pin_current_thread_to_core(0)"| ThreadPin
    NetThread -->|"2.2 init(), addUDPMulticastListener()"| NetEvent
    ExchEmu -->|"2.3 UDP Multicast (ITCH)"| NetEvent
    NetEvent -->|"2.4 parse(buffer, bytes)"| ITCHParser
    NetEvent -->|"2.5 push(InternalMessage)"| SPSC

    %% 3. Strategy Thread Execution & Data Processing
    StratThread -->|"3.1 pin_current_thread_to_core(1)"| ThreadPin
    StratThread -->|"3.2 pop(InternalMessage)"| SPSC
    StratThread -->|"3.3 add() / cancel() / modify()"| LOB
    StratThread -->|"3.4 onMarketDataUpdate(msg, lob)"| StratEng
    StratEng -->|"3.5 onMarketDataUpdate(msg, lob, engine)"| MMStrat
    
    %% 4. Order Generation & Execution Flow
    MMStrat -->|"4.1 sendOrder(side, shares, price)"| StratEng
    StratEng -->|"4.2 check_order(side, shares, price)"| Risk
    StratEng -->|"4.3 onOrderSent(id, price, shares)"| EMS
    StratEng -->|"4.4 build_enter_order(token, side, shares, symbol, price)"| OUCH
    StratEng -->|"4.5 update_position(risk_qty)"| Risk
    StratEng -->|"4.6 send(ouch_buffer, size)"| TCP
    
    TCP -->|"4.7 TCP OUCH EnterOrderMsg"| ExchEmu

    %% Background / Utility Messages
    StratEng -.->|"log_info() / log_error()"| Logger
    NetEvent -.->|"log_info() / log_warn()"| Logger
    ExchEmu -.->|"log_info() / log_warn()"| Logger
```

## Diagram Explanation

The UML Communication diagram above details the interactions between the primary components (instances) of the `numa-portfolio` system. The diagram emphasizes both the object connections and the sequential numbering of messages across the distributed architecture.

1. **Initialization (1.x)**: The `Main` thread sets up the globally accessible `AsyncLogger`, allocates the lock-free `SPSCQueue` specifically on NUMA Node 0 via `NumaMemoryUtils`, and spawns the worker threads (`StrategyThread` and `NetworkThread`).
2. **Network Thread Execution & Market Data Ingestion (2.x)**: `NetworkThread` pins itself to CPU core 0 using `ThreadPinning` and initiates the `NetworkEventLoop`. The `ExchangeEmulator` sends UDP multicast ITCH packets to the loop, which reads the data, parses it via `ITCH5Parser`, and pushes the result into the lock-free `SPSCQueue`.
3. **Strategy Thread Execution & Data Processing (3.x)**: The `StrategyThread` is pinned to CPU core 1. It spins by polling the `SPSCQueue` for market updates. It first updates the `LimitOrderBook` and then triggers the `StrategyEngine`, which subsequently triggers the `MarketMakerStrategy`.
4. **Order Generation & Execution Flow (4.x)**: Based on the update, the `MarketMakerStrategy` may call `sendOrder()` on the `StrategyEngine`. The engine then sequentially delegates tasks to `PreTradeRiskEngine` to validate the risk limits, `ExecutionManagementSystem` to track the outgoing order, and `OUCHBuilder` to encode the message. Finally, the TCP packets are sent via `TCPEgressGateway` to the `ExchangeEmulator`.
5. **Background Logging**: The dashed lines represent asynchronous logging sent from multiple system components to the `AsyncLogger` thread instance.
