#include "cli.hpp"

std::string side_to_str(Side side)
{
    if (side == Side::BUY) return "BUY";
    return "SELL";
}

void print_trades(const std::vector<Trade>& trades)
{
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

void print_book(const OrderBook& book)
{
    std::cout << "========== ORDER BOOK ==========\n\n"
              << "ASK\n"
              << "Price      Quantity\n";

    for (const auto& it : book.asks()) {
        for (const auto& order : it.second) {
            std::cout << std::setw(5) << order.price << "      " << order.quantity << "\n";
        }
    }

    std::cout << "\n"
              << "BID\n"
              << "Price      Quantity\n";
    
    const auto& bids = book.bids();
    for (auto it = bids.rbegin(); it != bids.rend(); ++it) {
        for (const auto& order : it->second) {
            std::cout << std::setw(5) << order.price << "      " << order.quantity << "\n";
        }
    }

    std::cout << "\n"
              << "================================\n\n";
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

void run_command(const Command& cmd, MatchingEngine& ME)
{
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