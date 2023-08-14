import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;
import <span>;
import util.io;
import intcode;

program::value_type run(program::const_span source, program::value_type value) {
  program::value_type input[] = {value};
  program::value_type output[100];
  auto result = program(source).run(input, output);
  check(!result.empty());
  for (auto test_output : result.first(result.size() - 1)) {
    check(test_output == 0);
  }
  return result.back();
}

int main(int argc, char* argv[]) {
  program::buffer buffer;
  auto source = program::load(init(argc, argv), buffer);
  std::cout << "part1 " << run(source, 1) << '\n'
            << "part2 " << run(source, 5) << '\n';
}
