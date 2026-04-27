#pragma once

#include <cstdint>
#include <cstddef>
#include <iostream>

// Standard Side enumeration
enum class Side { BUY, SELL };

// 1. Cache-Line Aligned Order Node
// 64-byte aligned to prevent false sharing and ensure perfect cache line fitting.
struct alignas(64) Order {
    uint64_t id;
    uint64_t price;
    uint64_t quantity;
    Side side;
    
    // Intrusive pointers for O(1) doubly-linked list removals
    Order* prev;
    Order* next;
};

// 2. Price Level Queue
struct PriceLevel {
    Order* head{nullptr};
    Order* tail{nullptr};
    uint64_t total_volume{0};
};

// Limit Order Book (LOB) matching module
// High-performance, zero dynamic allocation design.
class LimitOrderBook {
private:
    size_t max_orders_;
    size_t max_price_;
    
    // 3. Price-Indexed Flat Arrays for O(1) price-level access
    PriceLevel* bids_;
    PriceLevel* asks_;

    // 4. Memory Pool & Allocator using mmap
    Order* order_pool_;
    Order* free_list_head_;

    // 5. Pre-allocated Flat Hash Map for O(1) OrderID lookups
    struct HashEntry {
        uint64_t key;
        Order* value;
    };
    HashEntry* order_map_;
    size_t map_capacity_;

    // Memory tracking for munmap
    size_t total_mmap_size_;
    void* mmap_base_;

    // Internal initialization and management
    void init_memory_pool();
    void free_memory_pool();
    
    // Custom O(1) allocator from the pre-allocated pool
    Order* allocate_order();
    void deallocate_order(Order* order);

    // Custom Hash Map operations (Open Addressing with Linear Probing)
    void map_insert(uint64_t id, Order* order);
    Order* map_find(uint64_t id) const;
    void map_erase(uint64_t id);

    // Intrusive List operations
    void insert_order(Order* order);
    void remove_order(Order* order);

public:
    LimitOrderBook(size_t max_orders = 1000000, size_t max_price = 100000);
    ~LimitOrderBook();

    // Prevent copy/move
    LimitOrderBook(const LimitOrderBook&) = delete;
    LimitOrderBook& operator=(const LimitOrderBook&) = delete;
    LimitOrderBook(LimitOrderBook&&) = delete;
    LimitOrderBook& operator=(LimitOrderBook&&) = delete;

    // Core matching engine operations
    bool add(uint64_t id, Side side, uint64_t price, uint64_t quantity);
    bool cancel(uint64_t id);
    bool modify(uint64_t id, uint64_t new_quantity);
    
    // Utility for testing and introspection
    uint64_t get_volume_at_price(Side side, uint64_t price) const;
    const Order* get_order(uint64_t id) const;
};
