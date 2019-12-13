import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import <span>;
import util.io;
import intcode;
import util.aabb;
import util.vec2;

constexpr auto sign(int x) { return x < 0 ? -1 : x > 0 ? 1 : 0; }

struct arcade {
  enum tile { empty, wall, block, paddle, ball };
  enum output_mode_type { output_x, output_y, output_tile };

  static constexpr int width = 45, height = 24;

  void run(std::span<const std::int64_t> source) {
    program brain(source);
    while (!brain.done()) {
      switch (brain.resume()) {
        case program::waiting_for_input: {
          // Always put the paddle under the ball.
          brain.provide_input(sign(ball_position.x - paddle_position.x));
          break;
        }
        case program::output: {
          auto output = brain.get_output();
          switch (output_mode) {
            case output_x: input.x = output; output_mode = output_y; break;
            case output_y: input.y = output; output_mode = output_tile; break;
            case output_tile:
              if (input.x == -1 && input.y == 0) {
                score = output;
              } else {
                check(0 <= input.x && input.x < width);
                check(0 <= input.y && input.y < height);
                check(empty <= output && output <= ball);
                if (output == ball) ball_position = input;
                if (output == paddle) paddle_position = input;
                tiles[input.y][input.x] = tile(output);
              }
              output_mode = output_x;
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

  int score = 0;
  output_mode_type output_mode = output_x;
  vec2i input, ball_position, paddle_position;
  std::array<std::array<tile, width>, height> tiles = {};
};

int main(int argc, char* argv[]) {
  program::buffer buffer;
  const auto source = program::load(init(argc, argv), buffer);

  arcade machine;
  machine.run(source);
  int blocks = 0;
  for (auto& row : machine.tiles) {
    for (auto x : row) {
      if (x == arcade::block) blocks++;
    }
  }
  std::cout << "part1 " << blocks << '\n';

  buffer[0] = 2;
  machine = {};
  machine.run(source);
  std::cout << "part2 " << machine.score << '\n';
}
