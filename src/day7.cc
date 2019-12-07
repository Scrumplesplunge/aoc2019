import "util/check.h";
import <charconv>;  // bug
import <optional>;
import <span>;
import io;

constexpr int max_program_size = 1000;

enum class mode : unsigned char {
  position = 0,
  immediate = 1,
};

constexpr bool is_mode(int x) { return 0 <= x && x <= 1; }

enum class opcode : unsigned char {
  illegal = 0,
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

constexpr bool is_opcode(int x) { return (1 <= x && x <= 8) || x == 99; }

constexpr int op_size(opcode o) {
  switch (o) {
    case opcode::illegal: return -1;
    case opcode::add:
    case opcode::mul:
    case opcode::less_than:
    case opcode::equals:
      return 4;
    case opcode::jump_if_true:
    case opcode::jump_if_false:
      return 3;
    case opcode::input:
    case opcode::output:
      return 2;
    case opcode::halt:
      return 1;
  }
}

struct op {
  opcode code = opcode::illegal;
  mode params[3];
};

constexpr auto ops = [] {
  constexpr auto parse_op = [](int x) -> op {
    op result{};
    int code = x % 100;
    if (!is_opcode(code)) return {};
    result.code = opcode(code);
    x /= 100;
    for (int i = 0; i < 3; i++) {
      if (!is_mode(x % 10)) return {};
      result.params[i] = mode(x % 10);
      x /= 10;
    }
    // If this check fails, we have more parameter modes than any opcode uses.
    if (x != 0) return {};
    switch (result.code) {
      case opcode::add:
      case opcode::mul:
      case opcode::less_than:
      case opcode::equals:
        if (result.params[2] == mode::immediate) return {};
        break;
      case opcode::input:
        if (result.params[0] == mode::immediate) return {};
        break;
      case opcode::illegal:
      case opcode::halt:
      case opcode::jump_if_false:
      case opcode::jump_if_true:
      case opcode::output:
        break;
    }
    return result;
  };
  std::array<op, 9999> ops = {};
  for (int i = 0, n = ops.size(); i < n; i++) ops[i] = parse_op(i);
  return ops;
}();

op decode_op(int x) {
  check(0 <= x && x < (int)ops.size());
  return ops[x];
}

std::span<int> run(std::span<int> program,
                   std::span<const int> input, std::span<int> output) {
  int pc = 0;
  unsigned output_size = 0;
  while (true) {
    check(0 <= pc && pc < (int)program.size());
    const auto op = decode_op(program[pc]);
    check(pc + op_size(op.code) <= (int)program.size());
    auto get = [&](int param_index) {
      int x = program[pc + param_index + 1];
      switch (op.params[param_index]) {
        case mode::position: return program[x];
        case mode::immediate: return x;
      }
      assert(false);
    };
    auto put = [&](int param_index, int value) {
      int x = program[pc + param_index + 1];
      program[x] = value;
    };
    switch (op.code) {
      case opcode::illegal:
        std::cerr << "illegal instruction " << program[pc] << " at pc=" << pc
                  << "\n";
        std::abort();
      case opcode::add:
        put(2, get(0) + get(1));
        pc += 4;
        break;
      case opcode::mul:
        put(2, get(0) * get(1));
        pc += 4;
        break;
      case opcode::input:
        check(!input.empty());
        put(0, input.front());
        input = input.subspan(1);
        pc += 2;
        break;
      case opcode::output:
        check(output_size < output.size());
        output[output_size++] = get(0);
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
        return output.subspan(0, output_size);
      default:
        std::cerr << "illegal instruction " << program[pc] << " at pc=" << pc
                  << "\n";
        std::abort();
    }
  }
}

auto run_copy(std::span<const int> program, std::span<const int> input,
              std::span<int> output) {
  std::array<int, max_program_size> a;
  check(program.size() <= a.size());
  std::copy(std::begin(program), std::end(program), std::begin(a));
  return run(std::span<int>(a.data(), program.size()), input, output);
}

int part1(std::span<const int> program) {
  int output[1000];
  int max_signal = -1'000'000'000;
  std::array<int, 5> signal = {0, 1, 2, 3, 4}, best_signal = {};
  do {
    int value = 0;
    for (int phase : signal) {
      int input[] = {phase, value};
      auto out = run_copy(program, input, output);
      check(out.size() == 1);
      value = out[0];
    }
    if (value > max_signal) {
      max_signal = value;
      best_signal = signal;
    }
  } while (std::next_permutation(std::begin(signal), std::end(signal)));
  return max_signal;
}

int part2(std::span<const int> program) {
  int output[1000];
  int max_signal = -1'000'000'000;
  std::array<int, 5> signal = {5, 6, 7, 8, 9}, best_signal = {};
  do {
    std::array<int, max_program_size> programs[5];
    for (auto& x : programs) {
      std::copy(std::begin(program), std::end(program), std::begin(x));
    }
    int value = 0;
    for (int phase : signal) {
      int input[] = {phase, value};
      auto out = run_copy(program, input, output);
      check(out.size() == 1);
      value = out[0];
    }
    if (value > max_signal) {
      max_signal = value;
      best_signal = signal;
    }
  } while (std::next_permutation(std::begin(signal), std::end(signal)));
  return max_signal;
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
  // To benchmark: bin/day5 puzzles/day5benchmark.txt
  std::cout << "part1 " << part1(program) << "\n";
  //std::cout << "part1 " << run(program, 1) << '\n'
  //          << "part2 " << run(program, 5) << '\n';
}
