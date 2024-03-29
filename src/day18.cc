import "util/check.h";
import <array>;
import <charconv>;  // bug
import <coroutine>;  // bug
import <iostream>;
import <numeric>;
import <optional>;  // bug
import <set>;
import <span>;
import <string>;
import <queue>;
import <vector>;
import util.circular_buffer;
import util.coroutine;
import util.io;
import util.vec2;

using vec2b = vec2<signed char>;

constexpr int grid_width = 81, grid_height = 81;
using grid = std::array<std::array<char, grid_width>, grid_height>;

struct key_set {
  unsigned values = 0;
  bool operator[](int index) { return values & (1 << index); }
  void set(int index) { values |= 1 << index; }
  int count() const {
    unsigned x = values;
    x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
    x = (x & 0x00FF00FF) + ((x >> 8) & 0x00FF00FF);
    x = (x & 0x0000FFFF) + ((x >> 16) & 0x0000FFFF);
    return x;
  }
  friend constexpr bool operator<(const key_set& l, const key_set& r) {
    return l.values < r.values;
  }
};

struct state {
  vec2b position = {};
  key_set keys = {};
  int distance = 0;
};

constexpr bool is_key(char c) { return is_lower(c); }
constexpr bool is_door(char c) { return is_upper(c); }

generator<state> explore_unlocked(const grid& input, const state& start) {
  grid explored = {};
  circular_buffer<state, 1000> frontier;
  frontier.push(start);
  while (!frontier.empty()) {
    state current = frontier.front();
    frontier.pop();
    auto [x, y] = current.position;
    if (explored[y][x]) continue;
    explored[y][x] = true;
    if (is_key(input[y][x]) && !current.keys[input[y][x] - 'a']) {
      current.keys.set(input[y][x] - 'a');
      co_yield current;
    }
    check(frontier.size() < 995);
    for (auto offset : {vec2b{0, -1}, vec2b{-1, 0}, vec2b{1, 0}, vec2b{0, 1}}) {
      auto obstruction = input[y + offset.y][x + offset.x];
      if (obstruction == '#') continue;
      if (is_door(obstruction) && !current.keys[obstruction - 'A']) continue;
      auto next = current.position + 2 * offset;
      if (explored[next.y][next.x]) continue;
      state next_state{next, current.keys, current.distance + 2};
      frontier.push(next_state);
    }
  }
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

template <typename T>
using distance_queue =
    std::priority_queue<T, std::vector<T>, decltype(by_distance)>;

int part1(const grid& grid) {
  const int grid_keys = num_keys(grid);
  std::set<std::pair<vec2b, key_set>> explored;
  distance_queue<state> frontier;
  auto origin = find(grid, '@');
  for (auto offset : {vec2b{-1, -1}, vec2b{1, -1}, vec2b{-1, 1}, vec2b{1, 1}}) {
    frontier.push({origin + offset, {}, 2});
  }
  while (true) {
    check(!frontier.empty());
    state current = frontier.top();
    frontier.pop();
    if (!explored.insert({current.position, current.keys}).second) continue;
    for (auto& next : explore_unlocked(grid, current)) {
      if (next.keys.count() == grid_keys) return next.distance;
      frontier.push(next);
    }
  }
}

struct state4 {
  std::array<vec2b, 4> positions = {};
  key_set keys = {};
  int distance = 0;
  int cost = 0;
};

int part2(grid grid) {
  const int grid_keys = num_keys(grid);
  auto [x, y] = find(grid, '@');
  grid[y - 1][x - 1] = grid[y - 1][x + 1] = '@';
  grid[y + 1][x - 1] = grid[y + 1][x + 1] = '@';
  grid[y - 1][x] = grid[y][x] = grid[y + 1][x] = '#';
  grid[y][x - 1] = grid[y][x + 1] = '#';
  std::set<std::tuple<std::array<vec2b, 4>, key_set>> explored;
  distance_queue<state4> frontier;
  frontier.push({{{{x - 1, y - 1}, {x + 1, y - 1},
                   {x - 1, y + 1}, {x + 1, y + 1}}}, {}, 0});
  while (true) {
    check(!frontier.empty());
    state4 current = frontier.top();
    frontier.pop();
    if (current.keys.count() == grid_keys) return current.distance;
    if (!explored.emplace(current.positions, current.keys).second) continue;
    for (int i = 0; i < 4; i++) {
      state start = {current.positions[i], current.keys, current.distance};
      for (auto& next : explore_unlocked(grid, start)) {
        state4 combined = {current.positions, next.keys, next.distance};
        combined.positions[i] = next.position;
        frontier.push(combined);
      }
    }
  }
}

constexpr bool is_cell(char c) {
  return is_alpha(c) || c == '.' || c == '#' || c == '@';
}

auto match_cell(char& c) {
  return matches<is_cell>(c, "one of [A-Za-z.#@]", match_leading_whitespace);
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  grid grid;
  for (auto& row : grid) {
    for (auto& cell : row) (scanner >> match_cell(cell)).check_ok();
    (scanner >> exact("\n", "newline")).check_ok();
  }
  (scanner >> scanner::end).check_ok();

  // The fast scanning code assumes all passageways and keys have odd values in
  // their coordinates.
  for (int y = 0; y < grid_height; y++) {
    for (int x = 0; x < grid_width; x++) {
      if (x == 40 && y == 40) continue;
      auto cell = grid[y][x];
      if (x == 0 || y == 0 || x == grid_width - 1 || y == grid_height - 1) {
        // All boundaries must be walls.
        check(cell == '#');
      } else if (x % 2 == 1 && y % 2 == 1) {
        // Cells with two odd coordinates must be open passageway.
        check(cell == '.' || is_key(cell));
      } else if (x % 2 == 1 || y % 2 == 1) {
        // Cells with a single odd coordinate must not be keys.
        check(!is_key(cell));
      } else {
        // Cells with no odd coordinates must be walls.
        check(cell == '#' || is_door(cell));
      }
    }
  }

  std::cout << "part1 " << part1(grid) << "\n";
  std::cout << "part2 " << part2(grid) << "\n";
}
