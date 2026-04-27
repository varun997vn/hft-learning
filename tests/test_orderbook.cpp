#include "../include/OrderBook.hpp"

#include <iostream>
#include <cassert>

void test_empty_book() {
    std::cout << "Testing Empty Book..." << std::endl;
    LimitOrderBook lob(100, 100);

    assert(lob.get_volume_at_price(Side::BUY, 50) == 0);
    assert(lob.get_volume_at_price(Side::SELL, 50) == 0);
    
    // Cancel non-existent order
    assert(lob.cancel(1) == false);
    
    // Modify non-existent order
    assert(lob.modify(1, 100) == false);
}

void test_add_and_cancel() {
    std::cout << "Testing Add and Cancel..." << std::endl;
    LimitOrderBook lob(100, 100);

    // Add orders
    assert(lob.add(1, Side::BUY, 50, 100) == true);
    assert(lob.get_volume_at_price(Side::BUY, 50) == 100);

    assert(lob.add(2, Side::BUY, 50, 50) == true);
    assert(lob.get_volume_at_price(Side::BUY, 50) == 150);

    // Cancel order
    assert(lob.cancel(1) == true);
    assert(lob.get_volume_at_price(Side::BUY, 50) == 50);

    // Cancel remaining
    assert(lob.cancel(2) == true);
    assert(lob.get_volume_at_price(Side::BUY, 50) == 0);
}

void test_modify_order() {
    std::cout << "Testing Modify Order..." << std::endl;
    LimitOrderBook lob(100, 100);

    assert(lob.add(1, Side::SELL, 60, 200) == true);
    assert(lob.get_volume_at_price(Side::SELL, 60) == 200);

    // Reduce quantity (keeps priority)
    assert(lob.modify(1, 100) == true);
    assert(lob.get_volume_at_price(Side::SELL, 60) == 100);

    // Increase quantity (loses priority, but volume still updates correctly)
    assert(lob.modify(1, 300) == true);
    assert(lob.get_volume_at_price(Side::SELL, 60) == 300);

    // Modify to 0 (should cancel)
    assert(lob.modify(1, 0) == true);
    assert(lob.get_volume_at_price(Side::SELL, 60) == 0);
    assert(lob.cancel(1) == false); // Already canceled
}

void test_priority_integrity() {
    std::cout << "Testing Priority Integrity (FIFO)..." << std::endl;
    LimitOrderBook lob(100, 100);

    assert(lob.add(1, Side::BUY, 50, 100) == true);
    assert(lob.add(2, Side::BUY, 50, 200) == true);
    assert(lob.add(3, Side::BUY, 50, 300) == true);

    // To verify priority without matching engine logic, we inspect the intrusive list manually.
    // We can do this by using the public `get_order` and checking its `next`/`prev` relationships.
    const Order* o1 = lob.get_order(1);
    const Order* o2 = lob.get_order(2);
    const Order* o3 = lob.get_order(3);

    assert(o1 != nullptr && o2 != nullptr && o3 != nullptr);
    assert(o1->prev == nullptr); // head
    assert(o1->next == o2);
    assert(o2->prev == o1);
    assert(o2->next == o3);
    assert(o3->prev == o2);
    assert(o3->next == nullptr); // tail

    // Modify order 1 to increase quantity -> must go to tail
    assert(lob.modify(1, 150) == true);
    
    // Now o2 should be head, then o3, then o1.
    assert(o2->prev == nullptr);
    assert(o2->next == o3);
    assert(o3->prev == o2);
    assert(o3->next == o1);
    assert(o1->prev == o3);
    assert(o1->next == nullptr);
}

void test_price_holes() {
    std::cout << "Testing Price Holes..." << std::endl;
    LimitOrderBook lob(100, 100);

    assert(lob.add(1, Side::BUY, 10, 100) == true);
    assert(lob.add(2, Side::BUY, 90, 100) == true);
    
    // Check that holes (20-89) are empty without segfaulting
    for(uint64_t p = 11; p < 90; ++p) {
        assert(lob.get_volume_at_price(Side::BUY, p) == 0);
    }
}

int main() {
    test_empty_book();
    test_add_and_cancel();
    test_modify_order();
    test_priority_integrity();
    test_price_holes();

    std::cout << "All OrderBook tests passed!" << std::endl;
    return 0;
}
