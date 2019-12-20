import "util/check.h";
import <array>;
import <charconv>;  // bug
import <experimental/coroutine>;  // bug
import <iostream>;
import <map>;
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

constexpr int grid_width = 128, grid_height = 128, grid_levels = 128;
using grid = std::array<std::array<char, grid_width>, grid_height>;

auto line(std::string_view& out) {
  constexpr auto not_newline = [](char c) { return c != '\n'; };
  return sequence<+not_newline>(out, "line", match_leading_whitespace);
}

void getline(scanner& s, std::span<char> out) {
  std::string_view row;
  (s >> line(row) >> exact("\n", "newline")).check_ok();
  check(row.size() <= grid_width);
  std::copy(row.begin(), row.end(), out.begin());
}

using vec2s = vec2<short>;

// Find the positions of each portal with the given label.
generator<vec2s> find(const grid& grid, std::string_view label) {
  check(label.size() == 2);
  // Iterate over every cell searching for the first character of the label.
  for (short y = 0; y < grid_height; y++) {
    for (short x = 0; x < grid_width; x++) {
      if (grid[y][x] != label[0]) continue;
      // Trying both horizontally and vertically, check if the second character
      // matches and then find the grid cell which is next to the label.
      constexpr vec2s offsets[] = {{1, 0}, {0, 1}};
      for (vec2s offset : offsets) {
        auto p2 = vec2s{x, y} + offset;
        if (grid[p2.y][p2.x] == label[1]) {
          auto p0 = vec2s{x, y} - offset, p3 = vec2s{x, y} + 2 * offset;
          co_yield grid[p0.y][p0.x] == '.' ? p0 : p3;
        }
      }
    }
  }
}

// Find a label which has no counterpart.
vec2s find_one(const grid& grid, std::string_view label) {
  auto search = find(grid, label);
  search.next();
  check(!search.done());
  auto v = search.value();
  search.next();
  check(search.done());
  return v;
}

// Find a pair of matching labels (or crash if there are not exactly two).
std::pair<vec2s, vec2s> find_two(const grid& grid, std::string_view label) {
  auto search = find(grid, label);
  search.next();
  check(!search.done());
  auto a = search.value();
  search.next();
  check(!search.done());
  auto b = search.value();
  search.next();
  check(search.done());
  return {a, b};
}

struct gates {
  vec2s start, end;
  std::map<vec2s, vec2s> portals;
  vec2s box_min, box_max;
};

gates find_gates(const grid& grid) {
  gates output;
  output.start = find_one(grid, "AA");
  output.end = find_one(grid, "ZZ");
  // Find every portal.
  for (char a = 'A'; a <= 'Z'; a++) {
    for (char b = 'A'; b <= 'Z'; b++) {
      char label[] = {a, b, '\0'};
      if (label == std::string_view("AA") || label == std::string_view("ZZ")) {
        continue;
      }
      auto search = find(grid, label);
      search.next();
      if (search.done()) continue;  // Label not used.
      auto av = search.value();
      search.next();
      if (search.done()) continue;
      auto bv = search.value();
      search.next();
      check(search.done());
      output.portals.emplace(av, bv);
      output.portals.emplace(bv, av);
    }
  }
  // We will distinguish between inner and outer portals by whether or not they
  // are in contact with the bounding box edge.
  output.box_min = {grid_width, grid_height};
  output.box_max = {0, 0};
  for (const auto& [v, _] : output.portals) {
    output.box_min.x = std::min(output.box_min.x, v.x);
    output.box_min.y = std::min(output.box_min.y, v.y);
    output.box_max.x = std::max(output.box_max.x, v.x);
    output.box_max.y = std::max(output.box_max.y, v.y);
  }
  return output;
}

struct state {
  vec2s position;
  short level = 0;
  short distance = 0;
};

generator<state> part1_neighbours(
    const grid& grid, const gates& gates, state current) {
  constexpr vec2s offsets[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  for (vec2s offset : offsets) {
    vec2s v = current.position + offset;
    if (auto i = gates.portals.find(current.position);
        i != gates.portals.end() && is_alpha(grid[v.y][v.x])) {
      v = i->second;
    }
    if (grid[v.y][v.x] != '.') continue;
    co_yield state{v, 0, (short)(current.distance + 1)};
  }
}

generator<state> part2_neighbours(
    const grid& grid, const gates& gates, state current) {
  auto is_inner = [&](vec2s portal) {
    auto min = gates.box_min, max = gates.box_max;
    if (portal.x == min.x || portal.x == max.x) return false;
    if (portal.y == min.y || portal.y == max.y) return false;
    return true;
  };
  constexpr vec2s offsets[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  for (vec2s offset : offsets) {
    vec2s v = current.position + offset;
    short level = current.level;
    if (auto i = gates.portals.find(current.position);
        i != gates.portals.end() && is_alpha(grid[v.y][v.x])) {
      if (is_inner(current.position)) {
        v = i->second;
        level++;
      } else if (level > 0) {
        v = i->second;
        level--;
      } else {
        // Can't go to negative levels.
        continue;
      }
    }
    if (grid[v.y][v.x] != '.') continue;
    co_yield state{v, level, (short)(current.distance + 1)};
  }
}

template <auto neighbours>
int solve(const grid& input) {
  std::vector<state> frontier;
  constexpr auto by_distance = [](const auto& l, const auto& r) {
    return l.distance > r.distance;
  };
  auto empty = [&] { return frontier.empty(); };
  auto push = [&](state x) {
    frontier.push_back(x);
    std::push_heap(std::begin(frontier), std::end(frontier), by_distance);
  };
  auto pop = [&] {
    std::pop_heap(std::begin(frontier), std::end(frontier), by_distance);
    auto x = frontier.back();
    frontier.pop_back();
    return x;
  };
  const gates gates = find_gates(input);
  push({gates.start, 0, 0});
  grid explored[grid_levels] = {};
  while (true) {
    check(!empty());
    auto current = pop();
    if (current.position == gates.end && current.level == 0) {
      return current.distance;
    }
    auto [x, y] = current.position;
    if (explored[current.level][y][x]) continue;
    explored[current.level][y][x] = true;
    for (state next : neighbours(input, gates, current)) {
      check(next.level < grid_levels);
      if (!explored[next.level][next.position.y][next.position.x]) push(next);
    }
  }
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  grid grid;
  for (auto& row : grid) for (char& c : row) c = ' ';
  getline(scanner, grid[0]);
  int height = 1;
  while (!scanner.done()) {
    check(height < grid_height);
    getline(scanner, grid[height]);
    height++;
  }

  std::cout << "part1 " << solve<part1_neighbours>(grid) << "\n";
  std::cout << "part2 " << solve<part2_neighbours>(grid) << "\n";
}
