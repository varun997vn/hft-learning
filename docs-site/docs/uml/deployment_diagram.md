---
id: uml-deployment_diagram
title: Deployment Diagram
sidebar_label: Deployment Diagram
---

# Numa HFT Portfolio - Deployment Diagram

This deployment diagram illustrates the physical and logical architecture of the entire NUMA-aware High-Frequency Trading project. It captures the interaction between the exchange emulator and the core HFT system, highlighting the specific hardware topology, threading model, and network communications.

```mermaid
graph TB
    %% External Exchange Environment
    subgraph "Exchange Environment (Remote Node)"
        direction TB
        EE["Exchange Emulator Process<br/>(src/ExchangeEmulator.cpp)"]
        
        subgraph "Exchange Network Interfaces"
            UDP_OUT(("UDP Multicast Broadcaster<br/>239.255.0.1:12345"))
            TCP_IN(("TCP Listener<br/>0.0.0.0:9000"))
        end
        
        EE --> UDP_OUT
        TCP_IN --> EE
        
        E_LOG[("exchange_emulator.log")]
        EE -. "Logs" .-> E_LOG
    end

    %% Main HFT System Deployment
    subgraph "HFT Server (NUMA Optimized Topology)"
        direction TB
        
        subgraph "NUMA Node 0 (Execution Node)"
            direction TB
            
            subgraph "Core 0: Network & Parsing Thread"
                NW_LOOP["NetworkEventLoop (epoll)"]
                ITCH_P["ITCH5 Parser"]
                TCP_E["TCPEgressGateway (Client)"]
                UDP_IN(("UDP Multicast Listener"))
                
                UDP_IN --> NW_LOOP
                NW_LOOP --> ITCH_P
            end
            
            subgraph "NUMA Node 0 Memory (libnuma)"
                SPSC[("SPSC Queue<br/>(Lock-free Inter-thread)")]
                LOB_MEM[("LOB Memory Pool<br/>(mmap MAP_POPULATE)")]
            end
            
            subgraph "Core 1: Strategy & Execution Thread"
                STRAT["StrategyEngine<br/>(MarketMakerStrategy)"]
                LOB["LimitOrderBook<br/>(Zero-Allocation, O(1))"]
                RISK["PreTradeRiskEngine"]
                EMS["Execution Management System"]
                
                STRAT --> LOB
                STRAT --> EMS
                EMS --> RISK
            end
            
            %% Inter-thread flow
            ITCH_P -- "Push Parsed Market Data" --> SPSC
            SPSC -- "Poll / Yield" --> STRAT
            EMS -- "Route Order" --> TCP_E
            LOB -. "alignas(64) Memory" .- LOB_MEM
        end
        
        subgraph "Background Thread / Storage (Any Node)"
            AL["AsyncLogger<br/>(Zero-blocking)"]
            LOG_F[("hft_system.log")]
            AL -. "Write" .-> LOG_F
        end
    end

    %% Benchmarking Environment
    subgraph "Benchmarking & Testing Suite"
        direction LR
        BENCH["benchmark_lob (taskset -c 0)"]
        PERF(("perf stat (L1/LLC Cache Counters)"))
        TESTS["GoogleTest Suites (ctest)"]
        
        BENCH -. "Profile" .- PERF
    end

    %% External Network Connections
    UDP_OUT == "ITCH5 Market Data Feed" ===> UDP_IN
    TCP_E == "OUCH42 Order Entry" ===> TCP_IN
    
    %% Async Logger Connections
    STRAT -. "Log Events" .-> AL
    NW_LOOP -. "Log Events" .-> AL

    %% Styling for better visualization
    classDef node fill:#f9f9f9,stroke:#333,stroke-width:2px;
    classDef memory fill:#e1f5fe,stroke:#0288d1,stroke-width:2px;
    classDef network fill:#f3e5f5,stroke:#7b1fa2,stroke-width:2px;
    classDef process fill:#e8f5e9,stroke:#388e3c,stroke-width:2px;
    classDef core fill:#fff3e0,stroke:#f57c00,stroke-width:2px;
    
    class SPSC,LOB_MEM,LOG_F,E_LOG memory;
    class UDP_OUT,TCP_IN,UDP_IN network;
    class EE,AL,BENCH,TESTS process;
    class NW_LOOP,ITCH_P,TCP_E,STRAT,LOB,RISK,EMS core;
```

### Architecture Highlights:
- **Hardware Topology**: The system strictly maps specific responsibilities to physical CPU cores using `taskset` and `pthread_setaffinity_np` (Core 0 for Network, Core 1 for Strategy), ensuring maximum cache locality.
- **NUMA Memory Localization**: Both the SPSC queue and the memory-mapped Limit Order Book pool are strictly allocated on NUMA Node 0 to prevent cross-QPI interconnect latency during execution.
- **Network Boundaries**: 
  - Inbound market data travels over UDP Multicast (ITCH5 format).
  - Outbound execution orders travel over a direct TCP connection (OUCH42 format).
- **Asynchronous Logging**: All performance-critical threads defer logging to a non-blocking `AsyncLogger` to prevent I/O stalls during market data parsing or execution.
