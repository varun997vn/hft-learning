---
id: exchange-emulator
title: Exchange Emulator
sidebar_label: Exchange Emulator
sidebar_position: 8
---

# Exchange Emulator for End-to-End Testing

Testing a High-Frequency Trading (HFT) system requires a reliable, deterministic, and low-latency mock exchange. Our repository includes a built-in `ExchangeEmulator` designed to validate the end-to-end tick-to-trade pipeline on a single machine.

## Overview

The `ExchangeEmulator` acts as a complete stub for a real-world matching engine (like NASDAQ). It provides two critical network services simultaneously:
1. **UDP Multicast ITCH Broadcasts**: Streams market data updates (Adds, Executes, Cancels) to simulate market activity.
2. **TCP OUCH Listener**: Accepts incoming order entry connections from the trading strategy and processes execution logic.

By running both the Strategy Engine and the Exchange Emulator on local loopback interfaces (or locally bridged networks), we can benchmark true tick-to-trade latencies without requiring access to expensive proprietary exchange test environments.

## Architecture & Workflow

The emulator mimics the standard exchange interaction cycle:

1. **Market Data Generation**: The emulator spins up a UDP socket and broadcasts ITCH 5.0 packets containing an artificial order book state.
2. **Order Reception**: The strategy engine consumes the ITCH data, makes a trading decision, and fires a TCP OUCH `EnterOrder` message to the emulator.
3. **Execution Simulation**: The emulator's TCP listener accepts the OUCH message, parses it, and simulates matching logic. 
4. **Execution Acknowledgement**: Upon a match, the emulator immediately sends back an `Accepted` or `Executed` TCP response.
5. **Public Broadcast**: Following the execution, the emulator broadcasts the resulting trade on the UDP ITCH multicast feed, completing the loop.

## Synchronization and Networking

The `ExchangeEmulator` uses standard POSIX sockets optimized for loopback (`127.0.0.1`):
- **TCP Endpoint**: Usually bound to `127.0.0.1:9000` to receive OUCH messages.
- **UDP Multicast Endpoint**: Bound to a multicast group like `239.255.0.1:12345` for disseminating market data.

By operating in a dedicated thread or process, the emulator ensures that network egress and ingress paths in the main HFT application are heavily stressed, providing a realistic stress test for the `TCPEgress` layer, `NetworkEventLoop`, and `ITCH5Parser`.
