#include <exchange/order_book.hpp>

OrderBook::OrderBook() = default;

void OrderBook::add_order(const Order& od)
{
    if (od.side == Side::BUY) {
        auto [it, inserted] = bp_mp_.try_emplace(od.price);
        it->second.push_back(od);
        id_mp_[od.order_id] = std::prev(it->second.end());
    }
    else {
        auto [it, inserted] = sp_mp_.try_emplace(od.price);
        it->second.push_back(od);
        id_mp_[od.order_id] = std::prev(it->second.end());
    }
}

bool OrderBook::remove_order(u64 od_id)
{
    auto it = id_mp_.find(od_id);
    if (it == id_mp_.end()) return false;
    auto list_it = it->second;
    u32 price = list_it->price;

    if (list_it->side == Side::BUY) {
        bp_mp_[price].erase(list_it);
        if (bp_mp_[price].empty()) bp_mp_.erase(price);
    } else {
        sp_mp_[price].erase(list_it);
        if (sp_mp_[price].empty()) sp_mp_.erase(price);
    }

    id_mp_.erase(it);
    return true;
}

bool OrderBook::has_bid() const
{
    return !bp_mp_.empty();
}

bool OrderBook::has_ask() const
{
    return !sp_mp_.empty();
}

u32 OrderBook::best_bid() const
{
    if (bp_mp_.empty()) return 0;
    return bp_mp_.rbegin()->first;
}

u32 OrderBook::best_ask() const
{
    if (sp_mp_.empty()) return UINT32_MAX;
    return sp_mp_.begin()->first;
}

Order& OrderBook::best_bid_order()
{
    return bp_mp_.rbegin()->second.front();
}

Order& OrderBook::best_ask_order()
{
    return sp_mp_.begin()->second.front();
}

const BookSide& OrderBook::bids() const
{
    return bp_mp_;
}

const BookSide& OrderBook::asks() const
{
    return sp_mp_;
}