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
  int total = 0;
  for (auto [planet, count] : indirect_orbits) total += count;
  std::cout << "part1 " << total << "\n";
  std::vector<std::string_view> you_steps, san_steps;
  int xsteps = 0;
  int best = 999999;
  for (std::string_view x = orbits.at("YOU"); x != "COM"; xsteps++, x = orbits.at(x)) {
    int ysteps = 0;
    for (std::string_view y = orbits.at("SAN"); y != "COM";
         ysteps++, y = orbits.at(y)) {
      if (x == y && xsteps + ysteps < best) best = xsteps + ysteps;
    }
  }
  std::cout << "part2 " << best << "\n";
}
