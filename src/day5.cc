import "util/check.h";
import <charconv>;  // bug
import <optional>;
import <span>;
import io;

int run(std::span<const int> program, std::optional<int> input) {
  std::array<int, 1000> a;
  copy(begin(program), end(program), begin(a));
  enum mode {
    position = 0,
    immediate = 1,
  };
  auto get = [&](int m, int x) {
    check(m == position || m == immediate);
    switch (m) {
      case position:
        check(0 <= x && x < (int)program.size());
        return a[x];
      case immediate:
        return x;
    }
    assert(false);
  };
  std::optional<int> final_output;
  int pc = 0;
  while (true) {
    check(0 <= pc && pc < (int)program.size());
    int op = a[pc];
    switch (op % 100) {
      case 1:
      case 2: {
        const auto left = get(op / 100 % 10, a[pc + 1]),
                   right = get(op / 1000, a[pc + 2]);
        a[a[pc + 3]] = op % 100 == 1 ? left + right : left * right;
        pc += 4;
        break;
      }
      case 3:
        check(input);
        a[a[pc + 1]] = *input;
        input = std::nullopt;
        pc += 2;
        break;
      case 4:
        final_output = get(op / 100, a[pc + 1]);
        pc += 2;
        break;
      case 5:
        if (get(op / 100 % 10, a[pc + 1])) {
          pc = get(op / 1000, a[pc + 2]);
        } else {
          pc += 3;
        }
        break;
      case 6:
        if (get(op / 100 % 10, a[pc + 1]) == 0) {
          pc = get(op / 1000, a[pc + 2]);
        } else {
          pc += 3;
        }
        break;
      case 7:
        a[a[pc + 3]] =
            get(op / 100 % 10, a[pc + 1]) < get(op / 1000 % 10, a[pc + 2]);
        pc += 4;
        break;
      case 8:
        a[a[pc + 3]] =
            get(op / 100 % 10, a[pc + 1]) == get(op / 1000 % 10, a[pc + 2]);
        pc += 4;
        break;
      case 99:
        check(final_output);
        return *final_output;
      default:
        std::cerr << "illegal instruction " << a[pc] << " at pc=" << pc << "\n";
        std::abort();
    }
  }
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::array<int, 1000> values = {};
  (scanner >> values[0]).check_ok();
  unsigned n = 1;
  while (!scanner.done()) {
    check(n < values.size());
    (scanner >> exact(",") >> values[n++]).check_ok();
  }
  const std::span program(begin(values), n);
  std::cout << "part1 " << run(program, 1) << '\n'
            << "part2 " << run(program, 5) << '\n';
}
