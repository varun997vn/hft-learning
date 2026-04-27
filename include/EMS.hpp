#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace HFT {

enum class OrderState : uint8_t {
    NONE = 0,
    PENDING_NEW,
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELED,
    REJECTED
};

// Packed structure for memory efficiency
struct alignas(32) EMSOrder {
    uint64_t order_id;
    uint32_t price;
    uint32_t original_quantity;
    uint32_t executed_quantity;
    OrderState state;
    // Padding to 32 bytes implicitly
};

class ExecutionManagementSystem {
public:
    /**
     * @brief Construct an EMS with a predefined max order capacity
     * @param max_orders Maximum number of orders the system can track simultaneously
     */
    ExecutionManagementSystem(size_t max_orders = 1000000);
    ~ExecutionManagementSystem();

    // Prevent copy/move
    ExecutionManagementSystem(const ExecutionManagementSystem&) = delete;
    ExecutionManagementSystem& operator=(const ExecutionManagementSystem&) = delete;

    /**
     * @brief Called when the strategy decides to send a new order
     */
    bool onOrderSent(uint64_t order_id, uint32_t price, uint32_t quantity);

    /**
     * @brief Called when OUCH Gateway receives an Order Accepted message
     */
    bool onOrderAccepted(uint64_t order_id);

    /**
     * @brief Called when OUCH Gateway receives an Order Rejected message
     */
    bool onOrderRejected(uint64_t order_id);

    /**
     * @brief Called when OUCH Gateway receives an Execution/Fill message
     */
    bool onOrderExecuted(uint64_t order_id, uint32_t fill_quantity, uint32_t fill_price);

    /**
     * @brief Called when OUCH Gateway receives an Order Canceled message
     */
    bool onOrderCanceled(uint64_t order_id);

    /**
     * @brief Get the current state of an order
     */
    OrderState getOrderState(uint64_t order_id) const;

    /**
     * @brief Get the full order object. Returns nullptr if not found.
     */
    const EMSOrder* getOrder(uint64_t order_id) const;

private:
    size_t max_orders_;
    
    // Instead of a map, we use a flat array of pointers or objects for the hash table.
    // We implement a fast open-addressing hash map tailored for EMSOrders
    struct HashEntry {
        uint64_t key;
        EMSOrder value;
    };
    
    HashEntry* order_map_;
    size_t map_capacity_;

    void map_insert(const EMSOrder& order);
    EMSOrder* map_find(uint64_t id) const;
};

} // namespace HFT
