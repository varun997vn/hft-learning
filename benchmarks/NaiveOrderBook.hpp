#pragma once

#include <cstdint>
#include <map>
#include <unordered_map>
#include <list>

enum class NaiveSide { BUY, SELL };

struct NaiveOrder {
    uint64_t id;
    uint64_t price;
    uint64_t quantity;
    NaiveSide side;
};

class NaiveOrderBook {
private:
    // Dynamic allocations: tree nodes for map, list nodes for price levels
    std::map<uint64_t, std::list<NaiveOrder>> bids_;
    std::map<uint64_t, std::list<NaiveOrder>> asks_;
    
    // Unordered map for lookups (hashing overhead + dynamic allocations)
    struct OrderLocation {
        NaiveSide side;
        uint64_t price;
        std::list<NaiveOrder>::iterator it;
    };
    std::unordered_map<uint64_t, OrderLocation> orders_;

public:
    bool add(uint64_t id, NaiveSide side, uint64_t price, uint64_t quantity) {
        if (orders_.find(id) != orders_.end()) {
            return false;
        }

        NaiveOrder order{id, price, quantity, side};
        if (side == NaiveSide::BUY) {
            bids_[price].push_back(order);
            auto it = bids_[price].end();
            --it;
            orders_[id] = {side, price, it};
        } else {
            asks_[price].push_back(order);
            auto it = asks_[price].end();
            --it;
            orders_[id] = {side, price, it};
        }
        return true;
    }

    bool cancel(uint64_t id) {
        auto loc_it = orders_.find(id);
        if (loc_it == orders_.end()) return false;

        OrderLocation loc = loc_it->second;
        if (loc.side == NaiveSide::BUY) {
            bids_[loc.price].erase(loc.it);
            if (bids_[loc.price].empty()) {
                bids_.erase(loc.price);
            }
        } else {
            asks_[loc.price].erase(loc.it);
            if (asks_[loc.price].empty()) {
                asks_.erase(loc.price);
            }
        }
        orders_.erase(loc_it);
        return true;
    }
};
