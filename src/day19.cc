import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import <span>;
import <unordered_map>;
import intcode;
import util.io;
import util.vec2;

bool is_pulled(program program, vec2i position) {
  program::value_type input[] = {position.x, position.y}, output_buffer[1];
  auto output = program.run(input, output_buffer);
  check(!output.empty());
  return output[0];
}

int part1(const program& program) {
  int count = 0;
  for (int y = 0; y < 50; y++) {
    int x = 0;
    while (x < 50 && !is_pulled(program, {x, y})) x++;
    const int start = x;
    if (x < 50) {
      do { x++; } while (is_pulled(program, {x, y}));
    }
    count += x - start;
  }
  return count;
}

vec2i start_position(const program& program) {
  for (int y = 0; y < 100; y++) {
    if (is_pulled(program, {99, y})) return {99, y};
  }
  for (int x = 99; x >= 0; x--) {
    if (is_pulled(program, {x, 99})) return {x, 99};
  }
  std::cerr << "Can't find start position.\n";
  std::abort();
}

vec2i next(const program& program, vec2i p) {
  for (vec2i v : {p + vec2i(1, 0), p + vec2i(1, 1), p + vec2i(0, 1)}) {
    if (is_pulled(program, v)) return v;
  }
  std::cerr << "Can't find next position from (" << p.x << ", " << p.y
            << ").\n";
  std::abort();
}

int part2(const program& program) {
  vec2i position = start_position(program);
  while (true) {
    position = next(program, position);
    if (position.x < 99 || position.y < 99) continue;
    auto bottom_right = position + vec2i(-99, 99);
    if (is_pulled(program, bottom_right)) break;
  }
  vec2i top_left = position + vec2i(-99, 0);
  return top_left.x * 10'000 + top_left.y;
}

int main(int argc, char* argv[]) {
  program::buffer program_buffer;
  const auto source = program::load(init(argc, argv), program_buffer);

  std::cout << "part1 " << part1(program(source)) << '\n';
  std::cout << "part2 " << part2(program(source)) << '\n';
}
