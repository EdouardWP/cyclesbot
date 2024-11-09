#include "api.h"
#include "utils.h"
#include <iostream>
#include <random>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

using namespace cycles;

class BotClient {
    Connection connection;
    std::string name;
    GameState state;
    Player my_player;
    std::mt19937 rng;
    int previousDirection = -1;

    const std::vector<Direction> directions = {Direction::north, Direction::east, Direction::south, Direction::west};
    const std::vector<sf::Vector2i> directionVectors = {
        sf::Vector2i(0, -1), // north
        sf::Vector2i(1, 0),  // east
        sf::Vector2i(0, 1),  // south
        sf::Vector2i(-1, 0)  // west
    };

    bool is_valid_move(Direction direction) {
        auto new_pos = my_player.position + getDirectionVector(direction);
        if (!state.isInsideGrid(new_pos)) {
            return false;
        }
        if (state.getGridCell(new_pos) != 0) {
            return false;
        }
        return true;
    }

    // Count the number of empty cells in a given direction
    int countOpenCells(const sf::Vector2i& directionVec) {
        sf::Vector2i position = my_player.position; // bot starts counting empty cells from current place
        int openCells = 0;
        const int maxRange = 10; 

        for (int i = 1; i <= maxRange; ++i) {
            position += directionVec;

            // essentially making sure it stays in the grid 
            if (!state.isInsideGrid(position) || !state.isCellEmpty(position)) {
                break;
            }
            openCells++;
        }
        return openCells;
    }

    // Decide the best move based on the most open space
    Direction decideMove() {
        int maxOpenSpace = -1;
        Direction bestDirection = Direction::north;

        for (size_t i = 0; i < directions.size(); ++i) {
            if (is_valid_move(directions[i])) {
                int openSpace = countOpenCells(directionVectors[i]);
                // Swapping best direction to direction with most space
                if (openSpace > maxOpenSpace) { 
                    maxOpenSpace = openSpace;
                    bestDirection = directions[i]; 
                }
            }
        }
        
        // If no valid move is found, default to the previous direction
        if (maxOpenSpace == -1) {
            bestDirection = getDirectionFromValue(previousDirection);
        }

        previousDirection = getDirectionValue(bestDirection);
        return bestDirection;
    }

    void receiveGameState() {
        state = connection.receiveGameState();
        for (const auto &player : state.players) {
            if (player.name == name) {
                my_player = player;
                break;
            }
        }
    }

    void sendMove() {
        spdlog::debug("{}: Sending move", name);
        auto move = decideMove(); // using my added logic
        connection.sendMove(move);
    }

public:
    BotClient(const std::string &botName) : name(botName) {
        std::random_device rd;
        rng.seed(rd());
        connection.connect(name);
        if (!connection.isActive()) {
            spdlog::critical("{}: Connection failed", name);
            exit(1);
        }
    }

    void run() {
        while (connection.isActive()) {
            receiveGameState();
            sendMove();
        }
    }
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <bot_name>" << std::endl;
        return 1;
    }
#if SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_TRACE
    spdlog::set_level(spdlog::level::debug);
#endif
    std::string botName = argv[1];
    BotClient bot(botName);
    bot.run();
    return 0;
}
