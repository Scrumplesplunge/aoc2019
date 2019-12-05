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
  std::optional<int> final_output;
  int pc = 0;
  while (true) {
    check(0 <= pc && pc < (int)program.size());
    int op = a[pc];
    opcode code = opcode(op % 100);
    mode params[3];
    auto get = [&](int param_index) {
      check(pc + param_index + 1 < (int)program.size());
      int x = a[pc + param_index + 1];
      switch (params[param_index]) {
        case position:
          check(0 <= x && x < (int)program.size());
          return a[x];
        case immediate:
          return x;
      }
      assert(false);
    };
    auto put = [&](int param_index, int value) {
      check(pc + param_index + 1 < (int)program.size());
      int x = a[pc + param_index + 1];
      check(params[param_index] == position);
      a[x] = value;
    };
    int x = op / 100;
    for (int i = 0; i < 3; i++) {
      params[i] = mode(x % 10);
      // If this check fails, we have unsupported parameter modes.
      check(params[i] == position || params[i] == immediate);
      x /= 10;
    }
    // If this check fails, we have more parameter modes than any opcode uses.
    check(x == 0);
    switch (code) {
      case opcode::add:
      case opcode::mul: {
        put(2, code == opcode::add ? get(0) + get(1) : get(0) * get(1));
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
