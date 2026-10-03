#include <benchmark/benchmark.h>

#include <exchange/matching_engine.hpp>

static void BM_MatchingEngine_NonCrossingOrder(benchmark::State& state)
{
    const u32 num_levels = static_cast<u32>(state.range(0));

    MatchingEngine engine;

    for (u32 i = 0; i < num_levels; ++i) {
        engine.submit_order({
            0,
            Side::SELL,
            static_cast<u32>(10000 + i),
            1
        });
    }

    for (auto _ : state) {
        auto result = engine.submit_order({
            0,
            Side::BUY,
            5000,
            1
        });

        benchmark::DoNotOptimize(result);

        state.PauseTiming();
        engine.cancel_order(result.order_id);
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_MatchingEngine_NonCrossingOrder)
    ->RangeMultiplier(4)
    ->Range(1, 16384);

static void BM_MatchingEngine_SingleTrade(benchmark::State& state)
{
    MatchingEngine engine;

    for (auto _ : state) {
        state.PauseTiming();

        engine.submit_order({
            0,
            Side::SELL,
            10000,
            1
        });

        state.ResumeTiming();

        auto result = engine.submit_order({
            0,
            Side::BUY,
            10000,
            1
        });

        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_MatchingEngine_SingleTrade);

static void BM_MatchingEngine_MultiLevelMatch(benchmark::State& state)
{
    const u32 num_levels = static_cast<u32>(state.range(0));

    MatchingEngine engine;

    for (auto _ : state) {
        state.PauseTiming();

        for (u32 i = 0; i < num_levels; ++i) {
            engine.submit_order({
                0,
                Side::SELL,
                static_cast<u32>(10000 + i),
                1
            });
        }

        state.ResumeTiming();

        auto result = engine.submit_order({
            0,
            Side::BUY,
            static_cast<u32>(10000 + num_levels - 1),
            num_levels
        });

        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(
        state.iterations() * num_levels
    );
}

BENCHMARK(BM_MatchingEngine_MultiLevelMatch)
    ->RangeMultiplier(4)
    ->Range(1, 4096);

static void BM_MatchingEngine_SamePriceFIFOConsumption(
    benchmark::State& state)
{
    const u32 depth = static_cast<u32>(state.range(0));

    MatchingEngine engine;

    for (auto _ : state) {
        state.PauseTiming();

        for (u32 i = 0; i < depth; ++i) {
            engine.submit_order({
                0,
                Side::SELL,
                10000,
                1
            });
        }

        state.ResumeTiming();

        auto result = engine.submit_order({
            0,
            Side::BUY,
            10000,
            depth
        });

        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(
        state.iterations() * depth
    );
}

BENCHMARK(BM_MatchingEngine_SamePriceFIFOConsumption)
    ->RangeMultiplier(4)
    ->Range(1, 4096);