#include <gtest/gtest.h>

#include <exchange/matching_engine.hpp>

class MatchingEngineTest : public ::testing::Test
{
protected:
    MatchingEngine engine;
};

TEST_F(MatchingEngineTest, BuyWithNoAsksRestsOnBook)
{
    const auto result = engine.submit_order({0, Side::BUY, 100, 50});

    EXPECT_EQ(result.order_id, 1);
    EXPECT_EQ(result.remaining_quantity, 50U);
    EXPECT_TRUE(result.trades.empty());

    ASSERT_TRUE(engine.book().has_bid());
    ASSERT_EQ(engine.book().best_bid(), 100U);

    const auto& level = engine.book().bids().at(100);
    ASSERT_EQ(level.size(), 1U);
    EXPECT_EQ(level.front().order_id, result.order_id);
    EXPECT_EQ(level.front().quantity, 50U);
}

TEST_F(MatchingEngineTest, SellWithNoBidsRestsOnBook)
{
    const auto result = engine.submit_order({0, Side::SELL, 100, 50});

    EXPECT_EQ(result.order_id, 1);
    EXPECT_EQ(result.remaining_quantity, 50U);
    EXPECT_TRUE(result.trades.empty());

    ASSERT_TRUE(engine.book().has_ask());
    ASSERT_EQ(engine.book().best_ask(), 100U);

    const auto& level = engine.book().asks().at(100);
    ASSERT_EQ(level.size(), 1U);
    EXPECT_EQ(level.front().order_id, result.order_id);
    EXPECT_EQ(level.front().quantity, 50U);
}

TEST_F(MatchingEngineTest, ExactMatchProducesOneTrade)
{
    const auto sell = engine.submit_order({0, Side::SELL, 100, 100});
    const auto buy = engine.submit_order({0, Side::BUY, 100, 100});

    ASSERT_EQ(sell.order_id, 1U);
    ASSERT_EQ(buy.order_id, 2U);

    ASSERT_EQ(buy.trades.size(), 1U);

    const auto& trade = buy.trades.front();
    EXPECT_EQ(trade.trade_id, 1U);
    EXPECT_EQ(trade.buy_id, buy.order_id);
    EXPECT_EQ(trade.sell_id, sell.order_id);
    EXPECT_EQ(trade.price, 100U);
    EXPECT_EQ(trade.quantity, 100U);

    EXPECT_EQ(buy.remaining_quantity, 0U);
    EXPECT_FALSE(engine.book().has_bid());
    EXPECT_FALSE(engine.book().has_ask());
}

TEST_F(MatchingEngineTest, BuyPartiallyFillsRestingSell)
{
    const auto sell = engine.submit_order({0, Side::SELL, 100, 100});
    const auto buy = engine.submit_order({0, Side::BUY, 100, 40});

    ASSERT_EQ(buy.trades.size(), 1U);

    const auto& trade = buy.trades.front();
    EXPECT_EQ(trade.buy_id, buy.order_id);
    EXPECT_EQ(trade.sell_id, sell.order_id);
    EXPECT_EQ(trade.price, 100U);
    EXPECT_EQ(trade.quantity, 40U);

    EXPECT_EQ(buy.remaining_quantity, 0U);

    ASSERT_TRUE(engine.book().has_ask());
    ASSERT_EQ(engine.book().best_ask(), 100U);
    EXPECT_EQ(engine.book().asks().at(100).front().quantity, 60U);
    EXPECT_EQ(engine.book().asks().at(100).front().order_id, sell.order_id);
}

TEST_F(MatchingEngineTest, BuyPartiallyRestsAfterPartialFills)
{
    const auto sell = engine.submit_order({0, Side::SELL, 100, 40});
    const auto buy = engine.submit_order({0, Side::BUY, 100, 100});

    ASSERT_EQ(buy.trades.size(), 1U);
    EXPECT_EQ(buy.trades.front().quantity, 40U);
    EXPECT_EQ(buy.remaining_quantity, 60U);

    ASSERT_TRUE(engine.book().has_bid());
    ASSERT_EQ(engine.book().best_bid(), 100U);

    const auto& level = engine.book().bids().at(100);
    ASSERT_EQ(level.size(), 1U);
    EXPECT_EQ(level.front().order_id, buy.order_id);
    EXPECT_EQ(level.front().quantity, 60U);

    (void)sell;
}

TEST_F(MatchingEngineTest, BuyConsumesMultipleAskLevels)
{
    const auto sell1 = engine.submit_order({0, Side::SELL, 100, 100});
    const auto sell2 = engine.submit_order({0, Side::SELL, 101, 200});
    const auto sell3 = engine.submit_order({0, Side::SELL, 102, 300});

    const auto buy = engine.submit_order({0, Side::BUY, 102, 400});

    ASSERT_EQ(buy.trades.size(), 3U);

    EXPECT_EQ(buy.trades[0].sell_id, sell1.order_id);
    EXPECT_EQ(buy.trades[0].price, 100U);
    EXPECT_EQ(buy.trades[0].quantity, 100U);

    EXPECT_EQ(buy.trades[1].sell_id, sell2.order_id);
    EXPECT_EQ(buy.trades[1].price, 101U);
    EXPECT_EQ(buy.trades[1].quantity, 200U);

    EXPECT_EQ(buy.trades[2].sell_id, sell3.order_id);
    EXPECT_EQ(buy.trades[2].price, 102U);
    EXPECT_EQ(buy.trades[2].quantity, 100U);

    EXPECT_EQ(buy.remaining_quantity, 0U);

    ASSERT_TRUE(engine.book().has_ask());
    EXPECT_EQ(engine.book().best_ask(), 102U);
    EXPECT_EQ(engine.book().asks().at(102).front().quantity, 200U);
}

TEST_F(MatchingEngineTest, SellConsumesMultipleBidLevels)
{
    const auto buy1 = engine.submit_order({0, Side::BUY, 102, 100});
    const auto buy2 = engine.submit_order({0, Side::BUY, 101, 200});
    const auto buy3 = engine.submit_order({0, Side::BUY, 100, 300});

    const auto sell = engine.submit_order({0, Side::SELL, 100, 400});

    ASSERT_EQ(sell.trades.size(), 3U);

    EXPECT_EQ(sell.trades[0].buy_id, buy1.order_id);
    EXPECT_EQ(sell.trades[0].price, 102U);
    EXPECT_EQ(sell.trades[0].quantity, 100U);

    EXPECT_EQ(sell.trades[1].buy_id, buy2.order_id);
    EXPECT_EQ(sell.trades[1].price, 101U);
    EXPECT_EQ(sell.trades[1].quantity, 200U);

    EXPECT_EQ(sell.trades[2].buy_id, buy3.order_id);
    EXPECT_EQ(sell.trades[2].price, 100U);
    EXPECT_EQ(sell.trades[2].quantity, 100U);

    EXPECT_EQ(sell.remaining_quantity, 0U);

    ASSERT_TRUE(engine.book().has_bid());
    EXPECT_EQ(engine.book().best_bid(), 100U);
    EXPECT_EQ(engine.book().bids().at(100).front().quantity, 200U);
}

TEST_F(MatchingEngineTest, NonCrossingOrdersDoNotTrade)
{
    const auto sell = engine.submit_order({0, Side::SELL, 101, 100});
    const auto buy = engine.submit_order({0, Side::BUY, 100, 100});

    EXPECT_TRUE(sell.trades.empty());
    EXPECT_TRUE(buy.trades.empty());

    EXPECT_TRUE(engine.book().has_bid());
    EXPECT_TRUE(engine.book().has_ask());
    EXPECT_EQ(engine.book().best_bid(), 100U);
    EXPECT_EQ(engine.book().best_ask(), 101U);
}

TEST_F(MatchingEngineTest, TradeUsesRestingOrderPriceForIncomingBuy)
{
    const auto sell = engine.submit_order({0, Side::SELL, 100, 100});
    const auto buy = engine.submit_order({0, Side::BUY, 105, 100});

    ASSERT_EQ(buy.trades.size(), 1U);
    EXPECT_EQ(buy.trades.front().price, 100U);
    EXPECT_NE(buy.trades.front().price, 105U);

    (void)sell;
}

TEST_F(MatchingEngineTest, TradeUsesRestingOrderPriceForIncomingSell)
{
    const auto buy = engine.submit_order({0, Side::BUY, 105, 100});
    const auto sell = engine.submit_order({0, Side::SELL, 100, 100});

    ASSERT_EQ(sell.trades.size(), 1U);
    EXPECT_EQ(sell.trades.front().price, 105U);
    EXPECT_NE(sell.trades.front().price, 100U);

    (void)buy;
}

TEST_F(MatchingEngineTest, BuyRespectsTimePriorityAtSamePrice)
{
    const auto sell1 = engine.submit_order({0, Side::SELL, 100, 50});
    const auto sell2 = engine.submit_order({0, Side::SELL, 100, 50});
    const auto buy = engine.submit_order({0, Side::BUY, 100, 60});

    ASSERT_EQ(buy.trades.size(), 2U);

    EXPECT_EQ(buy.trades[0].sell_id, sell1.order_id);
    EXPECT_EQ(buy.trades[0].quantity, 50U);

    EXPECT_EQ(buy.trades[1].sell_id, sell2.order_id);
    EXPECT_EQ(buy.trades[1].quantity, 10U);

    ASSERT_TRUE(engine.book().has_ask());
    ASSERT_EQ(engine.book().asks().at(100).size(), 1U);
    EXPECT_EQ(engine.book().asks().at(100).front().order_id, sell2.order_id);
    EXPECT_EQ(engine.book().asks().at(100).front().quantity, 40U);
}

TEST_F(MatchingEngineTest, SellRespectsTimePriorityAtSamePrice)
{
    const auto buy1 = engine.submit_order({0, Side::BUY, 100, 50});
    const auto buy2 = engine.submit_order({0, Side::BUY, 100, 50});
    const auto sell = engine.submit_order({0, Side::SELL, 100, 60});

    ASSERT_EQ(sell.trades.size(), 2U);

    EXPECT_EQ(sell.trades[0].buy_id, buy1.order_id);
    EXPECT_EQ(sell.trades[0].quantity, 50U);

    EXPECT_EQ(sell.trades[1].buy_id, buy2.order_id);
    EXPECT_EQ(sell.trades[1].quantity, 10U);

    ASSERT_TRUE(engine.book().has_bid());
    ASSERT_EQ(engine.book().bids().at(100).size(), 1U);
    EXPECT_EQ(engine.book().bids().at(100).front().order_id, buy2.order_id);
    EXPECT_EQ(engine.book().bids().at(100).front().quantity, 40U);
}

TEST_F(MatchingEngineTest, OrderIdsAndTradeIdsIncreaseMonotonically)
{
    const auto buy1 = engine.submit_order({0, Side::BUY, 100, 10});
    const auto sell1 = engine.submit_order({0, Side::SELL, 100, 10});
    const auto buy2 = engine.submit_order({0, Side::BUY, 99, 10});

    ASSERT_EQ(buy1.order_id, 1U);
    ASSERT_EQ(sell1.order_id, 2U);
    ASSERT_EQ(buy2.order_id, 3U);

    ASSERT_EQ(sell1.trades.size(), 1U);
    EXPECT_EQ(sell1.trades.front().trade_id, 1U);
}

TEST_F(MatchingEngineTest, FullyFilledIncomingOrderDoesNotRest)
{
    const auto sell = engine.submit_order({0, Side::SELL, 100, 10});
    const auto buy = engine.submit_order({0, Side::BUY, 100, 10});

    EXPECT_EQ(buy.remaining_quantity, 0U);
    EXPECT_FALSE(engine.book().has_bid());
    EXPECT_FALSE(engine.book().has_ask());

    (void)sell;
}

TEST_F(MatchingEngineTest, CannotCancelFullyFilledOrder)
{
    const auto sell = engine.submit_order({0, Side::SELL, 100, 10});
    const auto buy = engine.submit_order({0, Side::BUY, 100, 10});

    EXPECT_FALSE(engine.cancel_order(buy.order_id));
    EXPECT_FALSE(engine.cancel_order(sell.order_id));
}

TEST_F(MatchingEngineTest, CancelRestingOrder)
{
    const auto buy = engine.submit_order({0, Side::BUY, 100, 25});

    EXPECT_TRUE(engine.cancel_order(buy.order_id));
    EXPECT_FALSE(engine.book().has_bid());
    EXPECT_FALSE(engine.cancel_order(buy.order_id));
}

TEST_F(MatchingEngineTest, CancelPartiallyFilledRestingOrder)
{
    const auto sell = engine.submit_order({0, Side::SELL, 100, 100});
    const auto buy = engine.submit_order({0, Side::BUY, 100, 40});

    EXPECT_EQ(buy.remaining_quantity, 0U);
    EXPECT_TRUE(engine.cancel_order(sell.order_id));
    EXPECT_FALSE(engine.book().has_ask());
}

TEST_F(MatchingEngineTest, OnlyTheRemainderGetsInserted)
{
    const auto sell = engine.submit_order({0, Side::SELL, 100, 40});
    const auto buy = engine.submit_order({0, Side::BUY, 100, 100});

    ASSERT_TRUE(engine.book().has_bid());
    const auto& bids = engine.book().bids().at(100);

    ASSERT_EQ(bids.size(), 1U);
    EXPECT_EQ(bids.front().order_id, buy.order_id);
    EXPECT_EQ(bids.front().quantity, 60U);

    (void)sell;
}

TEST_F(MatchingEngineTest, TradeIdsAreUniqueAcrossMultipleTrades)
{
    engine.submit_order({0, Side::SELL, 100, 10});
    engine.submit_order({0, Side::SELL, 101, 20});

    const auto buy = engine.submit_order({0, Side::BUY, 101, 30});

    ASSERT_EQ(buy.trades.size(), 2U);
    EXPECT_NE(buy.trades[0].trade_id, buy.trades[1].trade_id);
    EXPECT_EQ(buy.trades[0].trade_id, 1U);
    EXPECT_EQ(buy.trades[1].trade_id, 2U);
}
