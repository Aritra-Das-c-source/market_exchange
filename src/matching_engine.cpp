#include <exchange/matching_engine.hpp>

MatchingEngine::MatchingEngine() {
    book_ = OrderBook();
    next_order_id = 1;
    next_trade_id = 1;
}

const OrderBook& MatchingEngine::book() const
{
    return book_;
}

SubmitResult MatchingEngine::submit_order(Order od)
{
    SubmitResult sr;
    od.order_id = sr.order_id = next_order_id++;
    sr.remaining_quantity = od.quantity;

    if (od.side == Side::BUY) {
        while (od.quantity > 0 && !book_.sp_mp.empty() && od.price >= book_.best_ask()) {
            u32 best_ask_price = book_.best_ask();
            auto best_ask_it = book_.sp_mp[best_ask_price].begin();

            Trade t;
            t.trade_id = next_trade_id++;
            t.buy_id = od.order_id;
            t.sell_id = best_ask_it->order_id;
            t.price = best_ask_it->price;
            t.quantity = std::min(od.quantity, best_ask_it->quantity);
            sr.trades.push_back(t);

            od.quantity -= t.quantity;
            best_ask_it->quantity -= t.quantity;
            if (best_ask_it->quantity == 0) book_.remove_order(t.sell_id);
            sr.remaining_quantity = od.quantity;
        }

        if (od.quantity) {
            book_.add_order(od);
        }
    } else {
            while (od.quantity > 0 && !book_.bp_mp.empty() && od.price <= book_.best_bid()) {
            u32 best_bid_price = book_.best_bid();
            auto best_bid_it = book_.bp_mp[best_bid_price].begin();
            
            Trade t;
            t.trade_id = next_trade_id++;
            t.buy_id = best_bid_it->order_id;
            t.sell_id = od.order_id;
            t.price = best_bid_it->price;
            t.quantity = std::min(od.quantity, best_bid_it->quantity);
            sr.trades.push_back(t);

            od.quantity -= t.quantity;
            best_bid_it->quantity -= t.quantity;
            if (best_bid_it->quantity == 0) book_.remove_order(t.buy_id);
            sr.remaining_quantity = od.quantity;
        }

        if (od.quantity) {
            book_.add_order(od);
        }
    }

    return sr;
}

bool MatchingEngine::cancel_order(u64 od_id)
{
    return book_.remove_order(od_id);
}