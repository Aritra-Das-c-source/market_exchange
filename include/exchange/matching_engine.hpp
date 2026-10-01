#pragma once

#include "order_book.hpp"
#include "trade.hpp"
#include <unordered_map>
#include <list>
#include <map>
#include <vector>
#include <algorithm>

struct SubmitResult {
    u64 order_id;
    u32 remaining_quantity;
    std::vector<Trade> trades;
};

class MatchingEngine {
public:

    MatchingEngine();

    const OrderBook& book() const;
    SubmitResult submit_order(Order od);
    bool cancel_order(u64 od_id);

private:
    u64 next_order_id;
    u64 next_trade_id;
    OrderBook book_;
};