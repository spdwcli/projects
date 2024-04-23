#include <bits/stdc++.h>

using namespace std;

bool validate_move(string move) {
  for(auto &x: move)
    x = tolower(x);
  return (move == "rock") || (move == "paper") || (move == "scissors");
}

void get_user_input(string &move) {
  getline(cin, move);
}

string generate_random_move() {
  // random number generator
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> dist(0, 2);

  vector<string> moves = {"Rock", "Paper", "Scissors"};

  return moves[dist(gen)];
}

int compare_moves(string player_move, string system_move) {
  for(auto &x: player_move)
    x = tolower(x);
  for(auto &x: system_move)
    x = tolower(x);

  if(player_move == "rock") {
    if(system_move == "rock")
      return 0;
    if(system_move == "paper")
      return -1;
    if(system_move == "scissors")
      return 1;
  } else if(player_move == "paper") {
    if(system_move == "rock")
      return 1;
    if(system_move == "paper")
      return 0;
    if(system_move == "scissors")
      return -1;
  } else {
    if(system_move == "rock")
      return -1;
    if(system_move == "paper")
      return 1;
    if(system_move == "scissors")
      return 0;
  }
  return 2;
}

static int draws = 0;
static int wins = 0;
static int loses = 0;
static int total = 0;

void print_score() {
  cout << "Wins: " << wins << " | " << "Loses: " << loses << " | "
       << "Draws: " << draws << " | " << "Total: " <<  wins + loses + draws << "\n";
}

int main() {
  while(true) {
    cout << "---------------------------------------------\n";
    string player_move;
    cout << "Player move: ";
    get_user_input(player_move);
    
    bool valid = validate_move(player_move);
    if(!valid) {
      cout << "Invalid move.\n\n";
      continue;
    }
    
    string system_move = generate_random_move();
    cout << "System move: " << system_move << "\n";

    int duel_result = compare_moves(player_move, system_move);
    cout << "Match result: ";
    switch(duel_result) {
      case -1:
        cout << "You lose.\n";
        loses++;
        break;
      case 0:
        cout << "Draw.\n";
        draws++;
        break;
      case 1:
        cout << "Win.\n";
        wins++;
        break;
      default: 
        cout << "Error\n";
        return 0;
    }
    
    print_score();
    cout << "\n";
  }
  cout << endl;
  return 0;
}
