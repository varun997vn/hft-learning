# HFT Project Object Diagram

This diagram represents a runtime snapshot of the HFT Local Project as instantiated in `Main.cpp`. It illustrates the key object instances, their thread affinity, and their relationships during system execution.

```mermaid
classDiagram
    %% Core System Instances
    class logger {
        <<AsyncLogger>>
        filepath: "hft_system.log"
        mmap_size: 52428800
    }

    class queue {
        <<SPSCQueue~InternalMessage~>>
        capacity: 65536
        numa_node: 0
    }

    class tcp_egress {
        <<TCPEgressGateway>>
        socket_fd: tcp_fd
    }

    %% Network Thread (Core 0) Instances
    class network_thread {
        <<std::thread>>
        core: 0
    }

    class loop {
        <<NetworkEventLoop>>
    }

    class udp_listener {
        <<UDPSocket>>
        ip: "239.255.0.1"
        port: 12345
    }

    class tcp_client {
        <<TCPSocket>>
        ip: "127.0.0.1"
        port: 9000
    }

    network_thread ..> loop : "Runs"
    loop *-- udp_listener : "Manages"
    loop *-- tcp_client : "Manages"
    loop ..> queue : "Pushes parsed messages"
    loop ..> tcp_egress : "Provides tcp_fd"

    %% Strategy Thread (Core 1) Instances
    class strategy_thread {
        <<std::thread>>
        core: 1
    }

    class lob {
        <<LimitOrderBook>>
    }

    class ems {
        <<ExecutionManagementSystem>>
        max_orders: 10000
    }

    class risk_engine {
        <<PreTradeRiskEngine>>
        max_order_size: 500
        max_position: 10000
        max_price_deviation_pct: 0.05
    }

    class strategy_engine {
        <<StrategyEngine>>
    }

    class mm_strategy {
        <<MarketMakerStrategy>>
        symbol: "AAPL"
        order_qty: 100
        threshold_price: 15000
    }

    strategy_thread ..> queue : "Pops messages"
    strategy_thread ..> lob : "Updates"
    strategy_thread ..> strategy_engine : "Feeds data"
    
    strategy_engine o-- mm_strategy : "Contains"
    strategy_engine --> ems : "Uses"
    strategy_engine --> risk_engine : "Uses"
    strategy_engine --> tcp_egress : "Uses"
    
    %% Implicit connections
    network_thread ..> logger : "Logs events"
    strategy_thread ..> logger : "Logs events"
```

## Details of the Object Instantiation

- **AsyncLogger (`logger`)**: A singleton instance initialized early in the main thread with a 50MB memory-mapped file (`hft_system.log`). Both the `strategy_thread` and `network_thread` register themselves with this logger.
- **SPSCQueue (`queue`)**: Allocated explicitly on NUMA Node 0 to ensure fast access for the network thread. It serves as the primary lock-free message bus transferring parsed ITCH messages from the `network_thread` to the `strategy_thread`.
- **TCPEgressGateway (`tcp_egress`)**: Instantiated in the main thread but relies on the `network_thread` to establish a TCP connection to the exchange emulator (port 9000) and provide the valid socket file descriptor. It is later used by the `strategy_engine` to send orders.
- **Network Thread**: Pinned to CPU Core 0. It runs the `NetworkEventLoop` (`loop`), which listens to a multicast UDP stream for market data and maintains a TCP connection for order entry.
- **Strategy Thread**: Pinned to CPU Core 1. It initializes and owns the core trading instances:
  - **LimitOrderBook (`lob`)**: Maintains the state of the market based on messages popped from the `queue`.
  - **ExecutionManagementSystem (`ems`)**: Tracks the state of up to 10,000 active orders.
  - **PreTradeRiskEngine (`risk_engine`)**: Initialized with specific risk parameters (max order 500, max position 10000, 5% deviation limit).
  - **StrategyEngine (`strategy_engine`)**: The central coordinator that owns the `MarketMakerStrategy` (`mm_strategy`) configured to trade AAPL with a quantity of 100 and a threshold price of 15000. It routes market data from the `lob` to the strategy and routes resulting orders through the `risk_engine`, `ems`, and `tcp_egress`.
