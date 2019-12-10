import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import io;

enum pixel : char { black, white, transparent };
constexpr bool is_pixel(char c) {
  return c == '0' || c == '1' || c == '2';
}

scanner& operator>>(scanner& s, pixel& p) {
  char temp;
  if (s >> matches<is_pixel>(temp, "[0-2]")) p = pixel(temp - '0');
  return s;
}

using image = std::array<std::array<pixel, 25>, 6>;
constexpr std::array<short, 3> frequency(const image& image) {
  std::array<short, 3> counts = {};
  for (auto& row : image) for (pixel p : row) counts[p]++;
  return counts;
}

void draw(const image& image) {
  for (auto& row : image) {
    for (pixel p : row) {
      switch (p) {
        case black: std::cout << "\x1b[34m0\x1b[0m"; break;
        case white: std::cout << "\x1b[37;1m1\x1b[0m"; break;
        case transparent: std::cout << "\x1b[34m5\x1b[0m"; break;
      }
    }
    std::cout << '\n';
  }
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::array<image, 100> layers;
  (scanner >> layers >> scanner::end).check_ok();

  // Find the layer with the minimum number of zeros.
  std::array<std::array<short, 3>, 100> layer_frequencies;
  std::transform(std::begin(layers), std::end(layers),
                 std::begin(layer_frequencies), frequency);
  const auto& x = *std::min_element(
      std::begin(layer_frequencies), std::end(layer_frequencies),
      [](auto& a, auto& b) { return a[0] < b[0]; });
  std::cout << "part1 " << x[1] * x[2] << '\n';

  // Combine overlapping layers.
  image combined = layers.front();
  for (int i = 1, n = layers.size(); i < n; i++) {
    for (int y = 0; y < 6; y++) {
      for (int x = 0; x < 25; x++) {
        if (combined[y][x] == transparent) combined[y][x] = layers[i][y][x];
      }
    }
  }

  // Render the output.
  std::cout << "part2:\n";
  draw(combined);
}
