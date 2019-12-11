import "util/check.h";
import <charconv>;  // bug
import <optional>;  // bug
import <span>;
import <unordered_map>;
import <unordered_set>;
import io;
import util.aabb;
import util.vec2;

constexpr int max_program_size = 1000;

enum class mode : unsigned char {
  position = 0,
  immediate = 1,
  relative = 2,
};

constexpr bool is_mode(int x) { return 0 <= x && x <= 2; }

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
  adjust_relative_base = 9,
  halt = 99,
};

constexpr bool is_opcode(int x) { return (1 <= x && x <= 9) || x == 99; }

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
    case opcode::adjust_relative_base:
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
      case opcode::adjust_relative_base:
      case opcode::illegal:
      case opcode::halt:
      case opcode::jump_if_false:
      case opcode::jump_if_true:
      case opcode::output:
        break;
    }
    return result;
  };
  std::array<op, 29999> ops = {};
  for (int i = 0, n = ops.size(); i < n; i++) ops[i] = parse_op(i);
  return ops;
}();

op decode_op(std::int64_t x) {
  check(0 <= x && x < (int)ops.size());
  return ops[x];
}

class program {
 public:
  program() = default;

  explicit program(std::span<const std::int64_t> source) {
    for (std::int64_t i = 0, n = source.size(); i < n; i++) {
      memory_.emplace(i, source[i]);
    }
  }

  enum state {
    ready,
    waiting_for_input,
    output,
    halt,
  };

  bool done() const { return state_ == halt; }

  void provide_input(std::int64_t x) {
    check(state_ == waiting_for_input);
    state_ = ready;
    memory_[input_address_] = x;
    pc_ += 2;
  }

  std::int64_t get_output() {
    check(state_ == output);
    state_ = ready;
    pc_ += 2;
    return output_;
  }

  state resume() {
    check(state_ == ready);
    while (true) {
      check(0 <= pc_ && pc_ < (int)memory_.size());
      const auto op = decode_op(memory_[pc_]);
      check(pc_ + op_size(op.code) <= (int)memory_.size());
      auto get = [&](int param_index) {
        std::int64_t x = memory_[pc_ + param_index + 1];
        switch (op.params[param_index]) {
          case mode::position: return memory_[x];
          case mode::immediate: return x;
          case mode::relative: return memory_[relative_base_ + x];
        }
        assert(false);
      };
      auto put = [&](int param_index, std::int64_t value) {
        std::int64_t x = memory_[pc_ + param_index + 1];
        switch (op.params[param_index]) {
          case mode::position: memory_[x] = value; return;
          case mode::immediate: std::abort();
          case mode::relative: memory_[relative_base_ + x] = value; return;
        }
      };
      switch (op.code) {
        case opcode::illegal:
          std::cerr << "illegal instruction " << memory_[pc_]
                    << " at pc_=" << pc_ << "\n";
          std::abort();
        case opcode::add:
          put(2, get(0) + get(1));
          pc_ += 4;
          break;
        case opcode::mul:
          put(2, get(0) * get(1));
          pc_ += 4;
          break;
        case opcode::input:
          switch (op.params[0]) {
            case mode::position:
              input_address_ = memory_[pc_ + 1];
              break;
            case mode::immediate:
              std::abort();
            case mode::relative:
              input_address_ = relative_base_ + memory_[pc_ + 1];
              break;
          }
          return state_ = waiting_for_input;
        case opcode::output:
          output_ = get(0);
          return state_ = output;
        case opcode::jump_if_true:
          pc_ = get(0) ? get(1) : pc_ + 3;
          break;
        case opcode::jump_if_false:
          pc_ = get(0) ? pc_ + 3 : get(1);
          break;
        case opcode::less_than:
          put(2, get(0) < get(1));
          pc_ += 4;
          break;
        case opcode::equals:
          put(2, get(0) == get(1));
          pc_ += 4;
          break;
        case opcode::adjust_relative_base:
          relative_base_ += get(0);
          pc_ += 2;
          break;
        case opcode::halt:
          return state_ = halt;
        default:
          std::cerr << "illegal instruction " << memory_[pc_]
                    << " at pc_=" << pc_ << "\n";
          std::abort();
      }
    }
  }

  std::span<std::int64_t> run(std::span<const std::int64_t> input,
                              std::span<std::int64_t> output) {
    unsigned output_size = 0;
    while (true) {
      switch (resume()) {
        case state::ready:
          continue;
        case state::waiting_for_input:
          check(!input.empty());
          provide_input(input.front());
          input = input.subspan(1);
          break;
        case state::output:
          check(output_size < output.size());
          output[output_size++] = get_output();
          break;
        case state::halt:
          return output.subspan(0, output_size);
      }
    }
  }

 private:
  state state_ = ready;
  std::int64_t pc_ = 0, input_address_ = 0, output_ = 0, relative_base_ = 0;
  std::unordered_map<std::int64_t, std::int64_t> memory_;
};

struct robot {
  enum direction_type { up, right, down, left };
  enum output_mode_type { output_color, output_direction };

  void run(std::span<const std::int64_t> source) {
    program brain(source);
    while (!brain.done()) {
      switch (brain.resume()) {
        case program::waiting_for_input: {
          brain.provide_input(white_panels.count(position));
          break;
        }
        case program::output: {
          auto output = brain.get_output();
          check(output == 0 || output == 1);
          switch (output_mode) {
            case output_color:
              painted_panels.insert(position);
              if (output) {
                white_panels.insert(position);
              } else {
                white_panels.erase(position);
              }
              output_mode = output_direction;
              break;
            case output_direction:
              if (output) {
                direction = static_cast<direction_type>((direction + 1) % 4);
              } else {
                direction = static_cast<direction_type>((direction + 3) % 4);
              }
              switch (direction) {
                case up: position.y--; break;
                case right: position.x++; break;
                case down: position.y++; break;
                case left: position.x--; break;
              }
              output_mode = output_color;
              break;
          }
          break;
        }
        case program::halt:
          break;
        default: {
          std::cerr << "Unexpected program state\n";
          std::abort();
        }
      }
    }
  }

  vec2i position;
  direction_type direction = up;
  output_mode_type output_mode = output_color;
  std::unordered_set<vec2i> white_panels, painted_panels;
};

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::array<std::int64_t, max_program_size> values = {};
  (scanner >> values[0]).check_ok();
  unsigned n = 1;
  while (!scanner.done()) {
    check(n < values.size());
    (scanner >> exact(",") >> values[n++]).check_ok();
  }
  const std::span source(begin(values), n);

  robot robot;
  robot.run(source);
  std::cout << "part1 " << robot.painted_panels.size() << '\n';

  robot = {};
  robot.white_panels = {robot.position};
  robot.run(source);
  std::cout << "part2:\n";
  aabb<int> bounds(robot.white_panels);
  for (int y = bounds.min.y; y <= bounds.max.y; y++) {
    for (int x = bounds.min.x; x <= bounds.max.x; x++) {
      if (robot.white_panels.count({x, y})) {
        std::cout << "\x1b[37;1m1\x1b[0m";
      } else {
        std::cout << "\x1b[34m0\x1b[0m";
      }
    }
    std::cout << '\n';
  }
}
