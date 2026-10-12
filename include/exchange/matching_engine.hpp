#pragma once

#include <span>

#include "order_book.hpp"

#define MAX_EVENTS_PER_ORDER 100

class ExchangeClock {
private:
    uint64_t midnight_nanos_since_epoch;

public:
    ExchangeClock();

    inline uint64_t get_nanos_since_midnight() const;
};

enum class InputType {
    EnterOrder,
    CancelOrderRequest
};

struct EnterOrder {
    Side side;
    u32 quantity;
    u32 price;
};

struct CancelOrderRequest {
    u64 order_id;
};

struct InputEvent {
    InputType input_type;

    union {
        EnterOrder OEvent;
        CancelOrderRequest XEvent;
    };
};

enum class MarketEventType {
    AddOrder,
    OrderExecute,
    OrderDelete
};

struct AddOrder {
    u64 order_id;
    Side side;
    u32 shares;
    u32 price;
};

struct OrderExecute {
    u64 order_id;
    u32 executed_shares;
    u32 match_number;
};

struct OrderDelete {
    u64 order_id;
};

struct MarketEvent {
    MarketEventType event_type;
    u64 sequence;
    u64 timestamp;

    union {
        AddOrder AEvent;
        OrderExecute XEvent;
        OrderDelete DEvent;
    };
};

class MatchingEngine {
public:

    MatchingEngine();

    const OrderBook& book() const;
    std::span<const MarketEvent> process(const InputEvent& in);

private:
    u64 next_order_id_;
    u64 next_trade_id_;
    u64 next_sequence_;
    OrderBook book_;
    ExchangeClock clock_;

    std::vector<MarketEvent> out_;
    void emit(const MarketEvent& e);
    void submit_order(Order od);
    bool cancel_order(u64 od_id);
};