#include <exchange/matching_engine.hpp>

#include <chrono>
#include <cstdint>
#include <ctime>

ExchangeClock::ExchangeClock() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    
    std::tm local_tm;
    localtime_r(&t, &local_tm);
    
    local_tm.tm_hour = 0;
    local_tm.tm_min = 0;
    local_tm.tm_sec = 0;
    
    auto midnight = std::chrono::system_clock::from_time_t(std::mktime(&local_tm));
    midnight_nanos_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
        midnight.time_since_epoch()
    ).count();
}

inline uint64_t ExchangeClock::get_nanos_since_midnight() const {
    auto now = std::chrono::system_clock::now();
    uint64_t current_nanos_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()
    ).count();
    
    return current_nanos_since_epoch - midnight_nanos_since_epoch;
}

MatchingEngine::MatchingEngine() : next_order_id_(1), next_trade_id_(1), next_sequence_(1), clock_()
{
    out_.reserve(MAX_EVENTS_PER_ORDER);
}

const OrderBook& MatchingEngine::book() const
{
    return book_;
}

void MatchingEngine::emit(const MarketEvent &e) {
    out_.push_back(e);
}

void MatchingEngine::submit_order(Order od)
{
    od.order_id = next_order_id_++;
    u64 ts = clock_.get_nanos_since_midnight();

    if (od.side == Side::BUY) {
        while (od.quantity > 0 && book_.has_ask() && od.price >= book_.best_ask()) {
            Order& best_ask_order = book_.best_ask_order();

            MarketEvent e;
            e.event_type = MarketEventType::OrderExecute;
            e.timestamp = ts;
            e.sequence = next_sequence_++;
            e.XEvent.order_id = best_ask_order.order_id;
            e.XEvent.match_number = next_trade_id_++;
            e.XEvent.executed_shares = std::min(od.quantity, best_ask_order.quantity);
            emit(e);

            od.quantity -= e.XEvent.executed_shares;
            best_ask_order.quantity -= e.XEvent.executed_shares;
            if (best_ask_order.quantity == 0) book_.remove_order(e.XEvent.order_id);
        }

        if (od.quantity) {
            book_.add_order(od);

            MarketEvent e;
            e.event_type = MarketEventType::AddOrder;
            e.timestamp = ts;
            e.sequence = next_sequence_++;
            e.AEvent.order_id = od.order_id;
            e.AEvent.price = od.price;
            e.AEvent.shares = od.quantity;
            e.AEvent.side = Side::BUY;
            emit(e);
        }
    } else {
            while (od.quantity > 0 && book_.has_bid() && od.price <= book_.best_bid()) {
            Order& best_bid_order = book_.best_bid_order();

            MarketEvent e;
            e.event_type = MarketEventType::OrderExecute;
            e.timestamp = ts;
            e.sequence = next_sequence_++;
            e.XEvent.order_id = best_bid_order.order_id;
            e.XEvent.match_number = next_trade_id_++;
            e.XEvent.executed_shares = std::min(od.quantity, best_bid_order.quantity);
            emit(e);

            od.quantity -= e.XEvent.executed_shares;
            best_bid_order.quantity -= e.XEvent.executed_shares;
            if (best_bid_order.quantity == 0) book_.remove_order(e.XEvent.order_id);
        }

        if (od.quantity) {
            book_.add_order(od);

            MarketEvent e;
            e.event_type = MarketEventType::AddOrder;
            e.timestamp = ts;
            e.sequence = next_sequence_++;
            e.AEvent.order_id = od.order_id;
            e.AEvent.price = od.price;
            e.AEvent.shares = od.quantity;
            e.AEvent.side = Side::SELL;
            emit(e);
        }
    }
}

bool MatchingEngine::cancel_order(u64 od_id)
{
    bool status = book_.remove_order(od_id);

    if (status) {
        MarketEvent e;
        e.event_type = MarketEventType::OrderDelete;
        e.timestamp = clock_.get_nanos_since_midnight();
        e.sequence = next_sequence_++;
        e.DEvent.order_id = od_id;
        emit(e);
    }
    return status;
}

std::span<const MarketEvent> MatchingEngine::process(const InputEvent &in)
{
    out_.clear();
    
    switch (in.input_type)
    {
    case InputType::EnterOrder: {
        Order od;
        od.price = in.OEvent.price;
        od.quantity = in.OEvent.quantity;
        od.side = in.OEvent.side;
        submit_order(od);
        break;
    }

    case InputType::CancelOrderRequest:
        cancel_order(in.XEvent.order_id);
        break;

    default:
        break;
    }

    return {out_.data(), out_.size()};
}