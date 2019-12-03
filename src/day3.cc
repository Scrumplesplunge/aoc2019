import "util/check.h";
import <iostream>;
import <string_view>;
import <charconv>;
import <optional>;
import <vector>;
import io;
import util.vec2;
import util.void_iterator;

constexpr auto is_direction(char c) {
  return c == 'R' || c == 'U' || c == 'L' || c == 'D';
}

template <typename T> constexpr T abs(T x) { return x < 0 ? -x : x; }

auto matches_direction(char& c) {
  return matches<is_direction>(c, "direction");
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  // Read the wires into two vectors.
  struct entry { vec2i position; int steps; };
  std::vector<entry> wires[2];
  for (auto& wire : wires) {
    vec2i position;
    int steps = 0;
    while (true) {
      char direction;
      int distance;
      (scanner >> matches_direction(direction) >> distance).check_ok();
      for (int i = 0; i < distance; i++) {
        switch (direction) {
          case 'R': position.x--; break;
          case 'U': position.y--; break;
          case 'L': position.x++; break;
          case 'D': position.y++; break;
        }
        wire.push_back({position, ++steps});
      }
      if (scanner.remaining().substr(0, 1) != ",") break;
      (scanner >> exact(",")).check_ok();
    }
    sort(begin(wire), end(wire), [](const auto& l, const auto& r) {
      return l.position == r.position ? l.steps < r.steps
                                      : l.position < r.position;
    });
  }
  (scanner >> scanner::end).check_ok();

  // Find all intersection points and take note of the points which meet the
  // criteria for the questions.
  constexpr int max_distance = 999'999'999;
  vec2i closest = {max_distance, 0};
  int min_steps = max_distance;
  auto compare = [&](const auto& a, const auto& b) {
    if (a.position == b.position) {
      check(a.position.manhattan_length() < max_distance);
      check(a.steps + b.steps < max_distance);
      if (a.position.manhattan_length() < closest.manhattan_length()) {
        closest = a.position;
      }
      if (a.steps + b.steps < min_steps) min_steps = a.steps + b.steps;
    }
    return a.position < b.position;
  };
  set_intersection(begin(wires[0]), end(wires[0]), begin(wires[1]),
                   end(wires[1]), void_iterator{}, compare);
  std::cout << "part1 " << abs(closest.x) + abs(closest.y) << '\n'
            << "part2 " << min_steps << '\n';
}
