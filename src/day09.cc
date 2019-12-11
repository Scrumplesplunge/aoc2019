import "util/check.h";
import <charconv>;  // bug
import <optional>;  // bug
import <span>;
import <unordered_map>;
import util.io;
import intcode;

int main(int argc, char* argv[]) {
  program::buffer buffer;
  auto source = program::load(init(argc, argv), buffer);

  program::value_type output_buffer[100];

  program::value_type part1_input[] = {1};
  auto part1 = program(source).run(part1_input, output_buffer);
  check(!part1.empty());
  for (auto x : part1.first(part1.size() - 1)) check(x == 0);
  std::cout << "part1 " << part1.back() << '\n';

  program::value_type part2_input[] = {2};
  auto part2 = program(source).run(part2_input, output_buffer);
  check(part2.size() == 1);
  std::cout << "part2 " << part2.back() << '\n';
}
