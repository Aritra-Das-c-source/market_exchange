cmake_minimum_required(VERSION 3.28)
project(market_exchange)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXCHANGE_LIB_NAME "exchange")
set(CLI_EXE_NAME "exchange_cli")

add_library(${EXCHANGE_LIB_NAME} STATIC
    src/order_book.cpp
    src/matching_engine.cpp
)

target_include_directories(${EXCHANGE_LIB_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

add_executable(${CLI_EXE_NAME}
    app/main.cpp
    app/cli.cpp
)

target_link_libraries(${CLI_EXE_NAME} PRIVATE ${EXCHANGE_LIB_NAME})