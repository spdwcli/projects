#include <iostream>
#include <vector>
#include <random>
#include <tuple>

class NPC {
public:
    NPC(int pos_x, 
        int pos_y, 
        char logo, 
        std::vector<std::vector<char>> &grid) : pos_x(pos_x), pos_y(pos_y), logo(logo) {
        grid[pos_x][pos_y] = logo;
    }
    ~NPC() {}

    void make_action(int action, std::vector<std::vector<char>> &grid) {
        if(!action) {
            // idle 
        } else {
            // try to move in random direction
            std::pair<int, int> direction = directions[make_random(0, directions.size() - 1)];
            std::pair<int, int> new_pos = {pos_x + direction.first, pos_y + direction.second};

            if(new_pos.first >= 0 && new_pos.first < grid.size() 
                && new_pos.second >= 0 && new_pos.second < grid[0].size()) {
                if(grid[new_pos.first][new_pos.second] == ' ') {
                    grid[pos_x][pos_y] = ' ', grid[new_pos.first][new_pos.second] = logo;
                    pos_x = new_pos.first, pos_y = new_pos.second;
                }
            }
        }
    }

    std::tuple<int, int, char> get_parameters() {
        std::tuple<int, int, char> parameters = std::make_tuple(pos_x, pos_y, logo);
        return parameters;
    }

private:
    // private method, used to generate random number in specific range
    int make_random(int left, int right) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(left, right);
        return dist(gen);
    }

    // variables
    const std::vector<std::pair<int, int>> directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    int  pos_x{};
    int  pos_y{};
    char logo{};
};
