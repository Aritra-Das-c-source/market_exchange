#pragma once

#include "order.hpp"

#include <unordered_map>
#include <list>
#include <map>
#include <vector>
#include <algorithm>

using BookSide = std::map<u32, std::list<Order>>;

class OrderBook {
public:
    OrderBook();

    void add_order(const Order& od);
    bool remove_order(u64 od_id);

    bool has_bid() const;
    bool has_ask() const;

    u32 best_bid() const;
    u32 best_ask() const;

    Order& best_bid_order();
    Order& best_ask_order();

    const BookSide& bids() const;
    const BookSide& asks() const;

private:
    std::unordered_map<u64, std::list<Order>::iterator> id_mp_;
    std::map<u32, std::list<Order>> bp_mp_;
    std::map<u32, std::list<Order>> sp_mp_;
};