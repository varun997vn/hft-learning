#include "../include/EMS.hpp"
#include "../include/AsyncLogger.hpp"
#include <sys/mman.h>
#include <stdexcept>
#include <cstring>

namespace HFT {

ExecutionManagementSystem::ExecutionManagementSystem(size_t max_orders)
    : max_orders_(max_orders) 
{
    // Load factor of 0.5 for fast probing
    map_capacity_ = max_orders_ * 2;
    size_t map_size = sizeof(HashEntry) * map_capacity_;

    // Allocate zeroed memory for the hash map
    void* mmap_base = mmap(nullptr, map_size, PROT_READ | PROT_WRITE, 
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);

    if (mmap_base == MAP_FAILED) {
        throw std::runtime_error("EMS mmap failed");
    }

    order_map_ = static_cast<HashEntry*>(mmap_base);
}

ExecutionManagementSystem::~ExecutionManagementSystem() {
    if (order_map_ && order_map_ != MAP_FAILED) {
        munmap(order_map_, sizeof(HashEntry) * map_capacity_);
    }
}

void ExecutionManagementSystem::map_insert(const EMSOrder& order) {
    size_t index = order.order_id % map_capacity_;
    while (true) {
        // If empty slot found (key is 0 and state is NONE)
        if (order_map_[index].key == 0 && order_map_[index].value.state == OrderState::NONE) {
            order_map_[index].key = order.order_id;
            order_map_[index].value = order;
            return;
        }
        if (order_map_[index].key == order.order_id) {
            // Overwrite
            order_map_[index].value = order;
            return;
        }
        index = (index + 1) % map_capacity_;
    }
}

EMSOrder* ExecutionManagementSystem::map_find(uint64_t id) const {
    size_t index = id % map_capacity_;
    while (true) {
        if (order_map_[index].key == 0 && order_map_[index].value.state == OrderState::NONE) {
            return nullptr; // Not found
        }
        if (order_map_[index].key == id) {
            return &order_map_[index].value;
        }
        index = (index + 1) % map_capacity_;
    }
}

bool ExecutionManagementSystem::onOrderSent(uint64_t order_id, uint32_t price, uint32_t quantity) {
    if (map_find(order_id) != nullptr) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "EMS: Order %lu already exists", order_id);
        return false;
    }

    EMSOrder order;
    order.order_id = order_id;
    order.price = price;
    order.original_quantity = quantity;
    order.executed_quantity = 0;
    order.state = OrderState::PENDING_NEW;

    map_insert(order);
    AsyncLogger::getInstance().log(LogLevel::INFO, "EMS: Order %lu sent (PENDING_NEW)", order_id);
    return true;
}

bool ExecutionManagementSystem::onOrderAccepted(uint64_t order_id) {
    EMSOrder* order = map_find(order_id);
    if (!order) return false;

    if (order->state != OrderState::PENDING_NEW) {
        AsyncLogger::getInstance().log(LogLevel::WARN, "EMS: Order %lu accepted from invalid state", order_id);
        return false;
    }

    order->state = OrderState::NEW;
    AsyncLogger::getInstance().log(LogLevel::INFO, "EMS: Order %lu accepted (NEW)", order_id);
    return true;
}

bool ExecutionManagementSystem::onOrderRejected(uint64_t order_id) {
    EMSOrder* order = map_find(order_id);
    if (!order) return false;

    order->state = OrderState::REJECTED;
    AsyncLogger::getInstance().log(LogLevel::INFO, "EMS: Order %lu REJECTED", order_id);
    return true;
}

bool ExecutionManagementSystem::onOrderExecuted(uint64_t order_id, uint32_t fill_quantity, uint32_t fill_price) {
    EMSOrder* order = map_find(order_id);
    if (!order) return false;

    if (order->state == OrderState::CANCELED || order->state == OrderState::REJECTED) {
        AsyncLogger::getInstance().log(LogLevel::WARN, "EMS: Fill received on dead order %lu", order_id);
        return false;
    }

    order->executed_quantity += fill_quantity;
    
    if (order->executed_quantity >= order->original_quantity) {
        order->state = OrderState::FILLED;
        AsyncLogger::getInstance().log(LogLevel::INFO, "EMS: Order %lu fully FILLED", order_id);
    } else {
        order->state = OrderState::PARTIALLY_FILLED;
        AsyncLogger::getInstance().log(LogLevel::INFO, "EMS: Order %lu PARTIALLY_FILLED (%u/%u)", 
                                       order_id, order->executed_quantity, order->original_quantity);
    }
    
    return true;
}

bool ExecutionManagementSystem::onOrderCanceled(uint64_t order_id) {
    EMSOrder* order = map_find(order_id);
    if (!order) return false;

    order->state = OrderState::CANCELED;
    AsyncLogger::getInstance().log(LogLevel::INFO, "EMS: Order %lu CANCELED", order_id);
    return true;
}

OrderState ExecutionManagementSystem::getOrderState(uint64_t order_id) const {
    EMSOrder* order = map_find(order_id);
    if (!order) return OrderState::NONE;
    return order->state;
}

const EMSOrder* ExecutionManagementSystem::getOrder(uint64_t order_id) const {
    return map_find(order_id);
}

} // namespace HFT
