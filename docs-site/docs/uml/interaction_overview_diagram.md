---
id: uml-interaction_overview_diagram
title: Interaction Overview Diagram
sidebar_label: Interaction Overview Diagram
---

# Numa-Portfolio: Interaction Overview Diagram

An **Interaction Overview Diagram** provides a high-level, activity-diagram-like view of the flow of control across different interactions within a system. Each primary node (denoted by the `ref` subroutine shape) represents a distinct interaction or sequence diagram that encapsulates a specific functional phase of the `numa-portfolio` execution pipeline.

This diagram highlights:
- **Concurrent Execution:** The fork into the Main Thread Event Loop and the dedicated Asynchronous Logger Thread.
- **Interaction Uses:** The handoff between major system components, starting from Network Ingress, transitioning through the Strategy Engine, and concluding at Order Egress.
- **Conditional Control Flow:** Decision points for Order Generation, Pre-Trade Risk validation, and Event routing.

```mermaid
flowchart TD
    %% Start state
    Start((Start))

    %% Fork for concurrent threads
    Fork_Init[/Concurrent Thread Fork/]

    %% Main Interactions (Interaction Uses)
    Ref_InitSystem[["ref System Initialization\n(NumaAllocator, ThreadAffinity)"]]
    Ref_NetworkLoop[["ref NetworkEventLoop\n(Epoll Wait)"]]
    Ref_ProcessITCH[["ref ITCH5Parser\n(Parse Market Data)"]]
    Ref_UpdateBook[["ref OrderBook\n(Update Price Levels)"]]
    Ref_StrategyEngine[["ref StrategyEngine\n(MarketMakerStrategy Evaluation)"]]
    Ref_PreTradeRisk[["ref PreTradeRisk\n(Validate Order Limits)"]]
    Ref_EMS[["ref EMS\n(Order Routing)"]]
    Ref_OUCH42[["ref OUCH42\n(Message Encoding)"]]
    Ref_TCPEgress[["ref TCPEgress\n(Send to Network)"]]
    Ref_ExchangeEmu[["ref ExchangeEmulator\n(Order Matching)"]]
    Ref_AsyncLogger[["ref AsyncLogger\n(Dequeue & Write to Disk)"]]

    %% Main Flow
    Start --> Ref_InitSystem
    Ref_InitSystem --> Fork_Init

    %% Logger Thread Control Flow
    Fork_Init --> LoggerStart((Logger Thread))
    LoggerStart --> Ref_AsyncLogger
    Ref_AsyncLogger --> LoggerStart

    %% Main Event Loop Control Flow
    Fork_Init --> LoopStart((Main Thread))
    LoopStart --> Ref_NetworkLoop

    Ref_NetworkLoop --> EventType{Event Type?}

    %% Market Data Ingress Path
    EventType -- "ITCH5 Data" --> Ref_ProcessITCH
    Ref_ProcessITCH --> Ref_UpdateBook
    Ref_UpdateBook --> Ref_StrategyEngine

    Ref_StrategyEngine --> OrderDecision{Signal\nGenerated?}
    OrderDecision -- "No" --> Ref_NetworkLoop
    OrderDecision -- "Yes" --> Ref_PreTradeRisk

    Ref_PreTradeRisk --> RiskDecision{Risk\nPassed?}
    
    %% Order Execution Path
    RiskDecision -- "Yes" --> Ref_EMS
    Ref_EMS --> Ref_OUCH42
    Ref_OUCH42 --> Ref_TCPEgress
    
    %% Enqueue Log (Data Flow / Synchronization via SPSCQueue)
    Ref_TCPEgress -.-> |"Enqueue Log Event"| Ref_AsyncLogger
    Ref_TCPEgress --> Ref_NetworkLoop

    %% Risk Rejection Path
    RiskDecision -- "No" --> RejectLog[["ref Log Rejection"]]
    RejectLog -.-> |"Enqueue Error Log"| Ref_AsyncLogger
    RejectLog --> Ref_NetworkLoop

    %% Exchange Emulator Path (Testing/Mocking Loopback)
    EventType -- "Execution/Response" --> Ref_ExchangeEmu
    Ref_ExchangeEmu --> UpdateBookFromEx[["ref OrderBook\n(Update from Fill/Reject)"]]
    UpdateBookFromEx -.-> |"Enqueue Trade Log"| Ref_AsyncLogger
    UpdateBookFromEx --> Ref_NetworkLoop

    %% End state handling (Graceful shutdown)
    ShutdownSignal{Shutdown\nTriggered?}
    Ref_NetworkLoop -.-> |SIGINT / Terminate| ShutdownSignal
    ShutdownSignal -- "Yes" --> Join_Shutdown[\Thread Join\]
    LoggerStart -.-> Join_Shutdown
    Join_Shutdown --> End((End))

    %% Styling
    classDef refNode fill:#e1f5fe,stroke:#0288d1,stroke-width:2px,color:#000;
    class Ref_InitSystem,Ref_NetworkLoop,Ref_ProcessITCH,Ref_UpdateBook,Ref_StrategyEngine,Ref_PreTradeRisk,Ref_EMS,Ref_OUCH42,Ref_TCPEgress,Ref_ExchangeEmu,Ref_AsyncLogger,RejectLog,UpdateBookFromEx refNode;
```
