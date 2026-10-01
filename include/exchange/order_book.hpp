#pragma once

#include "order.hpp"
#include <unordered_map>
#include <list>
#include <map>
#include <vector>
#include <algorithm>

class OrderBook {
public:
    std::unordered_map<u64, std::list<Order>::iterator> id_mp;
    std::map<u32, std::list<Order>> bp_mp;
    std::map<u32, std::list<Order>> sp_mp;

    OrderBook();

    void add_order(Order od);
    bool remove_order(u64 od_id);

    u32 best_bid() const;
    u32 best_ask() const;
};