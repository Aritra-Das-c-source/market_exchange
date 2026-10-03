add_executable(exchange_benchmark
    order_book_bench.cpp
    matching_engine_bench.cpp
)

target_link_libraries(exchange_benchmark
    PRIVATE
    ${EXCHANGE_LIB_NAME}
    benchmark::benchmark_main
)