import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import io;

auto object(std::string_view& s) {
  return sequence<is_alnum>(s, "object name");
}

constexpr int ord(char c) {
  return is_upper(c) ? c - 'A' : is_lower(c) ? c - 'a' + 26 : c - '0' + 52;
}

constexpr int key(std::string_view x) {
  return (ord(x[2]) * 62 + ord(x[1])) * 62 + ord(x[0]);
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));

  struct planet {
    unsigned parent = 0;
    unsigned short value = 0;
    bool exists = false;
  };
  std::array<planet, 62 * 62 * 62> planets;

  // Read the input.
  std::string_view a, b;
  while (!scanner.done()) {
    (scanner >> object(a) >> exact(")") >> object(b) >> exact("\n")).check_ok();
    check(a.size() == 3);
    check(b.size() == 3);
    auto& x = planets[key(b)];
    check(!x.exists);
    x.exists = true;
    x.parent = key(a);
  }
  (scanner >> scanner::end).check_ok();

  // Part 1: count all direct and indirect orbits.
  planets[key("COM")].exists = true;
  planets[key("COM")].value = 1;
  for (int i = 0, n = planets.size(); i < n; i++) {
    if (!planets[i].exists) continue;
    int length = 0;
    int x = i;
    while (!planets[x].value) {
      length++;
      x = planets[x].parent;
      check(planets[x].exists);
    }
    length += planets[x].value;
    check(length < 65536);
    for (int steps = 0, x = i; !planets[x].value;
         steps++, x = planets[x].parent) {
      planets[x].value = length - steps;
    }
  }
  long total_orbits = 0;
  for (auto& planet : planets) {
    if (!planet.exists) continue;
    total_orbits += planet.value - 1;
    planet.value = 0;
  }
  std::cout << "part1 " << total_orbits << "\n";

  // Part 2: find the shortest number of orbital hops between YOU and SAN.
  for (int steps = 0, x = planets[key("YOU")].parent; true; steps++) {
    check(steps < 65535);
    planets[x].value = 1 + steps;
    if (x == key("COM")) break;
    x = planets[x].parent;
  }
  for (int steps = 0, x = planets[key("SAN")].parent; true; steps++) {
    if (planets[x].value) {
      std::cout << "part2 " << planets[x].value - 1 + steps << '\n';
      return 0;
    }
    x = planets[x].parent;
  }
}
