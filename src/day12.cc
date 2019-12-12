import <charconv>;  // bug
import <iostream>;
import <numeric>;
import <optional>;  // bug
import <set>;
import <unordered_set>;
import <span>;  // bug
import <string_view>;  // bug
import <type_traits>;  // bug
import util.io;
import util.vec3;

struct moon { vec3i position, velocity; };

constexpr bool operator<(moon a, moon b) {
  if (a.position < b.position) return true;
  if (a.position > b.position) return false;
  return a.velocity < b.velocity;
}

scanner& operator>>(scanner& s, moon& m) {
  return s >> exact("<x=") >> m.position.x >> exact(", y=") >> m.position.y >>
         exact(", z=") >> m.position.z >> exact(">");
}

constexpr auto sign(int x) { return x < 0 ? -1 : x > 0 ? 1 : 0; }
constexpr auto sign(vec3i v) { return vec3i{sign(v.x), sign(v.y), sign(v.z)}; }

struct moon_dimension {
  friend constexpr bool operator==(moon_dimension a, moon_dimension b) {
    return a.position == b.position && a.velocity == b.velocity;
  }
  int position = 0, velocity = 0;
};

std::int64_t find_cycle(std::array<moon_dimension, 4> moons_1d) {
  const auto original = moons_1d;
  std::int64_t iterations = 0;
  do {
    for (auto& a : moons_1d) {
      for (auto& b : moons_1d) a.velocity += sign(b.position - a.position);
    }
    for (auto& m : moons_1d) m.position += m.velocity;
    ++iterations;
  } while (moons_1d != original);
  return iterations;
}

template <auto d>
std::array<moon_dimension, 4> dimension(const std::array<moon, 4>& moons) {
  std::array<moon_dimension, 4> result;
  for (int i = 0; i < 4; i++) {
    result[i].position = moons[i].position.*d;
    result[i].velocity = moons[i].velocity.*d;
  }
  return result;
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::array<moon, 4> moons;
  for (moon& moon : moons) (scanner >> moon).check_ok();
  const auto original = moons;
  for (int i = 0; i < 1000; i++) {
    for (auto& a : moons) {
      for (auto& b : moons) a.velocity += sign(b.position - a.position);
    }
    for (auto& m : moons) m.position += m.velocity;
  }
  int energy = 0;
  for (auto& m : moons) {
    energy += m.position.manhattan_length() * m.velocity.manhattan_length();
  }
  std::cout << "part1 " << energy << '\n';

  std::int64_t cycle_x = find_cycle(dimension<&vec3i::x>(original)),
               cycle_y = find_cycle(dimension<&vec3i::y>(original)),
               cycle_z = find_cycle(dimension<&vec3i::z>(original));
  std::int64_t cycle = std::lcm(std::lcm(cycle_x, cycle_y), cycle_z);
  std::cout << "part2 " << cycle << '\n';
}
