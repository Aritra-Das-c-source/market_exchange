#pragma once

#include "../include/exchange/matching_engine.hpp"

#include <sstream>
#include <string>
#include <iostream>
#include <iomanip>

enum class CommandType {
    ADD,
    CANCEL,
    BOOK,
    HELP,
    UNKNOWN
};

struct Command {
    CommandType type;
    Order order;
};

std::string side_to_str(Side side);
void print_trades(const std::vector<Trade>& trades);
void print_book(const OrderBook& book);
Command parse_command(const std::string& line);
void run_command(Command& cmd, MatchingEngine& ME);