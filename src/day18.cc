import "util/check.h";
import <array>;
import <charconv>;  // bug
import <experimental/coroutine>;  // bug
import <iostream>;
import <optional>;  // bug
import <set>;
import <span>;
import <string>;
import <queue>;
import <vector>;
import util.coroutine;
import util.io;
import util.vec2;

using vec2b = vec2<signed char>;

constexpr int grid_width = 81, grid_height = 81;
using grid = std::array<std::array<char, grid_width>, grid_height>;

struct state {
  vec2b position = {};
  std::array<bool, 26> keys = {};
  int distance = 0;
};

constexpr bool is_key(char c) { return is_lower(c); }
constexpr bool is_door(char c) { return is_upper(c); }

generator<state> explore_unlocked(const grid& grid, const state& start) {
  std::set<vec2b> explored;
  std::queue<state> frontier;
  frontier.push(start);
  int found = 0;
  while (!frontier.empty()) {
    state current = frontier.front();
    frontier.pop();
    if (!explored.insert(current.position).second) continue;  // already visited
    auto [x, y] = current.position;
    if (is_key(grid[y][x]) && !current.keys[grid[y][x] - 'a']) {
      found++;
      current.keys[grid[y][x] - 'a'] = true;
      //std::cout << "found key " << grid[y][x] << " with distance "
      //          << current.distance << ".\n";
      co_yield current;
    }
    for (vec2b next : {vec2b{x, y - 1}, vec2b{x - 1, y},
                       vec2b{x + 1, y}, vec2b{x, y + 1}}) {
      auto cell = grid[next.y][next.x];
      if (cell == '#') continue;
      if (explored.count(next)) continue;
      state next_state{next, current.keys, current.distance + 1};
      //if (is_door(cell) && !start.keys[cell - 'A']) {
      //  std::cout << "found unopenable door " << cell << " with distance "
      //            << current.distance + 1 << ".\n";
      //}
      if (!is_door(cell) || current.keys[cell - 'A']) frontier.push(next_state);
    }
  }
  //std::cout << found << " reachable keys from ["
  //          << (int)start.position.x << ", " << (int)start.position.y
  //          << "].\n";
}

vec2b find_player(const grid& grid) {
  for (int y = 0; y < grid_height; y++) {
    for (int x = 0; x < grid_width; x++) {
      if (grid[y][x] == '@') return {x, y};
    }
  }
  std::cerr << "No player found in grid.\n";
  std::abort();
}

int num_keys(const state& state) {
  int keys = 0;
  for (bool k : state.keys) {
    if (k) keys++;
  }
  return keys;
}

int num_keys(const grid& grid) {
  int keys = 0;
  for (int y = 0; y < grid_height; y++) {
    for (int x = 0; x < grid_width; x++) {
      if (is_key(grid[y][x])) keys++;
    }
  }
  return keys;
}

constexpr auto by_distance = [](const state& l, const state& r) {
  return l.distance > r.distance;
};

int part1(const grid& grid) {
  const int grid_keys = num_keys(grid);
  std::cout << grid_keys << " keys total.\n";
  std::set<std::pair<vec2b, std::array<bool, 26>>> explored;
  std::priority_queue<state, std::vector<state>, decltype(by_distance)> frontier;
  frontier.push({find_player(grid)});
  while (true) {
    check(!frontier.empty());
    state current = frontier.top();
    frontier.pop();
    if (!explored.insert({current.position, current.keys}).second) continue;
    //auto [x, y] = current.position;
    //std::cout << "reached " << grid[y][x] << " ["
    //          << (int)current.position.x << ", "
    //          << (int)current.position.y << "]\n";
    for (auto& next : explore_unlocked(grid, current)) {
      if (num_keys(next) == grid_keys) return next.distance;
      frontier.push(next);
    }
  }
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::array<std::array<char, 81>, 81> grid;
  for (auto& row : grid) {
    (scanner >> row >> exact("\n")).check_ok();
  }
  (scanner >> scanner::end).check_ok();
  
  std::cout << "part1 " << part1(grid) << "\n";
}
