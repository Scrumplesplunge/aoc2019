import "util/check.h";
import <charconv>;  // bug
import <deque>;
import <optional>;  // bug
import <span>;
import <unordered_set>;
import <unordered_map>;  // bug
import <vector>;
import util.io;
import intcode;
import util.aabb;
import util.vec2;

struct state {
  int distance_travelled = 0;
  vec2i position;
  program brain;
};

constexpr bool by_distance(const state& left, const state& right) {
  return left.distance_travelled > right.distance_travelled;
}

enum move { failure = 0, success = 1, found_objective = 2 };
enum type : char { empty = '.', wall = '#', objective = '*' };
enum direction { north = 1, south = 2, west = 3, east = 4 };

constexpr vec2i move(vec2i v, direction d) {
  switch (d) {
    case north: return {v.x, v.y - 1};
    case south: return {v.x, v.y + 1};
    case west: return {v.x - 1, v.y};
    case east: return {v.x + 1, v.y};
  }
}

void show_state(const std::unordered_map<vec2i, type>& cells) {
  if (cells.empty()) return;
  vec2i min = cells.begin()->first, max = min;
  for (const auto [v, t] : cells) {
    min.x = std::min(min.x, v.x);
    min.y = std::min(min.y, v.y);
    max.x = std::max(max.x, v.x);
    max.y = std::max(max.y, v.y);
  }
  for (int y = min.y; y <= max.y; y++) {
    for (int x = min.x; x <= max.x; x++) {
      auto i = cells.find({x, y});
      std::cout << (i == cells.end() ? empty : i->second);
    }
    std::cout << '\n';
  }
}

int explore(state initial) {
  std::unordered_set<vec2i> explored;
  std::unordered_map<vec2i, type> cells;
  std::vector<state> work;
  check(initial.brain.resume() == program::waiting_for_input);
  initial.distance_travelled = 0;
  work.push_back(std::move(initial));

  int x = 0;
  int max_distance = 0;
  while (!work.empty()) {
    std::pop_heap(std::begin(work), std::end(work), by_distance);
    state current = std::move(work.back());
    if (++x % 1'000 == 0) {
      std::cout << "\rwork.size() = " << work.size() << "  explored.size() = "
                << explored.size() << "  distance = "
                << current.distance_travelled << "\n";
    }
    work.pop_back();
    if (!explored.insert(current.position).second) continue;
    max_distance = std::max(max_distance, current.distance_travelled);
    for (int i = 1; i <= 4; i++) {
      const vec2i position = move(current.position, direction(i));
      const auto distance = current.distance_travelled + 1;
      if (explored.contains(position)) continue;
      state next = {distance, position, current.brain};
      next.brain.provide_input(i);
      program::state s;
      for (s = next.brain.resume(); s == program::output;
           s = next.brain.resume()) {
        switch (auto x = next.brain.get_output(); x) {
          case failure: cells.emplace(position, wall); break;
          case success: cells.emplace(position, empty); break;
          case found_objective: cells.emplace(position, objective); break;
          default: std::cout << "ignoring " << x << '\n';
        }
      }
      check(s == program::waiting_for_input);
      if (cells.at(position) == empty) work.push_back(std::move(next));
      std::push_heap(std::begin(work), std::end(work), by_distance);
    }
  }
  return max_distance;
}

state find_objective(program::const_span source) {
  std::unordered_map<vec2i, type> cells;
  std::unordered_set<vec2i> explored;
  std::vector<state> work;
  {
    state initial = {0, {0, 0}, program(source)};
    check(initial.brain.resume() == program::waiting_for_input);
    work.push_back(std::move(initial));
  }

  int x = 0;
  while (true) {
    check(!work.empty());
    std::pop_heap(std::begin(work), std::end(work), by_distance);
    state current = std::move(work.back());
    if (++x % 1'000 == 0) {
      std::cout << "\rwork.size() = " << work.size() << "  explored.size() = "
                << explored.size() << "  distance = "
                << current.distance_travelled << "\n";
      show_state(cells);
    }
    work.pop_back();
    if (!explored.insert(current.position).second) continue;
    for (int i = 1; i <= 4; i++) {
      const vec2i position = move(current.position, direction(i));
      const auto distance = current.distance_travelled + 1;
      if (explored.contains(position)) continue;
      state next = {distance, position, current.brain};
      next.brain.provide_input(i);
      program::state s;
      for (s = next.brain.resume(); s == program::output;
           s = next.brain.resume()) {
        switch (auto x = next.brain.get_output(); x) {
          case failure: cells.emplace(position, wall); break;
          case success: cells.emplace(position, empty); break;
          case found_objective: return next; break;
          default: std::cout << "ignoring " << x << '\n';
        }
      }
      check(s == program::waiting_for_input);
      if (cells.at(position) == empty) work.push_back(std::move(next));
      std::push_heap(std::begin(work), std::end(work), by_distance);
    }
  }
}

int main(int argc, char* argv[]) {
  program::buffer buffer;
  const auto source = program::load(init(argc, argv), buffer);

  auto part1 = find_objective(source);
  std::cout << "part1 " << part1.distance_travelled << '\n';
  std::cout << "part2 " << explore(std::move(part1)) << '\n';
}
