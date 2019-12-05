import "util/check.h";
import <charconv>;  // bug
import <optional>;
import <span>;
import io;

constexpr int max_program_size = 1000;

enum class mode {
  position = 0,
  immediate = 1,
};

enum class opcode {
  add = 1,
  mul = 2,
  input = 3,
  output = 4,
  jump_if_true = 5,
  jump_if_false = 6,
  less_than = 7,
  equals = 8,
  halt = 99,
};

struct op {
  opcode code;
  mode params[3];
};

op parse_op(int x) {
  op result;
  result.code = opcode(x % 100);
  x /= 100;
  for (int i = 0; i < 3; i++) {
    mode param = result.params[i] = mode(x % 10);
    // If this check fails, we have unsupported parameter modes.
    check(param == mode::position || param == mode::immediate);
    x /= 10;
  }
  // If this check fails, we have more parameter modes than any opcode uses.
  check(x == 0);
  return result;
}

int run(std::span<const int> program, std::optional<int> input) {
  std::array<int, max_program_size> a;
  copy(begin(program), end(program), begin(a));
  std::optional<int> final_output;
  int pc = 0;
  while (true) {
    check(0 <= pc && pc < (int)program.size());
    const auto op = parse_op(a[pc]);
    auto get = [&](int param_index) {
      check(pc + param_index + 1 < (int)program.size());
      int x = a[pc + param_index + 1];
      switch (op.params[param_index]) {
        case mode::position:
          check(0 <= x && x < (int)program.size());
          return a[x];
        case mode::immediate:
          return x;
      }
      assert(false);
    };
    auto put = [&](int param_index, int value) {
      check(pc + param_index + 1 < (int)program.size());
      int x = a[pc + param_index + 1];
      check(op.params[param_index] == mode::position);
      a[x] = value;
    };
    switch (op.code) {
      case opcode::add:
      case opcode::mul: {
        put(2, op.code == opcode::add ? get(0) + get(1) : get(0) * get(1));
        pc += 4;
        break;
      }
      case opcode::input:
        check(input);
        put(0, *input);
        input = std::nullopt;
        pc += 2;
        break;
      case opcode::output:
        final_output = get(0);
        pc += 2;
        break;
      case opcode::jump_if_true:
        pc = get(0) ? get(1) : pc + 3;
        break;
      case opcode::jump_if_false:
        pc = get(0) ? pc + 3 : get(1);
        break;
      case opcode::less_than:
        put(2, get(0) < get(1));
        pc += 4;
        break;
      case opcode::equals:
        put(2, get(0) == get(1));
        pc += 4;
        break;
      case opcode::halt:
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
  std::array<int, max_program_size> values = {};
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
