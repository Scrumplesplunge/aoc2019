import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import <numeric>;
import <set>;
import <span>;
import <vector>;
import io;
import util.vec2;

using vec2b = vec2<signed char>;

bool unobstructed(vec2b a, vec2b b, const std::set<vec2b>& asteroids) {
  auto delta = b - a;
  if (delta == vec2b{0, 0}) return false;
  int steps = std::gcd(delta.x, delta.y);
  auto step = delta / steps;
  for (int i = 1; i < steps; i++) {
    if (asteroids.count(a + i * step)) return false;
  }
  return true;
}

int main(int argc, char* argv[]) {
  auto input = init(argc, argv);
  std::set<vec2b> asteroids;
  vec2b position = {};
  for (char c : input) {
    if (c == '#') {
      asteroids.insert(position);
    }
    if (c == '\n') {
      position.y++;
      position.x = 0;
    } else {
      position.x++;
    }
  }
  int count = 0;
  vec2b station;
  for (auto a : asteroids) {
    int visible =
        std::count_if(std::begin(asteroids), std::end(asteroids),
                      [&](auto b) { return unobstructed(a, b, asteroids); });
    if (visible > count) {
      count = visible;
      station = a;
    }
  }
  std::cout << "part1 " << count << '\n';

  auto remaining = asteroids;
  int index = 0;
  auto done = [](auto v) {
    std::cout << "part2 " << 100 * v.x + v.y << '\n';
    std::exit(0);
  };
  while (!remaining.empty()) {
    if (remaining.size() == 1) done(*remaining.begin());
    // Asteroids vaporised by the first rotation.
    std::vector<vec2b> vaporised;
    std::copy_if(std::begin(remaining), std::end(remaining),
                 std::back_inserter(vaporised),
                 [&](auto b) { return unobstructed(station, b, remaining); });
    check(!vaporised.empty());
    // Sort by angle.
    auto angle = [&](auto v) {
      auto delta = v - station;
      auto bearing = std::atan2(delta.x, delta.y);
      return -bearing;
    };
    std::sort(std::begin(vaporised), std::end(vaporised),
              [&](auto l, auto r) { return angle(l) < angle(r); });
    for (auto v : vaporised) {
      if (++index == 200) done(v);
      remaining.erase(v);
    }
  }
  std::cerr << "part2 unsolved\n";
  return 1;
}
