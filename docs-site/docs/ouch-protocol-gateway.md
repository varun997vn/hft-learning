---
id: ouch-protocol-gateway
title: OUCH 4.2 Protocol Gateway
sidebar_label: OUCH 4.2 Egress
sidebar_position: 2
---

# Order Execution Gateway: OUCH 4.2 Protocol

If the ITCH 5.0 protocol is how our HFT system *listens* to the market, the **OUCH 4.2 protocol** is how it *speaks* to it.

NASDAQ's OUCH protocol is the industry standard for low-latency order entry. It is designed to be as simple and lightweight as possible, stripping away the complex session management of FIX to allow orders to reach the matching engine in microseconds.

## Zero-Allocation Message Construction

To ensure minimal latency on the critical egress path, our `OUCHBuilder` creates messages directly in the pre-allocated network transmission buffers.

### Struct Packing
Like our ITCH parser, we use `#pragma pack(push, 1)` to tightly pack the C++ structs, ensuring memory layouts perfectly mirror the wire format:

```cpp
#pragma pack(push, 1)
struct EnterOrderMsg {
    char msg_type;           // 'O'
    char order_token[14];    // Alphanumeric order identifier
    char buy_sell_indicator; // 'B' for Buy, 'S' for Sell
    uint32_t shares;         // Number of shares
    char stock[8];           // Symbol
    uint32_t price;          // Price in integer form
    uint32_t time_in_force;  
    char firm[4];            
    char display;            
    char capacity;           
    char intermarket_sweep_eligibility; 
    uint32_t minimum_quantity; 
    char cross_type;         
    char customer_type;      
};
#pragma pack(pop)
```

### Endianness & Intrinsics
The OUCH protocol expects numeric values in **Network Byte Order (Big-Endian)**. Since our system runs on x86 processors (Little-Endian), we must swap the bytes.

To avoid the overhead of function calls like `htonl()`, we utilize compiler intrinsics (`__builtin_bswap32`), which compile directly into single-cycle CPU instructions:

```cpp
inline uint32_t bswap32(uint32_t x) { return __builtin_bswap32(x); }

// Inside the builder:
msg->shares = bswap32(shares);
msg->price = bswap32(price);
```

### Space Padded Strings
OUCH string fields (like `order_token` or `stock`) are not null-terminated. They must be strictly left-justified and padded with ASCII spaces (`' '`) on the right. Our builder handles this without dynamic memory allocation or `std::string` manipulation, writing directly to the `char` arrays in the struct.

## Next Steps
Once the OUCH buffer is constructed, it is ready to be flushed directly to the TCP/UDP socket via `send()` or advanced kernel-bypass frameworks like DPDK. However, before an order can be serialized, it must first pass through our [Pre-Trade Risk Engine](pre-trade-risk-engine.md).
