import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import <span>;
import <unordered_set>;
import util.io;
import intcode;
import util.aabb;
import util.vec2;

struct robot {
  enum direction_type { up, right, down, left };
  enum output_mode_type { output_color, output_direction };

  void run(std::span<const std::int64_t> source) {
    program brain(source);
    while (!brain.done()) {
      switch (brain.resume()) {
        case program::waiting_for_input: {
          brain.provide_input(white_panels.count(position));
          break;
        }
        case program::output: {
          auto output = brain.get_output();
          check(output == 0 || output == 1);
          switch (output_mode) {
            case output_color:
              painted_panels.insert(position);
              if (output) {
                white_panels.insert(position);
              } else {
                white_panels.erase(position);
              }
              output_mode = output_direction;
              break;
            case output_direction:
              if (output) {
                direction = static_cast<direction_type>((direction + 1) % 4);
              } else {
                direction = static_cast<direction_type>((direction + 3) % 4);
              }
              switch (direction) {
                case up: position.y--; break;
                case right: position.x++; break;
                case down: position.y++; break;
                case left: position.x--; break;
              }
              output_mode = output_color;
              break;
          }
          break;
        }
        case program::halt:
          break;
        default: {
          std::cerr << "Unexpected program state\n";
          std::abort();
        }
      }
    }
  }

  vec2i position;
  direction_type direction = up;
  output_mode_type output_mode = output_color;
  std::unordered_set<vec2i> white_panels, painted_panels;
};

int main(int argc, char* argv[]) {
  program::buffer buffer;
  const auto source = program::load(init(argc, argv), buffer);

  robot robot;
  robot.run(source);
  std::cout << "part1 " << robot.painted_panels.size() << '\n';

  robot = {};
  robot.white_panels = {robot.position};
  robot.run(source);
  std::cout << "part2:\n";
  aabb<int> bounds(robot.white_panels);
  for (int y = bounds.min.y; y <= bounds.max.y; y++) {
    for (int x = bounds.min.x; x <= bounds.max.x; x++) {
      if (robot.white_panels.count({x, y})) {
        std::cout << "\x1b[37;1m1\x1b[0m";
      } else {
        std::cout << "\x1b[34m0\x1b[0m";
      }
    }
    std::cout << '\n';
  }
}
