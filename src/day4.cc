import "util/check.h";
import <charconv>;  // bug
import <optional>;  // bug
import <string_view>;
import io;

bool is_six_digit(int x) { return std::to_string(x).length() == 6; }
auto password(int& x) { return matches<is_six_digit>(x, "password"); }

bool two_adjacent_equal(std::string_view x) {
  for (int i = 1; i < 6; i++) {
    if (x[i - 1] == x[i]) return true;
  }
  return false;
}

bool exactly_two_adjacent_equal(std::string_view x) {
  int counts[10] = {};
  for (char c : x) counts[c - '0']++;
  for (int count : counts) {
    if (count == 2) return true;
  }
  return false;
}

bool non_decreasing(std::string_view x) {
  for (int i = 1; i < 6; i++) {
    if (x[i - 1] > x[i]) return false;
  }
  return true;
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  int lower, upper;
  (scanner >> password(lower) >> exact("-") >> password(upper) >> scanner::end)
      .check_ok();
  int part1 = 0, part2 = 0;
  for (int i = lower; i <= upper; i++) {
    std::string x = std::to_string(i);
    if (two_adjacent_equal(x) && non_decreasing(x)) part1++;
    if (exactly_two_adjacent_equal(x) && non_decreasing(x)) part2++;
  }
  std::cout << "part1 " << part1 << "\npart2 " << part2 << '\n';
}
