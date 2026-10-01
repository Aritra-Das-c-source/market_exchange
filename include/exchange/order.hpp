#pragma once

#include "types.hpp"

enum class Side {
    BUY,
    SELL
};

struct Order {
    u64 order_id;
    Side side;
    u32 price;
    u32 quantity;
};