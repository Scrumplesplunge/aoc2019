import <fstream>;
import <iostream>;
import <span>;
import <unordered_map>;
import <vector>;

#include <cassert>

enum opcode {
  add,  // arg is how much to add.
  move,  // arg is how far to move.
  in,  // arg is unused.
  out,  // arg is unused.
  jump_if_zero,  // arg is a relative offset.
  jump_unless_zero,  // arg is a relative offset.
  halt,  // arg is unused.
  invalid,  // cannot be executed.
};

template <typename Tag>
struct op {
  unsigned short code : 3;
  short arg : 13;
};
static_assert(sizeof(op<int>) == 2);
static_assert(op<int>{0, -1}.arg == -1);

// op<jump_id> has ids for jumps rather than relative offsets.
struct jump_id {};
std::vector<op<jump_id>> parse(std::string_view filename,
                               std::string_view source) {
  // Read all instructions into an initial buffer. At this stage, jumps will
  // have indices rather than relative offsets. Those will be replaced at the
  // end.
  std::vector<op<jump_id>> output;
  short next_loop_id = 0;
  struct loop { int line, column; short id; };
  std::vector<loop> loops;
  int line = 1, column = 1;
  for (char c : source) {
    // Compile the instruction (if it is one).
    switch (c) {
      case '-': output.push_back(op<jump_id>{add, -1}); break;
      case '+': output.push_back(op<jump_id>{add, 1}); break;
      case '<': output.push_back(op<jump_id>{move, -1}); break;
      case '>': output.push_back(op<jump_id>{move, 1}); break;
      case ',': output.push_back(op<jump_id>{in, 0}); break;
      case '.': output.push_back(op<jump_id>{out, 0}); break;
      case '[':
        loops.push_back({line, column, next_loop_id});
        output.push_back(op<jump_id>{jump_if_zero, next_loop_id});
        next_loop_id++;
        break;
      case ']':
        if (loops.empty()) {
          std::cerr << filename << ':' << line << ':' << column
                    << ": unmatched ']'.\n";
          std::exit(1);
        }
        output.push_back(op<jump_id>{jump_unless_zero, loops.back().id});
        loops.pop_back();
        break;
    }
    // Advance the line tracker.
    if (c == '\n') {
      line++;
      column = 1;
    } else {
      column++;
    }
  }
  if (!loops.empty()) {
    std::cerr << filename << ':' << loops.back().line << ':'
              << loops.back().column << ": unmatched '['.\n";
    std::exit(1);
  }
  output.push_back(op<jump_id>{halt, 0});
  return output;
}

std::vector<op<jump_id>> combine(std::span<const op<jump_id>> input) {
  if (input.empty()) return {};
  std::vector<op<jump_id>> output;
  for (auto in : input) {
    switch (in.code) {
      case add:
      case move:
        if (!output.empty() && output.back().code == in.code) {
          output.back().arg += in.arg;
          break;
        }
      default:
        output.push_back(op<jump_id>{in.code, in.arg});
        break;
    }
  }
  return output;
}

// op<jump_relative> has relative offsets for jumps.
struct jump_relative {};
std::vector<op<jump_relative>> link_jumps(std::span<const op<jump_id>> input) {
  // Map from jump_id to the instruction offset in input.
  std::unordered_map<short, short> loop_start, loop_end;
  for (int i = 0, n = input.size(); i < n; i++) {
    switch (input[i].code) {
      case jump_if_zero: loop_start.emplace(input[i].arg, i); break;
      case jump_unless_zero: loop_end.emplace(input[i].arg, i); break;
    }
  }
  std::vector<op<jump_relative>> output;
  for (auto in : input) {
    op<jump_relative> out;
    out.code = in.code;
    switch (in.code) {
      case jump_if_zero:
        out.arg = loop_end.at(in.arg) - loop_start.at(in.arg);
        break;
      case jump_unless_zero:
        out.arg = loop_start.at(in.arg) - loop_end.at(in.arg);
        break;
      default: out.arg = in.arg; break;
    }
    output.push_back(out);
  }
  return output;
}

void run(std::span<const op<jump_relative>> program) {
  constexpr int num_cells = 16384;
  int cells[num_cells];
  int di = 0;
  int pc = 0;
  while (true) {
    auto op = program[pc++];
    switch (op.code) {
      case add:
        cells[di] += op.arg;
        break;
      case move:
        di += op.arg;
        assert(0 <= di && di < num_cells);
        break;
      case in:
        cells[di] = std::cin.get();
        break;
      case out:
        std::cout.put(cells[di]);
        break;
      case jump_if_zero:
        if (cells[di] == 0) pc += op.arg;
        break;
      case jump_unless_zero:
        if (cells[di] != 0) pc += op.arg;
        break;
      case halt:
        return;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: bf <program.bf>\n";
    return 1;
  }
  std::ifstream file(argv[1]);
  std::string source{std::istreambuf_iterator<char>(file), {}};
  auto program = link_jumps(combine(parse(argv[1], source)));
  run(program);
}
