import "util/check.h";
import <charconv>;  // bug
import <optional>;  // bug
import <iostream>;
import <unordered_map>;
import util.io;
import util.vec2;

enum direction : char {
  down = 'D',
  left = 'L',
  right = 'R',
  up = 'U',
};

constexpr bool is_direction(char c) {
  return c == 'D' || c == 'L' || c == 'R' || c == 'U';
}

constexpr vec2i step(vec2i v, direction d) {
  switch (d) {
    case down: v.y++; break;
    case left: v.x--; break;
    case right: v.x++; break;
    case up: v.y--; break;
  }
  return v;
}

struct command {
  direction direction;
  int distance;
};

scanner& operator>>(scanner& s, command& c) {
  char d;
  if (s >> matches<is_direction>(d) >> c.distance) {
    c.direction = direction{d};
  }
  return s;
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  // Read the first wire.
  std::unordered_map<vec2i, int> wire;
  {
    vec2i position;
    int steps = 0;
    while (true) {
      command c;
      (scanner >> c).check_ok();
      for (int i = 0; i < c.distance; i++) {
        position = step(position, c.direction);
        wire.emplace(position, ++steps);
      }
      if (scanner.remaining().substr(0, 1) != ",") break;
      (scanner >> exact(",")).check_ok();
    }
  }
  std::cout << wire.size() << '\n';
  // Process the second wire.
  constexpr int max_distance = 999'999'999;
  vec2i closest = {max_distance, 0};
  int min_steps = max_distance;
  {
    vec2i position;
    int steps = 0;
    while (true) {
      command c;
      (scanner >> c).check_ok();
      for (int i = 0; i < c.distance; i++) {
        position = step(position, c.direction);
        steps++;
        if (auto i = wire.find(position); i != wire.end()) {
          if (position.manhattan_length() < closest.manhattan_length()) {
            closest = position;
          }
          if (steps + i->second < min_steps) {
            min_steps = steps + i->second;
          }
        }
      }
      if (scanner.remaining().substr(0, 1) != ",") break;
      (scanner >> exact(",")).check_ok();
    }
    check(scanner.done());
  }
  // Print the results.
  std::cout << "part1 " << closest.manhattan_length() << '\n'
            << "part2 " << min_steps << '\n';
}
