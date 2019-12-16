import "util/check.h";
import <array>;
import <charconv>;  // bug
import <iostream>;
import <map>;
import <numeric>;
import <span>;
import <string_view>;
import <vector>;
import util.io;

enum digit : signed char {};

scanner& operator>>(scanner& s, digit& d) {
  char c;
  if (s >> matches<is_digit>(c, "digit")) d = digit(c - '0');
  return s;
}

std::vector<digit> input(std::span<const digit> values, int repeat = 1) {
  std::vector<digit> output;
  output.reserve(values.size() * repeat);
  for (int i = 0; i < repeat; i++) {
    output.insert(output.end(), values.begin(), values.end());
  }
  return output;
}

std::vector<digit> step(std::span<const digit> values, int offset = 0) {
  const int n = values.size();
  std::vector<int> cumulative(n + 1);
  cumulative[0] = 0;
  for (int i = 0; i < n; i++) cumulative[i + 1] = cumulative[i] + values[i];
  auto sum = [&](int i, int j) {
    return cumulative[std::clamp(j, 0, n)] - cumulative[std::clamp(i, 0, n)];
  };
  std::vector<digit> output(n);
  for (int i = 0; i < n; i++) {
    int total = 0;
    int multiplier = offset + i + 1;
    for (int j = i; j < n; j += 4 * multiplier) {
      total += sum(j, j + multiplier);
      total -= sum(j + 2 * multiplier, j + 3 * multiplier);
    }
    output[i] = digit((total < 0 ? -total : total) % 10);
  }
  return output;
}

std::vector<digit> fft(
    std::span<const digit> values, int iterations, int offset = 0) {
  auto output = step(values, offset);
  for (int i = 1; i < iterations; i++) output = step(output, offset);
  return output;
}

std::string code(std::span<const digit> values) {
  check(values.size() >= 8);
  std::string output;
  for (int i = 0; i < 8; i++) output.push_back((char)values[i] + '0');
  return output;
}

int index(std::span<const digit> values) {
  check(values.size() >= 7);
  int index = 0;
  for (int i = 0; i < 7; i++) index = 10 * index + values[i];
  check(0 <= index && index + 8 <= (int)values.size());
  return index;
}

// wa 50924767 - "too high"
int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::array<digit, 1000> buffer;
  std::span<digit> values = buffer;
  (scanner >> values >> scanner::end).check_ok();
  std::cout << "part1 " << code(fft(values, 100)) << '\n';
  auto initial = input(values, 10'000);
  int offset = index(initial);
  auto shortened = std::span<const digit>(initial).subspan(offset);
  auto part2 = fft(shortened, 100, offset);
  std::cout << "part2 " << code(part2) << '\n';
}
