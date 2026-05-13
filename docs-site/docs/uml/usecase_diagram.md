---
id: uml-usecase_diagram
title: Usecase Diagram
sidebar_label: Usecase Diagram
---

# NUMA-Portfolio Use Case Diagram

This document contains the comprehensive Use Case Diagram for the `numa-portfolio` project, illustrating the key actors (both human and external systems) and their interactions with the system's core functionalities, benchmarking suites, and high-frequency trading capabilities.

```mermaid
flowchart LR
    %% Define Actors
    Admin(("System Administrator\n(Admin)"))
    Researcher(("Quantitative Researcher\n(Researcher)"))
    BenchEng(("Benchmarking Engine\n(Automated/DevOps)"))
    ExchangeUDP(("Exchange Data Feed\n(External UDP)"))
    ExchangeTCP(("Exchange Simulator\n(External TCP)"))

    %% System Boundary: NUMA-Portfolio HFT Core
    subgraph HFT_System [NUMA-Portfolio HFT Core System]
        direction TB
        RunHFT([Run Main HFT Application])
        ConfNUMA([Configure NUMA Affinity & Thread Pinning])
        ManRisk([Manage Pre-Trade Risk Limits])
        MonLog([Monitor Asynchronous Logs])
        ProcITCH([Process ITCH Market Data])
        ExecStrat([Execute Trading Strategies])
        MainLOB([Maintain Limit Order Book])
        SendOUCH([Transmit OUCH4.2 Orders])
    end

    %% System Boundary: Benchmarking & Analysis Suite
    subgraph Bench_Suite [Benchmarking & Analysis Suite]
        direction TB
        RunBench([Run Latency & Throughput Benchmarks])
        ProfCache([Profile Hardware Cache Misses])
        AnalMet([Analyze Performance Metrics])
        OptKernel([Optimize Kernel & NUMA Settings])
        ParseRes([Parse CSV Benchmark Results])
    end

    %% Actor to Use Case Relationships: Administrator
    Admin --- RunHFT
    Admin --- ConfNUMA
    Admin --- ManRisk
    Admin --- MonLog
    Admin --- OptKernel

    %% Actor to Use Case Relationships: Researcher
    Researcher --- ExecStrat
    Researcher --- ManRisk
    Researcher --- AnalMet
    Researcher --- ParseRes

    %% Actor to Use Case Relationships: Benchmarking Engine
    BenchEng --- RunBench
    BenchEng --- ProfCache
    BenchEng --- ParseRes

    %% External Systems Interactions
    ExchangeUDP -- "Sends ITCH Data" --> ProcITCH
    SendOUCH -- "Sends OUCH Orders" --> ExchangeTCP
    ExchangeTCP -- "Sends Executions/Accepts" --> RunHFT

    %% Internal Include/Extend Relationships
    RunHFT -. "<<includes>>" .-> ConfNUMA
    RunHFT -. "<<includes>>" .-> ProcITCH
    RunHFT -. "<<includes>>" .-> MainLOB
    RunHFT -. "<<includes>>" .-> ExecStrat
    RunHFT -. "<<includes>>" .-> SendOUCH
    RunHFT -. "<<includes>>" .-> MonLog

    ExecStrat -. "<<includes>>" .-> ManRisk
    ProcITCH -. "<<includes>>" .-> MainLOB
    
    RunBench -. "<<includes>>" .-> ProfCache
    AnalMet -. "<<extends>>" .-> ParseRes
    OptKernel -. "<<extends>>" .-> ProfCache

    %% Styling for visual clarity
    classDef actor fill:#f9f9f9,stroke:#333,stroke-width:2px,color:#000;
    classDef usecase fill:#e1f5fe,stroke:#0288d1,stroke-width:2px,color:#000;
    classDef system fill:#ffffff,stroke:#666,stroke-width:2px,stroke-dasharray: 5 5;
    
    class Admin,Researcher,BenchEng,ExchangeUDP,ExchangeTCP actor;
    class RunHFT,ConfNUMA,ManRisk,MonLog,ProcITCH,ExecStrat,MainLOB,SendOUCH,RunBench,ProfCache,AnalMet,OptKernel,ParseRes usecase;
    class HFT_System,Bench_Suite system;
```

## Description of Actors

1. **System Administrator**: Responsible for deploying the system, configuring NUMA node affinity, pinning threads to specific CPU cores, setting risk boundaries, and monitoring system health via the asynchronous logger.
2. **Quantitative Researcher**: Focuses on designing, implementing, and analyzing trading strategies (like Market Maker), configuring strategy thresholds, and reviewing post-run data parsed from benchmarks to inform further logic improvements.
3. **Benchmarking Engine**: An automated runner (e.g., `benchmark_runner.py`) or DevOps workflow that executes targeted micro-benchmarks, measures TSC latencies, and uses tools like `perf stat` to evaluate L1/LLC cache misses.
4. **Exchange Data Feed (External UDP)**: The external venue broadcasting market data via UDP multicast (ITCH5 protocol).
5. **Exchange Simulator (External TCP)**: The external venue simulator accepting order flow over TCP connections (OUCH4.2 protocol) and providing fill/reject acknowledgments.
