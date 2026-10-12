#pragma once

#include <gtest/gtest.h>

#include <exchange/matching_engine.hpp>

#include <vector>

class ReconstructionBook {
public:
    const BookSide& bids() const;
    const BookSide& asks() const;

    void apply_market_event(const MarketEvent& e);

private:
    std::unordered_map<u64, std::list<Order>::iterator> id_mp_;
    std::map<u32, std::list<Order>> bp_mp_;
    std::map<u32, std::list<Order>> sp_mp_;

    void add_order(const Order& od);
    bool remove_order(u64 od_id);
};