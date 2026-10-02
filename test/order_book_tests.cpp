#include <gtest/gtest.h>

#include <exchange/order_book.hpp>

TEST(OrderBookTest, StartsEmpty)
{
    OrderBook book;

    EXPECT_FALSE(book.has_bid());
    EXPECT_FALSE(book.has_ask());
}

TEST(OrderBookTest, SingleBuyInsertion)
{
    OrderBook book;

    book.add_order({0, Side::BUY, 100, 10});

    EXPECT_TRUE(book.has_bid());
    EXPECT_FALSE(book.has_ask());
    EXPECT_EQ(book.best_bid(), 100);
    EXPECT_EQ(book.bids().at(100).front().quantity, 10);
}

TEST(OrderBookTest, SingleSellInsertion)
{
    OrderBook book;

    book.add_order({0, Side::SELL, 100, 10});

    EXPECT_FALSE(book.has_bid());
    EXPECT_TRUE(book.has_ask());
    EXPECT_EQ(book.best_ask(), 100);
    EXPECT_EQ(book.asks().at(100).front().quantity, 10);
}

TEST(OrderBookTest, MultipleBuyInsertion)
{
    OrderBook book;

    book.add_order({0, Side::BUY, 100, 10});
    book.add_order({1, Side::BUY, 50, 15});
    book.add_order({2, Side::BUY, 200, 5});

    EXPECT_EQ(book.best_bid(), 200);
}

TEST(OrderBookTest, MultipleSellInsertion)
{
    OrderBook book;

    book.add_order({0, Side::SELL, 100, 10});
    book.add_order({1, Side::SELL, 50, 15});
    book.add_order({2, Side::SELL, 200, 5});

    EXPECT_EQ(book.best_ask(), 50);
}

TEST(OrderBookTest, BidsAndAsksCanExistTogether)
{
    OrderBook book;

    book.add_order({0, Side::BUY, 100, 10});
    book.add_order({1, Side::SELL, 101, 20});

    EXPECT_TRUE(book.has_bid());
    EXPECT_TRUE(book.has_ask());
    EXPECT_EQ(book.best_bid(), 100);
    EXPECT_EQ(book.best_ask(), 101);
}

TEST(OrderBookTest, BookDoesNotPerformMatching)
{
    OrderBook book;

    // OrderBook is only a state container. It does not care that these prices cross.
    book.add_order({0, Side::BUY, 100, 10});
    book.add_order({1, Side::SELL, 99, 20});

    EXPECT_EQ(book.best_bid(), 100);
    EXPECT_EQ(book.best_ask(), 99);
}

TEST(OrderBookTest, RemoveBestOrder)
{
    OrderBook book;

    book.add_order({0, Side::BUY, 100, 10});
    book.add_order({1, Side::BUY, 50, 15});
    book.add_order({2, Side::BUY, 200, 5});

    EXPECT_TRUE(book.remove_order(2));
    EXPECT_EQ(book.best_bid(), 100);
    EXPECT_FALSE(book.remove_order(3));
}

TEST(OrderBookTest, RemoveNonBestOrder)
{
    OrderBook book;

    book.add_order({0, Side::BUY, 100, 10});
    book.add_order({1, Side::BUY, 50, 15});
    book.add_order({2, Side::BUY, 200, 5});

    EXPECT_TRUE(book.remove_order(1));
    EXPECT_EQ(book.best_bid(), 200);
}

TEST(OrderBookTest, RemoveLastOrderAtPriceRemovesPriceLevel)
{
    OrderBook book;

    book.add_order({0, Side::BUY, 100, 10});
    book.add_order({1, Side::BUY, 200, 20});

    EXPECT_TRUE(book.remove_order(1));
    EXPECT_TRUE(book.has_bid());
    EXPECT_EQ(book.best_bid(), 100);

    EXPECT_TRUE(book.remove_order(0));
    EXPECT_FALSE(book.has_bid());
}

TEST(OrderBookTest, RemoveOrderFromEitherSide)
{
    OrderBook book;

    book.add_order({0, Side::BUY, 100, 10});
    book.add_order({1, Side::SELL, 101, 20});

    EXPECT_TRUE(book.remove_order(0));
    EXPECT_FALSE(book.has_bid());
    EXPECT_TRUE(book.has_ask());

    EXPECT_TRUE(book.remove_order(1));
    EXPECT_FALSE(book.has_ask());
}

TEST(OrderBookTest, PreservesBuyFifoAtSamePrice)
{
    OrderBook book;

    book.add_order({1, Side::BUY, 100, 10});
    book.add_order({2, Side::BUY, 100, 20});
    book.add_order({3, Side::BUY, 100, 30});

    EXPECT_EQ(book.best_bid_order().order_id, 1);

    EXPECT_TRUE(book.remove_order(1));
    EXPECT_EQ(book.best_bid_order().order_id, 2);

    EXPECT_TRUE(book.remove_order(2));
    EXPECT_EQ(book.best_bid_order().order_id, 3);
}

TEST(OrderBookTest, PreservesSellFifoAtSamePrice)
{
    OrderBook book;

    book.add_order({1, Side::SELL, 100, 10});
    book.add_order({2, Side::SELL, 100, 20});
    book.add_order({3, Side::SELL, 100, 30});

    EXPECT_EQ(book.best_ask_order().order_id, 1);

    EXPECT_TRUE(book.remove_order(1));
    EXPECT_EQ(book.best_ask_order().order_id, 2);

    EXPECT_TRUE(book.remove_order(2));
    EXPECT_EQ(book.best_ask_order().order_id, 3);
}

TEST(OrderBookTest, RemovalPreservesRemainingFifo)
{
    OrderBook book;

    book.add_order({1, Side::SELL, 100, 10});
    book.add_order({2, Side::SELL, 100, 20});
    book.add_order({3, Side::SELL, 100, 30});

    EXPECT_TRUE(book.remove_order(2));
    EXPECT_EQ(book.best_ask_order().order_id, 1);

    EXPECT_TRUE(book.remove_order(1));
    EXPECT_EQ(book.best_ask_order().order_id, 3);
}

TEST(OrderBookTest, OrderQuantityRemainsIndependentAtSamePrice)
{
    OrderBook book;

    book.add_order({1, Side::BUY, 100, 10});
    book.add_order({2, Side::BUY, 100, 20});

    ASSERT_EQ(book.bids().at(100).size(), 2U);

    auto it = book.bids().at(100).begin();
    EXPECT_EQ(it->order_id, 1);
    EXPECT_EQ(it->quantity, 10);

    ++it;
    EXPECT_EQ(it->order_id, 2);
    EXPECT_EQ(it->quantity, 20);
}