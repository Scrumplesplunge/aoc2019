import "util/check.h";
import <array>;
import <charconv>;  // bug
import <experimental/coroutine>;  // bug
import <iostream>;
import <map>;
import <set>;
import <optional>;  // bug
import <span>;
import <vector>;
import <numeric>;
import util.io;

constexpr bool is_cell(char c) {
  return c == '#' || c == '.';
}

struct state { std::bitset<25> value; };

std::ostream& operator<<(std::ostream& output, state s) {
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      std::cout << (x == 2 && y == 2 ? '?' : s.value[5 * y + x] ? '#' : '.');
    }
    std::cout << '\n';
  }
  return output;
}

std::bitset<25> part1_next(std::bitset<25> grid) {
  std::bitset<25> result = {};
  auto cell = [&](int x, int y) -> int {
    if (y < 0 || x < 0 || 5 <= y || 5 <= x) return 0;
    return grid[5 * y + x];
  };
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      int neighbours =
          cell(x - 1, y) + cell(x + 1, y) + cell(x, y - 1) + cell(x, y + 1);
      if (grid[5 * y + x]) {
        result[5 * y + x] = (neighbours == 1);
      } else {
        result[5 * y + x] = (neighbours == 1 || neighbours == 2);
      }
    }
  }
  //std::cout << "before:\n" << state{grid} << "after:\n" << state{result};
  return result;
}

int part1(const std::bitset<25> initial) {
  auto grid = initial;
  std::vector<char> seen(1 << 25);
  for (int i = 0; !seen[grid.to_ulong()]; i++) {
    seen[grid.to_ulong()] = true;
    grid = part1_next(grid);
  }
  return grid.to_ulong();
}

enum direction {
  up = -5,
  left = -1,
  right = 1,
  down = 5,
};

int neighbours(const std::map<int, std::bitset<25>>& layers,
               int layer, int x, int y, direction d) {
  std::bitset<25> outside = {};
  if (auto i = layers.find(layer - 1); i != layers.end()) outside = i->second;
  if (y == 0 && d == up) return outside[5 * 1 + 2];
  if (x == 0 && d == left) return outside[5 * 2 + 1];
  if (x == 4 && d == right) return outside[5 * 2 + 3];
  if (y == 4 && d == down) return outside[5 * 3 + 2];
  int index = 5 * y + x;
  std::bitset<25> inside = {};
  if (auto i = layers.find(layer + 1); i != layers.end()) inside = i->second;
  if (index == 5 * 1 + 2 && d == down) {
    // Top row.
    int count = 0;
    for (int i = 0; i < 5; i++) {
      if (inside[5 * 0 + i]) count++;
    }
    return count;
  } else if (index == 5 * 2 + 1 && d == right) {
    // Left side.
    int count = 0;
    for (int i = 0; i < 5; i++) {
      if (inside[5 * i]) count++;
    }
    return count;
  } else if (index == 5 * 2 + 3 && d == left) {
    // Right side.
    int count = 0;
    for (int i = 0; i < 5; i++) {
      if (inside[5 * i + 4]) count++;
    }
    return count;
  } else if (index == 5 * 3 + 2 && d == up) {
    // Bottom row.
    int count = 0;
    for (int i = 0; i < 5; i++) {
      if (inside[5 * 4 + i]) count++;
    }
    return count;
  } else {
    std::bitset<25> grid = {};
    if (auto i = layers.find(layer); i != layers.end()) grid = i->second;
    switch (d) {
      case up: y--; break;
      case left: x--; break;
      case right: x++; break;
      case down: y++; break;
    }
    check(0 <= y && y < 5);
    check(0 <= x && x < 5);
    check(!(x == 2 && y == 2));
    return grid[5 * y + x];
  }
  std::abort();  // unreachable
}

std::bitset<25> part2_next_layer(
    const std::map<int, std::bitset<25>>& layers, int layer) {
  auto cell = [&](int x, int y, direction d) {
    return neighbours(layers, layer, x, y, d);
  };
  std::bitset<25> grid = {};
  if (auto i = layers.find(layer); i != layers.end()) grid = i->second;
  std::bitset<25> result = {};
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      if (x == 2 && y == 2) continue;
      int neighbours = cell(x, y, up) + cell(x, y, left) + cell(x, y, right) +
                       cell(x, y, down);
      //std::cout << "(" << x << ", " << y << ") has " << neighbours << "\n";
      if (grid[5 * y + x]) {
        result[5 * y + x] = (neighbours == 1);
      } else {
        result[5 * y + x] = (neighbours == 1 || neighbours == 2);
      }
    }
  }
  return result;
}

std::map<int, std::bitset<25>> part2_next(
    const std::map<int, std::bitset<25>>& layers) {
  std::map<int, std::bitset<25>> result;
  for (const auto [layer, grid] : layers) {
    result.emplace(layer, part2_next_layer(layers, layer));
  }
  const auto next_up = result.begin()->first - 1;
  const auto next_down = result.rbegin()->first + 1;
  auto up_grid = part2_next_layer(layers, next_up);
  if (up_grid != 0) result.emplace(next_up, up_grid);
  auto down_grid = part2_next_layer(layers, next_down);
  if (down_grid != 0) result.emplace(next_down, down_grid);
  return result;
}

// wa 557066
// wa 1010
// wa 1995
// wa 2002  // bug, test input (oops)
int part2(const std::bitset<25> initial) {
  check(!initial[5 * 2 + 2]);
  std::map<int, std::bitset<25>> layers = {{0, initial}};
  //std::cout << "Start:\n" << state{initial};
  for (int i = 0; i < 200; i++) {
    layers = part2_next(layers);
    //std::cout << "After round " << i << ":\n";
    //for (auto [layer, grid] : layers) {
    //  std::cout << "layer " << layer << ":\n" << state{grid};
    //}
    //std::cin.get();
  }
  int total = 0;
  for (auto [layer, grid] : layers) {
    for (int i = 0; i < 25; i++) total += grid[i];
  }
  return total;
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::bitset<25> grid;
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      char c;
      (scanner >> matches<is_cell>(c, "grid cell")).check_ok();
      grid[5 * y + x] = (c == '#');
    }
    (scanner >> exact("\n", "newline")).check_ok();
  }
  std::cout << "part1 " << part1(grid) << '\n';
  std::cout << "part2 " << part2(grid) << '\n';
}
