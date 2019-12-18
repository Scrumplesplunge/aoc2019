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

struct state4 {
  std::array<vec2b, 4> positions = {};
  std::array<bool, 26> keys = {};
  int distance = 0;
  int cost = 0;
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

vec2b find(const grid& grid, char c) {
  for (int y = 0; y < grid_height; y++) {
    for (int x = 0; x < grid_width; x++) {
      if (grid[y][x] == c) return {x, y};
    }
  }
  std::cerr << "No '" << c << "' found in grid.\n";
  std::abort();
}

template <typename T>
int num_keys(const T& state) {
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

constexpr auto by_distance = [](const auto& l, const auto& r) {
  return l.distance > r.distance;
};
//
//int part1(const grid& grid) {
//  const int grid_keys = num_keys(grid);
//  std::cout << grid_keys << " keys total.\n";
//  std::set<std::pair<vec2b, std::array<bool, 26>>> explored;
//  std::priority_queue<state, std::vector<state>, decltype(by_distance)> frontier;
//  frontier.push({find_player(grid)});
//  while (true) {
//    check(!frontier.empty());
//    state current = frontier.top();
//    frontier.pop();
//    if (!explored.insert({current.position, current.keys}).second) continue;
//    //auto [x, y] = current.position;
//    //std::cout << "reached " << grid[y][x] << " ["
//    //          << (int)current.position.x << ", "
//    //          << (int)current.position.y << "]\n";
//    for (auto& next : explore_unlocked(grid, current)) {
//      if (num_keys(next) == grid_keys) return next.distance;
//      frontier.push(next);
//    }
//  }
//}

struct heuristic {
  heuristic(const grid& grid) {
    for (int i = 0; i < 26; i++) keys[i] = find(grid, 'a' + i);
  }

  //int operator()(const state4& state) const {
  //  int total = state.distance;
  //  for (int i = 0; i < 26; i++) {
  //    int min_additional = 0;
  //    if (state.keys[i]) {
  //      int best = 999'999;
  //      for (vec2b v : state.positions) {
  //        int distance = (keys[i] - v).manhattan_length();
  //        if (distance < best) best = distance;
  //      }
  //      for (vec2b v : keys) {
  //        int distance = (keys[i] - v).manhattan_length();
  //        if (distance > 0 && distance < best) best = distance;
  //      }
  //      if (best > min_additional) min_additional = best;
  //    }
  //    total += min_additional;
  //  }
  //  return total;
  //}

  int operator()(const state4& state) const {
    struct entry {
      int distance = 0;
      std::array<vec2b, 4> positions;
      std::array<bool, 26> keys;
    };
    constexpr auto by_distance = [](const entry& l, const entry& r) {
      return l.distance > r.distance;
    };
    std::priority_queue<entry, std::vector<entry>, decltype(by_distance)>
        frontier;
    frontier.push(entry{0, state.positions, state.keys});
    std::set<std::array<vec2b, 4>> explored;
    while (true) {
      check(!frontier.empty());
      auto current = frontier.top();
      frontier.pop();
      if (!explored.insert(current.positions).second) continue;
      if (num_keys(current) == 26) return state.distance + current.distance;
      for (int i = 0; i < 26; i++) {
        if (current.keys[i]) continue;
        for (int j = 0; j < 4; j++) {
          auto next = current;
          next.keys[i] = true;
          next.positions[j] = keys[i];
          next.distance += (keys[i] - current.positions[j]).manhattan_length();
          if (!explored.count(next.positions)) frontier.push(next);
        }
      }
    }
  }

  std::array<vec2b, 26> keys = {};
};

int part2(grid grid) {
  const int grid_keys = num_keys(grid);
  //std::cout << grid_keys << " keys total.\n";
  auto [x, y] = find(grid, '@');
  grid[y - 1][x - 1] = grid[y - 1][x + 1] = '@';
  grid[y + 1][x - 1] = grid[y + 1][x + 1] = '@';
  grid[y - 1][x] = grid[y][x] = grid[y + 1][x] = '#';
  grid[y][x - 1] = grid[y][x + 1] = '#';
  for (int y = 37; y <= 43; y++) {
    for (int x = 35; x <= 45; x++) {
      std::cout << grid[y][x];
    }
    std::cout << '\n';
  }
  std::set<std::tuple<std::array<vec2b, 4>, std::array<bool, 26>>> explored;
  //heuristic heuristic(grid);
  auto new_state4 = [&](const std::array<vec2b, 4>& positions,
                        const std::array<bool, 26>& keys,
                        int distance) {
    state4 state{positions, keys, distance};
    //state.cost = heuristic(state);
    return state;
  };
  //constexpr auto by_cost = [](const state4& l, const state4& r) {
  //  return l.cost > r.cost;
  //};
  std::priority_queue<state4, std::vector<state4>, decltype(by_distance)>
      frontier;
  frontier.push(new_state4(
      {{{x - 1, y - 1}, {x + 1, y - 1}, {x - 1, y + 1}, {x + 1, y + 1}}},
      {}, 0));
  long count = 0;
  while (true) {
    check(!frontier.empty());
    state4 current = frontier.top();
    frontier.pop();
    if (num_keys(current) == grid_keys) return current.distance;
    if (++count % 1'000 == 0) {
      std::cout << "\riterations: " << count << ", explored: "
                << explored.size() << ", distance: " << current.distance
                << "  " << std::flush;
//                << ", cost: " << heuristic(current) << "  " << std::flush;
    }
    if (!explored.emplace(current.positions, current.keys).second) {
      continue;
    }
    //bool found = false;
    //for (auto i = explored.lower_bound(
    //         {current.positions, keys_found, {}});
    //     i != explored.end() && std::get<0>(*i) == current.positions; ++i) {
    //  bool has_all = true;
    //  const auto& keys = std::get<2>(*i);
    //  for (int j = 0; j < 26; j++) {
    //    if (current.keys[j] && !keys[j]) has_all = false;
    //  }
    //  if (has_all) found = true;
    //}
    //if (found) continue;
    //explored.emplace(current.positions, keys_found, current.keys);
    //auto [x, y] = current.position;
    //std::cout << "reached " << grid[y][x] << " ["
    //          << (int)current.position.x << ", "
    //          << (int)current.position.y << "]\n";
    for (int i = 0; i < 4; i++) {
      state start = {current.positions[i], current.keys, current.distance};
      for (auto& next : explore_unlocked(grid, start)) {
        state4 combined = {current.positions, next.keys, next.distance};
        combined.positions[i] = next.position;
        //combined.cost = heuristic(combined);
        frontier.push(combined);
      }
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
  
  //std::cout << "part1 " << part1(grid) << "\n";
  //wa 4722 (probably garbage, says too high)
  //wa 1716 (too high)
  std::cout << "part2 " << part2(grid) << "\n";
}
