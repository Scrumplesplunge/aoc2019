import "util/check.h";
import <charconv>;  // bug
import <iostream>;
import <optional>;  // bug
import <span>;
import <string>;
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

std::string fft(std::span<const digit> values, int offset = 0) {
  auto output = step(values, offset);
  for (int i = 1; i < 100; i++) output = step(output, offset);
  check(output.size() >= 8);
  std::string code;
  for (int i = 0; i < 8; i++) code.push_back((char)output[i] + '0');
  return code;
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::array<digit, 1000> buffer;
  std::span<digit> values = buffer;
  (scanner >> values >> scanner::end).check_ok();

  std::cout << "part1 " << fft(values) << '\n';

  // Compute the offset of the output.
  check(values.size() >= 7);
  int offset = 0;
  for (int i = 0; i < 7; i++) offset = 10 * offset + values[i];
  check(0 <= offset && offset + 8 <= 10'000 * (int)values.size());

  // We never need to compute any values before the index of the output, since
  // the update sequence only ever needs values that come after it.
  std::vector<digit> input;
  input.reserve(10'000 * values.size() - offset);
  input.insert(
      input.end(), values.begin() + offset % values.size(), values.end());
  for (int i = offset / values.size() + 1; i < 10'000; i++) {
    input.insert(input.end(), values.begin(), values.end());
  }
  std::cout << "part2 " << fft(input, offset) << '\n';
}
