import "util/check.h";
import <charconv>;  // bug
import <optional>;  // bug
import <map>;
import <vector>;
import io;

auto object(std::string_view& s) {
  return sequence<is_alnum>(s, "object name");
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::map<std::string_view, std::string_view> orbits;
  std::string_view a, b;
  while (!scanner.done()) {
    (scanner >> object(a) >> exact(")") >> object(b) >> exact("\n")).check_ok();
    orbits.emplace(b, a);
  }
  (scanner >> scanner::end).check_ok();
  std::map<std::string_view, int> indirect_orbits;
  for (auto entry : orbits) {
    auto a = entry.first, b = entry.second;
    while (true) {
      indirect_orbits[b]++;
      if (b == "COM") break;
      a = b;
      b = orbits.at(a);
    }
  }
  int total_indirect_orbits = 0;
  for (auto [planet, count] : indirect_orbits) total_indirect_orbits += count;
  std::cout << "part1 " << total_indirect_orbits << "\n";

  int x_steps = 0;
  for (auto x = orbits.at("YOU"); x != "COM"; x_steps++, x = orbits.at(x)) {
    int y_steps = 0;
    for (auto y = orbits.at("SAN"); y != "COM"; y_steps++, y = orbits.at(y)) {
      if (x == y) {
        std::cout << "part2 " << x_steps + y_steps << '\n';
        return 0;
      }
    }
  }
}
