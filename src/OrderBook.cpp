#include "../include/OrderBook.hpp"
#include "../include/AsyncLogger.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

// Special pointer value for Hash Map tombstones
static Order* const TOMBSTONE = reinterpret_cast<Order*>(~0ULL);

LimitOrderBook::LimitOrderBook(size_t max_orders, size_t max_price)
    : max_orders_(max_orders), max_price_(max_price) 
{
    // Load factor of 0.5 to keep probing chains extremely short
    map_capacity_ = max_orders_ * 2;
    init_memory_pool();
}

LimitOrderBook::~LimitOrderBook() {
    free_memory_pool();
}

void LimitOrderBook::init_memory_pool() {
    size_t bids_size = sizeof(PriceLevel) * max_price_;
    size_t asks_size = sizeof(PriceLevel) * max_price_;
    size_t pool_size = sizeof(Order) * max_orders_;
    size_t map_size  = sizeof(HashEntry) * map_capacity_;

    // Align to page size for safety (though mmap gives page-aligned addresses anyway)
    total_mmap_size_ = bids_size + asks_size + pool_size + map_size;
    
    // MAP_POPULATE ensures page faults happen during initialization, not at runtime.
    // MAP_ANONYMOUS | MAP_PRIVATE creates a memory region zeroed out and private to this process.
    mmap_base_ = mmap(nullptr, total_mmap_size_, PROT_READ | PROT_WRITE, 
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);

    if (mmap_base_ == MAP_FAILED) {
        throw std::runtime_error("mmap failed to allocate LOB memory pool.");
    }

    // Partition the single contiguous mmap block
    char* ptr = static_cast<char*>(mmap_base_);

    bids_ = reinterpret_cast<PriceLevel*>(ptr);
    ptr += bids_size;

    asks_ = reinterpret_cast<PriceLevel*>(ptr);
    ptr += asks_size;

    order_pool_ = reinterpret_cast<Order*>(ptr);
    ptr += pool_size;

    order_map_ = reinterpret_cast<HashEntry*>(ptr);
    // map_size is the rest of the block

    // Initialize free list
    free_list_head_ = nullptr;
    for (size_t i = 0; i < max_orders_; ++i) {
        order_pool_[i].next = free_list_head_;
        free_list_head_ = &order_pool_[i];
    }
}

void LimitOrderBook::free_memory_pool() {
    if (mmap_base_ && mmap_base_ != MAP_FAILED) {
        munmap(mmap_base_, total_mmap_size_);
    }
}

Order* LimitOrderBook::allocate_order() {
    if (!free_list_head_) {
        return nullptr; // Out of memory pool
    }
    Order* order = free_list_head_;
    free_list_head_ = free_list_head_->next;
    return order;
}

void LimitOrderBook::deallocate_order(Order* order) {
    order->next = free_list_head_;
    free_list_head_ = order;
}

void LimitOrderBook::map_insert(uint64_t id, Order* order) {
    size_t index = id % map_capacity_;
    while (true) {
        if (order_map_[index].value == nullptr || order_map_[index].value == TOMBSTONE) {
            order_map_[index].key = id;
            order_map_[index].value = order;
            return;
        }
        if (order_map_[index].key == id) {
            // Overwrite existing (should not happen in proper usage without cancel)
            order_map_[index].value = order;
            return;
        }
        index = (index + 1) % map_capacity_;
    }
}

const Order* LimitOrderBook::get_order(uint64_t id) const {
    return map_find(id);
}

Order* LimitOrderBook::map_find(uint64_t id) const {
    size_t index = id % map_capacity_;
    while (true) {
        if (order_map_[index].value == nullptr) {
            return nullptr; // Not found
        }
        if (order_map_[index].key == id && order_map_[index].value != TOMBSTONE) {
            return order_map_[index].value;
        }
        index = (index + 1) % map_capacity_;
    }
}

void LimitOrderBook::map_erase(uint64_t id) {
    size_t index = id % map_capacity_;
    while (true) {
        if (order_map_[index].value == nullptr) {
            return; // Not found
        }
        if (order_map_[index].key == id && order_map_[index].value != TOMBSTONE) {
            order_map_[index].value = TOMBSTONE; // Mark as deleted
            return;
        }
        index = (index + 1) % map_capacity_;
    }
}

void LimitOrderBook::insert_order(Order* order) {
    PriceLevel* level;
    if (order->side == Side::BUY) {
        level = &bids_[order->price];
    } else {
        level = &asks_[order->price];
    }

    // Append to tail (Time Priority: FIFO)
    order->prev = level->tail;
    order->next = nullptr;

    if (level->tail) {
        level->tail->next = order;
    } else {
        level->head = order; // First order at this price level
    }
    level->tail = order;

    level->total_volume += order->quantity;
}

void LimitOrderBook::remove_order(Order* order) {
    PriceLevel* level;
    if (order->side == Side::BUY) {
        level = &bids_[order->price];
    } else {
        level = &asks_[order->price];
    }

    // Unlink from intrusive list
    if (order->prev) {
        order->prev->next = order->next;
    } else {
        level->head = order->next; // Was head
    }

    if (order->next) {
        order->next->prev = order->prev;
    } else {
        level->tail = order->prev; // Was tail
    }

    level->total_volume -= order->quantity;
}

bool LimitOrderBook::add(uint64_t id, Side side, uint64_t price, uint64_t quantity) {
    if (price >= max_price_) {
        return false; // Price out of bounds
    }
    if (map_find(id) != nullptr) {
        return false; // Duplicate Order ID
    }

    Order* order = allocate_order();
    if (!order) {
        HFT::AsyncLogger::getInstance().log(HFT::LogLevel::ERROR, "OrderBook: Failed to allocate order %lu", id);
        return false; // Pool exhausted
    }

    order->id = id;
    order->side = side;
    order->price = price;
    order->quantity = quantity;

    insert_order(order);
    map_insert(id, order);

    HFT::AsyncLogger::getInstance().log(HFT::LogLevel::INFO, "OrderBook: Added order %lu", id);
    return true;
}

bool LimitOrderBook::cancel(uint64_t id) {
    Order* order = map_find(id);
    if (!order) {
        HFT::AsyncLogger::getInstance().log(HFT::LogLevel::WARN, "OrderBook: Cancel failed, order %lu not found", id);
        return false; // Order not found
    }

    remove_order(order);
    map_erase(id);
    deallocate_order(order);

    HFT::AsyncLogger::getInstance().log(HFT::LogLevel::INFO, "OrderBook: Canceled order %lu", id);
    return true;
}

bool LimitOrderBook::modify(uint64_t id, uint64_t new_quantity) {
    Order* order = map_find(id);
    if (!order) {
        return false;
    }

    if (new_quantity == 0) {
        return cancel(id);
    }

    PriceLevel* level = (order->side == Side::BUY) ? &bids_[order->price] : &asks_[order->price];

    if (new_quantity > order->quantity) {
        // Increasing quantity loses queue priority, must move to tail
        remove_order(order);
        order->quantity = new_quantity;
        insert_order(order);
    } else {
        // Reducing quantity retains queue priority
        level->total_volume -= (order->quantity - new_quantity);
        order->quantity = new_quantity;
    }

    return true;
}

uint64_t LimitOrderBook::get_volume_at_price(Side side, uint64_t price) const {
    if (price >= max_price_) return 0;
    if (side == Side::BUY) {
        return bids_[price].total_volume;
    } else {
        return asks_[price].total_volume;
    }
}
