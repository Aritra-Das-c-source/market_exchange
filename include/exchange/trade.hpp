#pragma once

#include <cstdint>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

struct Trade {
    u64 trade_id;
    u64 buy_id;
    u64 sell_id;

    u32 price;
    u32 quantity;
};