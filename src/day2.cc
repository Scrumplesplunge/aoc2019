import "util/check.h";
import <array>;
import <charconv>;  // BUG
import <optional>;  // BUG
import <span>;
import io;

int run(std::span<const int> program, int x, int y) {
  std::array<int, 256> a;
  copy(begin(program), end(program), begin(a));
  a[1] = x, a[2] = y;
  for (int pc = 0; a[pc] != 99; pc += 4) {
    check(a[pc] == 1 || a[pc] == 2);
    const auto left = a[a[pc + 1]], right = a[a[pc + 2]];
    a[a[pc + 3]] = a[pc] == 1 ? left + right : left * right;
  }
  return a[0];
}

int bruteforce(std::span<const int> program) {
  for (int noun = 0, n = program.size(); noun < n; noun++) {
    for (int verb = 0; verb < n; verb++) {
      if (run(program, noun, verb) == 1969'07'20) return 100 * noun + verb;
    }
  }
  return -1;
}

int main(int argc, char* argv[]) {
  scanner input(init(argc, argv));
  std::array<int, 256> values = {};
  (input >> values[0]).check_ok();
  int n = 1;
  while (!input.done()) (input >> exact{","} >> values[n++]).check_ok();
  const std::span program(begin(values), n);
  std::cout << "part1 " << run(program, 12, 2) << '\n'
            << "part2 " << bruteforce(program) << '\n';
}
