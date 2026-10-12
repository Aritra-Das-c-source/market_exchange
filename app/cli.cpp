#include "cli.hpp"

std::string side_to_str(Side side)
{
    if (side == Side::BUY) return "BUY";
    return "SELL";
}

void print_event(const MarketEvent& e) {
    switch (e.event_type) {
        case MarketEventType::AddOrder:
            std::cout << "Added " << side_to_str(e.AEvent.side) << " Order ID=" << e.AEvent.order_id << ":\n"
                      << "  price   " << std::setw(5) << e.AEvent.price << "\n"
                      << "  quantity" << std::setw(5) << e.AEvent.shares << "\n\n";
            break;

        case MarketEventType::OrderExecute:
            std::cout << "Executed Order Against ID=" << e.XEvent.order_id << ":\n"
                      << "  executed shares" << std::setw(5) << e.XEvent.executed_shares << "\n"
                      << "  match number   " << std::setw(5) << e.XEvent.match_number << "\n\n";
            break;

        case MarketEventType::OrderDelete:
            std::cout << "Deleted Order ID=" << e.DEvent.order_id << "\n\n";
            break;

        default:
            break;
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
    InputEvent in;

    iss >> command;

    if (command == "buy") {
        in.input_type = InputType::EnterOrder;
        in.OEvent.side = Side::BUY;
        i64 price, quantity;
        if (!(iss >> price >> quantity)) return {CommandType::UNKNOWN, in};
        if (price <= 0 ||  price >= UINT32_MAX || quantity <= 0 || quantity >= UINT32_MAX || iss >> extra) return {CommandType::UNKNOWN, in};
        in.OEvent.price = price;
        in.OEvent.quantity = quantity;

        return {CommandType::ADD, in};
    }

    else if (command == "sell") {
        in.input_type = InputType::EnterOrder;
        in.OEvent.side = Side::SELL;
        i64 price, quantity;
        if (!(iss >> price >> quantity)) return {CommandType::UNKNOWN, in};
        if (price <= 0 ||  price >= UINT32_MAX || quantity <= 0 || quantity >= UINT32_MAX || iss >> extra) return {CommandType::UNKNOWN, in};
        in.OEvent.price = price;
        in.OEvent.quantity = quantity;

        return {CommandType::ADD, in};
    }

    else if (command == "cancel") {
        i64 id;
        if (!(iss >> id) || iss >> extra) return {CommandType::UNKNOWN, in};
        if (id < 0 || id > UINT64_MAX) return {CommandType::UNKNOWN, in};
        in.input_type = InputType::CancelOrderRequest;
        in.XEvent.order_id = id;
        return {CommandType::CANCEL, in};
    }

    else if (command == "book") {
        if (iss >> extra) return {CommandType::UNKNOWN, in};
        return {CommandType::BOOK, in}; 
    }

    else if (command == "help") {
        if (iss >> extra) return {CommandType::UNKNOWN, in};
        return {CommandType::HELP, in}; 
    }

    else return {CommandType::UNKNOWN, in};
};

void run_command(const Command& cmd, MatchingEngine& ME)
{
    switch (cmd.type) {

    case CommandType::ADD: {
        for(const MarketEvent& e : ME.process(cmd.in)){
            print_event(e);
        }        
        break;
    }

    case CommandType::CANCEL: {
        for(const MarketEvent& e : ME.process(cmd.in)){
            print_event(e);
        }
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