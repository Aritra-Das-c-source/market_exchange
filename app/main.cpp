#include "cli.hpp"

int main() {
    MatchingEngine ME = MatchingEngine();
    std::string line;
    
    while (std::getline(std::cin, line)) {
        if (line == "exit") break;

        Command cmd = parse_command(line);
        run_command(cmd, ME);
    }

    return 0;
}