add_executable(exchange_tests
    order_book_tests.cpp
    matching_engine_tests.cpp
    cli_tests.cpp
    ${PROJECT_SOURCE_DIR}/app/cli.cpp
)

target_include_directories(exchange_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/app
)

target_link_libraries(
    exchange_tests
    PRIVATE 
        ${EXCHANGE_LIB_NAME}
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(exchange_tests)