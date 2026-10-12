#include "matching_engine_tests.hpp"

#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

void ReconstructionBook::add_order(const Order& od)
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

bool ReconstructionBook::remove_order(u64 od_id)
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

const BookSide& ReconstructionBook::bids() const
{
    return bp_mp_;
}

const BookSide& ReconstructionBook::asks() const
{
    return sp_mp_;
}

void ReconstructionBook::apply_market_event(const MarketEvent& e) {
    switch (e.event_type) {
    case MarketEventType::AddOrder: {
        ASSERT_EQ(id_mp_.find(e.AEvent.order_id), id_mp_.end())
            << "ADD references an order ID that already exists: " << e.AEvent.order_id;
        ASSERT_GT(e.AEvent.shares, 0U)
            << "ADD has zero quantity for order " << e.AEvent.order_id;

        Order od{};
        od.order_id = e.AEvent.order_id;
        od.price = e.AEvent.price;
        od.quantity = e.AEvent.shares;
        od.side = e.AEvent.side;
        add_order(od);
        break;
    }

    case MarketEventType::OrderDelete: {
        const bool removed = remove_order(e.DEvent.order_id);
        ASSERT_TRUE(removed)
            << "DELETE references an order that is not resting: " << e.DEvent.order_id;
        break;
    }

    case MarketEventType::OrderExecute: {
        const u64 order_id = e.XEvent.order_id;
        auto order_it = id_mp_.find(order_id);
        ASSERT_NE(order_it, id_mp_.end())
            << "EXECUTE references an order that is not resting: " << order_id;
        ASSERT_GT(e.XEvent.executed_shares, 0U)
            << "EXECUTE has zero quantity for order " << order_id;
        ASSERT_LE(e.XEvent.executed_shares, order_it->second->quantity)
            << "EXECUTE exceeds remaining quantity for order " << order_id;

        order_it->second->quantity -= e.XEvent.executed_shares;
        if (order_it->second->quantity == 0) {
            const bool removed = remove_order(order_id);
            ASSERT_TRUE(removed) << "Fully executed order could not be removed: " << order_id;
        }
        break;
    }

    default:
        FAIL() << "Unknown MarketEventType value: " << static_cast<int>(e.event_type);
        return;
    }
}

static std::string compare_book_side(
    const BookSide& exchange_side,
    const BookSide& reconstructed_side,
    const char* side_name)
{
    if (exchange_side.size() != reconstructed_side.size()) {
        std::ostringstream out;
        out << side_name << " price-level count differs: exchange=" << exchange_side.size()
            << ", reconstructed=" << reconstructed_side.size();
        return out.str();
    }

    auto exchange_level = exchange_side.begin();
    auto reconstructed_level = reconstructed_side.begin();
    std::size_t level_index = 0;

    for (; exchange_level != exchange_side.end(); ++exchange_level, ++reconstructed_level, ++level_index) {
        if (exchange_level->first != reconstructed_level->first) {
            std::ostringstream out;
            out << side_name << " price differs at level index " << level_index
                << ": exchange=" << exchange_level->first
                << ", reconstructed=" << reconstructed_level->first;
            return out.str();
        }

        const auto& exchange_orders = exchange_level->second;
        const auto& reconstructed_orders = reconstructed_level->second;
        if (exchange_orders.size() != reconstructed_orders.size()) {
            std::ostringstream out;
            out << side_name << " order count differs at price " << exchange_level->first
                << ": exchange=" << exchange_orders.size()
                << ", reconstructed=" << reconstructed_orders.size();
            return out.str();
        }

        auto exchange_order = exchange_orders.begin();
        auto reconstructed_order = reconstructed_orders.begin();
        std::size_t order_index = 0;

        for (; exchange_order != exchange_orders.end(); ++exchange_order, ++reconstructed_order, ++order_index) {
            if (exchange_order->order_id != reconstructed_order->order_id ||
                exchange_order->price != reconstructed_order->price ||
                exchange_order->quantity != reconstructed_order->quantity ||
                exchange_order->side != reconstructed_order->side) {
                std::ostringstream out;
                out << side_name << " order differs at price " << exchange_level->first
                    << ", FIFO index " << order_index
                    << ": exchange(order_id=" << exchange_order->order_id
                    << ", price=" << exchange_order->price
                    << ", quantity=" << exchange_order->quantity
                    << ", side=" << static_cast<int>(exchange_order->side)
                    << "), reconstructed(order_id=" << reconstructed_order->order_id
                    << ", price=" << reconstructed_order->price
                    << ", quantity=" << reconstructed_order->quantity
                    << ", side=" << static_cast<int>(reconstructed_order->side) << ')';
                return out.str();
            }
        }
    }

    return {};
}

static std::string book_mismatch(
    const OrderBook& exchange_book,
    const ReconstructionBook& reconstructed_book)
{
    std::string mismatch = compare_book_side(exchange_book.bids(), reconstructed_book.bids(), "bids");
    if (!mismatch.empty()) return mismatch;
    return compare_book_side(exchange_book.asks(), reconstructed_book.asks(), "asks");
}

void expect_book_equality(const OrderBook& book, const ReconstructionBook& localbook) {
    const std::string mismatch = book_mismatch(book, localbook);
    EXPECT_TRUE(mismatch.empty()) << mismatch;
}

class MatchingEngineTest : public ::testing::Test
{
protected:
    MatchingEngine engine;
};

static InputEvent make_add(Side side, u32 price, u32 quantity)
{
    InputEvent in{};
    in.input_type = InputType::EnterOrder;
    in.OEvent.side = side;
    in.OEvent.price = price;
    in.OEvent.quantity = quantity;
    return in;
}

static InputEvent make_cancel(u64 order_id)
{
    InputEvent in{};
    in.input_type = InputType::CancelOrderRequest;
    in.XEvent.order_id = order_id;
    return in;
}

static u64 expect_add_order(
    std::span<const MarketEvent> events,
    Side side,
    u32 price,
    u32 quantity,
    u64 sequence)
{
    EXPECT_EQ(events.size(), 1U);
    if (events.size() != 1U) return 0;

    const auto& event = events.front();
    EXPECT_EQ(event.event_type, MarketEventType::AddOrder);
    EXPECT_EQ(event.sequence, sequence);
    EXPECT_EQ(event.AEvent.side, side);
    EXPECT_EQ(event.AEvent.price, price);
    EXPECT_EQ(event.AEvent.shares, quantity);

    if (event.event_type != MarketEventType::AddOrder) return 0;
    return event.AEvent.order_id;
}

TEST_F(MatchingEngineTest, BuyWithNoAsksRestsOnBook)
{
    const auto events = engine.process(make_add(Side::BUY, 100, 50));

    const u64 order_id = expect_add_order(events, Side::BUY, 100, 50, 1);
    EXPECT_EQ(order_id, 1U);

    ASSERT_TRUE(engine.book().has_bid());
    ASSERT_EQ(engine.book().best_bid(), 100U);

    const auto& level = engine.book().bids().at(100);
    ASSERT_EQ(level.size(), 1U);
    EXPECT_EQ(level.front().order_id, order_id);
    EXPECT_EQ(level.front().quantity, 50U);
}

TEST_F(MatchingEngineTest, SellWithNoBidsRestsOnBook)
{
    const auto events = engine.process(make_add(Side::SELL, 100, 50));

    const u64 order_id = expect_add_order(events, Side::SELL, 100, 50, 1);
    EXPECT_EQ(order_id, 1U);

    ASSERT_TRUE(engine.book().has_ask());
    ASSERT_EQ(engine.book().best_ask(), 100U);

    const auto& level = engine.book().asks().at(100);
    ASSERT_EQ(level.size(), 1U);
    EXPECT_EQ(level.front().order_id, order_id);
    EXPECT_EQ(level.front().quantity, 50U);
}

TEST_F(MatchingEngineTest, ExactMatchProducesOneExecuteEvent)
{
    const auto sell_events = engine.process(make_add(Side::SELL, 100, 100));
    const u64 sell_id = expect_add_order(sell_events, Side::SELL, 100, 100, 1);
    ASSERT_EQ(sell_id, 1U);

    const auto buy_events = engine.process(make_add(Side::BUY, 100, 100));

    ASSERT_EQ(buy_events.size(), 1U);
    const auto& event = buy_events.front();
    EXPECT_EQ(event.event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(event.sequence, 2U);
    EXPECT_EQ(event.XEvent.order_id, sell_id);
    EXPECT_EQ(event.XEvent.executed_shares, 100U);
    EXPECT_EQ(event.XEvent.match_number, 1U);

    EXPECT_FALSE(engine.book().has_bid());
    EXPECT_FALSE(engine.book().has_ask());
}

TEST_F(MatchingEngineTest, BuyPartiallyFillsRestingSell)
{
    const auto sell_events = engine.process(make_add(Side::SELL, 100, 100));
    const u64 sell_id = expect_add_order(sell_events, Side::SELL, 100, 100, 1);

    const auto buy_events = engine.process(make_add(Side::BUY, 100, 40));

    ASSERT_EQ(buy_events.size(), 1U);
    const auto& event = buy_events.front();
    EXPECT_EQ(event.event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(event.sequence, 2U);
    EXPECT_EQ(event.XEvent.order_id, sell_id);
    EXPECT_EQ(event.XEvent.executed_shares, 40U);
    EXPECT_EQ(event.XEvent.match_number, 1U);

    ASSERT_TRUE(engine.book().has_ask());
    ASSERT_EQ(engine.book().best_ask(), 100U);
    EXPECT_EQ(engine.book().asks().at(100).front().quantity, 60U);
    EXPECT_EQ(engine.book().asks().at(100).front().order_id, sell_id);
}

TEST_F(MatchingEngineTest, BuyPartiallyRestsAfterPartialFills)
{
    const auto sell_events = engine.process(make_add(Side::SELL, 100, 40));
    const u64 sell_id = expect_add_order(sell_events, Side::SELL, 100, 40, 1);

    const auto buy_events = engine.process(make_add(Side::BUY, 100, 100));

    ASSERT_EQ(buy_events.size(), 2U);

    const auto& execute = buy_events[0];
    EXPECT_EQ(execute.event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(execute.sequence, 2U);
    EXPECT_EQ(execute.XEvent.order_id, sell_id);
    EXPECT_EQ(execute.XEvent.executed_shares, 40U);
    EXPECT_EQ(execute.XEvent.match_number, 1U);

    const auto& add = buy_events[1];
    const u64 buy_id = add.AEvent.order_id;
    EXPECT_EQ(add.event_type, MarketEventType::AddOrder);
    EXPECT_EQ(add.sequence, 3U);
    EXPECT_EQ(add.AEvent.side, Side::BUY);
    EXPECT_EQ(add.AEvent.price, 100U);
    EXPECT_EQ(add.AEvent.shares, 60U);

    ASSERT_TRUE(engine.book().has_bid());
    ASSERT_EQ(engine.book().best_bid(), 100U);

    const auto& level = engine.book().bids().at(100);
    ASSERT_EQ(level.size(), 1U);
    EXPECT_EQ(level.front().order_id, buy_id);
    EXPECT_EQ(level.front().quantity, 60U);
}

TEST_F(MatchingEngineTest, BuyConsumesMultipleAskLevels)
{
    const auto sell1_events = engine.process(make_add(Side::SELL, 100, 100));
    const u64 sell1_id = expect_add_order(sell1_events, Side::SELL, 100, 100, 1);

    const auto sell2_events = engine.process(make_add(Side::SELL, 101, 200));
    const u64 sell2_id = expect_add_order(sell2_events, Side::SELL, 101, 200, 2);

    const auto sell3_events = engine.process(make_add(Side::SELL, 102, 300));
    const u64 sell3_id = expect_add_order(sell3_events, Side::SELL, 102, 300, 3);

    const auto buy_events = engine.process(make_add(Side::BUY, 102, 400));

    ASSERT_EQ(buy_events.size(), 3U);

    EXPECT_EQ(buy_events[0].event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(buy_events[0].sequence, 4U);
    EXPECT_EQ(buy_events[0].XEvent.order_id, sell1_id);
    EXPECT_EQ(buy_events[0].XEvent.executed_shares, 100U);
    EXPECT_EQ(buy_events[0].XEvent.match_number, 1U);

    EXPECT_EQ(buy_events[1].event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(buy_events[1].sequence, 5U);
    EXPECT_EQ(buy_events[1].XEvent.order_id, sell2_id);
    EXPECT_EQ(buy_events[1].XEvent.executed_shares, 200U);
    EXPECT_EQ(buy_events[1].XEvent.match_number, 2U);

    EXPECT_EQ(buy_events[2].event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(buy_events[2].sequence, 6U);
    EXPECT_EQ(buy_events[2].XEvent.order_id, sell3_id);
    EXPECT_EQ(buy_events[2].XEvent.executed_shares, 100U);
    EXPECT_EQ(buy_events[2].XEvent.match_number, 3U);

    ASSERT_TRUE(engine.book().has_ask());
    EXPECT_EQ(engine.book().best_ask(), 102U);
    EXPECT_EQ(engine.book().asks().at(102).front().quantity, 200U);
}

TEST_F(MatchingEngineTest, SellConsumesMultipleBidLevels)
{
    const auto buy1_events = engine.process(make_add(Side::BUY, 102, 100));
    const u64 buy1_id = expect_add_order(buy1_events, Side::BUY, 102, 100, 1);

    const auto buy2_events = engine.process(make_add(Side::BUY, 101, 200));
    const u64 buy2_id = expect_add_order(buy2_events, Side::BUY, 101, 200, 2);

    const auto buy3_events = engine.process(make_add(Side::BUY, 100, 300));
    const u64 buy3_id = expect_add_order(buy3_events, Side::BUY, 100, 300, 3);

    const auto sell_events = engine.process(make_add(Side::SELL, 100, 400));

    ASSERT_EQ(sell_events.size(), 3U);

    EXPECT_EQ(sell_events[0].event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(sell_events[0].sequence, 4U);
    EXPECT_EQ(sell_events[0].XEvent.order_id, buy1_id);
    EXPECT_EQ(sell_events[0].XEvent.executed_shares, 100U);
    EXPECT_EQ(sell_events[0].XEvent.match_number, 1U);

    EXPECT_EQ(sell_events[1].event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(sell_events[1].sequence, 5U);
    EXPECT_EQ(sell_events[1].XEvent.order_id, buy2_id);
    EXPECT_EQ(sell_events[1].XEvent.executed_shares, 200U);
    EXPECT_EQ(sell_events[1].XEvent.match_number, 2U);

    EXPECT_EQ(sell_events[2].event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(sell_events[2].sequence, 6U);
    EXPECT_EQ(sell_events[2].XEvent.order_id, buy3_id);
    EXPECT_EQ(sell_events[2].XEvent.executed_shares, 100U);
    EXPECT_EQ(sell_events[2].XEvent.match_number, 3U);

    ASSERT_TRUE(engine.book().has_bid());
    EXPECT_EQ(engine.book().best_bid(), 100U);
    EXPECT_EQ(engine.book().bids().at(100).front().quantity, 200U);
}

TEST_F(MatchingEngineTest, NonCrossingOrdersDoNotTrade)
{
    const auto sell_events = engine.process(make_add(Side::SELL, 101, 100));
    const u64 sell_id = expect_add_order(sell_events, Side::SELL, 101, 100, 1);

    const auto buy_events = engine.process(make_add(Side::BUY, 100, 100));
    const u64 buy_id = expect_add_order(buy_events, Side::BUY, 100, 100, 2);

    ASSERT_EQ(sell_id, 1U);
    ASSERT_EQ(buy_id, 2U);

    ASSERT_TRUE(engine.book().has_bid());
    ASSERT_TRUE(engine.book().has_ask());
    EXPECT_EQ(engine.book().best_bid(), 100U);
    EXPECT_EQ(engine.book().best_ask(), 101U);
}

TEST_F(MatchingEngineTest, TradeUsesRestingOrderPriceForIncomingBuy)
{
    const auto sell_events = engine.process(make_add(Side::SELL, 100, 100));
    const u64 sell_id = expect_add_order(sell_events, Side::SELL, 100, 100, 1);

    const auto buy_events = engine.process(make_add(Side::BUY, 105, 100));

    ASSERT_EQ(buy_events.size(), 1U);
    const auto& event = buy_events.front();
    EXPECT_EQ(event.event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(event.XEvent.order_id, sell_id);
    EXPECT_EQ(event.XEvent.executed_shares, 100U);
}

TEST_F(MatchingEngineTest, TradeUsesRestingOrderPriceForIncomingSell)
{
    const auto buy_events = engine.process(make_add(Side::BUY, 105, 100));
    const u64 buy_id = expect_add_order(buy_events, Side::BUY, 105, 100, 1);

    const auto sell_events = engine.process(make_add(Side::SELL, 100, 100));

    ASSERT_EQ(sell_events.size(), 1U);
    const auto& event = sell_events.front();
    EXPECT_EQ(event.event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(event.XEvent.order_id, buy_id);
    EXPECT_EQ(event.XEvent.executed_shares, 100U);
}

TEST_F(MatchingEngineTest, BuyRespectsTimePriorityAtSamePrice)
{
    const auto sell1_events = engine.process(make_add(Side::SELL, 100, 50));
    const u64 sell1_id = expect_add_order(sell1_events, Side::SELL, 100, 50, 1);

    const auto sell2_events = engine.process(make_add(Side::SELL, 100, 50));
    const u64 sell2_id = expect_add_order(sell2_events, Side::SELL, 100, 50, 2);

    const auto buy_events = engine.process(make_add(Side::BUY, 100, 60));

    ASSERT_EQ(buy_events.size(), 2U);

    EXPECT_EQ(buy_events[0].XEvent.order_id, sell1_id);
    EXPECT_EQ(buy_events[0].XEvent.executed_shares, 50U);
    EXPECT_EQ(buy_events[0].XEvent.match_number, 1U);

    EXPECT_EQ(buy_events[1].XEvent.order_id, sell2_id);
    EXPECT_EQ(buy_events[1].XEvent.executed_shares, 10U);
    EXPECT_EQ(buy_events[1].XEvent.match_number, 2U);

    ASSERT_TRUE(engine.book().has_ask());
    ASSERT_EQ(engine.book().asks().at(100).size(), 1U);
    EXPECT_EQ(engine.book().asks().at(100).front().order_id, sell2_id);
    EXPECT_EQ(engine.book().asks().at(100).front().quantity, 40U);
}

TEST_F(MatchingEngineTest, SellRespectsTimePriorityAtSamePrice)
{
    const auto buy1_events = engine.process(make_add(Side::BUY, 100, 50));
    const u64 buy1_id = expect_add_order(buy1_events, Side::BUY, 100, 50, 1);

    const auto buy2_events = engine.process(make_add(Side::BUY, 100, 50));
    const u64 buy2_id = expect_add_order(buy2_events, Side::BUY, 100, 50, 2);

    const auto sell_events = engine.process(make_add(Side::SELL, 100, 60));

    ASSERT_EQ(sell_events.size(), 2U);

    EXPECT_EQ(sell_events[0].XEvent.order_id, buy1_id);
    EXPECT_EQ(sell_events[0].XEvent.executed_shares, 50U);
    EXPECT_EQ(sell_events[0].XEvent.match_number, 1U);

    EXPECT_EQ(sell_events[1].XEvent.order_id, buy2_id);
    EXPECT_EQ(sell_events[1].XEvent.executed_shares, 10U);
    EXPECT_EQ(sell_events[1].XEvent.match_number, 2U);

    ASSERT_TRUE(engine.book().has_bid());
    ASSERT_EQ(engine.book().bids().at(100).size(), 1U);
    EXPECT_EQ(engine.book().bids().at(100).front().order_id, buy2_id);
    EXPECT_EQ(engine.book().bids().at(100).front().quantity, 40U);
}

TEST_F(MatchingEngineTest, OrderIdsAndMatchIdsIncreaseMonotonically)
{
    const auto buy1_events = engine.process(make_add(Side::BUY, 100, 10));
    const u64 buy1_id = expect_add_order(buy1_events, Side::BUY, 100, 10, 1);

    const auto sell1_events = engine.process(make_add(Side::SELL, 100, 10));
    ASSERT_EQ(sell1_events.size(), 1U);
    EXPECT_EQ(sell1_events.front().event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(sell1_events.front().XEvent.order_id, buy1_id);
    EXPECT_EQ(sell1_events.front().XEvent.match_number, 1U);
    EXPECT_EQ(sell1_events.front().sequence, 2U);

    const auto buy2_events = engine.process(make_add(Side::BUY, 99, 10));
    const u64 buy2_id = expect_add_order(buy2_events, Side::BUY, 99, 10, 3);

    EXPECT_EQ(buy1_id, 1U);
    EXPECT_EQ(buy2_id, 3U);
}

TEST_F(MatchingEngineTest, FullyFilledIncomingOrderDoesNotRest)
{
    const auto sell_events = engine.process(make_add(Side::SELL, 100, 10));
    expect_add_order(sell_events, Side::SELL, 100, 10, 1);

    const auto buy_events = engine.process(make_add(Side::BUY, 100, 10));

    ASSERT_EQ(buy_events.size(), 1U);
    EXPECT_EQ(buy_events.front().event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(buy_events.front().XEvent.executed_shares, 10U);

    EXPECT_FALSE(engine.book().has_bid());
    EXPECT_FALSE(engine.book().has_ask());
}

TEST_F(MatchingEngineTest, CannotCancelFullyFilledOrder)
{
    const auto sell_events = engine.process(make_add(Side::SELL, 100, 10));
    const u64 sell_id = expect_add_order(sell_events, Side::SELL, 100, 10, 1);

    const auto buy_events = engine.process(make_add(Side::BUY, 100, 10));
    ASSERT_EQ(buy_events.size(), 1U);

    const auto cancel_sell = engine.process(make_cancel(sell_id));
    EXPECT_TRUE(cancel_sell.empty());

    // The incoming order was fully consumed and never became resting state,
    // so there is no market-data order ID for it to cancel either.
    const auto cancel_unknown = engine.process(make_cancel(2));
    EXPECT_TRUE(cancel_unknown.empty());
}

TEST_F(MatchingEngineTest, CancelRestingOrderProducesDeleteEvent)
{
    const auto add_events = engine.process(make_add(Side::BUY, 100, 25));
    const u64 order_id = expect_add_order(add_events, Side::BUY, 100, 25, 1);

    const auto cancel_events = engine.process(make_cancel(order_id));

    ASSERT_EQ(cancel_events.size(), 1U);
    const auto& event = cancel_events.front();
    EXPECT_EQ(event.event_type, MarketEventType::OrderDelete);
    EXPECT_EQ(event.sequence, 2U);
    EXPECT_EQ(event.DEvent.order_id, order_id);

    EXPECT_FALSE(engine.book().has_bid());

    const auto second_cancel = engine.process(make_cancel(order_id));
    EXPECT_TRUE(second_cancel.empty());
}

TEST_F(MatchingEngineTest, CancelPartiallyFilledRestingOrderProducesDeleteEvent)
{
    const auto sell_events = engine.process(make_add(Side::SELL, 100, 100));
    const u64 sell_id = expect_add_order(sell_events, Side::SELL, 100, 100, 1);

    const auto buy_events = engine.process(make_add(Side::BUY, 100, 40));
    ASSERT_EQ(buy_events.size(), 1U);

    const auto cancel_events = engine.process(make_cancel(sell_id));

    ASSERT_EQ(cancel_events.size(), 1U);
    EXPECT_EQ(cancel_events.front().event_type, MarketEventType::OrderDelete);
    EXPECT_EQ(cancel_events.front().sequence, 3U);
    EXPECT_EQ(cancel_events.front().DEvent.order_id, sell_id);
    EXPECT_FALSE(engine.book().has_ask());
}

TEST_F(MatchingEngineTest, OnlyTheRemainderGetsInserted)
{
    const auto sell_events = engine.process(make_add(Side::SELL, 100, 40));
    expect_add_order(sell_events, Side::SELL, 100, 40, 1);

    const auto buy_events = engine.process(make_add(Side::BUY, 100, 100));

    ASSERT_EQ(buy_events.size(), 2U);
    ASSERT_EQ(buy_events[0].event_type, MarketEventType::OrderExecute);
    ASSERT_EQ(buy_events[0].XEvent.executed_shares, 40U);

    ASSERT_EQ(buy_events[1].event_type, MarketEventType::AddOrder);
    const u64 buy_id = buy_events[1].AEvent.order_id;

    ASSERT_TRUE(engine.book().has_bid());
    const auto& bids = engine.book().bids().at(100);

    ASSERT_EQ(bids.size(), 1U);
    EXPECT_EQ(bids.front().order_id, buy_id);
    EXPECT_EQ(bids.front().quantity, 60U);
}

TEST_F(MatchingEngineTest, SequenceNumbersIncreaseAcrossEvents)
{
    const auto first = engine.process(make_add(Side::BUY, 100, 10));
    ASSERT_EQ(first.size(), 1U);
    EXPECT_EQ(first.front().sequence, 1U);

    const u64 order_id = first.front().AEvent.order_id;

    const auto second = engine.process(make_add(Side::BUY, 101, 20));
    ASSERT_EQ(second.size(), 1U);
    EXPECT_EQ(second.front().sequence, 2U);

    const auto third = engine.process(make_cancel(order_id));
    ASSERT_EQ(third.size(), 1U);
    EXPECT_EQ(third.front().sequence, 3U);
}

TEST_F(MatchingEngineTest, FailedCancelDoesNotAdvanceSequence)
{
    const auto first = engine.process(make_add(Side::BUY, 100, 10));
    ASSERT_EQ(first.size(), 1U);
    EXPECT_EQ(first.front().sequence, 1U);

    const auto failed_cancel = engine.process(make_cancel(999999));
    EXPECT_TRUE(failed_cancel.empty());

    const auto second = engine.process(make_add(Side::SELL, 200, 10));
    ASSERT_EQ(second.size(), 1U);
    EXPECT_EQ(second.front().sequence, 2U);
}

TEST_F(MatchingEngineTest, EventsAreReturnedInProcessingOrder)
{
    const auto sell1 = engine.process(make_add(Side::SELL, 100, 10));
    const auto sell1_id = sell1.front().AEvent.order_id;
    (void)sell1_id;

    const auto sell2 = engine.process(make_add(Side::SELL, 101, 20));
    const auto sell2_id = sell2.front().AEvent.order_id;

    const auto buy = engine.process(make_add(Side::BUY, 101, 30));

    ASSERT_EQ(buy.size(), 2U);

    EXPECT_EQ(buy[0].sequence, 3U);
    EXPECT_EQ(buy[0].event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(buy[0].XEvent.order_id, sell1_id);
    EXPECT_EQ(buy[0].XEvent.executed_shares, 10U);

    EXPECT_EQ(buy[1].sequence, 4U);
    EXPECT_EQ(buy[1].event_type, MarketEventType::OrderExecute);
    EXPECT_EQ(buy[1].XEvent.order_id, sell2_id);
    EXPECT_EQ(buy[1].XEvent.executed_shares, 20U);
}

TEST_F(MatchingEngineTest, ReconstructionTest)
{
    ReconstructionBook localbook;
    InputEvent in = make_add(Side::BUY, 100, 10);
    for (const MarketEvent& e : engine.process(in)) {
        localbook.apply_market_event(e);
    }
    expect_book_equality(engine.book(), localbook);

    in = make_add(Side::BUY, 101, 15);
    for (const MarketEvent& e : engine.process(in)) {
        localbook.apply_market_event(e);
    }
    expect_book_equality(engine.book(), localbook);

    in = make_add(Side::BUY, 102, 10);
    for (const MarketEvent& e : engine.process(in)) {
        localbook.apply_market_event(e);
    }
    expect_book_equality(engine.book(), localbook);

    in = make_add(Side::SELL, 101, 20);
    for (const MarketEvent& e : engine.process(in)) {
        localbook.apply_market_event(e);
    }
    expect_book_equality(engine.book(), localbook);

    in = make_cancel(3);
    for (const MarketEvent& e : engine.process(in)) {
        localbook.apply_market_event(e);
    }
    expect_book_equality(engine.book(), localbook);

    in = make_cancel(1);
    for (const MarketEvent& e : engine.process(in)) {
        localbook.apply_market_event(e);
    }
    expect_book_equality(engine.book(), localbook);
}

static std::vector<u64> collect_resting_order_ids(const OrderBook& book)
{
    std::vector<u64> ids;
    for (const auto& level : book.bids()) {
        for (const Order& order : level.second) ids.push_back(order.order_id);
    }
    for (const auto& level : book.asks()) {
        for (const Order& order : level.second) ids.push_back(order.order_id);
    }
    return ids;
}

static std::string describe_input(const InputEvent& input)
{
    std::ostringstream out;
    if (input.input_type == InputType::EnterOrder) {
        out << "EnterOrder(side="
            << (input.OEvent.side == Side::BUY ? "BUY" : "SELL")
            << ", price=" << input.OEvent.price
            << ", quantity=" << input.OEvent.quantity << ')';
    } else if (input.input_type == InputType::CancelOrderRequest) {
        out << "CancelOrder(order_id=" << input.XEvent.order_id << ')';
    } else {
        out << "UnknownInputType(" << static_cast<int>(input.input_type) << ')';
    }
    return out.str();
}

TEST_F(MatchingEngineTest, RandomizedEventStreamReconstructsBook)
{
    constexpr u32 seed = 0x5EED1234;
    constexpr std::size_t operation_count = 10'000;

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> action_dist(0, 99);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<u32> price_dist(90, 110);
    std::uniform_int_distribution<u32> quantity_dist(1, 100);

    ReconstructionBook reconstructed_book;
    u64 expected_sequence = 1;
    std::size_t add_event_count = 0;
    std::size_t execute_event_count = 0;
    std::size_t delete_event_count = 0;

    for (std::size_t operation_index = 0; operation_index < operation_count; ++operation_index) {
        InputEvent input{};
        const int action = action_dist(rng);

        if (action < 70) {
            const Side side = side_dist(rng) == 0 ? Side::BUY : Side::SELL;
            input = make_add(side, price_dist(rng), quantity_dist(rng));
        } else if (action < 90) {
            const std::vector<u64> resting_ids = collect_resting_order_ids(engine.book());
            if (resting_ids.empty()) {
                input = make_cancel(std::numeric_limits<u64>::max());
            } else {
                std::uniform_int_distribution<std::size_t> id_index_dist(0, resting_ids.size() - 1);
                input = make_cancel(resting_ids[id_index_dist(rng)]);
            }
        } else {
            input = make_cancel(std::numeric_limits<u64>::max());
        }

        const auto events = engine.process(input);
        for (const MarketEvent& event : events) {
            ASSERT_EQ(event.sequence, expected_sequence)
                << "seed=" << seed
                << ", operation=" << operation_index
                << ", input=" << describe_input(input);
            ++expected_sequence;

            switch (event.event_type) {
            case MarketEventType::AddOrder:
                ++add_event_count;
                break;
            case MarketEventType::OrderExecute:
                ++execute_event_count;
                break;
            case MarketEventType::OrderDelete:
                ++delete_event_count;
                break;
            default:
                FAIL() << "Unexpected market event type at operation " << operation_index;
                break;
            }

            reconstructed_book.apply_market_event(event);
        }

        const std::string mismatch = book_mismatch(engine.book(), reconstructed_book);
        ASSERT_TRUE(mismatch.empty())
            << "Book reconstruction diverged. seed=" << seed
            << ", operation=" << operation_index
            << ", input=" << describe_input(input)
            << ", mismatch=" << mismatch;
    }

    EXPECT_GT(add_event_count, 0U) << "Seed did not produce any ADD events";
    EXPECT_GT(execute_event_count, 0U) << "Seed did not produce any EXECUTE events";
    EXPECT_GT(delete_event_count, 0U) << "Seed did not produce any DELETE events";
}