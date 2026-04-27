---
id: lob-data-structures
title: O(1) Data Structures
sidebar_position: 3
---

# O(1) Data Structures

The theoretical limit for order book operations (add, cancel, modify) is O(1)—constant time. To achieve this in practice without hitting hidden O(N) bottlenecks requires carefully chosen, interconnected data structures.

## 1. The Pre-allocated Free List (Object Pool)

When an order arrives, it needs memory. Because we cannot use `new` (as discussed in System Design), we use a pre-allocated object pool managed by a Free List.

```cpp
Order* order_pool_;
Order* free_list_head_;
```

During initialization, all `max_orders_` elements in the `order_pool_` are linked together using their internal `next` pointers.
- **Allocation (`allocate_order`)**: Takes O(1) time. It simply pops the head of the `free_list_head_`.
- **Deallocation (`deallocate_order`)**: Takes O(1) time. It pushes the freed order back onto the `free_list_head_`.

## 2. Price-Indexed Flat Arrays

A limit order book needs to rapidly aggregate volume at specific price levels and find the best bid or ask. Traditional implementations might use a Red-Black Tree (`std::map`) which offers O(log N) operations. We use flat arrays to achieve O(1) access.

```cpp
struct PriceLevel {
    Order* head{nullptr};
    Order* tail{nullptr};
    uint64_t total_volume{0};
};

PriceLevel* bids_;
PriceLevel* asks_;
```

Assuming price levels are discrete ticks (e.g., cents) and bounded by a known maximum (`max_price_`), we allocate a contiguous array of `PriceLevel` structs. To find the bid queue at price $100.00 (represented as integer 10000), we simply access `bids_[10000]`. This is an instantaneous pointer offset calculation.

## 3. Intrusive Doubly-Linked Lists

Once we find the correct `PriceLevel`, we need to insert the order into the queue to maintain Time Priority (FIFO). 

Standard `std::list` wraps your data inside a node structure:
```cpp
// What std::list does under the hood
struct Node {
    Order data;
    Node* prev;
    Node* next;
};
```
This forces dynamic allocation for the Node. Instead, we use an **intrusive** linked list. The `Order` struct *embeds* the `prev` and `next` pointers directly.

```cpp
struct alignas(64) Order {
    // ... data ...
    Order* prev;
    Order* next;
};
```

When an order is added to a `PriceLevel`, we manipulate the embedded pointers. When an order is canceled, we already have the pointer to the `Order` object, allowing us to splice it out of the doubly-linked list in exactly O(1) time.

## 4. O(1) Flat Hash Map with Linear Probing

When an exchange user sends a `Cancel` request, they provide an `OrderID`. The matching engine must find the memory address of the `Order` object associated with that ID.

Standard `std::unordered_map` uses chaining (linked lists for hash collisions), causing cache misses.

We implement a custom **Flat Hash Map using Open Addressing with Linear Probing**:

```cpp
struct HashEntry {
    uint64_t key;
    Order* value;
};
HashEntry* order_map_;
```

The `order_map_` is a contiguous array.
1. We hash the `OrderID` to find an index.
2. If the slot is occupied by a different ID, we linearly probe the adjacent slots in memory.
3. Because the map is oversized (Load Factor of 0.5), probing chains are statistically extremely short.
4. Linear probing is highly cache-friendly because traversing an array triggers the CPU prefetcher.

### Tombstones
When an order is canceled, we cannot simply wipe its slot in the hash map, as that would break the probing chain for subsequent elements. We use a special memory address (`~0ULL`) as a **Tombstone** to mark a slot as deleted but still part of a probing sequence.
