#include <benchmark/benchmark.h>

#include <exchange/order_book.hpp>

static void BM_OrderBook_AddNewPriceLevel(benchmark::State& state)
{
    const u32 num_levels = static_cast<u32>(state.range(0));

    OrderBook book;

    for (u32 i = 0; i < num_levels; ++i) {
        book.add_order({
            static_cast<u64>(i + 1),
            Side::BUY,
            static_cast<u32>(1000 + i),
            1
        });
    }

    const u64 order_id = static_cast<u64>(num_levels + 1);
    const u32 price = static_cast<u32>(1000 + num_levels + 100);

    for (auto _ : state) {
        book.add_order({
            order_id,
            Side::BUY,
            price,
            1
        });

        benchmark::DoNotOptimize(book);

        state.PauseTiming();
        book.remove_order(order_id);
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_OrderBook_AddNewPriceLevel)
    ->RangeMultiplier(4)
    ->Range(1, 16384);

static void BM_OrderBook_AddExistingPriceLevel(benchmark::State& state)
{
    const u32 depth = static_cast<u32>(state.range(0));

    OrderBook book;

    constexpr u32 price = 10000;

    for (u32 i = 0; i < depth; ++i) {
        book.add_order({
            static_cast<u64>(i + 1),
            Side::BUY,
            price,
            1
        });
    }

    const u64 order_id = static_cast<u64>(depth + 1);

    for (auto _ : state) {
        book.add_order({
            order_id,
            Side::BUY,
            price,
            1
        });

        benchmark::DoNotOptimize(book);

        state.PauseTiming();
        book.remove_order(order_id);
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_OrderBook_AddExistingPriceLevel)
    ->RangeMultiplier(4)
    ->Range(1, 16384);

static void BM_OrderBook_CancelNonLastAtPrice(benchmark::State& state)
{
    const u32 num_levels = static_cast<u32>(state.range(0));

    OrderBook book;

    for (u32 i = 0; i < num_levels; ++i) {
        book.add_order({
            static_cast<u64>(i + 1),
            Side::BUY,
            static_cast<u32>(1000 + i),
            1
        });
    }

    const u64 first_id = static_cast<u64>(num_levels + 1);
    const u64 second_id = static_cast<u64>(num_levels + 2);

    book.add_order({
        first_id,
        Side::BUY,
        50000,
        1
    });

    book.add_order({
        second_id,
        Side::BUY,
        50000,
        1
    });

    for (auto _ : state) {
        benchmark::DoNotOptimize(book.remove_order(first_id));

        state.PauseTiming();
        book.add_order({
            first_id,
            Side::BUY,
            50000,
            1
        });
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_OrderBook_CancelNonLastAtPrice)
    ->RangeMultiplier(4)
    ->Range(1, 16384);

static void BM_OrderBook_CancelLastAtPrice(benchmark::State& state)
{
    const u32 num_levels = static_cast<u32>(state.range(0));

    OrderBook book;

    for (u32 i = 0; i < num_levels; ++i) {
        book.add_order({
            static_cast<u64>(i + 1),
            Side::BUY,
            static_cast<u32>(1000 + i),
            1
        });
    }

    const u64 order_id = static_cast<u64>(num_levels + 1);
    constexpr u32 price = 50000;

    for (auto _ : state) {
        state.PauseTiming();

        book.add_order({
            order_id,
            Side::BUY,
            price,
            1
        });

        state.ResumeTiming();

        benchmark::DoNotOptimize(book.remove_order(order_id));
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_OrderBook_CancelLastAtPrice)
    ->RangeMultiplier(4)
    ->Range(1, 16384);

static void BM_OrderBook_BestBid(benchmark::State& state)
{
    const u32 num_levels = static_cast<u32>(state.range(0));

    OrderBook book;

    for (u32 i = 0; i < num_levels; ++i) {
        book.add_order({
            static_cast<u64>(i + 1),
            Side::BUY,
            static_cast<u32>(1000 + i),
            1
        });
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(book.best_bid());
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_OrderBook_BestBid)
    ->RangeMultiplier(4)
    ->Range(1, 16384);

static void BM_OrderBook_BestAsk(benchmark::State& state)
{
    const u32 num_levels = static_cast<u32>(state.range(0));

    OrderBook book;

    for (u32 i = 0; i < num_levels; ++i) {
        book.add_order({
            static_cast<u64>(i + 1),
            Side::SELL,
            static_cast<u32>(1000 + i),
            1
        });
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(book.best_ask());
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_OrderBook_BestAsk)
    ->RangeMultiplier(4)
    ->Range(1, 16384);