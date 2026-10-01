#include <exchange/order_book.hpp>

OrderBook::OrderBook() {

}

void OrderBook::add_order(const Order& od)
{
    if (od.side == Side::BUY) {
        auto [it, inserted] = bp_mp.try_emplace(od.price);
        it->second.push_back(od);
        id_mp[od.order_id] = std::prev(it->second.end());
    }
    else {
        auto [it, inserted] = sp_mp.try_emplace(od.price);
        it->second.push_back(od);
        id_mp[od.order_id] = std::prev(it->second.end());
    }
}

bool OrderBook::remove_order(u64 od_id)
{
    auto it = id_mp.find(od_id);
    if (it == id_mp.end()) return false;
    auto list_it = it->second;
    u32 price = list_it->price;

    if (list_it->side == Side::BUY) {
        bp_mp[price].erase(list_it);
        if (bp_mp[price].empty()) bp_mp.erase(price);
    } else {
        sp_mp[price].erase(list_it);
        if (sp_mp[price].empty()) sp_mp.erase(price);
    }

    id_mp.erase(it);
    return true;
}

u32 OrderBook::best_bid() const
{
    if (bp_mp.empty()) return 0;
    return bp_mp.rbegin()->first;
}

u32 OrderBook::best_ask() const
{
    if (sp_mp.empty()) return UINT32_MAX;
    return sp_mp.begin()->first;
}