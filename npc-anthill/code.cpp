#include <iostream>
#include <thread>
#include <mutex>
#include <random>
#include <functional>
#include <set>
#include <map>
#include <tuple>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include "npc.h"

/* Defines */ 
#define lock_mutex(x) (x).lock()
#define unlock_mutex(x) (x).unlock()

// Mersenne's twister pseudo-random number generator
int make_random(int left, int right) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(left, right);
    return dist(gen);
}

/* ---------------- */
/* Global variables */
/* ---------------- */

std::vector<std::vector<char>> grid;
std::vector<NPC*> npcs;
int error_counter = 0;
std::set<int> st;

std::map<std::string, std::string> blocks_replace = {
    // non-frame cells
    // ---------------
    {" # "
     "###"
     " # ", "\u256C"},

    {"   "
     " # "
     " # ", "\u2551"},

    {" # "
     " # "
     "   ", "\u2551"},

    {"   "
     "## "
     "   ", "\u2550"},

    {"   "
     " ##"
     "   ", "\u2550"},

    {"   "
     "###"
     " # ", "\u2566"},

    {" # "
     " ##"
     " # ", "\u2560"},

    {" # "
     "## "
     " # ", "\u2563"},

    {" # "
     "###"
     "   ", "\u2569"},

    {" # "
     "## "
     "   ", "\u255D"},

    {" # "
     " ##"
     "   ", "\u255A"},

    {"   "
     "## "
     " # ", "\u2557"},

    {"   "
     " ##"
     " # ", "\u2554"},

    {" # "
     " # "
     " # ", "\u2551"},

    {"   "
     "###"
     "   ", "\u2550"},

    {"   "
     " # "
     "   ", "\u25A1"},

    // on-frame cells
    // --------------
    
    // first row
    {"###"
     " # ", "\u2566"},

    {"## "
     " # ", "\u2557"},

    {" ##"
     " # ", "\u2554"},

    {"###"
     "   ", "\u2550"},

    {" # "
     " # ", "\u2551"},

    {" ##"
     "   ", "\u2550"},

    {"## "
     "   ", "\u2550"},

    {" # "
     "   ", "#"},

    // last row
    {" # "
     "###", "\u2569"},

    {" # "
     "## ", "\u255D"},

    {" # "
     " ##", "\u255A"},

    {"   "
     "## ", "\u2550"},

    {"   "
     " ##", "\u2550"},

    {"   "
     "###", "\u2550"},

    {"   "
     " # ", "\u2573"},

    // upper left corner
    {"##"
     "# ", "\u2554"},

    {"# "
     "  ", "\u25A1"},

    {"# "
     "# ", "\u2551"},

    // upper right corner
    {"##"
     " #", "\u2557"},

    {" #"
     "  ", "\u25A1"},

    {" #"
     " #", "\u2551"},

    {"##"
     "  ", "\u2550"},

    // lower left corner
    {"# "
     "##", "\u255A"},

    {"  "
     "##", "\u2550"},

    {"  "
     "# ", "\u25A1"},

    // upper right corner
    {" #"
     "##", "\u255D"},

    // first column non-corner
    {"# "
     "##"
     "# ", "\u2560"},

    {"# "
     "##"
     "  ", "\u255A"},

    {"  "
     "##"
     "# ", "\u2554"},

    {"# "
     "# "
     "# ", "\u2551"},

    {"# "
     "# "
     "  ", "\u2551"},

    {"  "
     "# "
     "# ", "\u2551"},

    {"  "
     "##"
     "  ", "\u2550"},

    {"  "
     "# "
     "  ", "\u25A1"},

    // second column non-corner
    {" #"
     "##"
     " #", "\u2563"},

    {" #"
     "##"
     "  ", "\u255D"},

    {"  "
     "##"
     " #", "\u2557"},

    {" #"
     " #"
     " #", "\u2551"},

    {" #"
     " #"
     "  ", "\u2551"},

    {"  "
     " #"
     " #", "\u2551"},

    {"  "
     " #"
     "  ", "\u25A1"},
};

std::vector<std::vector<std::string>> beautified_grid_random_blocks;

/* ------------------- */
/* Mutexes for threads */
/* ------------------- */

std::mutex print_grid_mutex; // probably don't need this, because we're 
                             // not changing any data in <grid_print_thread>
std::mutex process_npc_mutex;
std::mutex npc_zone_scanning_mutex;

/* ----------------- */
/* Threads functions */
/* ----------------- */

// Executed in <grid_print_thread> - prints grid 
void print_grid() {
    system("clear");

    // I have no idea how this works
    // printf("\n1b[2J");

    while(true) {
        // I have no idea how this works too
        printf("\x1b[H");

        // Print grid with frame around
        for(int it = 0; it < grid[0].size() + 2; it++) 
            if(!it)
                std::cout << "\u250C";
            else if(it < grid[0].size() + 1)
                std::cout << "\u2500";
            else 
                std::cout << "\u2510";
        std::cout << "\n";

        // Converts '#' to something more pretty depending on surrounding elements
        auto beautify_grid_elements = [](std::vector<std::vector<char>> grid) -> void {
            int rows = grid.size(), columns = grid[0].size();

            for(int row = 0; row < rows; row++) {
                std::cout << "\u2502";
                for(int column = 0; column < columns; column++) {
                    if(grid[row][column] != '#') {
                        std::cout << grid[row][column]; 
                        continue;
                    }

                    if(row == 0 ||
                       row == rows - 1 ||
                       column == 0 ||
                       column == columns - 1) {

                        if(row == 0) {
                            std::string block;

                            if(column == 0) {
                                block += grid[row][column];
                                block += grid[row][column + 1] == '#' ? grid[row][column + 1] : ' ';
                                block += grid[row + 1][column] == '#' ? grid[row + 1][column] : ' ';
                                block += ' ';

                                if(blocks_replace.find(block) == blocks_replace.end())
                                    std::cout << grid[row][column];
                                else 
                                   std::cout << blocks_replace[block];

                                continue;
                            }

                            if(column == columns - 1) {
                                block += grid[row][column - 1] == '#' ? grid[row][column - 1] : ' ';
                                block += grid[row][column];
                                block += ' ';
                                block += grid[row + 1][column] == '#' ? grid[row + 1][column] : ' ';

                                if(blocks_replace.find(block) == blocks_replace.end())
                                    std::cout << grid[row][column];
                                else 
                                    std::cout << blocks_replace[block];

                                continue;
                            }

                            block += grid[row][column - 1] == '#' ? grid[row][column - 1] : ' ';
                            block += grid[row][column];
                            block += grid[row][column + 1] == '#' ? grid[row][column + 1] : ' ';
                            block += ' ';
                            block += grid[row + 1][column] == '#' ? grid[row + 1][column] : ' ';
                            block += ' ';

                            if(blocks_replace.find(block) == blocks_replace.end())
                                std::cout << grid[row][column];
                            else
                                std::cout << blocks_replace[block];
                        } else if(row == rows - 1) {
                            std::string block;

                            if(column == 0) {
                                block += grid[row - 1][column] == '#' ? grid[row - 1][column] : ' ';
                                block += ' ';
                                block += grid[row][column];
                                block += grid[row][column + 1] == '#' ? grid[row][column + 1] : ' ';

                                if(blocks_replace.find(block) == blocks_replace.end())
                                    std::cout << grid[row][column];
                                else 
                                    std::cout << blocks_replace[block];

                                continue;
                            }

                            if(column == columns - 1) {
                                block += ' ';
                                block += grid[row - 1][column] == '#' ? grid[row - 1][column] : ' ';
                                block += grid[row][column - 1] == '#' ? grid[row][column - 1] : ' ';
                                block += grid[row][column];

                                if(blocks_replace.find(block) == blocks_replace.end())
                                    std::cout << grid[row][column];
                                else 
                                    std::cout << blocks_replace[block];

                                continue;
                            }

                            block += ' ';
                            block += grid[row - 1][column] == '#' ? grid[row - 1][column] : ' ';
                            block += ' ';
                            block += grid[row][column - 1] == '#' ? grid[row][column - 1] : ' ';
                            block += grid[row][column]; 
                            block += grid[row][column + 1] == '#' ? grid[row][column + 1] : ' ';

                            if(blocks_replace.find(block) == blocks_replace.end()) 
                                std::cout << grid[row][column];
                            else
                                std::cout << blocks_replace[block];
                        } else {
                            std::string block;

                            if(column == 0) {
                                block += grid[row - 1][column] == '#' ? grid[row - 1][column] : ' ';
                                block += ' ';
                                block += grid[row][column];
                                block += grid[row][column + 1] == '#' ? grid[row][column + 1] : ' ';
                                block += grid[row + 1][column] == '#' ? grid[row + 1][column] : ' ';
                                block += ' ';

                                if(blocks_replace.find(block) == blocks_replace.end())
                                    std::cout << grid[row][column];
                                else
                                    std::cout << blocks_replace[block];
                                continue;
                            }

                            if(column == columns - 1) {
                                block += ' ';
                                block += grid[row - 1][column] == '#' ? grid[row - 1][column] : ' ';
                                block += grid[row][column - 1] == '#' ? grid[row][column - 1] : ' ';
                                block += grid[row][column];
                                block += ' ';
                                block += grid[row + 1][column] == '#' ? grid[row + 1][column] : ' ';

                                if(blocks_replace.find(block) == blocks_replace.end())
                                    std::cout << grid[row][column];
                                else
                                    std::cout << blocks_replace[block];
                            }
                        }
                        continue;
                    }

                    std::string block;

                    block += ' ';
                    block += grid[row - 1][column] == '#' ? grid[row - 1][column] : ' ';
                    block += ' ';
                    block += grid[row][column - 1] == '#' ? grid[row][column - 1] : ' ';
                    block += grid[row][column];
                    block += grid[row][column + 1] == '#' ? grid[row][column + 1] : ' ';
                    block += ' ';
                    block += grid[row + 1][column] == '#' ? grid[row + 1][column] : ' ';
                    block += ' ';

                    std::vector<std::string> random_blocks = {"\u2550", "\u2551", "\u2554", 
                                                              "\u2557", "\u255A", "\u255D", 
                                                              "\u2560", "\u2563", "\u2566", 
                                                              "\u2569", "\u256C"};

                    if(block == " # ### # ") {
                        std::string real_block;

                        real_block += grid[row - 1][column - 1];
                        real_block += grid[row - 1][column];
                        real_block += grid[row - 1][column + 1];
                        real_block += grid[row][column - 1];
                        real_block += grid[row][column];
                        real_block += grid[row][column + 1];
                        real_block += grid[row + 1][column - 1];
                        real_block += grid[row + 1][column];
                        real_block += grid[row + 1][column + 1];

                        if(real_block == "#########") {
                            if(beautified_grid_random_blocks[row][column] == "empty")
                                beautified_grid_random_blocks[row][column] = 
                                    random_blocks[make_random(0, random_blocks.size() - 1)];
                            std::cout << beautified_grid_random_blocks[row][column];
                            continue;
                        }
                    }

                    if(blocks_replace.find(block) == blocks_replace.end()) 
                        std::cout << grid[row][column];
                    else {
                        std::cout << blocks_replace[block];
                    }
                }
                std::cout << "\u2502\n";
            }
        };

        // Beautify grid elements
        beautify_grid_elements(grid);

        for(int it = 0; it < grid[0].size() + 2; it++)
            if(!it)
                std::cout << "\u2514";
            else if(it < grid[0].size() + 1)
                std::cout << "\u2500";
            else 
                std::cout << "\u2518";
        std::cout << "\n\n";

        // Print NPC list header
        std::string header_title = "NPC statistics";
        std::cout << header_title << "\n";
        for(int it = 0; it < header_title.size(); it++)
            std::cout << "\u2500";
        std::cout << "\n";

        // Print NPC list
        for(auto npc: npcs) {
            std::tuple<int, int, char, bool, char> npc_parameters = npc->get_parameters();

            // NPC parameters
            int pos_x = std::get<0>(npc_parameters);
            int pos_y = std::get<1>(npc_parameters);
            char logo = std::get<2>(npc_parameters);
            bool state = std::get<3>(npc_parameters);
            char pair_npc = std::get<4>(npc_parameters);

            // Print NPC parameters
            std::cout << logo << ": ";
            std::cout << "Coordinates [" << pos_x << "; " << pos_y << "], ";
            std::cout << "State: " << (state ? "blocked" : "active");
            if(state) {
                std::cout << ", ";
                std::cout << "Pair: " << pair_npc;
            }

            // Needed to make sure that next NPC position 
            // will not overlaps with current position in output
            for(int i = 0; i < 10; i++) {
                std::cout << " ";
            }

            std::cout << "\n";
        }
    }
}

// Executed in <npc_zone_scanning_thread> - scans zone around NPC
void npc_zone_scanning(char logo) {
    for(auto npc: npcs) {
        if(npc->get_logo() != logo) 
            continue;

        int distance = 5;

        while(true) {
            lock_mutex(npc_zone_scanning_mutex);
            npc->scan_zone(distance, grid);
            unlock_mutex(npc_zone_scanning_mutex);
        }
    }
}

// Executed in <process_npc_thread> - processes NPC actions
void process_npc(std::pair<int, int> position, char logo) {
    lock_mutex(process_npc_mutex);
    // Create NPC and place it on grid
    NPC npc(position.first, position.second, logo, grid);

    // Add NPC for NPC list (just in case we need it somewhere)
    npcs.push_back(&npc);
    unlock_mutex(process_npc_mutex);

    // Create thread for scanning zone around NPC
    std::thread npc_zone_scanning_thread(npc_zone_scanning, logo);

    // Detach zone scanning thread from this NPC thread
    npc_zone_scanning_thread.detach();

    // NPC processor
    while(true) {
        bool action = make_random(0, 1);

        lock_mutex(process_npc_mutex);
        // Make NPC action
        npc.make_action(action, grid);    
        unlock_mutex(process_npc_mutex);
        
        // Random movement speed
        std::this_thread::sleep_for(std::chrono::milliseconds(make_random(0, 300)));
    }
}

int main() {
    // will it execute on Windows?
    system("tput civis");
    
    int rows{}, columns{}; 
    
    // Input rows & columns (grid size)
    std::cout << "Rows: ";
    std::cin >> rows;

    std::cout << "Columns: ";
    std::cin >> columns;

    //assert(rows >= 3 && columns >= 3);
    
    // Initialize grids
    grid = std::vector<std::vector<char>> (rows, std::vector<char> (columns, ' '));
    beautified_grid_random_blocks = std::vector<std::vector<std::string>> 
                                    (rows, std::vector<std::string> (columns, "empty"));
    
    // DFS algorithm
    std::function<void(int, int, int, int)> DFS;
    DFS = [&DFS, rows, columns](int row, 
                                int column, 
                                int filled_cells, 
                                int total_cells) -> void {
        if(filled_cells == total_cells)
           return;

        grid[row][column] = '#';
        filled_cells++;
           
        std::vector<std::pair<int, int>> possible_directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}, 
                                                                {-1, 1}, {1, -1}, {1, 1}, {-1, -1}};
        std::set<std::pair<int, int>> used_directions;

        // Try to go in all directions
        while(true) {
            std::pair<int, int> direction = possible_directions[make_random(0, possible_directions.size() - 1)];
            std::pair<int, int> next_cell = std::make_pair(row + direction.first, column + direction.second);

            // check if we can place '#' in this cell 
            if(next_cell.first >= 0 && next_cell.first < rows && 
                next_cell.second >= 0 && next_cell.second < columns) {
                if(grid[next_cell.first][next_cell.second] == ' ') {
                    DFS(next_cell.first, next_cell.second, filled_cells + 1, total_cells);
                    break;
                }
            }

            used_directions.insert(next_cell);
            
            // if all possible directions were used -> return
            if(used_directions.size() == possible_directions.size()) 
                break;
        }

        return;
    };

    // CLEAR algorith is the same DFS, but it removes small groups of empty cells from grid
    std::function<void(int, int, std::vector<std::vector<bool>>&, std::vector<std::pair<int, int>>&, char)> CLEAR;
    CLEAR = [&CLEAR, rows, columns](int row, 
                                    int column, 
                                    std::vector<std::vector<bool>> &used,
                                    std::vector<std::pair<int, int>> &cords,
                                    char symbol) -> void {
        cords.push_back(std::make_pair(row, column));
        used[row][column] = true;
        
        std::vector<std::pair<int, int>> possible_directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
        
        for(auto direction: possible_directions) {
            std::pair<int, int> next_cell = std::make_pair(row + direction.first, 
                                                           column + direction.second);      
            if(next_cell.first >= 0 && next_cell.first < rows &&
                next_cell.second >= 0 && next_cell.second < columns) {
                if(grid[next_cell.first][next_cell.second] == symbol && !used[next_cell.first][next_cell.second]) {
                    CLEAR(next_cell.first, next_cell.second, used, cords, symbol);
                }
            }
        }

        return;
    };

    // Generate grid using DFS algorithm
    int runs = make_random(10, 20);
    while(runs--) {
        int cells = make_random(std::min(rows, columns), 
                                std::max(rows, columns) * 2);
        std::pair<int, int> start_cell = std::make_pair(make_random(0, rows - 1), 
                                                        make_random(0, columns - 1));
        int filled_cells = 0;

        // Run DFS
        DFS(start_cell.first, start_cell.second, filled_cells, cells);
    }

    // Grid of used cells for CLEAR algorithm
    std::vector<std::vector<bool>> used(rows, std::vector<bool> (columns, false));

    // Clear groups of ' ' characters
    for(int row = 0; row < rows; row++) {
        for(int column = 0; column < columns; column++) {
            if(grid[row][column] == ' ' && !used[row][column]) {
                std::vector<std::pair<int, int>> cords;

                // Run CLEAR from current cell
                CLEAR(row, column, used, cords, ' ');

                // Fill group of cells with '#'
                if(cords.size() <= 10)
                    for(auto &[x, y]: cords)
                        grid[x][y] = '#';
            }
        }
    }

    // Clear grid of used cells
    used = std::vector<std::vector<bool>> (rows, std::vector<bool> (columns, false));

    // Clear groups of '#' characters
    for(int row = 0; row < rows; row++) {
        for(int column = 0; column < columns; column++) {
            if(grid[row][column] == '#' && !used[row][column]) {
                std::vector<std::pair<int, int>> cords;

                // Run CLEAR from current cell
                CLEAR(row, column, used, cords, '#');

                // Fill group of cells with '#'
                if(cords.size() <= 10)
                    for(auto &[x, y]: cords)
                        grid[x][y] = ' ';
            }
        }
    }

    // Collecting list of all free cells
    std::vector<std::pair<int, int>> free_cells;
    for(int row = 0; row < rows; row++) {
        for(int column = 0; column < columns; column++) {
            if(grid[row][column] == ' ') {
                free_cells.push_back({row, column});
            }
        }
    }

    // Available logo for NPC
    char first_available_logo = 'A';

    // Placing NPC's on grid
    std::cout << "Number of NPC's: ";

    int number_of_npc{}; 
    std::cin >> number_of_npc;

    assert(number_of_npc >= 0 && number_of_npc <= 26);
    
    for(int it = 0; it < number_of_npc; it++) {
        if(free_cells.size() == 0)
            continue;

        // get random free cell
        int index = make_random(0, free_cells.size() - 1);
        std::pair<int, int> position = free_cells[index];

        // select available logo for NPC
        char logo = first_available_logo++;

        // Create thread for NPC processing
        std::thread npc_processing_thread(process_npc, position, logo);

        // Detach thread from main thread
        npc_processing_thread.detach();

        // remove free cell from list
        free_cells.erase(free_cells.begin() + index);
    }

    // Run thread that prints the grid
    std::thread grid_print_thread(print_grid);

    // Detach thread from main thread
    grid_print_thread.detach();

    // Infinite loop for main thread
    while(true) {
        // do nothing
    }

    // End of main
    std::cout << std::endl;
    return 0;
}
