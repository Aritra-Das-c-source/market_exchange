#include <benchmark/benchmark.h>

#include <exchange/matching_engine.hpp>

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

static u64 add_and_get_order_id(
    MatchingEngine& engine,
    Side side,
    u32 price,
    u32 quantity)
{
    const auto events = engine.process(make_add(side, price, quantity));
    return events.front().AEvent.order_id;
}

static void BM_MatchingEngine_NonCrossingOrder(benchmark::State& state)
{
    const u32 num_levels = static_cast<u32>(state.range(0));

    MatchingEngine engine;

    for (u32 i = 0; i < num_levels; ++i) {
        engine.process(make_add(
            Side::SELL,
            static_cast<u32>(10000 + i),
            1
        ));
    }

    for (auto _ : state) {
        const auto events = engine.process(make_add(Side::BUY, 5000, 1));

        benchmark::DoNotOptimize(events.data());
        benchmark::DoNotOptimize(events.size());

        const u64 order_id = events.front().AEvent.order_id;

        state.PauseTiming();
        engine.process(make_cancel(order_id));
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
        engine.process(make_add(Side::SELL, 10000, 1));
        state.ResumeTiming();

        const auto events = engine.process(make_add(Side::BUY, 10000, 1));

        benchmark::DoNotOptimize(events.data());
        benchmark::DoNotOptimize(events.size());
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
            engine.process(make_add(Side::SELL, static_cast<u32>(10000 + i), 1));
        }

        state.ResumeTiming();

        const auto events = engine.process(make_add(Side::BUY, static_cast<u32>(10000 + num_levels - 1), num_levels
        ));

        benchmark::DoNotOptimize(events.data());
        benchmark::DoNotOptimize(events.size());
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
            engine.process(make_add(Side::SELL, 10000, 1));
        }

        state.ResumeTiming();

        const auto events = engine.process(make_add(Side::BUY, 10000, depth));
        benchmark::DoNotOptimize(events.data());
        benchmark::DoNotOptimize(events.size());
    }

    state.SetItemsProcessed(
        state.iterations() * depth
    );
}

BENCHMARK(BM_MatchingEngine_SamePriceFIFOConsumption)
    ->RangeMultiplier(4)
    ->Range(1, 4096);