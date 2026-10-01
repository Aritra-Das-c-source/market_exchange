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

std::string side_to_str(Side side) {
    if (side == Side::BUY) return "BUY";
    return "SELL";
}

Command parse_command(const std::string& line)
{
    std::istringstream iss(line);
    std::string command;
    std::string extra;
    Order od;

    iss >> command;

    if (command == "buy") {
        od.side = Side::BUY;
        if (!(iss >> od.price >> od.quantity)) return {CommandType::UNKNOWN, od};
        if (od.price == 0 || od.quantity == 0 || iss >> extra) return {CommandType::UNKNOWN, od};

        return {CommandType::ADD, od};
    }

    else if (command == "sell") {
        od.side = Side::SELL;
        if (!(iss >> od.price >> od.quantity)) return {CommandType::UNKNOWN, od};
        if (od.price == 0 || od.quantity == 0 || iss >> extra) return {CommandType::UNKNOWN, od};

        return {CommandType::ADD, od};
    }

    else if (command == "cancel") {
        if (!(iss >> od.order_id) || iss >> extra) return {CommandType::UNKNOWN, od};
        return {CommandType::CANCEL, od};
    }

    else if (command == "book") {
        if (iss >> extra) return {CommandType::UNKNOWN, od};
        return {CommandType::BOOK, od}; 
    }

    else if (command == "help") {
        if (iss >> extra) return {CommandType::UNKNOWN, od};
        return {CommandType::HELP, od}; 
    }

    else return {CommandType::UNKNOWN, od};
};

void print_trades(const std::vector<Trade>& trades) {
    for (const auto& trade : trades) {
        std::cout << "Trade:\n"
                  << "  trade_id: " << trade.trade_id << "\n"
                  << "  buy_order_id: " << trade.buy_id << "\n"
                  << "  sell_order_id: " << trade.sell_id << "\n"
                  << "  price: " << trade.price << "\n"
                  << "  quantity: " << trade.quantity << "\n"
                  << "\n";
    }
}

void print_book(const OrderBook& book) {
    std::cout << "========== ORDER BOOK ==========\n\n"
              << "ASK\n"
              << "Price      Quantity\n";

    for (const auto& it : book.sp_mp) {
        for (const auto& order : it.second) {
            std::cout << std::setw(5) << order.price << "      " << order.quantity << "\n";
        }
    }

    std::cout << "\n"
              << "BID\n"
              << "Price      Quantity\n";
    
    for (auto it = book.bp_mp.rbegin(); it != book.bp_mp.rend(); ++it) {
        for (const auto order : it->second) {
            std::cout << std::setw(5) << order.price << "      " << order.quantity << "\n";
        }
    }

    std::cout << "\n"
              << "================================\n\n";
}

int main() {
    MatchingEngine ME = MatchingEngine();
    std::string line;
    
    while (std::getline(std::cin, line)) {
        if (line == "exit") break;

        Command cmd = parse_command(line);

        switch (cmd.type) {

            case CommandType::ADD: {
                SubmitResult sr = ME.submit_order(cmd.order);
                std::cout << "Order accepted: ID=" << sr.order_id << " " << side_to_str(cmd.order.side) << " " << cmd.order.quantity << " @ " << cmd.order.price << "\n\n"; 
                print_trades(sr.trades);

                if (sr.remaining_quantity) {
                    std::cout << "Remaining:\n"
                              << "  order_id: " << sr.order_id << "\n"
                              << "  quantity: " << sr.remaining_quantity << "\n\n";
                }
                break;
            }

            case CommandType::CANCEL: {
                bool status = ME.cancel_order(cmd.order.order_id);
                if (status) std::cout << "Order cancelled: ID=" << cmd.order.order_id << "\n\n";
                else std::cout << "Cancel failed: Order " << cmd.order.order_id << " does not exist\n\n";
                break;
            }
            
            case CommandType::BOOK:
                print_book(ME.book());
                break;
            
            case CommandType::HELP:
                std::cout << "buy <price> <quantity>\tSubmit a buy limit order\n"
                          << "sell <price> <quantity>\tSubmit a sell limit order\n"
                          << "cancel <order_id>\tCancel an existing order\n"
                          << "book\t\t\tDisplay order book\n"
                          << "help\t\t\tDisplay available commands\n"
                          << "exit\t\t\tExit\n";
                break;
            
            case CommandType::UNKNOWN:
                std::cout << "Invalid command\n";
                break;
        }
    }

    return 0;
}