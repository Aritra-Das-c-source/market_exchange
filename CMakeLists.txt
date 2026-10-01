cmake_minimum_required(VERSION 3.28)
project(market_exchange)

add_executable(exchange_cli
    app/main.cpp
    app/cli.cpp
    src/matching_engine.cpp
    src/order_book.cpp
)

