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

#include <cassert>

constexpr bool is_cell(char c) {
  return c == '#' || c == '.';
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
  return result;
}

int part1(const std::bitset<25> initial) {
  auto grid = initial;
  std::vector<char> seen(1 << 25);
  while (true) {
    if (seen[grid.to_ulong()]) break;
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

struct layers {
  std::bitset<25>& operator[](int x) {
    assert(-128 <= x && x < 128);
    return contents[x + 128];
  }
  std::bitset<25> get(int x) const {
    assert(-128 <= x && x < 128);
    return contents[x + 128];
  }

  int neighbours(int layer, int x, int y, direction d) const {
    std::bitset<25> outside = get(layer - 1);
    if (y == 0 && d == up) return outside[5 * 1 + 2];
    if (x == 0 && d == left) return outside[5 * 2 + 1];
    if (x == 4 && d == right) return outside[5 * 2 + 3];
    if (y == 4 && d == down) return outside[5 * 3 + 2];
    int index = 5 * y + x;
    std::bitset<25> inside = get(layer + 1);
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
      std::bitset<25> grid = get(layer);
      switch (d) {
        case up: y--; break;
        case left: x--; break;
        case right: x++; break;
        case down: y++; break;
      }
      assert(0 <= y && y < 5);
      assert(0 <= x && x < 5);
      assert(!(x == 2 && y == 2));
      return grid[5 * y + x];
    }
  }
  
  std::bitset<25> next(int layer) const {
    auto cell = [&](int x, int y, direction d) {
      return neighbours(layer, x, y, d);
    };
    std::bitset<25> grid = get(layer);
    std::bitset<25> result = {};
    for (int y = 0; y < 5; y++) {
      for (int x = 0; x < 5; x++) {
        if (x == 2 && y == 2) continue;
        int neighbours = cell(x, y, up) + cell(x, y, left) + cell(x, y, right) +
                         cell(x, y, down);
        if (grid[5 * y + x]) {
          result[5 * y + x] = (neighbours == 1);
        } else {
          result[5 * y + x] = (neighbours == 1 || neighbours == 2);
        }
      }
    }
    return result;
  }

  layers next() const {
    layers result;
    for (int i = -127; i < 127; i++) result[i] = next(i);
    check(result[-127] == 0);
    check(result[126] == 0);
    return result;
  }

  std::array<std::bitset<25>, 256> contents;
};

int part2(const std::bitset<25> initial) {
  check(!initial[5 * 2 + 2]);
  layers state;
  state[0] = initial;
  for (int i = 0; i < 200; i++) state = state.next();
  int total = 0;
  for (auto layer : state.contents) {
    for (int i = 0; i < 25; i++) total += layer[i];
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
