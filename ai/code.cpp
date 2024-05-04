#include <iostream>
#include <thread>
#include <mutex>
#include <random>
#include <functional>
#include <set>
#include <map>
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

/* ------------------- */
/* Mutexes for threads */
/* ------------------- */

std::mutex print_grid_mutex; // probably don't need this, because we're 
                             // not changing any data in <grid_print_thread>
std::mutex process_npc_mutex;

/* ----------------- */
/* Threads functions */
/* ----------------- */

// Executed in <grid_print_thread> - prints grid 
void print_grid() {
    // I have no idea what this thing does
    printf("\n1b[2J");

    while(true) {
        printf("\x1b[H");

        for(auto line: grid) {
            // print grid cells
            for(auto cell: line)
                std::cout << cell;

            std::cout << "\n";
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

    // NPC processor
    while(true) {
        bool action = make_random(0, 1);

        lock_mutex(process_npc_mutex);
        // Make NPC action
        npc.make_action(action, grid);    
        unlock_mutex(process_npc_mutex);
        
        // Random movement speed
        std::this_thread::sleep_for(std::chrono::milliseconds(make_random(100, 1000)));
    }
}

int main() {
    int rows{}, columns{}; 
    
    // Input rows & columns (grid size)
    std::cout << "Rows: ";
    std::cin >> rows;

    std::cout << "Columns: ";
    std::cin >> columns;
    
    // Initialize grid
    grid = std::vector<std::vector<char>> (rows, std::vector<char> (columns, ' '));
    
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
                if(cords.size() <= 5)
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
                if(cords.size() <= 5)
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
    int number_of_npc = make_random(5, 10);
    for(int it = 0; it < number_of_npc; it++) {
        // get random free cell
        int index = make_random(0, free_cells.size() - 1);
        std::pair<int, int> position = free_cells[index];

        // select available logo for NPC
        char logo = first_available_logo++;

        // Create thread for NPC processing;
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
