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

constexpr int grid_width = 128, grid_height = 128;
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

generator<vec2i> find(const grid& grid, char c) {
  for (int y = 0; y < grid_height; y++) {
    for (int x = 0; x < grid_width; x++) {
      if (grid[y][x] == c) co_yield vec2i{x, y};
    }
  }
}

std::optional<vec2i> find_from(
    const grid& grid, vec2i p, std::string_view label) {
  check(label.size() == 2);
  check(grid[p.y][p.x] == label[0]);
  constexpr vec2i offsets[] = {{1, 0}, {0, 1}};
  for (vec2i offset : offsets) {
    auto p2 = p + offset;
    if (grid[p2.y][p2.x] == label[1]) {
      auto p0 = p - offset, p3 = p + 2 * offset;
      return grid[p0.y][p0.x] == '.' ? p0 : p3;
    }
  }
  return std::nullopt;
}

generator<vec2i> find(const grid& grid, std::string_view label) {
  check(label.size() == 2);
  //std::cout << "searching for " << label << "..\n";
  for (auto candidate : find(grid, label[0])) {
    //std::cout << "trying (" << candidate.x << ", " << candidate.y << ")..\n";
    if (auto v = find_from(grid, candidate, label); v) {
      //std::cout << "match!\n";
      co_yield *v;
    }
  }
}

vec2i find_one(const grid& grid, std::string_view label) {
  auto search = find(grid, label);
  search.next();
  check(!search.done());
  auto v = search.value();
  search.next();
  check(search.done());
  return v;
}

std::pair<vec2i, vec2i> find_two(const grid& grid, std::string_view label) {
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

vec2i find_other(const grid& grid, std::string_view label, vec2i current) {
  auto [a, b] = find_two(grid, label);
  check(current == a || current == b);
  return a == current ? b : a;
}

struct gates {
  vec2i start, end;
  std::map<vec2i, vec2i> portals;
};

gates find_gates(const grid& grid) {
  gates output;
  output.start = find_one(grid, "AA");
  output.end = find_one(grid, "ZZ");
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
      if (search.done()) {
        //std::cout << "No matching end for " << label << "\n";
        continue;
      }
      auto bv = search.value();
      search.next();
      check(search.done());
      output.portals.emplace(av, bv);
      output.portals.emplace(bv, av);
    }
  }
  //std::cout << "start = (" << output.start.x << ", " << output.start.y << ")\n";
  //std::cout << "end = (" << output.end.x << ", " << output.end.y << ")\n";
  //for (const auto& [from, to] : output.portals) {
  //  std::cout << "portal (" << from.x << ", " << from.y << ") -> ("
  //            << to.x << ", " << to.y << ")\n";
  //}
  return output;
}

int part1(const grid& input) {
  struct entry {
    vec2i position;
    int distance = 0;
  };
  std::vector<entry> frontier;
  constexpr auto by_distance = [](const auto& l, const auto& r) {
    return l.distance > r.distance;
  };
  auto empty = [&] { return frontier.empty(); };
  auto push = [&](entry x) {
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
  push({gates.start, 0});
  grid explored = {};
  while (true) {
    check(!empty());
    auto current = pop();
    if (current.position == gates.end) return current.distance;
    auto [x, y] = current.position;
    if (explored[y][x]) continue;
    explored[y][x] = true;
    constexpr vec2i offsets[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (vec2i offset : offsets) {
      vec2i v = current.position + offset;
      if (auto i = gates.portals.find(current.position);
          i != gates.portals.end() && is_alpha(input[v.y][v.x])) {
        v = i->second;
      }
      if (explored[v.y][v.x] || input[v.y][v.x] != '.') continue;
      push({v, current.distance + 1});
    }
  }
}

int part2(const grid& input) {
  struct entry {
    vec2i position;
    int level = 0;
    int distance = 0;
  };
  std::vector<entry> frontier;
  constexpr auto by_distance = [](const auto& l, const auto& r) {
    return l.distance > r.distance;
  };
  auto empty = [&] { return frontier.empty(); };
  auto push = [&](entry x) {
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
  vec2i min_gate = {grid_width, grid_height}, max_gate = {0, 0};
  for (const auto& [v, _] : gates.portals) {
    min_gate.x = std::min(min_gate.x, v.x);
    min_gate.y = std::min(min_gate.y, v.y);
    max_gate.x = std::max(max_gate.x, v.x);
    max_gate.y = std::max(max_gate.y, v.y);
  }
  auto is_inner = [&](vec2i portal) {
    if (portal.x == min_gate.x || portal.x == max_gate.x) return false;
    if (portal.y == min_gate.y || portal.y == max_gate.y) return false;
    return true;
  };
  push({gates.start, 0, 0});
  std::set<std::pair<vec2i, int>> explored;
  while (true) {
    check(!empty());
    auto current = pop();
    if (current.position == gates.end && current.level == 0) {
      return current.distance;
    }
    if (!explored.emplace(current.position, current.level).second) continue;
    constexpr vec2i offsets[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (vec2i offset : offsets) {
      vec2i v = current.position + offset;
      int level = current.level;
      if (auto i = gates.portals.find(current.position);
          i != gates.portals.end() && is_alpha(input[v.y][v.x])) {
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
      if (explored.count({v, level}) || input[v.y][v.x] != '.') continue;
      push({v, level, current.distance + 1});
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
  //for (int y = 0; y < height; y++) {
  //  for (int x = 0; x < grid_width; x++) {
  //    std::cout << grid[y][x];
  //  }
  //  std::cout << "\n";
  //}

  std::cout << "part1 " << part1(grid) << "\n";
  std::cout << "part2 " << part2(grid) << "\n";
}
