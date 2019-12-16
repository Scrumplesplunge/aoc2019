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

std::vector<digit> fft(std::span<const digit> values, int offset = 0) {
  auto output = step(values, offset);
  for (int i = 1; i < 100; i++) output = step(output, offset);
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

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::array<digit, 1000> buffer;
  std::span<digit> values = buffer;
  (scanner >> values >> scanner::end).check_ok();

  std::cout << "part1 " << code(fft(values)) << '\n';

  std::vector<digit> input;
  input.reserve(values.size() * 10'000);
  for (int i = 0; i < 10'000; i++) {
    input.insert(input.end(), values.begin(), values.end());
  }
  // We never need to compute any values before the index of the output, since
  // the update sequence only ever needs values that come after it.
  int offset = index(input);
  auto shortened = std::span<const digit>(input).subspan(offset);
  auto part2 = fft(shortened, offset);
  std::cout << "part2 " << code(part2) << '\n';
}
