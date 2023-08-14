import "util/check.h";
import <array>;
import <charconv>;  // bug
import <deque>;
import <optional>;  // bug
import <span>;
import <unordered_set>;
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
enum direction { north = 1, south = 2, west = 3, east = 4 };

constexpr vec2i move(vec2i v, direction d) {
  switch (d) {
    case north: return {v.x, v.y - 1};
    case south: return {v.x, v.y + 1};
    case west: return {v.x - 1, v.y};
    case east: return {v.x + 1, v.y};
  }
}

state find_objective(program::const_span source) {
  std::unordered_set<vec2i> explored;
  std::vector<state> work;  // priority queue of states, by min distance.
  {
    state initial = {0, {0, 0}, program(source)};
    check(initial.brain.resume() == program::waiting_for_input);
    work.push_back(std::move(initial));
  }

  while (true) {
    check(!work.empty());
    std::pop_heap(std::begin(work), std::end(work), by_distance);
    state current = std::move(work.back());
    work.pop_back();
    if (!explored.insert(current.position).second) continue;
    for (int i = 1; i <= 4; i++) {
      const vec2i position = move(current.position, direction(i));
      const auto distance = current.distance_travelled + 1;
      if (explored.contains(position)) continue;  // already explored.
      state next = {distance, position, current.brain};
      next.brain.provide_input(i);
      check(next.brain.resume() == program::output);
      auto x = next.brain.get_output();
      check(0 <= x && x <= 2);
      if (x == found_objective) return next;
      check(next.brain.resume() == program::waiting_for_input);
      // If the response was failure then there is a wall and we can't explore
      // in that cell.
      if (x != failure) work.push_back(std::move(next));
      std::push_heap(std::begin(work), std::end(work), by_distance);
    }
  }
}

// Explore all reachable cells and establish which one is furthest from the
// oxygen generator.
int max_distance(state initial) {
  std::unordered_set<vec2i> explored;
  std::vector<state> work;
  check(initial.brain.resume() == program::waiting_for_input);
  initial.distance_travelled = 0;
  work.push_back(std::move(initial));

  int max_distance = 0;
  while (!work.empty()) {
    std::pop_heap(std::begin(work), std::end(work), by_distance);
    state current = std::move(work.back());
    work.pop_back();
    if (!explored.insert(current.position).second) continue;
    max_distance = std::max(max_distance, current.distance_travelled);
    for (int i = 1; i <= 4; i++) {
      const vec2i position = move(current.position, direction(i));
      const auto distance = current.distance_travelled + 1;
      if (explored.contains(position)) continue;
      state next = {distance, position, current.brain};
      next.brain.provide_input(i);
      check(next.brain.resume() == program::output);
      auto x = next.brain.get_output();
      check(0 <= x && x <= 2);
      check(next.brain.resume() == program::waiting_for_input);
      if (x != failure) work.push_back(std::move(next));
      std::push_heap(std::begin(work), std::end(work), by_distance);
    }
  }
  return max_distance;
}

int main(int argc, char* argv[]) {
  program::buffer buffer;
  const auto source = program::load(init(argc, argv), buffer);

  auto part1 = find_objective(source);
  std::cout << "part1 " << part1.distance_travelled << '\n';
  std::cout << "part2 " << max_distance(std::move(part1)) << '\n';
}
