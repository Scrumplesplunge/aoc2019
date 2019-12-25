import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import <span>;
import <unordered_map>;  // bug
import intcode;
import util.circular_buffer;
import util.io;
import util.vec2;

constexpr char solution[] = R"(
north
north
north
take mutex
south
south
east
north
take loom
south
west
south
west
west
take sand
south
east
north
take wreath
south
west
north
north
east
east
)";

int part1(program::const_span source) {
  std::array<program::value_type, 5000> input_buffer, output_buffer;
  std::copy(std::begin(solution), std::end(solution), std::begin(input_buffer));
  program::const_span input(input_buffer.data(), sizeof(solution) - 1);
  auto output = program(source).run(input, output_buffer);
  std::array<char, 200> last_line_buffer;
  std::string_view last_line;
  {
    const auto first = output.data(), last = first + output.size();
    auto j = std::find(std::make_reverse_iterator(last),
                       std::make_reverse_iterator(first), '\n').base();
    auto i = std::find(std::make_reverse_iterator(j - 1),
                       std::make_reverse_iterator(first), '\n').base();
    std::copy(i, j, last_line_buffer.begin());
    last_line = std::string_view(last_line_buffer.data(), j - i);
  }
  scanner scanner(last_line);
  int code;
  (scanner >> exact("\"Oh, hello! You should be able to get in by typing ")
           >> code
           >> exact(" on the keypad at the main airlock.\"")).check_ok();
  return code;
}

int main(int argc, char* argv[]) {
  program::buffer program_buffer;
  const auto source = program::load(init(argc, argv), program_buffer);
  std::cout << "part1 " << part1(source) << '\n';
  std::cout << "part2 n/a\n";
}
