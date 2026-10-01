#pragma once

#include "types.hpp"

struct Trade {
    u64 trade_id;
    u64 buy_id;
    u64 sell_id;

    u32 price;
    u32 quantity;
};