import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import <span>;
import intcode;
import util.io;
import util.vec2;

// Jump when either there is a hole right in front of a robot or when there is a
// hole right before where we would land, and only jump if the place where we
// would land is not a hole.
constexpr std::string_view jump_program =
    "NOT A J\n"
    "NOT C T\n"
    "OR T J\n"
    "AND D J\n"
    "WALK\n";

// If any of the places between here and where we would land are holes, we will
// consider jumping. We will only go through with it if we can see that from
// where we land we will either be able to roll forwards or can successfully
// jump and land again.
constexpr std::string_view run_program =
    "NOT A T\n"
    "OR T J\n"
    "NOT B T\n"
    "OR T J\n"
    "NOT C T\n"
    "OR T J\n"
    "AND D J\n"
    "NOT E T\n"
    "AND H T\n"
    "OR E T\n"
    "AND T J\n"
    "RUN\n";

int run(program::const_span source, std::string_view springscript) {
  program brain(source);
  program::value_type input_buffer[100];
  std::copy(springscript.begin(), springscript.end(), std::begin(input_buffer));
  program::value_type output_buffer[1000];
  auto output = brain.run(
      program::const_span(input_buffer, springscript.size()), output_buffer);
  check(!output.empty());
  if (output.back() == '\n') {
    char temp[1001];
    std::copy(output.begin(), output.end(), temp);
    temp[output.size()] = '\0';
    std::cerr << temp;
    std::abort();
  } else {
    return output.back();
  }
}

int part1(program::const_span source) { return run(source, jump_program); }
int part2(program::const_span source) { return run(source, run_program); }

int main(int argc, char* argv[]) {
  program::buffer program_buffer;
  const auto source = program::load(init(argc, argv), program_buffer);

  std::cout << "part1 " << part1(source) << '\n';
  std::cout << "part2 " << part2(source) << '\n';
}
