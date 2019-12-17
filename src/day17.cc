import "util/check.h";
import <array>;
import <charconv>;  // bug
import <deque>;
import <optional>;  // bug
import <span>;
import <unordered_set>;
import <unordered_map>;  // bug
import <vector>;
import <variant>;
import util.io;
import intcode;
import util.aabb;
import util.vec2;

enum cell {
  empty = '.',
  scaffolding = '#',
  robot_up = '^',
  robot_left = '<',
  robot_right = '>',
  robot_down = 'v',
  robot_tumbling = 'X',
};

constexpr bool is_grid_cell(program::value_type c) {
  return c == empty || c == scaffolding || c == robot_up || c == robot_left ||
         c == robot_right || c == robot_down || c == robot_tumbling;
}

constexpr int grid_width = 37, grid_height = 33;
using grid = std::array<std::array<cell, grid_width>, grid_height>;

grid read_grid(program::const_span program_output) {
  check(!program_output.empty());
  check(program_output.back() == '\n');
  check(program_output.size() == 1 + (1 + grid_width) * grid_height);
  // Translate the grid into a map.
  grid grid;
  for (int y = 0; y < grid_height; y++) {
    for (int x = 0; x < grid_width; x++) {
      auto value = program_output[(grid_width + 1) * y + x];
      std::cout << (char)value << std::flush;
      check(is_grid_cell(value));
      grid[y][x] = cell(value);
    }
    check(program_output[(grid_width + 1) * y + grid_width] == '\n');
    std::cout << '\n';
  }
  return grid;
}

auto get(const grid& grid, int x, int y) {
  if (x < 0 || grid_width <= x || y < 0 || grid_height <= y) return empty;
  return grid[y][x];
}

int count_paths(const grid& grid, int x, int y) {
  const auto up = get(grid, x, y - 1),
             left = get(grid, x - 1, y),
             right = get(grid, x + 1, y),
             down = get(grid, x, y + 1);
  return (up != empty) + (left != empty) + (right != empty) + (down != empty);
}

enum direction { up, right, down, left };
direction path_direction(const grid& grid, int x, int y) {
  check(count_paths(grid, x, y) == 1);
  const auto up = get(grid, x, y - 1),
             left = get(grid, x - 1, y),
             right = get(grid, x + 1, y);
  if (up != empty) return direction::up;
  if (left != empty) return direction::left;
  if (right != empty) return direction::right;
  return direction::down;
}

direction robot_direction(cell c) {
  switch (c) {
    case robot_up: return up;
    case robot_left: return left;
    case robot_right: return right;
    case robot_down: return down;
    default:
      std::cerr << "Not a robot: " << (char)c << '\n';
      std::abort();
  }
}

int part1(const grid& grid) {
  int total = 0;
  for (int y = 0; y < grid_height; y++) {
    for (int x = 0; x < grid_width; x++) {
      if (grid[y][x] == empty) continue;
      const int paths = count_paths(grid, x, y);
      // We assume that lines always fully cross for part 2.
      check(paths != 3);  ;
      if (paths > 2) {
        total += x * y;
      }
    }
  }
  return total;
}

vec2i find_robot(const grid& grid) {
  for (int y = 0; y < grid_height; y++) {
    for (int x = 0; x < grid_width; x++) {
      cell c = grid[y][x];
      check(c != robot_tumbling);
      if (c == robot_up || c == robot_left || c == robot_right ||
          c == robot_down) {
        return {x, y};
      }
    }
  }
  std::cerr << "Could not locate robot.\n";
  std::abort();
}

enum class turn : signed char { left, right };
enum go : int {};
enum call { call_a, call_b, call_c };

using move = std::variant<turn, go, call>;

template <typename... Ts>
struct overload : Ts... {
  using Ts::operator()...;
};

template <typename... Ts>
overload(Ts...) -> overload<Ts...>;

void print(std::string& output, move m) {
  std::visit(overload{
    [&](turn t) { output += t == turn::left ? 'L' : 'R'; },
    [&](call c) { output += 'A' + c; },
    [&](go g) { output += std::to_string((int)g); },
  }, m);
}

void print(std::string& output, std::span<const move> moves) {
  if (moves.empty()) return;
  for (auto step : moves) {
    print(output, step);
    output += ',';
  }
  output.back() = '\n';
}

turn compute_turn(const grid& grid, int x, int y, direction direction) {
  switch (direction) {
    case up: return get(grid, x - 1, y) == empty ? turn::right : turn::left;
    case right: return get(grid, x, y - 1) == empty ? turn::right : turn::left;
    case down: return get(grid, x + 1, y) == empty ? turn::right : turn::left;
    case left: return get(grid, x, y + 1) == empty ? turn::right : turn::left;
  }
}

std::span<move> compute_moves(const grid& grid, std::span<move> output) {
  auto [x, y] = find_robot(grid);
  check(count_paths(grid, x, y) == 1);
  int moves = 0;
  auto emit = [&](move m) {
    check(moves < (int)output.size());
    output[moves++] = m;
  };
  direction robot = path_direction(grid, x, y);
  // Turn the robot to face down the path.
  int turn = (4 + robot - robot_direction(grid[y][x])) % 4;
  switch (turn) {
    case 0: break;
    case 1: emit(turn::right); break;
    case 2: emit(turn::right); emit(turn::right); break;
    case 3: emit(turn::left); break;
  }
  while (true) {
    // Travel the extent of the path.
    const vec2i start = {x, y};
    switch (robot) {
      case up: while (get(grid, x, y - 1) != empty) y--; break;
      case right: while (get(grid, x + 1, y) != empty) x++; break;
      case down: while (get(grid, x, y + 1) != empty) y++; break;
      case left: while (get(grid, x - 1, y) != empty) x--; break;
    }
    const int distance = (vec2i(x, y) - start).manhattan_length();
    for (int i = 0; i < distance; i++) emit(go{1});
    if (count_paths(grid, x, y) == 1) break;
    // Pick the new target direction (either left or right).
    auto turn = compute_turn(grid, x, y, robot);
    emit(turn);
    robot = direction((robot + (turn == turn::left ? 3 : 1)) % 4);
  }
  return output.subspan(0, moves);
}

std::span<move> encode(std::span<const move> moves, std::span<move> output) {
  int length = 0;
  int i = 0, n = moves.size();
  while (i < n) {
    if (length == (int)output.size()) return {};
    if (auto* op = std::get_if<go>(&moves[i])) {
      int steps = 0;
      while (i < n && (op = std::get_if<go>(&moves[i]))) {
        steps += *op;
        i++;
      }
      output[length++] = go{steps};
    } else {
      output[length++] = moves[i++];
    }
  }
  std::string temp;
  print(temp, output.first(length));
  if (temp.length() > 21) return {};
  return output.first(length);
}

std::span<move> encode(std::span<const move> moves,
                       std::span<const move> a,
                       std::span<const move> b,
                       std::span<const move> c,
                       std::span<move> main) {
  const int n = moves.size();
  int i = 0;
  auto starts_with = [&](std::span<const move> sequence) {
    return (int)sequence.size() <= n - i &&
           std::equal(sequence.begin(), sequence.end(), moves.begin() + i);
  };
  int length = 0;
  while (i < n) {
    if (length == (int)main.size()) {
      std::cout << "\rtoo long at i=" << i << ", n=" << n << "  " << std::flush;
      return {};
    }
    if (starts_with(a)) {
      i += a.size();
      main[length++] = call_a;
    } else if (starts_with(b)) {
      i += b.size();
      main[length++] = call_b;
    } else if (starts_with(c)) {
      i += c.size();
      main[length++] = call_c;
    } else {
      std::cout << "\rmismatch at i=" << i << ", n=" << n << "  " << std::flush;
      return {};
    }
  }
  return main.first(length);
}

struct compile_result {
  std::span<move> a, b, c, main;
};

std::optional<compile_result> compile(
    std::span<const move> moves, std::span<move> a, std::span<move> b,
    std::span<move> c, std::span<move> main) {
  std::cout << "input:\n";
  std::string temp;
  print(temp, moves);
  std::cout << temp;

  compile_result result;
  //std::string temp;
  //print(temp, moves);
  //std::cout << "moves: " << temp << '\n';
  auto has_sequence = [&](std::span<const move> sequence, int i) {
    return (int)sequence.size() <= (int)moves.size() - i &&
           std::equal(sequence.begin(), sequence.end(), moves.begin() + i);
  };
  for (int i = 1, i_max = moves.size(); i <= i_max; i++) {
    std::span<const move> temp_a = moves.subspan(0, i);
    result.a = encode(temp_a, a);
    if (result.a.empty()) break;
    //std::string temp;
    //print(temp, temp_a);
    //std::cout << "input a: " << temp << '\n';
    //temp.clear();
    //print(temp, result.a);
    //std::cout << "output a: " << temp << '\n';
    
    // allow for repetitions of A before B.
    int b_start = i;
    while (has_sequence(temp_a, b_start)) b_start += i;

    for (int j = 1, j_max = moves.size() - b_start; j <= j_max; j++) {
      std::span<const move> temp_b = moves.subspan(b_start, j);
      result.b = encode(temp_b, b);
      if (result.b.empty()) break;
      //std::string temp;
      //print(temp, temp_b);
      //std::cout << "input b: " << temp << '\n';
      //temp.clear();
      //print(temp, result.b);
      //std::cout << "output b: " << temp << '\n';
    
      // allow for repetitions of A or B before C.
      int c_start = b_start + j;
      while (true) {
        if (has_sequence(temp_a, c_start)) {
          c_start += temp_a.size();
          continue;
        } else if (has_sequence(temp_b, c_start)) {
          c_start += temp_b.size();
          continue;
        } else break;
      }
      for (int k = 1, k_max = moves.size() - c_start; k <= k_max; k++) {
        std::span<const move> temp_c = moves.subspan(c_start, k);
        result.c = encode(temp_c, c);
        if (result.c.empty()) break;
        //std::string temp;
        //print(temp, temp_c);
        //std::cout << "input c: " << temp << '\n';
        //temp.clear();
        //print(temp, result.c);
        //std::cout << "output c: " << temp << '\n';
        result.main = encode(moves, temp_a, temp_b, temp_c, main);
        if (!result.main.empty()) return result;
      }
    }
  }
  return std::nullopt;
}

std::string format(const compile_result& result) {
  std::string output;
  print(output, result.main);
  print(output, result.a);
  print(output, result.b);
  print(output, result.c);
  output += "n\n";
  return output;
}

int part2(const grid& grid, program::span source) {
  move move_buffer[1000];
  auto moves = compute_moves(grid, move_buffer);

  move a_buffer[20], b_buffer[20], c_buffer[20], main_buffer[20];
  auto result = compile(moves, a_buffer, b_buffer, c_buffer, main_buffer);
  check(result.has_value());
  auto commands = format(*result);
  std::cout << "commands:\n" << commands;

  program::value_type input_buffer[1000];
  check(commands.size() <= 1000);
  std::copy(commands.begin(), commands.end(), std::begin(input_buffer));

  check(!source.empty());
  source[0] = 2;
  program brain(source);
  program::value_type output_buffer[10000];
  auto output = brain.run(program::const_span(input_buffer, commands.size()),
                          output_buffer);

  check(!output.empty());
  return output.back();
}

int main(int argc, char* argv[]) {
  program::buffer program_buffer;
  const auto source = program::load(init(argc, argv), program_buffer);

  program::value_type buffer[10000];
  auto grid = read_grid(program(source).run({}, buffer));
  std::cout << "part1 " << part1(grid) << '\n';
  std::cout << "part2 " << part2(grid, source) << '\n';
}
